#include "RandomGenerator.h"

namespace utils
{
	RandomGenerator::RandomGenerator()
	{
		// Инициализация генераторов с разными семенами
		for (size_t i = 0; i < num_generators; ++i) {
			gen[i].seed(std::random_device{}() + i);  // Используем разные семена
		}
	}

	int RandomGenerator::generate_random_int(int min, int max)
	{
		std::lock_guard<std::mutex> lock(mutex);          // Защита от одновременного доступа
		size_t index = get_thread_id() % num_generators;  // Определяем генератор по ID потока
		std::uniform_int_distribution<> dis(min, max);    // Определяем диапазон
		return dis(gen[index]);                           // Генерируем случайное число
	}

	int RandomGenerator::operator()(int min, int max)
	{
		return generate_random_int(min, max);
	}

	size_t RandomGenerator::get_thread_id()
	{
		return std::hash<std::thread::id>()(std::this_thread::get_id());
	}

	void RandomGenerator::update_seed(unsigned int new_seed)
	{
		if (!new_seed)
			new_seed = std::random_device{}();
		std::lock_guard<std::mutex> lock(mutex);  // Защита от одновременного доступа
		for (size_t i = 0; i < num_generators; ++i) {
			gen[i].seed(new_seed + i);  // Обновляем семена для каждого генератора
		}
	}
}
