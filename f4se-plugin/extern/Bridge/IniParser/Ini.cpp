#include "Ini.h"
#include "utility.h"

namespace ini
{
	//Конструктор по умолчанию
	map::map() :
		m_path("") {}
	
	// Реализация конструктора
	map::map(const std::filesystem::path& path)
	{
		readFile(path);  // Читаем файл при создании объекта
	}

	map::map(const std::string& path) :
		map(std::filesystem::path(path)) { }

	map::map(const char* path) :
		map(std::filesystem::path(path)) {}

    map::map(const map& m)
    {
        if (this != &m) {
            m_path = m.m_path;
            m_inimap = m.m_inimap;
        }
    }

    map::map(map&& m) {
        if (this != &m) {
            m_path = std::move(m.m_path);
            m_inimap = std::move(m.m_inimap);
        }
	}

	map& map::operator=(const map& m)
	{
        if (this != &m) {
            m_path = m.m_path;
            m_inimap = m.m_inimap;
        }
        return *this;  // Возвращаем ссылку на текущий объект
	}

	map& map::operator=(map&& m) noexcept
	{
		if (this != &m) { 
			m_path = std::move(m.m_path);
			m_inimap = std::move(m.m_inimap);               
		}
		return *this;  // Возвращаем ссылку на текущий объект
	}

	bool map::empty() const noexcept{
		return m_inimap.empty();
	}

	bool map::exists() const noexcept{
		return m_path.empty() || !std::filesystem::exists(m_path);
	}

	bool map::update() noexcept {
		if (m_path.empty() || !std::filesystem::exists(m_path)) {
			std::cerr << "INI file does not exist: " << m_path << std::endl;
			return false;  // Возвращаем false, если файл не существует
		}
		return readFile(m_path);  // Обновляем данные из файла
	}
	
	bool map::readFile(const std::filesystem::path& path)
	{
		if (!std::filesystem::exists(path)) {
			std::cerr << "INI file does not exist: " << path << std::endl;
			m_path = "";
			//throw std::runtime_error("INI file does not exist: " + std::filesystem::absolute(path).string());
			return false;  // Возвращаем false, если файл не существует
		}

		auto file = utils::file::open_file(path, utils::FileMode::Read);
		if (!file.get()) {
			m_path = "";
			return false;
		}

		m_inimap.clear();  // Очищаем текущую карту перед загрузкой нового файла

		m_path = path.string();  // Сохраняем путь к файлу

		std::string line;
		std::string current_section;
		while (std::getline(*file, line)) {
			// Удаляем ведущие пробелы
			auto first_non_space = line.find_first_not_of(" \t\r\n");
			if (first_non_space == std::string::npos)
				continue;
			char first_char = line[first_non_space];
			// Пропускаем комментарии
			if (first_char == '#' || first_char == ';')
				continue;

			// Проверяем секцию
			if (first_char == '[' && line.find(']', first_non_space) != std::string::npos) {
				auto end = line.find(']', first_non_space);
				current_section = line.substr(first_non_space + 1, end - first_non_space - 1);
				current_section = utils::string::trim(current_section, " \t\r\n");
				continue;
			}

			// Парсим ключ=значение
			auto eq_pos = line.find('=', first_non_space);
			if (eq_pos == std::string::npos)
				continue;

			std::string key = line.substr(first_non_space, eq_pos - first_non_space);
			key = utils::string::trim(key, " \t\r\n");

			std::string value = line.substr(eq_pos + 1);
			value = utils::string::trim(value, " \t\r\n");

			// Если значение в кавычках, ищем закрывающую кавычку
			if (!value.empty() && value.front() == '"') {
				size_t end_quote = value.find('"', 1);
				while (end_quote != std::string::npos && value[end_quote - 1] == '\\') {
					// Экранированная кавычка, ищем дальше
					end_quote = value.find('"', end_quote + 1);
				}
				if (end_quote != std::string::npos) {
					std::string quoted = value.substr(1, end_quote - 1);
					m_inimap[current_section][key] = quoted;
				} else {
					// Некорректная строка, пропускаем
					continue;
				}
			} else {
				// Обрезаем комментарий после значения (если не в кавычках)
				size_t comment_pos = value.find_first_of("#;");
				if (comment_pos != std::string::npos) {
					value = value.substr(0, comment_pos);
					value = utils::string::trim(value, " \t\r\n");
				}
				m_inimap[current_section][key] = value;
			}
		}
		file->close();
		return true;
	}

	bool map::readFile(const std::string& path) {
		return readFile(std::filesystem::path(path));
	}

	bool map::reload() {
		return readFile(m_path);
	}

	// Реализация метода contains
	bool map::contains(std::string key, std::string section) const
	{
		if (!m_inimap.contains(section))
			return false;
		return (m_inimap.at(section).contains(key));
	}

	std::ostream& operator<<(std::ostream& os, const map& m)
	{
		os << "ini : " << m.m_path << "\n";

		for (auto it = m.m_inimap.begin(), end = m.m_inimap.end(); it != end; ++it) {
			os << "[" << it->first << "]" << '\n';
			for (auto i = it->second.begin(), endi = it->second.end(); i != endi; ++i) {
				os << i->first << "=" << i->second << "\n";
			}
		}

		return os;
	}

	std::array<std::string, 2> map::split_section_key(const std::string& section_slash_key) const {
		auto pos = section_slash_key.find('/');
		if (pos == std::string::npos) {
			return { "" , section_slash_key };  // Возвращаем только ключ, если слеш не найден
		}
		return { section_slash_key.substr(0, pos), section_slash_key.substr(pos + 1) };  // Возвращаем секцию и ключ
	}
}
