#include "Data/Uid.h"
#include "TaskFunctor.h"
#pragma once

namespace Tasks
{
	class TimedTask
	{
	public:
		std::shared_ptr<TaskFunctor> task;
		double duration = 0;
		double initDuration = 0;
		int64_t repeats = 0;
		int64_t maxRepeats = 0;
		uint64_t uid = 0;
		bool initialized = false;
		double timeScale = 1.0;

		static std::shared_ptr<TimedTask> MakeTask(std::shared_ptr<TaskFunctor> _task, double _duration, int64_t _repeats)
		{
			_duration = _duration / 1000;
			auto tsk = std::make_shared<TimedTask>(_task, _duration, _repeats);
			tsk->GetUid();
			return tsk;
		}

		static std::shared_ptr<TimedTask> MakeTask(std::shared_ptr<TaskFunctor> _task, double _duration)
		{
			return MakeTask(_task, _duration, 0);
		}

		TimedTask(std::shared_ptr<TaskFunctor> _task, double _duration) :
			task(_task), duration(_duration), initDuration(_duration)
		{
		}

		TimedTask(std::shared_ptr<TaskFunctor> _task, double _duration, uint64_t _repeats) :
			task(_task), duration(_duration), initDuration(_duration), maxRepeats(_repeats)
		{
		}

		TimedTask()
		{
		}

		template <class Archive>
		void serialize(Archive& ar, const uint32_t)
		{
			ar(task, duration, initDuration, repeats, maxRepeats, uid, initialized, timeScale);
		}

		bool operator==(TimedTask tsk)
		{
			return uid == tsk.uid;
		}

		inline static std::atomic<uint64_t> nextUid = 1;

	private:
		void GetUid()
		{
			uid = Data::Uid::Get();
		}
	};

	class TimerThread : public RE::BSTEventSink<RE::MenuModeChangeEvent>
	{
	public:
		struct PersistentState
		{
			std::unordered_map<uint64_t, std::shared_ptr<TimedTask>> timedTasks;

			template <class Archive>
			void serialize(Archive& ar, const uint32_t)
			{
				ar(timedTasks);
			}
		};

		std::unique_ptr<PersistentState> state;

		std::thread threadHandle;
		bool timerPaused = false;
		bool stateDirty = false;
		bool stopRequested = false;
		std::condition_variable timerStateChanged;
		std::condition_variable threadStopped;
		std::mutex timerLock;

		static TimerThread* GetSingleton()
		{
			static TimerThread singleton;
			return &singleton;
		}

		TimerThread()
		{
			state = std::make_unique<PersistentState>();
		}

		~TimerThread()
		{
			std::unique_lock l{ timerLock };
			StopThread(l);
		}

		bool IsRunning() const
		{
			return threadHandle.joinable();
		}

		// Timed tasks run on the timer thread when they expire.
		// If they affect game objects, they should be synced back to the main thread with F4SE's TaskInterface.
		void AddTimedTask(std::shared_ptr<TimedTask> task)
		{
			std::unique_lock l{ timerLock };
			state->timedTasks.insert(std::pair(task->uid, task));
			stateDirty = true;

			if (!IsRunning()) {
				StartThread(l);
			} else {
				timerStateChanged.notify_one();
			}
		}

		bool RemoveTimedTask(uint64_t taskId)
		{
			std::unique_lock l{ timerLock };
			if (state->timedTasks.contains(taskId)) {
				state->timedTasks.erase(taskId);
				stateDirty = true;
				timerStateChanged.notify_one();
				return true;
			} else {
				return false;
			}
		}

		void VisitTask(uint64_t taskId, std::function<void(TimedTask*)> func)
		{
			std::unique_lock l{ timerLock };
			auto iter = state->timedTasks.find(taskId);
			if (iter != state->timedTasks.end()) {
				func(iter->second.get());
				stateDirty = true;
				timerStateChanged.notify_one();
			}
		}

		// Ensures synchronous removal of multiple tasks.
		bool RemoveTimedTasks(std::vector<uint64_t> taskIds)
		{
			std::unique_lock l{ timerLock };
			bool removedAll = true;

			for (auto& id : taskIds) {
				if (state->timedTasks.contains(id)) {
					state->timedTasks.erase(id);
				} else {
					removedAll = false;
				}
			}

			stateDirty = true;
			timerStateChanged.notify_one();
			return removedAll;
		}

