#pragma once
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
#include <optional>
#include <array>
#include <unordered_map>

namespace ini
{	
	/**
	 * @brief Класс для работы с ini-файлами в виде map<секция, map<ключ, значение>>.
	 */
	class map
	{
		using SectionMap = std::unordered_map<std::string, std::string>;
		using IniMap = std::unordered_map<std::string, SectionMap>;
	public:
		/**
		 * @brief Конструктор по умолчанию.
		 */
		map();
		/**
		 * @brief Копирующий конструктор.
		 */
		map(const map&);
		/**
		 * @brief Перемещающий конструктор.
		 */
		map(map&& m);
		/**
		 * @brief Конструктор, загружает ini-файл из указанного пути.
		 * @param path Путь к ini-файлу.
		 */
		map(const std::filesystem::path&);
		/**
		 * @brief Конструктор, загружает ini-файл из строки пути.
		 * @param path Путь к ini-файлу.
		 */
		map(const std::string&);
		/**
		 * @brief Конструктор, загружает ini-файл из C-строки пути.
		 * @param path Путь к ini-файлу.
		 */
		map(const char*);
		~map() = default;

		/**
		 * @brief Считывает ini-файл в карту.
		 * @param path Путь к ini-файлу.
		 * @return true если успешно, иначе false.
		 */
		bool readFile(const std::filesystem::path& path);

		/**
		 * @brief Считывает ini-файл в карту.
		 * @param path Путь к ini-файлу.
		 * @return true если успешно, иначе false.
		 */
		bool readFile(const std::string& path);

		/**
		 * @brief Обновляет карту повторно считывая ini-файл.
		 * @return true если успешно, иначе false.
		 */
		bool reload();

		/**
		 * @brief Проверяет, существует ли ini-файл.
		 * @return true если файл существует, иначе false.
		 */
		bool exists() const noexcept;
		/**
		 * @brief Проверяет, пуста ли карта.
		 * @return true если карта пуста, иначе false.
		 */
		bool empty() const noexcept;

		/**
		 * @brief Обновляет данные, если m_path существует.
		 * @return true если обновил, иначе false
		 */
		bool update() noexcept;

		/**
		 * @brief Проверяет наличие ключа в секции.
		 * @param key Ключ.
		 * @param section Секция.
		 * @return true если найдено, иначе false.
		 */
		bool contains(std::string key, std::string section) const;
																
		map& operator=(const map& m);
		map& operator=(map&& m) noexcept;
		
		/**
		 * @brief Получить значение по ключу и секции в формате "section/key". Бросает исключение, если не найдено. Не поддерживает установку значений.
		 * @tparam T Тип значения.
		 * @param section_slash_key Строка вида "section/key".
		 * @return Значение типа T.
		 */
		template <class T>
		T operator[](std::string section_slash_key) const;

		/**
		 * @brief Получить значение по ключу и секции в формате "section/key". Возвращает default_value, если не найдено.
		 * @tparam T Тип значения.
		 * @param section_slash_key Строка вида "section/key".
		 * @param default_value Значение по умолчанию.
		 * @return Значение типа T или default_value.
		 */
		template <class T>
		T at(std::string section_slash_key, const T& default_value = T{}) const noexcept;

		/**
		 * @brief Получить значение по ключу и секции.
		 * @tparam T Тип значения.
		 * @param key Ключ.
		 * @param section Секция.
		 * @return std::optional<T> — значение или std::nullopt.
		 */
		template <class T>
		std::optional<T> get(const std::string& key, const std::string& section) const noexcept;

		/**
		 * @brief Получить значение по ключу и секции. Бросает исключение, если не найдено.
		 * @tparam T Тип значения.
		 * @param key Ключ.
		 * @param section Секция.
		 * @return Значение типа T.
		 */
		template <class T>
		T getAt(const std::string& key, const std::string& section) const;

		/**
		 * @brief Установить значение по ключу и секции.
		 * @tparam T Тип значения.
		 * @param key Ключ.
		 * @param section Секция.
		 * @param val Значение.
		 * @return true если успешно, иначе false.
		 */
		template <class T>
		bool set(const std::string& key, const std::string& section, const T& val);

		/**
		 * @brief Установить значение по ключу и секции в формате "section/key".
		 * @tparam T Тип значения.
		 * @param section_slash_key Строка вида "section/key".
		 * @param val Значение.
		 * @return true если успешно, иначе false.
		 */
		template <class T>
		bool set(const std::string& section_slash_key, const T& val);

