#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <utils/utility.h>


namespace ini
{
	template <class T1, class T2>
	constexpr bool is_almost_same_v = std::is_same_v<const T1&, const T2&>;

	class map
	{
		using SectionMap = std::unordered_map<std::string, std::string>;
		using IniMap = std::unordered_map<std::string, SectionMap>;

		std::string pth = "";
		IniMap inimap;
		std::unordered_map<std::string, std::string> modified_settings;  // Хранит изменённые параметры
		bool modified = false;                                                     // Флаг для отслеживания изменений
		friend std::ostream& operator<<(std::ostream& os, const ini::map& m);

	public:
		map();
		map(const map&);
		map(map&& m);
		map(const std::filesystem::path&);  // Конструктор считывает файл в IniMap
		map(const std::string&);
		map(const char*);
		bool contains(std::string key, std::string section);
		bool update_file();              // Сохраняет обновленную конфигурацию в ini файл
		void clear_modified_settings();  // Очищает хранилище изменённых настроек
		~map();                          // Деструктор обновляет данные в ini-файле, если данные были изменены

		map operator=(const map& m);
		map& operator=(map&& m);

		template <class T>
		T get(std::string key, std::string section);
		template <class T>
		void set(const T val, std::string key, std::string section);
	};
	
	template <class T>
	inline T map::get(std::string key, std::string section)
	{
		if (contains(key, section)) {
			std::string value_str = std::string(inimap[section][key]);  // Получаем значение как строку
			std::istringstream iss(value_str);             // Создаем поток для преобразования
			T value;
			if (!(iss >> value)) {  // Пробуем считать значение
				throw std::runtime_error("Failed to convert value for key: " + std::string(key) + " in section: " + std::string(section));
			}
			return value;  // Возвращаем преобразованное значение
		}
		throw std::runtime_error("Key not found: " + std::string(key) + " in section: " + std::string(section));
	}

	template <>
	inline std::string map::get<std::string>(std::string key, std::string section)
	{
		if (contains(key, section)) {
			return std::string(inimap[section][key]);  // Прямое возвращение значения для std::string
		}
		throw std::runtime_error("Key not found: " + std::string(key) + " in section: " + std::string(section));
	}

	template <>
	inline bool map::get<bool>(std::string key, std::string section)
	{
		if (!contains(key, section)) {
			throw std::runtime_error("Key not found: " + key + " in section: " + section);
		}

		const std::string& value_str = inimap[section][key];

		if (value_str == "true") {
			return true;
		}
		if (value_str == "false") {
			return false;
		}

		try {
			int int_value = std::stoi(value_str);
			return int_value != 0;
		} catch (const std::invalid_argument&) {
			throw std::runtime_error("Invalid value for key: " + key + " in section: " + section + ". Expected 'true', 'false', or an integer.");
		} catch (const std::out_of_range&) {
			throw std::runtime_error("Value out of range for key: " + key + " in section: " + section + ".");
		}
	}

	template <class T>
	inline void map::set(const T val, std::string key, std::string section)
	{
		// Преобразование значения в строку
		std::ostringstream oss;
		oss << val;                         // Используем ostringstream для преобразования в строку
		std::string value_str = oss.str();  // Получаем строку
		// Сохраняем изменённые настройки
		modified_settings[section][key] = value_str;
		// Проверяем, существует ли секция
		if (!contains(key, section)) {
			// Если секция не существует, создаем её
			inimap[section][key] = value_str;  // Сохраняем новое значение
		} else {
			// Если секция существует, обновляем значение
			inimap[section][key] = value_str;  // Обновляем значение
		}
		modified = true;  // Устанавливаем флаг изменения
	}

	std::ostream& operator<<(std::ostream& os, const ini::map& m);
}
