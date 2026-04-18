#pragma once
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <utility>
#include <stdexcept>
#include <filesystem>

namespace utils
{
	namespace fs = std::filesystem;
	using namespace std::string_literals;
	using namespace std::string_view_literals;

	enum class FileMode
	{
		Read = std::ios::in,                      // Открыть для чтения
		Write = std::ios::out | std::ios::trunc,  // Стереть файл и открыть для записи
		Append = std::ios::out | std::ios::app    // Открыть для добавления новых записей
	};

	namespace file
	{
		bool file_exists(std::string_view filename);
		std::unique_ptr<std::fstream> open_file(const fs::path& filename, FileMode mode = FileMode::Read, bool binary = false);
		std::unique_ptr<std::fstream> open_file(std::string_view filename, FileMode mode = FileMode::Read, bool binary = false);
		bool copy_file(std::string_view source, std::string_view destination, bool overwrite = false);
		bool delete_file(const fs::path& filename);
		bool move_file(const fs::path& old_filename, const fs::path& new_filename);
		std::vector<fs::path> get_files_in_directory(const fs::path& directory, bool only_files = false, std::string_view extension = "");
	}
	
	namespace string
	{
		std::vector<std::string> split(const std::string& fullString, const std::string& delimiter);
		std::vector<std::string> split(const std::string_view& fullString, const std::string& delimiter);
		bool replace(std::string& from, const std::string& remove, const std::string& add);
		std::string to_lower(const std::string& str);
		std::string to_lower(std::string_view str);
		void to_lower(std::string& str);
		std::string clean_string_from_BOM(const std::string& str);

		std::string& ltrim(std::string&, const std::string& chars = " \t");
		std::string& rtrim(std::string&, const std::string& chars = " \t");
		std::string& trim(std::string&, const std::string& chars = " \t");
		
	}

	namespace view
	{
		std::vector<std::string_view> split(std::string_view fullString, std::string_view delimiter);
		std::string_view ltrim(std::string_view str, const std::string_view chars = " \t");
		std::string_view rtrim(std::string_view str, const std::string_view chars = " \t");
		std::string_view trim(std::string_view str, const std::string_view chars = " \t");
	}

	namespace xml
	{
		std::string trim(const std::string& str);
		std::string replace_xml_escape_sequences(const std::string& input);
		std::string replace_xml_escape_sequences(const std::string& input);
		std::string replace_xml_escape_sequences(std::string_view input);
		std::string escape_xml_characters(const std::string& input);
		std::string escape_xml_characters(std::string_view input);
		bool remove_inline_spaces(std::string& line);
		size_t find_attribute_pos(const std::string& line, const std::string& attribute);
		std::string pop_attribute(std::string& line, const std::string& attribute);
		std::string_view get_attribute_value(std::string_view line, std::string_view attribut);
		std::string get_attribute_value(const std::string& line, const std::string& attribute);
		std::vector<std::pair<std::string, std::string>> get_all_attributes(const std::string& line);
		std::string get_node_name(const std::string& line);
		std::string pop_node_name(std::string& str);
		bool change_node_name(std::string& str, const std::string& new_node_name);
		bool change_node_name(std::string& str, std::string_view new_node_name);
		std::vector<std::pair<std::string, std::string>> pop_all_attributes(std::string& line);
		void add_attribute(std::string& line, const std::string& attribute, const std::string& value);
		void add_attribute(std::string& line, std::string_view attribute, std::string_view value);
		void add_attribute_to_end(std::string& line, const std::string& attribute, const std::string& value);
		void add_attribute_to_end(std::string& line, std::string_view attribute, std::string_view value);
		
		//string view
		std::string_view get_attribute_value(std::string_view line, std::string_view attribute);
		size_t find_attribute_pos(std::string_view line, std::string_view attribute);
		std::vector<std::pair<std::string_view, std::string_view>> get_all_attributes_view(std::string_view line);
	}

	namespace ini
	{
		std::string remove_comments(const std::string&);
		std::string get_section(const std::string&);
		std::pair<std::string, std::string> get_key_value_pair(const std::string& str);
		void set_value(const fs::path& filename, const std::string& section, const std::string& key, const std::string& value);
	}
}
