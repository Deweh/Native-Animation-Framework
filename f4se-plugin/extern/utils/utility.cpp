#include "utility.h"

namespace utils
{
	namespace string
	{
		// Разделение строки на подстроки по разделителю
		std::vector<std::string> split(const std::string& fullString, const std::string& delimiter)
		{
			if (delimiter.empty()) {
				throw std::invalid_argument("'SplitString' : The delimiter cannot be empty.");
			}
			std::vector<std::string> substrings;
			size_t start = 0;
			size_t end = fullString.find(delimiter);
			while (end != std::string::npos) {
				substrings.push_back(fullString.substr(start, end - start));
				start = end + delimiter.length();
				end = fullString.find(delimiter, start);
			}
			if (start < fullString.length()) {
				substrings.push_back(fullString.substr(start));
			}
			return substrings;
		}

		std::vector<std::string> split(const std::string_view& fullString, const std::string& delimiter)
		{
			return split(std::string(fullString), delimiter);
		}

		std::string& ltrim(std::string& str, const std::string& chars)
		{
			str.erase(0, str.find_first_not_of(chars));
			return str;
		}

		std::string& rtrim(std::string& str, const std::string& chars)
		{
			str.erase(str.find_last_not_of(chars) + 1);
			return str;
		}

		std::string& trim(std::string& str, const std::string& chars)
		{
			return ltrim(rtrim(str, chars), chars);
		}
		// Замена подстроки
		bool replace(std::string& line, const std::string& remove, const std::string& add)
		{
			if (remove.empty())
				return false;  // Если строка для удаления пустая, возвращаем false
			size_t pos = line.find(remove);
			if (pos != std::string::npos) {
				line.replace(pos, remove.length(), add);
				return true;
			}
			return false;
		}

		// Функция для преобразования std::string в нижний регистр
		std::string to_lower(std::string_view str)
		{
			std::string copy(str);
			std::transform(copy.begin(), copy.end(), copy.begin(),
				[](unsigned char c) { return std::tolower(c); });
			return copy;
		}

		void to_lower(std::string& str)
		{
			std::transform(str.begin(), str.end(), str.begin(),
				[](unsigned char c) { return std::tolower(c); });
		}

		std::string to_lower(const std::string& str)
		{
			std::string copy(str);
			std::transform(copy.begin(), copy.end(), copy.begin(),
				[](unsigned char c) { return std::tolower(c); });
			return copy;
		}

		//Функция для удаления BOM-символов
		std::string clean_string_from_BOM(const std::string& str)
		{
			// Проверяем наличие BOM в начале строки
			if (str.size() >= 3 &&
				(unsigned char)str[0] == 0xEF &&
				(unsigned char)str[1] == 0xBB &&
				(unsigned char)str[2] == 0xBF) {
				return str.substr(3);  // Удаляем BOM
			}
			return str;
		}
	}

	namespace view
	{
		// Разделение строки на подстроки по разделителю
		std::vector<std::string_view> split(std::string_view fullString, std::string_view delimiter)
		{
			if (delimiter.empty()) {
				throw std::invalid_argument("'SplitString' : The delimiter cannot be empty.");
			}
			std::vector<std::string_view> substrings;
			size_t start = 0;
			size_t end = fullString.find(delimiter);
			while (end != std::string_view::npos) {
				substrings.emplace_back(fullString.substr(start, end - start));
				start = end + delimiter.length();
				end = fullString.find(delimiter, start);
			}
			if (start < fullString.length()) {
				substrings.emplace_back(fullString.substr(start));
			}
			return substrings;
		}

		std::string_view ltrim(std::string_view str, const std::string_view chars)
		{
			return str.substr(str.find_first_not_of(chars));
		}

		std::string_view rtrim(std::string_view str, const std::string_view chars)
		{
			return str.substr(0, str.find_last_not_of(chars) + 1);
		}
		
		std::string_view trim(std::string_view str, const std::string_view chars)
		{
			return ltrim(rtrim(str, chars), chars);
		}
	}

