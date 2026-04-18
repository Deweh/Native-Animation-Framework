#include <IniParser/Ini.h>

namespace ini
{
	//Конструктор по умолчанию
	map::map() :
		pth(""), modified(false){};
	
	// Реализация конструктора
	map::map(const std::filesystem::path& path) :
		pth(path.string())
	{
		if (!utils::file::file_exists(pth)) {
			std::cerr << "INI file does not exist: " << pth << std::endl;
			pth = "";
			return;  // Файл не существует
		}

		auto file = utils::file::open_file(path, utils::FileMode::Read);
		if (!file.get()) {
			pth = "";
			return;
			//throw std::runtime_error("Unable to open INI file: " + pth);
		}
		std::string line;
		std::string current_section;
		while (std::getline(*file, line)) {
			line = utils::ini::remove_comments(line);  // Удаляем комментарии
			line = utils::string::trim(line, " \t\n\r");  // Убираем лишние пробелы и символы
			if (line.empty()) {
				continue;  // Пропускаем пустые строки
			}
			// Проверяем, является ли строка секцией
			std::string section = utils::ini::get_section(line);
			if (!section.empty()) {
				current_section = section;  // Устанавливаем текущую секцию
				continue;
			}
			// Получаем пару ключ-значение
			try {
				auto p = utils::ini::get_key_value_pair(line);
				inimap[current_section][p.first] = p.second;  // Сохраняем в inimap
			} catch (const std::invalid_argument& e) {
				continue;
				//file->close();
				//std::cerr << "'map' : error in line: " << line << " - " << e.what() << std::endl;
			}
		}
		file->close();
	}

	map::map(const std::string& path) :
		map(std::filesystem::path(path)) { }

	map::map(const char* path) :
		map(std::filesystem::path(path)) {}

	map::map(const map& m) :
		pth(m.pth), inimap(m.inimap), modified(false) {}

	map::map(map&& m)
		:
		pth(std::move(m.pth)),
		inimap(std::move(m.inimap)),
		modified_settings(std::move(m.modified_settings)),
		modified(m.modified)
	{
		m.modified = false; 
	}

	map map::operator=(const map& m)
	{
		pth = m.pth;
		inimap = m.inimap;
		modified_settings.clear();
		modified = false;
	}

	map& map::operator=(map&& m)
	{
		if (this != &m) { 
			pth = std::move(m.pth);
			inimap = std::move(m.inimap);
			modified_settings = std::move(m.modified_settings); 
			modified = m.modified;                               
			m.modified = false;                                 
		}
		return *this;  // Возвращаем ссылку на текущий объект
	}

	// Реализация метода contains
	bool map::contains(std::string key, std::string section)
	{
		if (!inimap.contains(section))
			return false;
		return (inimap[section].contains(key));
	}

	// Метод для обновления файла
	bool map::update_file()
	{
		if (!modified) {
			return false;  // Если изменений нет, ничего не делаем
		}
		std::ofstream out_file(pth);
		if (!out_file.is_open()) {
			std::cerr << "Unable to open INI file for writing: " << pth << std::endl;
			return false;
		}
		for (const auto& section_pair : inimap) {
			out_file << "[" << section_pair.first << "]\n";  // Записываем секцию
			for (const auto& key_value_pair : section_pair.second) {
				out_file << key_value_pair.first << "=" << key_value_pair.second << "\n";  // Записываем ключ-значение
			}
			out_file << "\n";  // Пустая строка между секциями
		}
		modified = false;  // Сбрасываем флаг изменений
		return true;
	}
	// Метод для очистки хранилища изменённых настроек
	void map::clear_modified_settings()
	{
		modified_settings.clear();  // Очищаем хранилище изменённых настроек
	}

	// Деструктор
	map::~map()
	{
		if (modified) {
			update_file();
		}
	}

	std::ostream& operator<<(std::ostream& os, const map& m)
	{
		os << "ini : " << m.pth << "\n";

		for (auto it = m.inimap.begin(), end = m.inimap.end(); it != end; ++it) {
			os << "[" << it->first << "]" << '\n';
			for (auto i = it->second.begin(), endi = it->second.end(); i != endi; ++i) {
				os << i->first << "=" << i->second << "\n";
			}
		}

		return os;
	}
}
