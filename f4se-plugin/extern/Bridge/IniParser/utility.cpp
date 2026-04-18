#include "utility.h"

namespace utils
{
	namespace string
	{
		/// @brief Разделяет строку на подстроки по разделителю. Разделитель не возвращается в результат.
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

		/// @brief Разделяет строку на подстроки по разделителю (std::string_view). Разделитель не возвращается в результат.
		std::vector<std::string> split(const std::string_view& fullString, const std::string& delimiter)
		{
			return split(std::string(fullString), delimiter);
		}

		/// @brief Удаляет указанные символы слева строки.
		std::string& ltrim(std::string& str, const std::string& chars)
		{
			str.erase(0, str.find_first_not_of(chars));
			return str;
		}

		/// @brief Удаляет указанные символы справа строки.
		std::string& rtrim(std::string& str, const std::string& chars)
		{
			str.erase(str.find_last_not_of(chars) + 1);
			return str;
		}

		/// @brief Удаляет указанные символы слева и справа строки.
		std::string& trim(std::string& str, const std::string& chars)
		{
			return ltrim(rtrim(str, chars), chars);
		}

		/// @brief Заменяет первую найденную подстроку remove на add в строке line.
		/// @return true если подстрока была заменена, false если remove пустой или не найден.
		bool replace(std::string& line, const std::string& remove, const std::string& add)
		{
			if (remove.empty())
				return false;
			size_t pos = line.find(remove);
			if (pos != std::string::npos) {
				line.replace(pos, remove.length(), add);
				return true;
			}
			return false;
		}

		/// @brief Возвращает строку в нижнем регистре (std::string_view).
		std::string to_lower(std::string_view str)
		{
			std::string copy(str);
			std::transform(copy.begin(), copy.end(), copy.begin(),
				[](unsigned char c) { return std::tolower(c); });
			return copy;
		}

		/// @brief Преобразует строку в нижний регистр (in-place).
		void to_lower(std::string& str)
		{
			std::transform(str.begin(), str.end(), str.begin(),
				[](unsigned char c) { return std::tolower(c); });
		}

		/// @brief Возвращает копию строки в нижнем регистре.
		std::string to_lower(const std::string& str)
		{
			std::string copy(str);
			std::transform(copy.begin(), copy.end(), copy.begin(),
				[](unsigned char c) { return std::tolower(c); });
			return copy;
		}

		/// @brief Удаляет BOM (Byte Order Mark) из начала строки, если он есть.
		std::string clean_string_from_BOM(const std::string& str)
		{
			if (str.size() >= 3 &&
				(unsigned char)str[0] == 0xEF &&
				(unsigned char)str[1] == 0xBB &&
				(unsigned char)str[2] == 0xBF) {
				return str.substr(3);
			}
			return str;
		}
	}

	namespace view
	{
		/// @brief Разделяет строку на подстроки по разделителю, возвращает std::string_view.
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

		/// @brief Удаляет указанные символы слева строки (view).
		std::string_view ltrim(std::string_view str, const std::string_view chars)
		{
			return str.substr(str.find_first_not_of(chars));
		}

		/// @brief Удаляет указанные символы справа строки (view).
		std::string_view rtrim(std::string_view str, const std::string_view chars)
		{
			return str.substr(0, str.find_last_not_of(chars) + 1);
		}

		/// @brief Удаляет указанные символы слева и справа строки (view).
		std::string_view trim(std::string_view str, const std::string_view chars)
		{
			return ltrim(rtrim(str, chars), chars);
		}
	}

	namespace xml
	{
		/// @brief Удаляет пробелы и табуляцию слева и справа строки.
		std::string trim(const std::string& str)
		{
			size_t first = str.find_first_not_of(" \t");
			size_t last = str.find_last_not_of(" \t");
			return (first == std::string::npos) ? "" : str.substr(first, (last - first + 1));
		}

		/// @brief Заменяет escape-последовательности XML на символы.
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

		/// @brief Заменяет escape-последовательности XML на символы (перегрузка для std::string_view).
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

		/// @brief Экранирует специальные символы XML в строке.
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