	namespace xml
	{
		// Удаление пробелов слева и справа
		std::string trim(const std::string& str)
		{
			size_t first = str.find_first_not_of(" \t");
			size_t last = str.find_last_not_of(" \t");
			return (first == std::string::npos) ? "" : str.substr(first, (last - first + 1));
		}
		// Замена escape-последовательностей
		std::string replace_xml_escape_sequences(const std::string& input)
		{
			std::unordered_map<std::string, std::string> escape_sequences = {
				{ "&amp;", "&" },
				{ "&lt;", "<" },
				{ "&gt;", ">" },
				{ "&quot;", "\"" },
				{ "&apos;", "'" }
			};
			std::string output = input;
			for (const auto& pair : escape_sequences) {
				size_t pos = 0;
				while ((pos = output.find(pair.first, pos)) != std::string::npos) {
					output.replace(pos, pair.first.length(), pair.second);
					pos += pair.second.length();
				}
			}
			return output;
		}

		// Перегрузка для std::string_view
		std::string replace_xml_escape_sequences(std::string_view input)
		{
			std::unordered_map<std::string, std::string> escape_sequences = {
				{ "&amp;", "&" },
				{ "&lt;", "<" },
				{ "&gt;", ">" },
				{ "&quot;", "\"" },
				{ "&apos;", "'" }
			};
			std::string output = std::string(input);
			for (const auto& pair : escape_sequences) {
				size_t pos = 0;
				while ((pos = output.find(pair.first, pos)) != std::string::npos) {
					output.replace(pos, pair.first.length(), pair.second);
					pos += pair.second.length();
				}
			}
			return output;
		}

		// Экранирование символов
		std::string escape_xml_characters(const std::string& input)
		{
			std::unordered_map<char, std::string> escape_sequences = {
				{ '&', "&amp;" },
				{ '<', "&lt;" },
				{ '>', "&gt;" },
				{ '"', "&quot;" },
				{ '\'', "&apos;" }
			};
			std::string output;
			for (char ch : input) {
				output += escape_sequences.count(ch) ? escape_sequences[ch] : std::string(1, ch);
			}
			return output;
		}

		// Перегрузка для std::string_view
		std::string escape_xml_characters(std::string_view input)
		{
			std::unordered_map<char, std::string> escape_sequences = {
				{ '&', "&amp;" },
				{ '<', "&lt;" },
				{ '>', "&gt;" },
				{ '"', "&quot;" },
				{ '\'', "&apos;" }
			};
			std::string output;
			for (char ch : std::string(input)) {
				if (escape_sequences.find(ch) != escape_sequences.end()) {
					output += escape_sequences[ch];
				} else {
					output += ch;
				}
			}
			return output;
		}

		bool remove_inline_spaces(std::string& line)
		{
			if (line.empty())
				return false;
			std::string result;
			bool in_quotes = false;
			bool last_was_space = false;  // Флаг для отслеживания последнего символа
			for (size_t i = 0; i < line.length(); ++i) {
				char current = line[i];
				// Проверяем, находимся ли мы внутри кавычек
				if (current == '\"') {
					in_quotes = !in_quotes;  // Переключаем состояние наличия кавычек
				}
				// Если мы находимся вне кавычек, обрабатываем символы
				if (!in_quotes) {
					if (current == '<' || current == '>') {
						result += current;       // Добавляем '<' или '>' в результат
						last_was_space = false;  // Сбрасываем флаг пробела
						continue;                // Переходим к следующему символу
					}
				}
				// Заменяем табуляцию на пробел
				if (current == '\t') {
					current = ' ';  // Заменяем табуляцию на пробел
				}
				// Обрабатываем пробелы
				if (current == ' ') {
					if (!last_was_space) {
						result += current;      // Добавляем пробел в результат, если он не повторяется
						last_was_space = true;  // Устанавливаем флаг, что последний символ был пробелом
					}
				} else {
					result += current;       // Добавляем текущий символ в результат
					last_was_space = false;  // Сбрасываем флаг пробела
				}
			}
			line = result;  // Обновляем исходную строку
			return true;
		}

