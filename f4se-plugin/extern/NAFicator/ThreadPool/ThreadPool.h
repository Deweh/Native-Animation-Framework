#pragma once
#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <chrono>

class ThreadPool
{
public:
	ThreadPool(size_t numThreads);
	~ThreadPool();

	template <class F>
	auto enqueue(F&& f) -> std::future<typename std::invoke_result<std::decay_t<F>>::type>;
	void resize(size_t numThreads);

private:
	std::vector<std::thread> workers;
	std::queue<std::function<void()>> tasks;

	std::mutex queueMutex;
	std::condition_variable condition;
	bool stop;
};

template <class F>
inline auto ThreadPool::enqueue(F&& f) -> std::future<typename std::invoke_result<std::decay_t<F>>::type>
{
	using return_type = typename std::invoke_result<std::decay_t<F>>::type;

	auto task = std::make_shared<std::packaged_task<return_type()>>(std::forward<F>(f));
	std::future<return_type> res = task->get_future();
	{
		std::lock_guard<std::mutex> lock(queueMutex);
		tasks.emplace([task]() { (*task)(); });
	}
	condition.notify_one();
	return res;
}

extern uint32_t GetThreadsMax();

template <class T>
class ThreadHandler
{
	inline static std::unique_ptr<ThreadPool> threadPool;
	inline static uint32_t previousMaxThreads = 0;
	inline static std::mutex poolMutex;

	inline static void InitializeThreadPool()
	{
		std::lock_guard<std::mutex> lock(poolMutex);  // Обеспечиваем потокобезопасность
		uint32_t currentMaxThreads = GetThreadsMax();

		if (currentMaxThreads != previousMaxThreads) {
			threadPool.reset(new ThreadPool(currentMaxThreads));  // Создаем новый ThreadPool
			previousMaxThreads = currentMaxThreads;               // Обновляем предыдущее значение
		}
	}

public:
	inline static ThreadPool* get_thread()
	{
		InitializeThreadPool();
		return threadPool.get();  // Возвращаем указатель на ThreadPool
	}
};