		/// @brief Экранирует специальные символы XML в строке (перегрузка для std::string_view).
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
				}
				else {
					output += ch;
				}
			}
			return output;
		}

		/// @brief Удаляет лишние пробелы вне кавычек, заменяет табуляцию на пробел.
		bool remove_inline_spaces(std::string& line)
		{
			if (line.empty())
				return false;
			std::string result;
			bool in_quotes = false;
			bool last_was_space = false;
			for (size_t i = 0; i < line.length(); ++i) {
				char current = line[i];
				if (current == '\"') {
					in_quotes = !in_quotes;
				}
				if (!in_quotes) {
					if (current == '<' || current == '>') {
						result += current;
						last_was_space = false;
						continue;
					}
				}
				if (current == '\t') {
					current = ' ';
				}
				if (current == ' ') {
					if (!last_was_space) {
						result += current;
						last_was_space = true;
					}
				}
				else {
					result += current;
					last_was_space = false;
				}
			}
			line = result;
			return true;
		}

		/// @brief Находит позицию атрибута в строке XML.
		size_t find_attribute_pos(const std::string& line, const std::string& attribute)
		{
			std::string search = attribute + "=";
			size_t pos = line.find(search);
			while (pos != std::string::npos) {
				if (pos == 0 || line[pos - 1] == ' ' || line[pos - 1] == '\t') {
					return pos;
				}
				pos = line.find(search, pos + search.length());
			}
			return std::string::npos;
		}

		/// @brief Извлекает значение атрибута из строки и удаляет его.
		std::string pop_attribute(std::string& line, const std::string& attribute)
		{
			size_t pos = find_attribute_pos(line, attribute);
			if (pos == std::string::npos)
				return "";
			size_t start = line.find_first_of("\"'", pos);
			size_t end = line.find_first_of("\"'", start + 1);
			if (start == std::string::npos || end == std::string::npos)
				return "";
			std::string value = line.substr(start + 1, end - start - 1);
			line.erase(pos, end - pos + 1);
			line = trim(line);
			return value;
		}

		/// @brief Получает значение атрибута из строки.
		std::string get_attribute_value(const std::string& line, const std::string& attribute)
		{
			size_t pos = find_attribute_pos(line, attribute);
			if (pos == std::string::npos)
				return "";
			size_t start = line.find_first_of("\"'", pos);
			size_t end = line.find_first_of("\"'", start + 1);
			if (start == std::string::npos || end == std::string::npos)
				return "";
			return line.substr(start + 1, end - start - 1);
		}

		/// @brief Получает все атрибуты и их значения из строки XML.
		std::vector<std::pair<std::string, std::string>> get_all_attributes(const std::string& line)
		{
			//СОМНЕНИЕ: функция ищет все слова с '=', но для значения всегда берет get_attribute_value(line, attribute), 
			// то есть для каждого слова ищет значение в исходной строке, а не только в этом слове. Это может возвращать 
			// неверные значения, если встречается несколько одинаковых атрибутов или невалидные атрибуты.
			// 	
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

		/// @brief Получает имя узла XML из строки.
		std::string get_node_name(const std::string& line)
		{
			size_t start = line.find('<');
			size_t end = line.find('>', start);
			if (start == std::string::npos || end == std::string::npos) {
				return "";
			}
			if (start + 1 >= line.size()) {
				return "";
			}
			size_t name_start = (line[start + 1] == '/') ? start + 2 : start + 1;
			size_t name_end = line.find_first_of(" \t/>", name_start);
			if (name_end == std::string::npos) {
				name_end = end;
			}
			if (name_start < name_end) {
				return line.substr(name_start, name_end - name_start);
			}
			return "";
		}

		/// @brief Извлекает имя узла из строки и удаляет его.
		std::string pop_node_name(std::string& str)
		{
			std::string node_name = get_node_name(str);
			size_t start = str.find('<');
			size_t end = str.find('>', start);
			str.erase(start, end - start + 1);
			str = trim(str);
			return node_name;
		}

		/// @brief Заменяет имя узла XML на новое (string_view).
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
			str = trim(str);
			return true;
		}

		/// @brief Заменяет имя узла XML на новое (std::string).
		bool change_node_name(std::string& str, const std::string& new_node_name)
		{
			return change_node_name(str, std::string_view(new_node_name));
		}

		/// @brief Извлекает все атрибуты из строки и удаляет их.
		std::vector<std::pair<std::string, std::string>> pop_all_attributes(std::string& line)
		{
			std::vector<std::pair<std::string, std::string>> attributes = get_all_attributes(line);
			for (const auto& attr : attributes) {
				pop_attribute(line, attr.first);
			}
			return attributes;
		}

		/// @brief Добавляет атрибут к узлу XML непосредственно после имени узла.
		void add_attribute(std::string& line, const std::string& attribute, const std::string& value)
		{
			std::string new_attribute = " " + attribute + "=\"" + value + "\"";
			size_t start = line.find('<');
			size_t end = line.find('>', start);
			size_t self_closing_pos = line.find("/>", start);
			if (start != std::string::npos) {
				if (self_closing_pos != std::string::npos && self_closing_pos < end) {
					end = self_closing_pos;
				}
				size_t name_end = line.find_first_of(" \t/>", start + 1);
				if (name_end == std::string::npos) {
					name_end = end;
				}
				line.insert(name_end, new_attribute);
			}
		}

		/// @brief Добавляет атрибут к узлу XML (string_view).
		void add_attribute(std::string& line, std::string_view attribute, std::string_view value)
		{
			add_attribute(line, attribute, value);
		}

		/// @brief Добавляет атрибут к узлу XML перед закрывающим тегом.
		void add_attribute_to_end(std::string& line, const std::string& attribute, const std::string& value)
		{
			std::string new_attribute = " " + attribute + "=\"" + value + "\"";
			size_t pos = line.find('>');
			size_t self_closing_pos = line.find("/>", pos);
			if (self_closing_pos != std::string::npos) {
				pos = self_closing_pos;
			}
			if (pos != std::string::npos) {
				line.insert(pos, new_attribute);
			}
		}

		/// @brief Добавляет атрибут к узлу XML перед закрывающим тегом (string_view).
		void add_attribute_to_end(std::string& line, std::string_view attribute, std::string_view value)
		{
			add_attribute_to_end(line, attribute, value);
		}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//VIEW
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/// @brief Получает значение атрибута из строки XML (string_view).
		std::string_view get_attribute_value(std::string_view line, std::string_view attribute)
		{
			size_t pos = find_attribute_pos(line, attribute);
			if (pos == std::string_view::npos)
				return "";
			size_t start = line.find_first_of("\"'", pos);
			size_t end = line.find_first_of("\"'", start + 1);
			if (start == std::string_view::npos || end == std::string_view::npos)
				return "";
			return line.substr(start + 1, end - start - 1);
		}

		/// @brief Находит позицию атрибута в строке XML (string_view).
		size_t find_attribute_pos(std::string_view line, std::string_view attribute)
		{
			std::string_view search = attribute;
			size_t pos = line.find(search);
			while (pos != std::string_view::npos) {
				if (pos + search.length() < line.size() && line[pos + search.length()] == '=') {
					if (pos == 0 || line[pos - 1] == ' ' || line[pos - 1] == '\t') {
						return pos;
					}
				}
				pos = line.find(search, pos + 1);
			}
			return std::string_view::npos;
		}

		/// @brief Получает все атрибуты и их значения из строки XML (string_view).
		std::vector<std::pair<std::string_view, std::string_view>> get_all_attributes_view(std::string_view line)
		{
			std::vector<std::pair<std::string_view, std::string_view>> attributes;
			size_t pos = 0;
			while (true) {
				pos = line.find('=', pos);
				if (pos == std::string_view::npos) {
					break;
				}
				size_t attr_end = pos;
				while (attr_end > 0 && (line[attr_end - 1] == ' ' || line[attr_end - 1] == '\t')) {
					--attr_end;
				}
				size_t attr_start = line.find_last_of(" \t", attr_end - 1);
				if (attr_start == std::string_view::npos) {
					attr_start = 0;
				}
				else {
					++attr_start;
				}
				std::string_view attribute = line.substr(attr_start, attr_end - attr_start);
				size_t value_start = pos + 2;
				size_t value_end = line.find('"', value_start);
				std::string_view value;
				if (value_end != std::string_view::npos) {
					value = line.substr(value_start, value_end - value_start);
					attributes.emplace_back(attribute, value);
					pos = value_end + 1;
				}
				else {
					pos = value_start;
				}
			}
			return attributes;
		}
	}

	namespace ini
	{
		/// @brief Удаляет комментарии (все после ';') из строки.
		std::string remove_comments(const std::string& str)
		{
			size_t f = str.find(';');
			return (f == std::string::npos) ? str : str.substr(0, f);
		}

		/// @brief Возвращает имя секции, если строка имеет вид [section].
		std::string get_section(const std::string& str)
		{
			if (str.length() > 2 && str.front() == '[' && str.back() == ']') {
				return str.substr(1, str.length() - 2);
			}
			return "";
		}

		/// @brief Получает пару ключ-значение из строки вида key=value.
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

		/// @brief Устанавливает/добавляет значение ключа в секции ini-файла.
		void set_value(const std::filesystem::path& filename, const std::string& section, const std::string& key, const std::string& value)
		{
			// корректно работает только если ключ в секции на одной строке и не учитывает многострочные 
			// значения или сложные случаи. Но в рамках базового INI-парсинга подходит.
			
			if (!utils::file::file_exists(filename.string())) {
				std::cerr << "INI file does not exist: " << filename << std::endl;
				return;
			}
			std::ifstream file(filename);
			std::stringstream buffer;
			buffer << file.rdbuf();
			std::string content = buffer.str();
			std::string section_str = "[" + std::string(section) + "]\n";
			size_t section_pos = content.find(section_str);
			bool section_found = (section_pos != std::string::npos);
			if (!section_found) {
				content += section_str;
				section_pos = content.length();
			}
			size_t section_end = content.find('\n', section_pos + section_str.length());
			size_t key_pos = std::string::npos;
			if (section_found) {
				key_pos = content.find((std::string(key) + "=").c_str(), section_pos, section_end - section_pos);
			}
			if (key_pos == std::string::npos) {
				content.insert(section_end, std::string(key) + "=" + std::string(value) + "\n");
			}
			else {
				size_t value_start = content.find('=', key_pos) + 1;
				size_t value_end = content.find(';', value_start);
				if (value_end == std::string::npos) {
					value_end = content.find('\n', value_start);
				}
				content.replace(value_start, value_end - value_start, value.data(), value.length());
			}
			std::ofstream out_file(filename);
			out_file << content;
		}
	}

	namespace file
	{
		/// @brief Проверяет существование файла.
		bool file_exists(std::string_view filename)
		{
			return fs::exists(filename);
		}

		/// @brief Открывает файл с заданным режимом и бинарностью. Возвращает уникальный указатель на fstream.
		std::unique_ptr<std::fstream> open_file_impl(std::string_view filename, FileMode mode, bool binary)
		{
			if (mode != FileMode::Read && mode != FileMode::Write && mode != FileMode::Append) {
				std::cerr << "'open_file' : wrong file opening mode" << std::endl;
				return nullptr;
			}
			auto file = std::make_unique<std::fstream>();
			std::ios_base::openmode openMode = std::ios::in;
			openMode = static_cast<std::ios_base::openmode>(mode);
			if (binary) {
				openMode |= std::ios::binary;
			}
			file->open(filename.data(), openMode);
			if (!file->is_open()) {
				std::cerr << "'open_file' : failed to open file : " << filename << std::endl;
				return nullptr;
			}
			return file;
		}

		/// @brief Открывает файл по fs::path с заданным режимом и бинарностью.
		std::unique_ptr<std::fstream> open_file(const fs::path& filename, FileMode mode, bool binary)
		{
			return open_file_impl(filename.string(), mode, binary);
		}

		/// @brief Открывает файл по string_view с заданным режимом и бинарностью.
		std::unique_ptr<std::fstream> open_file(std::string_view filename, FileMode mode, bool binary)
		{
			return open_file_impl(filename, mode, binary);
		}

		/// @brief Копирует файл из source в destination. Если overwrite=true, перезаписывает.
		bool copy_file(std::string_view source, std::string_view destination, bool overwrite)
		{
			try {
				fs::copy_options options = overwrite ? fs::copy_options::overwrite_existing : fs::copy_options::none;
				fs::copy(source, destination, options);
				return true;
			}
			catch (const fs::filesystem_error& e) {
				std::cerr << "'copy_file' : error when copying a file: " << e.what() << std::endl;
				return false;
			}
		}

		/// @brief Удаляет файл.
		bool delete_file(const fs::path& filename)
		{
			try {
				if (fs::remove(filename)) {
					return true;
				}
				else {
					std::cerr << "'delete_file' : File '" << filename << "' does not exist." << std::endl;
					return false;
				}
			}
			catch (const fs::filesystem_error& e) {
				std::cerr << "'delete_file' : failed to delete the file '" << filename << "': " << e.what() << std::endl;
				return false;
			}
		}

		/// @brief Перемещает файл.
		bool move_file(const fs::path& old_filename, const fs::path& new_filename)
		{
			try {
				fs::rename(old_filename, new_filename);
				return true;
			}
			catch (const fs::filesystem_error& e) {
				std::cerr << "'move_file' : failed to move file '" << old_filename << "' to '" << new_filename << "': " << e.what() << std::endl;
				return false;
			}
		}

		/// @brief Получает список файлов в каталоге. Если only_files=true, только файлы. Можно фильтровать по расширению.
		std::vector<fs::path> get_files_in_directory(const fs::path& directory, bool only_files, std::string_view extension)
		{
			std::vector<fs::path> files;
			if (!file_exists(directory.string())) {
				std::cerr << "Directory does not exist: " << directory << std::endl;
				return files;
			}
			for (const auto& entry : fs::directory_iterator(directory)) {
				if (only_files && !fs::is_regular_file(entry.status())) {
					continue;
				}
				if (!extension.empty() && entry.path().extension() != extension) {
					continue;
				}
				files.push_back(entry.path());
			}
			return files;
		}
	}
}