		size_t find_attribute_pos(const std::string& line, const std::string& attribute)
		{
			std::string search = attribute + "=";
			size_t pos = line.find(search);
			while (pos != std::string::npos) {
				// Проверяем, что предыдущий символ - пробел или табуляция, или это начало строки
				if (pos == 0 || line[pos - 1] == ' ' || line[pos - 1] == '\t') {
					return pos;  // Возвращаем позицию, если проверка прошла успешно
				}
				// Ищем следующий экземпляр атрибута
				pos = line.find(search, pos + search.length());
			}

			return std::string::npos;  // Атрибут не найден с нужными условиями
		}

		std::string pop_attribute(std::string& line, const std::string& attribute)
		{
			size_t pos = find_attribute_pos(line, attribute);
			if (pos == std::string::npos)
				return "";  // Атрибут не найден
			// Находим начало и конец значения атрибута
			size_t start = line.find_first_of("\"'", pos);
			size_t end = line.find_first_of("\"'", start + 1);
			if (start == std::string::npos || end == std::string::npos)
				return "";  // Ошибка, если кавычки не найдены
			// Извлекаем значение
			std::string value = line.substr(start + 1, end - start - 1);
			// Удаляем атрибут из строки
			line.erase(pos, end - pos + 1);
			line = trim(line);  // Убираем лишние пробелы после изъятия
			return value;
		}

		std::string get_attribute_value(const std::string& line, const std::string& attribute)
		{
			size_t pos = find_attribute_pos(line, attribute);
			if (pos == std::string::npos)
				return "";  // Атрибут не найден
			// Находим начало и конец значения атрибута
			size_t start = line.find_first_of("\"'", pos);
			size_t end = line.find_first_of("\"'", start + 1);
			if (start == std::string::npos || end == std::string::npos)
				return "";  // Ошибка, если кавычки не найдены
			return line.substr(start + 1, end - start - 1);
		}

		std::vector<std::pair<std::string, std::string>> get_all_attributes(const std::string& line)
		{
			std::vector<std::pair<std::string, std::string>> attributes;
			std::istringstream stream(line);
			std::string word;
			while (stream >> word) {
				if (word.find('=') != std::string::npos) {
					std::string attribute = word.substr(0, word.find('='));
					std::string value(get_attribute_value(line, attribute));
					attributes.emplace_back(attribute, value);
				}
			}
			return attributes;
		}

		std::string get_node_name(const std::string& line)
		{
			size_t start = line.find('<');
			size_t end = line.find('>', start);

			// Проверяем, что нашли открывающий тег и закрывающий тег
			if (start == std::string::npos || end == std::string::npos) {
				return "";  // Возвращаем пустую строку, если имя узла не найдено
			}

			// Проверяем, что start + 1 не выходит за пределы строки
			if (start + 1 >= line.size()) {
				return "";  // Возвращаем пустую строку, если имя узла не найдено
			}

			// Начинаем после '</' если это закрывающий тег
			size_t name_start = (line[start + 1] == '/') ? start + 2 : start + 1;

			// Ищем первый пробел, табуляцию или закрывающий символ
			size_t name_end = line.find_first_of(" \t/>", name_start);

			// Если name_end не найден, это значит, что до конца строки нет пробелов или табуляций
			if (name_end == std::string::npos) {
				name_end = end;  // Устанавливаем конец на позицию закрывающего тега
			}

			// Проверяем, что имя узла найдено
			if (name_start < name_end) {
				return line.substr(name_start, name_end - name_start);  // Возвращаем имя узла
			}

			return "";  // Возвращаем пустую строку, если имя узла не найдено
		}

		std::string pop_node_name(std::string& str)
		{
			std::string node_name = get_node_name(str);
			size_t start = str.find('<');
			size_t end = str.find('>', start);
			str.erase(start, end - start + 1);
			str = trim(str);  // Убираем лишние пробелы после изъятия
			return node_name;
		}

		bool change_node_name(std::string& str, std::string_view new_node_name)
		{
			size_t start = str.find('<');
			size_t end = str.find(' ', start);
			if (end == std::string::npos) {
				end = str.find('>', start);
			}
			if (start == std::string::npos || end == std::string::npos)
				return false;
			str.replace(start + 1, end - start - 1, new_node_name);
			str = trim(str);  // Убираем лишние пробелы после операции
			return true;
		}

