#pragma once
#include <chrono>
#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

#include <NAFicator/ThreadPool/ThreadPool.h>

extern int get_update_wait_time();

struct ActorComparator
{
	bool operator()(const RE::Actor* lhs, const RE::Actor* rhs) const
	{
		return lhs->formID == rhs->formID;
	}
};

class UniqueObjectQueueManager
{
public:
	UniqueObjectQueueManager() = default;

	void order_for_update_3d(RE::Actor* a, RE::RESET_3D_FLAGS flags = (RE::RESET_3D_FLAGS)(0))
	{
		std::lock_guard<std::mutex> lock(queueMutex);

		// Если объект уже в очереди, обновляем флаги и сбрасываем таймер
		auto it = actorQueue.find(a);
		if (it != actorQueue.end()) {
			it->second.flags = static_cast<uint32_t>(combine3DFlags({ (RE::RESET_3D_FLAGS)it->second.flags, flags }));  // Обновляем флаги
			// Сбрасываем таймер
			it->second.resetTimer();
			return;
		}

		// Добавляем новый объект в очередь
		ActorData actorData{ static_cast<uint32_t>(flags) };
		actorData.resetTimer();  // Инициализируем таймер
		actorQueue[a] = actorData;

		// Запускаем поток для обработки
		ThreadHandler<UniqueObjectQueueManager>::get_thread()->enqueue([this, a]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(global::get_update_wait_time()));

			// После ожидания запускаем Reset3D
			std::lock_guard<std::mutex> lock(queueMutex);
			auto it = actorQueue.find(a);
			if (it != actorQueue.end()) {
				uint32_t flags = it->second.flags;
				actorQueue.erase(it);                               // Удаляем объект из очереди
				a->Reset3D(false, 0, true, static_cast<uint32_t>(disable3DFlags({ (RE::RESET_3D_FLAGS)flags })));  // Запуск метода
			}
		});
	}

	void resetActorTimer(RE::Actor* a)
	{
		std::lock_guard<std::mutex> lock(queueMutex);
		auto it = actorQueue.find(a);
		if (it != actorQueue.end()) {
			it->second.resetTimer();  // Сбрасываем таймер для данного актора
		}
	}

private:
	struct ActorData
	{
		uint32_t flags;
		std::chrono::steady_clock::time_point lastUpdateTime;

		void resetTimer()
		{
			lastUpdateTime = std::chrono::steady_clock::now();
		}
	};

	std::mutex queueMutex;
	std::unordered_map<RE::Actor*, ActorData, std::hash<RE::Actor*>, ActorComparator> actorQueue;
};
