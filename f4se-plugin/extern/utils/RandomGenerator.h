#pragma once
#include <random>
#include <array>
#include <mutex>

namespace utils
{

	class RandomGenerator
	{
	public:
		RandomGenerator();
		RandomGenerator(RandomGenerator&) = default;
		RandomGenerator(const RandomGenerator&) = default;

		RandomGenerator& operator=(RandomGenerator&) = default;
		RandomGenerator& operator=(const RandomGenerator&) = default;

		int generate_random_int(int min, int max);
		int operator()(int min, int max);

		void update_seed(unsigned int new_seed = 0);

	private:
		static const size_t num_generators = 8;        // Количество генераторов
		std::array<std::mt19937, num_generators> gen;  // Массив генераторов случайных чисел
		std::mutex mutex;                              // Мьютекс для защиты доступа

		// Функция для получения ID потока
		size_t get_thread_id();
	};

}