		bool change_node_name(std::string& str, const std::string& new_node_name)
		{
			return change_node_name(str, std::string_view(new_node_name));
		}

		std::vector<std::pair<std::string, std::string>> pop_all_attributes(std::string& line)
		{
			std::vector<std::pair<std::string, std::string>> attributes = get_all_attributes(line);
			for (const auto& attr : attributes) {
				pop_attribute(line, attr.first);
			}
			return attributes;
		}

		void add_attribute(std::string& line, const std::string& attribute, const std::string& value)
		{
			std::string new_attribute = " " + attribute + "=\"" + value + "\"";  // Формируем новый атрибут
			size_t start = line.find('<');                                       // Ищем начало узла
			size_t end = line.find('>', start);                                  // Ищем закрывающий тег
			size_t self_closing_pos = line.find("/>", start);                    // Ищем самозакрывающий тег
			// Проверяем, что нашли открывающий тег
			if (start != std::string::npos) {
				// Если найден самозакрывающий тег, используем его позицию, иначе - позицию закрывающего тега
				if (self_closing_pos != std::string::npos && self_closing_pos < end) {
					end = self_closing_pos;
				}

				size_t name_end = line.find_first_of(" \t/>", start + 1);  // Ищем конец имени узла
				if (name_end == std::string::npos) {
					name_end = end;  // Устанавливаем конец на позицию закрывающего тега
				}
				// Вставляем новый атрибут сразу после имени узла
				line.insert(name_end, new_attribute);
			}
		}

		void add_attribute(std::string& line, std::string_view attribute, std::string_view value)
		{
			add_attribute(line, attribute, value);
		}

		void add_attribute_to_end(std::string& line, const std::string& attribute, const std::string& value)
		{
			std::string new_attribute = " " + attribute + "=\"" + value + "\"";  // Формируем новый атрибут
			size_t pos = line.find('>');                                         // Ищем позицию закрывающего тега
			size_t self_closing_pos = line.find("/>", pos);                      // Ищем самозакрывающий тег
			// Если найден самозакрывающий тег, используем его позицию, иначе - позицию закрывающего тега
			if (self_closing_pos != std::string::npos) {
				pos = self_closing_pos;
			}
			// Проверяем, нашли ли закрывающий тег
			if (pos != std::string::npos) {
				line.insert(pos, new_attribute);  // Вставляем новый атрибут перед закрывающим тегом
			}
		}

		void add_attribute_to_end(std::string& line, std::string_view attribute, std::string_view value)
		{
			add_attribute_to_end(line, attribute, value);
		}
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//VIEW
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		std::string_view get_attribute_value(std::string_view line, std::string_view attribute)
		{
			size_t pos = find_attribute_pos(line, attribute);
			if (pos == std::string_view::npos)
				return "";  // Атрибут не найден

			// Находим начало и конец значения атрибута
			size_t start = line.find_first_of("\"'", pos);
			size_t end = line.find_first_of("\"'", start + 1);
			if (start == std::string_view::npos || end == std::string_view::npos)
				return "";  // Ошибка, если кавычки не найдены

			return line.substr(start + 1, end - start - 1);  // Возвращаем значение атрибута
		}

		size_t find_attribute_pos(std::string_view line, std::string_view attribute)
		{
			// Создаем std::string_view для поиска
			std::string_view search = attribute;
			//size_t search_length = search.length() + 1;  // Длина искомой строки с учетом "="

			size_t pos = line.find(search);

			while (pos != std::string_view::npos) {
				// Проверяем, что следующий символ после атрибута - это "="
				if (pos + search.length() < line.size() && line[pos + search.length()] == '=') {
					// Проверяем, что предыдущий символ - пробел, табуляция или это начало строки
					if (pos == 0 || line[pos - 1] == ' ' || line[pos - 1] == '\t') {
						return pos;  // Возвращаем позицию, если проверка прошла успешно
					}
				}
				// Ищем следующий экземпляр атрибута
				pos = line.find(search, pos + 1);
			}

			return std::string_view::npos;  // Атрибут не найден с нужными условиями
		}

