#include <NAFicator/ThreadPool/ThreadPool.h>

ThreadPool::ThreadPool(size_t numThreads) :
	stop(false)
{
	workers.reserve(numThreads);
	for (size_t i = 0; i < numThreads; ++i) {
		workers.emplace_back([&]() {
			for (;;) {
				std::function<void()> task;
				{
					std::unique_lock<std::mutex> lock(this->queueMutex);
					this->condition.wait(lock, [this] {
						return this->stop || !this->tasks.empty();
					});

					if (this->stop && this->tasks.empty())
						return;
					task = std::move(this->tasks.front());
					this->tasks.pop();
				}
				task();
			}
		});
	}
}

void ThreadPool::resize(size_t numThreads)
{
	std::lock_guard<std::mutex> lock(queueMutex);

	// Если количество потоков не изменилось, просто возвращаемся
	if (workers.size() == numThreads) {
		return;
	}

	// Останавливаем текущие потоки
	stop = true;
	condition.notify_all();  // Уведомление всех потоков

	// Ждем завершения всех текущих потоков
	for (std::thread& worker : workers) {
		if (worker.joinable()) {
			worker.join();
		}
	}

	// Очищаем вектор потоков
	workers.clear();
	stop = false;  // Сбрасываем флаг остановки

	// Создаем новые потоки
	workers.reserve(numThreads);
	for (size_t i = 0; i < numThreads; ++i) {
		workers.emplace_back([this]() {
			for (;;) {
				std::function<void()> task;
				{
					std::unique_lock<std::mutex> lock(this->queueMutex);
					this->condition.wait(lock, [this] {
						return this->stop || !this->tasks.empty();
					});

					if (this->stop && this->tasks.empty())
						return;

					task = std::move(this->tasks.front());
					this->tasks.pop();
				}
				task();
			}
		});
	}
}

ThreadPool::~ThreadPool()
{
	{
		std::unique_lock<std::mutex> lock(queueMutex);
		stop = true;
	}
	condition.notify_all();  // Уведомление всех потоков
	for (std::thread& worker : workers) {
		worker.join();  // Ожидание завершения всех потоков
	}
}
