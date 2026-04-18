#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
using namespace std::string_view_literals;

namespace ver
{
	inline constexpr std::size_t MAJOR = 0;
	inline constexpr std::size_t MINOR = 11;
	inline constexpr std::size_t PATCH = 73;

	// Функция для вычисления версии в формате INT
	inline constexpr uint32_t computeVersionInt(std::size_t major, std::size_t minor, std::size_t patch)
	{
		return (static_cast<uint32_t>(major) << 16) | (static_cast<uint32_t>(minor) << 8) | static_cast<uint32_t>(patch);
	}

	// Функция для формирования строковой версии (поддерживает многозначные числа)
	inline std::string makeVersionString(std::size_t major, std::size_t minor, std::size_t patch)
	{
		return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
	}

	// Генерация строковой версии и имени (время компиляции)
	inline const std::string VER = makeVersionString(MAJOR, MINOR, PATCH);
	inline const std::string NAME = std::string("Panda NAFicator ") + VER;

	// Генерация целочисленной версии
	inline constexpr uint32_t VER_INT = computeVersionInt(MAJOR, MINOR, PATCH);

	inline constexpr auto PROJECT = "NAFicator"sv;
}