		std::vector<std::pair<std::string_view, std::string_view>> get_all_attributes_view(std::string_view line)
		{
			std::vector<std::pair<std::string_view, std::string_view>> attributes;
			size_t pos = 0;

			while (true) {
				// Ищем символ '='
				pos = line.find('=', pos);
				if (pos == std::string_view::npos) {
					break;  // Если не найдено, выходим из цикла
				}

				// Найдем название атрибута (слева от '=')
				size_t attr_end = pos;
				while (attr_end > 0 && (line[attr_end - 1] == ' ' || line[attr_end - 1] == '\t')) {
					--attr_end;  // Пропускаем пробелы
				}

				size_t attr_start = line.find_last_of(" \t", attr_end - 1);
				if (attr_start == std::string_view::npos) {
					attr_start = 0;  // Если пробелов нет, начинаем с начала
				} else {
					++attr_start;  // Пропускаем пробел
				}

				std::string_view attribute = line.substr(attr_start, attr_end - attr_start);

				// Ищем значение атрибута (справа от '=')
				size_t value_start = pos + 2;                    // Пропускаем '="'
				size_t value_end = line.find('"', value_start);  // Находим закрывающую кавычку

				std::string_view value;
				if (value_end != std::string_view::npos) {
					value = line.substr(value_start, value_end - value_start);
					attributes.emplace_back(attribute, value);
					pos = value_end + 1;  // Пропускаем закрывающую кавычку
				} else {
					// Если закрывающая кавычка не найдена, просто пропускаем это значение
					pos = value_start;  // Продолжаем с текущей позиции
				}
			}

			return attributes;
		}
	}

	namespace ini
	{
		// Удаление комментариев
		std::string remove_comments(const std::string& str)
		{
			size_t f = str.find(';');
			return (f == std::string::npos) ? str : str.substr(0, f);
		}
		// Получение секции
		std::string get_section(const std::string& str)
		{
			if (str.length() > 2 && str.front() == '[' && str.back() == ']') {
				return str.substr(1, str.length() - 2);
			}
			return "";
		}
		// Получение пары ключ-значение
		std::pair<std::string, std::string> get_key_value_pair(const std::string& str)
		{
			std::vector<std::string> splited = utils::string::split(str, "=");
			if (splited.size() != 2) {
				throw std::invalid_argument("'get_key_value_pair' : input string must contain exactly one '=' character.");
			}
			std::string key = utils::string::trim(splited[0], " \t");
			std::string value = utils::string::trim(splited[1], " \t");
			return std::make_pair(key, value);
		}

		void set_value(const std::filesystem::path& filename, const std::string& section, const std::string& key, const std::string& value)
		{
			if (!utils::file::file_exists(filename.string())) {
				std::cerr << "INI file does not exist: " << filename << std::endl;
				return;  // Файл не существует
			}

			std::ifstream file(filename);
			std::stringstream buffer;
			buffer << file.rdbuf();              // Читаем содержимое файла в строковый поток
			std::string content = buffer.str();  // Получаем строку из потока
			std::string section_str = "[" + std::string(section) + "]\n";

			size_t section_pos = content.find(section_str);
			bool section_found = (section_pos != std::string::npos);

			if (!section_found) {
				// Если секция не найдена, добавляем её в конец файла
				content += section_str;
				section_pos = content.length();  // Обновляем позицию секции
			}

			// Найти конец секции
			size_t section_end = content.find('\n', section_pos + section_str.length());
			size_t key_pos = std::string::npos;
			// Если секция найдена, ищем ключ
			if (section_found) {
				key_pos = content.find((std::string(key) + "=").c_str(), section_pos, section_end - section_pos);
			}
			if (key_pos == std::string::npos) {
				// Если ключ не найден, добавляем его в секцию
				content.insert(section_end, std::string(key) + "=" + std::string(value) + "\n");
			} else {
				// Обновить значение ключа, если он найден
				size_t value_start = content.find('=', key_pos) + 1;
				size_t value_end = content.find(';', value_start);
				if (value_end == std::string::npos) {
					value_end = content.find('\n', value_start);
				}
				content.replace(value_start, value_end - value_start, value.data(), value.length());
			}
			// Записать обновленное содержимое обратно в файл
			std::ofstream out_file(filename);
			out_file << content;
		}
	}