		void Reset()
		{
			std::unique_lock l{ timerLock };
			state->timedTasks.clear();
			StopThread(l);

			TimedTask::nextUid = 1;
		}

		bool Start()
		{
			std::unique_lock l{ timerLock };
			return StartThread(l);
		}

		void Stop()
		{
			std::unique_lock l{ timerLock };
			StopThread(l);
		}

		virtual RE::BSEventNotifyControl ProcessEvent(const RE::MenuModeChangeEvent& a_event, RE::BSTEventSource<RE::MenuModeChangeEvent>*) override
		{
			std::unique_lock l{ timerLock };
			if (a_event.enteringMenuMode && !timerPaused) {
				SetPaused(true);
			} else if (!a_event.enteringMenuMode && timerPaused) {
				SetPaused(false);
			}

			return RE::BSEventNotifyControl::kContinue;
		}

	private:
		void SetPaused(bool paused)
		{
			timerPaused = paused;
			if (IsRunning()) {
				stateDirty = true;
				timerStateChanged.notify_one();
			}
		}

		bool StartThread(std::unique_lock<std::mutex>& l)
		{
			if (IsRunning()) {
				return false;
			}

			if (stopRequested) {
				threadStopped.wait(l, [&]() { return !stopRequested; });
				if (IsRunning()) {
					return false;
				}
			}

			stateDirty = true;
			threadHandle = std::thread(&TimerThread::MainRoutine, this);
			return true;
		}

		void StopThread(std::unique_lock<std::mutex>& l)
		{
			if (IsRunning()) {
				stopRequested = true;
				stateDirty = true;
				timerStateChanged.notify_one();
				threadHandle.detach();
				threadStopped.wait(l, [&]() { return !stopRequested; });
			}
		}

		void MainRoutine()
		{
			std::unique_lock l{ timerLock };
			logger::trace("Timer thread started.");
			auto lastTimestamp = std::chrono::steady_clock::now();
			bool doWait = false;
			double soonestExpiry = 0;
			std::vector<std::shared_ptr<TimedTask>> expiredTasks;

			while (true) {
				if (doWait) {
					// Go to sleep until time expires for the soonest task, or the timer's state changes.
					// timerLock is released while thread is asleep, then re-acquired when awakened.
					timerStateChanged.wait_for(l, std::chrono::duration<double>(soonestExpiry), [&]() { return stateDirty; });
				} else {
					doWait = true;
				}
				stateDirty = false;

				double timeDelta = std::chrono::duration<double>(std::chrono::steady_clock::now() - lastTimestamp).count();
				if (timeDelta < 0) {
					timeDelta = 0;
				}

				expiredTasks.clear();
				soonestExpiry = 0;

				for (auto it = state->timedTasks.begin(); it != state->timedTasks.end(); it++) {
					auto tsk = it->second;

					// If this TimedTask was just added (uninitialized), wait until next loop to begin subtracting time from it.
					// The thread is immediately awakened when tasks are added, so this will safely set the task's start point.
					if (tsk->initialized) {
						tsk->duration -= timeDelta * tsk->timeScale;
					} else {
						tsk->initialized = true;
					}

					if ((soonestExpiry > tsk->duration || soonestExpiry == 0) && tsk->duration > 0) {
						soonestExpiry = tsk->duration * tsk->timeScale;
					}

					if (tsk->duration <= 0) {
						expiredTasks.push_back(tsk);
					}
				}

				// After updating the remaining duration on all tasks, check if the thread should be stopped before running any expired tasks.
				if (stopRequested) {
					break;
				}

				if (timerPaused == true) {
					// If timer is set to paused, put thread to sleep until state changes.
					timerStateChanged.wait(l, [&]() { return stateDirty; });
					// After state changes, zero-out time delta and run the loop again without waiting.
					// If the timer is still set to paused, the thread will just go to sleep again when it reaches this point,
					// or exit completely if threadPendingCancel is true.
					lastTimestamp = std::chrono::steady_clock::now();
					doWait = false;
					continue;
				}

				for (size_t i = 0; i < expiredTasks.size(); i++) {
					auto tsk = expiredTasks[i];

					tsk->task->Run();

					// If task is set to repeat, set duration back to initial duration, otherwise remove the task.
					// If maxRepeats is -1 or less (repeat until task is removed by other code), don't increase the repeats field.
					if (tsk->maxRepeats > tsk->repeats || tsk->maxRepeats < 0) {
						tsk->duration = tsk->initDuration;
						tsk->repeats = tsk->maxRepeats < 0 ? -1 : tsk->repeats + 1;

						if ((soonestExpiry > tsk->duration || soonestExpiry == 0) && tsk->duration > 0) {
							soonestExpiry = tsk->duration * tsk->timeScale;
						}
					} else {
						if (tsk->repeats > 0) {
							tsk->task->Finalize();
						}

						state->timedTasks.erase(expiredTasks[i]->uid);
					}
				}

				// If there are no tasks remaining, wait until state changes, otherwise continue loop.
				if (state->timedTasks.size() < 1) {
					timerStateChanged.wait(l, [&]() { return stateDirty; });
					doWait = false;
				}
				lastTimestamp = std::chrono::steady_clock::now();
			}

			stopRequested = false;
			threadStopped.notify_all();
			logger::trace("Timer thread stopped.");
		}
	};