	private:
		std::string m_path = "";
		IniMap m_inimap;

		/**
		 * @brief Разделяет строку "section/key" на массив из двух строк: {section, key}.
		 * @param section_slash_key Строка вида "section/key".
		 * @return std::array<std::string, 2> — {section, key}.
		 */
		std::array<std::string, 2> split_section_key(const std::string& section_slash_key) const;

		friend std::ostream& operator<<(std::ostream& os, const ini::map& m);
	};

	template <class T>
	inline std::optional<T> map::get(const std::string& key, const std::string& section) const noexcept
	{
		if (contains(key, section)) {
			std::string value_str = std::string(m_inimap.at(section).at(key));  // Получаем значение как строку
			std::istringstream iss(value_str);                          // Создаем поток для преобразования
			T value;
			if (!(iss >> value)) {    // Пробуем считать значение
				return std::nullopt;  // Если не удалось преобразовать, возвращаем std::nullopt
			}
			return value;  // Возвращаем преобразованное значение
		}
		return std::nullopt;  // Если не удалось преобразовать, возвращаем std::nullopt
	}

	template <>
	inline std::optional<std::string> map::get<std::string>(const std::string& key, const std::string& section) const noexcept
	{
		if (contains(key, section)) {
			return std::string(m_inimap.at(section).at(key));  // Прямое возвращение значения для std::string
		}
		return std::nullopt;  // Если не удалось преобразовать, возвращаем std::nullopt
	}