	namespace file
	{
		bool file_exists(std::string_view filename)
		{
			return fs::exists(filename);
		}

		// Основная функция для открытия файла
		std::unique_ptr<std::fstream> open_file_impl(std::string_view filename, FileMode mode, bool binary)
		{
			if (mode != FileMode::Read && mode != FileMode::Write && mode != FileMode::Append) {
				std::cerr << "'open_file' : wrong file opening mode" << std::endl;
				return nullptr;  // Вернуть nullptr в случае ошибки
			}
			auto file = std::make_unique<std::fstream>();
			std::ios_base::openmode openMode = std::ios::in;  // По умолчанию открываем для чтения
			openMode = static_cast<std::ios_base::openmode>(mode);
			if (binary) {
				openMode |= std::ios::binary;
			}
			file->open(filename.data(), openMode);  // Используем .data() для получения указателя на строку
			if (!file->is_open()) {
				std::cerr << "'open_file' : failed to open file : " << filename << std::endl;
				return nullptr;  // Вернуть nullptr в случае неудачи
			}
			return file;  // Возвращаем уникальный указатель на fstream
		}

		 // Перегруженная функция, принимающая fs::path
		std::unique_ptr<std::fstream> open_file(const fs::path& filename, FileMode mode, bool binary)
		{
			return open_file_impl(filename.string(), mode, binary);
		}
		// Функция, принимающая std::string_view
		std::unique_ptr<std::fstream> open_file(std::string_view filename, FileMode mode, bool binary)
		{
			return open_file_impl(filename, mode, binary);
		}

		bool copy_file(std::string_view source, std::string_view destination, bool overwrite)
		{
			try {
				fs::copy_options options = overwrite ? fs::copy_options::overwrite_existing : fs::copy_options::none;
				fs::copy(source, destination, options);
				return true;  // Копирование прошло успешно
			} catch (const fs::filesystem_error& e) {
				std::cerr << "'copy_file' : error when copying a file: " << e.what() << std::endl;
				return false;  // Произошла ошибка при копировании
			}
		}

		bool delete_file(const fs::path& filename)
		{
			try {
				if (fs::remove(filename)) {
					return true;  // Удаление успешно
				} else {
					std::cerr << "'delete_file' : File '" << filename << "' does not exist." << std::endl;
					return false;  // Файл не существует
				}
			} catch (const fs::filesystem_error& e) {
				std::cerr << "'delete_file' : failed to delete the file '" << filename << "': " << e.what() << std::endl;
				return false;  // Удаление не удалось
			}
		}

		bool move_file(const fs::path& old_filename, const fs::path& new_filename)
		{
			try {
				fs::rename(old_filename, new_filename);
				return true;  // Перемещение успешно
			} catch (const fs::filesystem_error& e) {
				std::cerr << "'move_file' : failed to move file '" << old_filename << "' to '" << new_filename << "': " << e.what() << std::endl;
				return false;  // Перемещение не удалось
			}
		}

		// Функция для получения всех файлов в каталоге
		std::vector<fs::path> get_files_in_directory(const fs::path& directory, bool only_files, std::string_view extension)
		{
			std::vector<fs::path> files;
			// Проверяем, существует ли каталог
			if (!file_exists(directory.string())) {
				std::cerr << "Directory does not exist: " << directory << std::endl;
				return files;  // Возвращаем пустой вектор, если каталог не существует
			}
			// Проходим по всем элементам в каталоге
			for (const auto& entry : fs::directory_iterator(directory)) {
				// Проверяем, является ли элемент файлом
				if (only_files && !fs::is_regular_file(entry.status())) {
					continue;  // Пропускаем, если нужно только файлы
				}
				// Если указано расширение, проверяем его
				if (!extension.empty() && entry.path().extension() != extension) {
					continue;  // Пропускаем, если расширение не совпадает
				}
				// Добавляем файл в вектор
				files.push_back(entry.path());
			}
			return files;  // Возвращаем вектор с найденными файлами
		}
	}
}