	// Container used for objects to take ownership of tasks.
	// Provides a RAII-style mechanism that will ensure no tasks belonging to an object continue to run after the object has been destroyed.
	class TaskContainer
	{
	public:
		std::map<std::string, uint64_t> tasks;

		~TaskContainer()
		{
			StopAll();
		}

		void Start(const std::string_view _tskName, std::shared_ptr<TimedTask> task)
		{
			auto tThread = TimerThread::GetSingleton();
			std::string tskName(_tskName);

			if (tasks.contains(tskName)) {
				Stop(tskName);
			}

			tThread->AddTimedTask(task);
			tasks.insert(std::make_pair(tskName, task->uid));
		}

		template <typename T, class... Args>
		void Start(double durationMs, Args... _args)
		{
			Start(typeid(T).name(), TimedTask::MakeTask(std::make_shared<T>(_args...), durationMs));
		}

		template <typename T, class... Args>
		void StartNumbered(uint64_t number, double durationMs, Args... _args)
		{
			std::string id = typeid(T).name();
			id.append(std::to_string(number));
			Start(id, TimedTask::MakeTask(std::make_shared<T>(_args...), durationMs));
		}

		template <typename T>
		void StopNumbered(uint64_t number)
		{
			std::string id = typeid(T).name();
			id.append(std::to_string(number));
			Stop(id);
		}

		template <typename T, class... Args>
		void StartByFrames(double frames, double frameRate, int64_t repeats, Args... _args)
		{
			double singleFrame = 1000.0 / frameRate;
			Start(typeid(T).name(), TimedTask::MakeTask(std::make_shared<T>(_args...), singleFrame * frames, repeats));
		}

		template <typename T, class... Args>
		void StartWithRepeats(double durationMs, int64_t repeats, Args... _args)
		{
			Start(typeid(T).name(), TimedTask::MakeTask(std::make_shared<T>(_args...), durationMs, repeats));
		}

		template <typename T>
		bool IsStarted()
		{
			return tasks.contains(typeid(T).name());
		}

		void Stop(const std::string& tskName)
		{
			auto tThread = TimerThread::GetSingleton();
			auto tskIter = tasks.find(tskName);

			if (tskIter != tasks.end()) {
				tThread->RemoveTimedTask(tskIter->second);
				tasks.erase(tskIter);
			}
		}

		template <typename T>
		void Stop()
		{
			Stop(std::string(typeid(T).name()));
		}

		template <typename T>
		bool UpdateTimeScale(double s = 1.0)
		{
			bool result = false;
			auto tskIter = tasks.find(typeid(T).name());
			if (tskIter != tasks.end()) {
				TimerThread::GetSingleton()->VisitTask(tskIter->second, [&](TimedTask* tsk) {
					tsk->timeScale = s;
					result = true;
				});
			}
			return result;
		}

		template <typename T>
		double GetRemainingTime()
		{
			double result = 0.0;
			auto tskIter = tasks.find(typeid(T).name());
			if (tskIter != tasks.end()) {
				TimerThread::GetSingleton()->VisitTask(tskIter->second, [&](TimedTask* tsk) {
					result = tsk->duration;
				});
			}
			return result;
		}

		void StopAll()
		{
			auto tThread = TimerThread::GetSingleton();

			std::vector<uint64_t> taskIds;
			for (auto& tsk : tasks) {
				taskIds.push_back(tsk.second);
			}

			tThread->RemoveTimedTasks(taskIds);
			tasks.clear();
		}

		template <class Archive>
		void serialize(Archive& ar, const uint32_t)
		{
			ar(tasks);
		}
	};
}