	template <>
	inline std::optional<bool> map::get<bool>(const std::string& key, const std::string& section) const noexcept
	{
		if (!contains(key, section)) {
			return std::nullopt;  // Если не удалось преобразовать, возвращаем std::nullopt
		}

		const std::string& value_str = m_inimap.at(section).at(key);

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
			return std::nullopt;  // Если не удалось преобразовать, возвращаем std::nullopt
		} catch (const std::out_of_range&) {
			return std::nullopt;  // Если не удалось преобразовать, возвращаем std::nullopt
		} catch (...) {
			return std::nullopt;  // Если не удалось преобразовать, возвращаем std::nullopt
		}
	}

    template <class T>
    T map::operator[](std::string section_slash_key) const
    {
		auto section_key = split_section_key(section_slash_key);
		auto result = get<T>(section_key[1], section_key[0]);
		if (result.has_value()) {
			return *result;
		} else {
			throw std::runtime_error("Key not found: " + section_key[1] + " in section: " + section_key[0]);
		}
    }

	template <class T>
	inline T map::at(std::string section_slash_key, const T& default_value) const noexcept
	{
		auto section_key = split_section_key(section_slash_key);	// Разделяем строку на секцию и ключ
		auto result = get<T>(section_key[1], section_key[0]);		// Используем метод get для получения значения
		if (result.has_value()) {
			return *result;		// Если значение найдено, возвращаем его
		} else {
			return default_value;  // Если значение не найдено, возвращаем значение по умолчанию для типа T
		}
	}

	template <class T>
	inline T map::getAt(const std::string& key, const std::string& section) const
	{
		if (contains(key, section)) {
			std::string value_str = std::string(m_inimap[section][key]);  // Получаем значение как строку
			std::istringstream iss(value_str);                          // Создаем поток для преобразования
			T value;
			if (!(iss >> value)) {  // Пробуем считать значение
				throw std::runtime_error("Failed to convert value for key: " + std::string(key) + " in section: " + std::string(section));
			}
			return value;  // Возвращаем преобразованное значение
		}
		throw std::runtime_error("Key not found: " + std::string(key) + " in section: " + std::string(section));
	}

	template <>
	inline std::string map::getAt<std::string>(const std::string& key, const std::string& section) const
	{
		if (contains(key, section)) {
			return std::string(m_inimap.at(section).at(key));  // Прямое возвращение значения для std::string
		}
		throw std::runtime_error("Key not found: " + std::string(key) + " in section: " + std::string(section));
	}

	template <>
	inline bool map::getAt<bool>(const std::string& key, const std::string& section) const
	{
		if (!contains(key, section)) {
			throw std::runtime_error("Key not found: " + key + " in section: " + section);
		}

		const std::string& value_str = m_inimap.at(section).at(key);

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
		} catch (...) {
			throw std::runtime_error("Unknown error occurred while parsing value for key: " + key + " in section: " + section + ".");
		}
	}

    template <class T>
    inline bool map::set(const std::string& key, const std::string& section, const T& val)
    {
        // Преобразование значения в строку
        std::ostringstream oss;
        oss << val;
        std::string value_str = oss.str();

        // Обновляем значение в памяти
        m_inimap[section][key] = value_str;

        // Удваиваем только завершающие '\' чтобы избежать эффекта "продолжения строки"
        if (!value_str.empty()) {
            size_t i = value_str.size();
            size_t trailing = 0;
            while (i > 0 && value_str[i - 1] == '\\') {
                --i;
                ++trailing;
            }
            if (trailing > 0) {
                // Добавляем ещё столько же слэшей в конец (удваиваем завершающую группу)
                value_str.append(trailing, '\\');
            }
        }

        // Читаем весь файл в память
        std::ifstream in_file(m_path);
        if (!in_file.is_open()) {
            return false;
        }
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(in_file, line)) {
            lines.push_back(line);
        }
        in_file.close();

        // Поиск секции и ключа
        bool section_found = false;
        bool key_found = false;
        std::string section_header = "[" + section + "]";
        int insert_section_pos = -1;
        int last_section_pos = -1;
        int idx = 0;

        for (; idx < static_cast<int>(lines.size()); ++idx) {
            std::string trimmed = lines[idx];
            trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
            trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);

            if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#')
                continue;

            if (trimmed.size() > 2 && trimmed.front() == '[' && trimmed.back() == ']') {
                last_section_pos = idx;
                if (trimmed == section_header) {
                    section_found = true;
                    insert_section_pos = idx;
                    // ищем ключ в секции
                    int j = idx + 1;
                    for (; j < static_cast<int>(lines.size()); ++j) {
                        std::string l = lines[j];
                        std::string ltrim = l;
                        ltrim.erase(0, ltrim.find_first_not_of(" \t\r\n"));
                        ltrim.erase(ltrim.find_last_not_of(" \t\r\n") + 1);
                        if (ltrim.empty() || ltrim[0] == ';' || ltrim[0] == '#')
                            continue;
                        if (ltrim.size() > 2 && ltrim.front() == '[' && ltrim.back() == ']')
                            break;  // новая секция
                        auto eq = ltrim.find('=');
                        if (eq != std::string::npos) {
                            std::string k = ltrim.substr(0, eq);
                            k.erase(k.find_last_not_of(" \t\r\n") + 1);
                            if (k == key) {
                                // нашли ключ, обновляем строку
                                lines[j] = key + "=" + value_str;
                                key_found = true;
                                break;
                            }
                        }
                    }
                    if (!key_found) {
                        // добавить ключ в конец секции (перед следующей секцией или концом файла)
                        int insert_pos = j;
                        while (insert_pos > idx && insert_pos < static_cast<int>(lines.size()) &&
                               (lines[insert_pos].empty() || lines[insert_pos][0] == ';' || lines[insert_pos][0] == '#'))
                            ++insert_pos;
                        lines.insert(lines.begin() + insert_pos, key + "=" + value_str);
                    }
                    break;
                }
            }
        }

        if (!section_found) {
            // добавить секцию и ключ в конец файла (без добавления лишней пустой строки)
            lines.push_back(section_header);
            lines.push_back(key + "=" + value_str);
        }

        // Записываем обратно в файл
        std::ofstream out_file(m_path, std::ios::trunc);
        if (!out_file.is_open()) {
            return false;
        }
        for (const auto& l : lines) {
            out_file << l << "\n";
        }
        return true;
    }

	template <class T>
	inline bool map::set(const std::string& section_slash_key, const T& val)
	{
		auto section_key = split_section_key(section_slash_key);		 // Разделяем строку на секцию и ключ
		return set<T>(section_key[1], section_key[0], val);              // Используем метод set для установки значения
	}

	std::ostream& operator<<(std::ostream& os, const ini::map& m);

	template <>
	inline bool map::set<bool>(const std::string& key, const std::string& section, const bool& val)
	{
		// Преобразуем bool в строку "true"/"false"
		return set<std::string>(key, section, val ? "true" : "false");
	}
}
