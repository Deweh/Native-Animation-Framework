#pragma once
#include <boost/algorithm/string.hpp>
#include <filesystem>
#include <forward_list>
#include <fstream>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>
#include <regex>

using namespace std::string_literals;

//settings
#include "F4SE/F4SE.h"
#include "F4SE/Logger.h"

namespace logger = F4SE::log;

//end

namespace utils
{
	inline bool copy_file(const std::string& SRC, const std::string& DEST)
	{
		if (std::filesystem::exists(SRC)) {
			std::ifstream src(SRC, std::ios::binary);
			std::ofstream dest(DEST, std::ios::binary);
			if (src.is_open() && dest.is_open()) {
				dest << src.rdbuf();
				return src && dest;
			}
		}
		return false;
	}

	inline void delim(const std::string& string_line, const std::string delim, std::vector<std::string>& result_arr)
	{
		if (!result_arr.empty())
			result_arr.clear();
		size_t pos = 0;
		size_t pos_last = 0;
		bool _continue = true;
		while (_continue) {
			pos = string_line.find(delim, pos_last);
			pos = pos == std::string::npos ? _continue = false, string_line.find(delim, '\x0') : pos;
			pos = pos == std::string::npos ? _continue = false, string_line.find(delim, '\r') : pos;

			result_arr.push_back(string_line.substr(pos_last, pos - pos_last));
			pos_last = pos + delim.length();
		}
	}

	inline std::string& remove_escape_sequence(std::string& str)
	{
		if (str.empty())
			return str;
		std::vector<std::string> esc = { "&quot;", "&apos;", "&lt;", "&gt;", "&amp;" };
		std::vector<std::string> symb = { "\"", "\'", "<", ">", "&" };
		size_t n = 0;
		for (auto& el : esc) {
			size_t f = str.find(el);
			if (f != std::string::npos) {
				str.erase(f, el.size());
				str.insert(f, symb[n].c_str());
			}
			++n;
		}
		return str;
	}

	inline std::string get_escape_sequence(const char& c)
	{
		switch (c) {
		case '\"':
			return "&quot;"s;
		case '\'':
			return "&apos;"s;
		case '<':
			return "&lt;"s;
		case '>':
			return "&gt;"s;
		case '&':
			return "&amp;"s;
		default:
			return ""s + c;
		}
	}

	inline std::string get_escape_sequence(std::string sequence)
	{
		if (sequence == "\"")
			return get_escape_sequence('"');
		else if (sequence == "\'")
			return get_escape_sequence('\'');
		else if (sequence == "<")
			return get_escape_sequence('<');
		else if (sequence == ">")
			return get_escape_sequence('>');
		else if (sequence == "&")
			return get_escape_sequence('&');
		else if (sequence == "&quot;"s)
			return "\'"s;
		else if (sequence == "&apos;"s)
			return "\'"s;
		else if (sequence == "&lt;"s)
			return "<"s;
		else if (sequence == "&gt;"s)
			return ">"s;
		else if (sequence == "&amp;"s)
			return "&"s;
		else
			return sequence;
	}

	inline std::string& replace_escape_symbols_with_sequences(std::string& str)
	{
		auto fix_incorrect_sequence_symbols = [](std::string& str) {
			if (str.empty())
				return;
			auto escape_symbols_for_attributes = { '\"', '\'', '<', '>', '&' };

			auto isEscape = [](std::string& str, size_t& start) {
				if (start + 1 > str.length() - 1)
					return false;
				std::string s;
				for (auto it = str.begin() + start; it != str.end() && *it != ';'; ++it) {
					s += *it;
				}
				if (s == "&quot"s || s == "&apos"s || s == "&lt"s || s == "&gt"s || s == "&amp"s)
					return true;
				return false;
			};

			for (auto& e_symb : escape_symbols_for_attributes) {
				for (size_t f = str.find(e_symb); f != std::string::npos; f = str.find(e_symb, f)) {
					if (e_symb == '&' && isEscape(str, f)) {
						f += 1;
						continue;
					}
					str.erase(f, 1);
					;
					str.insert(f, utils::get_escape_sequence(e_symb).c_str());
				}
			}
		};

		if (str.empty())
			return str;

		fix_incorrect_sequence_symbols(str);
		return str;
	};

	inline std::string& remove_all_spaces(std::string& str)
	{
		if (str.empty())
			return str;
		bool skip = false;
		size_t c = 0;
		for (auto el : str) {
			if (el == '\"')
				skip = !skip;
			if (!skip) {
				if (el == '\t' || el == ' ') {
					str.erase(c, 1);
				}
			}
			++c;
		}
		return str;
	}

	inline bool replace(std::string& line, const std::string& remove, const std::string& add)
	{
		if (line.empty() || remove.empty())
			return false;
		size_t f = line.find(remove);
		if (f != std::string::npos) {
			line.erase(f, remove.length());
			line.insert(f, add);
			return true;
		}
		return false;
	}

	inline bool replace(std::string* line, const std::string& remove, const std::string& add)
	{
		if (line)
			return replace(*line, remove, add);
		return false;
	}

	inline std::string& remove_spaces_from_left(std::string& str)
	{
		if (str.empty())
			return str;

		for (auto el = str.begin(); el != str.end(); ++el) {
			if (*el == '\t' || *el == ' ')
				str.erase(el);
			else
				break;
		}

		return str;
	}

	inline std::string* remove_spaces_from_left(std::string* str)
	{
		if (str)
			return &remove_spaces_from_left(*str);
		return str;
	}

	inline std::string& remove_spaces_from_right(std::string& str)
	{
		if (str.empty())
			return str;

		for (auto el = (str.end() - 1); el != str.begin(); --el) {
			if (*el == '\t' || *el == ' ')
				str.erase(el);
			else
				break;
		}
		if (!str.empty() && (*str.begin() == '\t' || *str.begin() == ' '))
			str.erase(str.begin());

		return str;
	}

	inline std::string* remove_spaces_from_right(std::string* str)
	{
		if (str)
			return &remove_spaces_from_right(*str);
		return str;
	}

	inline std::string& remove_spaces_from_sides(std::string& str)
	{
		if (str.empty())
			return str;

		remove_spaces_from_left(str);
		remove_spaces_from_right(str);

		return str;
	}

	inline std::string* remove_spaces_from_sides(std::string* str)
	{
		if (str)
			return &remove_spaces_from_sides(*str);
		return str;
	}

	inline bool remove_inline_spaces(std::string& line)
	{
		if (line.empty())
			return false;

		enum state
		{
			PREPARE,
			PROCCESS,
			VALUE
		};

		state s = PREPARE;
		auto i = line.begin();

		auto proccess = [&]() {
			auto next = i + 1;
			if ((*i == '<') && *next == ' ') {
				line.erase(next);
				return true;
			} else if (*i == ' ' && *next == ' ') {
				line.erase(i);
				return true;
			} else if (*i == '\t' && (*next == ' ' || *next == '\t' || *next == '\r' || *next == '\n')) {
				line.erase(next);
				return true;
			} else if (*i == ' ' && (*next == '/' || *next == '>')) {
				line.erase(i);
				return true;
			}
			return false;
		};

		while (i < (line.end() - 1)) {
			switch (s) {
			case PREPARE:
				if (*i == '<') {
					s = PROCCESS;
					if (proccess())
						break;
				}
				++i;
				break;
			case VALUE:
				if (*i == '\"') {
					s = PROCCESS;
				}
				++i;
				break;
			case PROCCESS:
				if (*i == '\"') {
					s = VALUE;
				} else {
					if (proccess())
						break;
				}
				++i;
			}
		}
		return true;
	}

	inline bool remove_inline_spaces(std::string* line)
	{
		if (line)
			return remove_inline_spaces(*line);
		return false;
	}

	//inline auto find_attribute_pos(const std::string& line, const std::string& attribute) 
	//{
	//	enum
	//	{
	//		SEARCH,
	//		VALUE,
	//		FORWARD
	//	};
	//	auto state = SEARCH;

	//	static std::vector<char> break_chars = { ' ', '\"', '<', '>', '\t' };

	//	auto isBreak = [&](const char ch) {
	//		for (auto& c : break_chars) {
	//			if (c == ch)
	//				return true;
	//		}
	//		return false;
	//	};

	//	auto a_it = attribute.end() - 1;
	//	auto a_end = attribute.begin() - 1;

	//	auto reset = [&]() {
	//		a_it = attribute.end() - 1;
	//	};

	//	for (auto it = line.end() - 1, end = line.begin() - 1; it != end; --it) {
	//		auto compare_finished = [&]() {
	//			if (a_it == a_end + 1 && *a_it == *it) {
	//				if ((it - 1) == line.end())
	//					return true;
	//				else if (isBreak(*(it - 1))) {
	//					return true;
	//				} else {
	//					reset();
	//					return false;
	//				}
	//			} else if (*it == *a_it) {
	//				return false;
	//			} else {
	//				reset();
	//				return false;
	//			}
	//		};

	//		switch (state)
	//		{
	//		case SEARCH:
	//			if (*it == '\"') {
	//				reset();
	//				state = VALUE;
	//			}
	//			--a_it;
	//			break;
	//		case VALUE:
	//			if (*it == '\"') {
	//				state = SEARCH;
	//				if (compare_finished())
	//					return it;
	//			}
	//			break;
	//		case FORWARD:
	//			if (*it == '=') {
	//				state = SEARCH;
	//			} else if (*it == '\"') {
	//				state = VALUE;
	//			}
	//		}
	//		--it;
	//	}
	//	return line.end();
	//}


	inline size_t find_attribute_pos(const std::string& line, const std::string& attribute) 
	{
		if (line.empty() || attribute.empty())
			return std::string::npos;
		for (size_t start = line.find(attribute + "=\""); start != std::string::npos; start = line.find(attribute + "=\"", ++start)) {
			if (start != std::string::npos) {
				try {
					if ((start == 0) || (line.at(start - 1) == ' ' || line.at(start - 1) == '\t')) {
						return start;
					}
				} catch (...) {
					logger::error("'find_attribute_pos' : exception in line : {}, attribute {} ", line, attribute);
					return std::string::npos;
				}
			}
		}
		return std::string::npos;
	}

	inline size_t remove_attribute(std::string& line, const std::string& attribute)
	{
		if (line.empty() || attribute.empty()) {
			return std::string::npos;  // Возвращаем npos, если строка или атрибут пусты
		}

		auto start = find_attribute_pos(line, attribute);
		if (start == std::string::npos) {
			return std::string::npos;  // Атрибут не найден
		}

		// Найдем позицию открытия и закрытия кавычек
		size_t quote_start = line.find('"', start);
		if (quote_start == std::string::npos) {
			return std::string::npos;  // Не найдены кавычки
		}

		size_t quote_end = line.find('"', quote_start + 1);
		if (quote_end == std::string::npos) {
			return std::string::npos;  // Не найдены закрывающие кавычки
		}

		// Удаляем атрибут вместе с кавычками
		line.erase(quote_start, quote_end - quote_start + 1);  // Удаляем кавычки и содержимое атрибута
		// Удаляем пробелы после атрибута
		line.erase(start, quote_start - start);  // Удаляем пробелы перед атрибутом

		return start;  // Возвращаем позицию, с которой был удален атрибут
	}

	inline size_t remove_attribute(std::string* line, const std::string& attribute)
	{
		if (line)
			return remove_attribute(*line, attribute);
		return false;
	}

	inline std::string get_attribute_value(const std::string& line, const std::string& attribute)
	{
		if (line.empty() || attribute.empty())
			return ""s;
		auto start = find_attribute_pos(line, attribute);
		if (start == std::string::npos)
			return ""s;
		start = line.find('\"', start);
		if (start == std::string::npos)
			return ""s;

		std::string res("");
		for (auto c = line.begin() + start + 1; c != line.end(); ++c) {
			if (*c != '\"')
				res += *c;
			else
				return res;
		}
		return ""s;
	}

	/*inline std::string get_attribute_value(std::string& line, const std::string& attribute)
	{
		if (line.empty() || attribute.empty())
			return ""s;
		size_t start = 0;
		if (start = line.find(attribute + "=\"", start); start != std::string::npos) {
			start += attribute.length() + 2;
			std::string res("");
			for (auto c = line.begin() + start; c != line.end(); ++c) {
				if (*c != '\"')
					res += *c;
				else
					return res;
			}
		}
		return ""s;
	}*/

	inline std::string get_attribute_value(const std::string* line, const std::string& attribute)
	{
		if (line)
			return get_attribute_value(*line, attribute);
		return ""s;
	}

	inline std::vector<std::pair<std::string, std::string>> get_all_attributes(const std::string& line)
	{
		std::vector<std::pair<std::string, std::string>> p;
		if (line.empty())
			return p;
		enum S
		{
			PARSE_ATTR,
			PARSE_VAR,
			SEARCH
		};
		S state = SEARCH;
		std::string val("");
		std::string attr("");

		auto fill_pair = [](std::string& val, std::string& attr, std::vector<std::pair<std::string, std::string>>& p) {
			if (!attr.empty() && !val.empty()) {
				p.push_back(std::pair(attr, val));
			}
			attr.clear();
			val.clear();
		};

		auto print_parsed = [&]() {
			std::string s("");
			for (auto& pr : p) {
				s += pr.first + "=\""s + pr.second + "\n"s;
			}
			if (!s.empty()) {
				logger::info("Parsed nodes:\n{}", s);
			}
		};

		for (auto c = line.rbegin(); c != line.rend(); ++c) {
			switch (state) {
			case SEARCH:
				if (*c == '\"') {
					state = PARSE_VAR;
				} else if (*c == '=') {
					state = PARSE_ATTR;
				} else if (*c == '<') {
					state = PARSE_ATTR;
					return p;
				}
				break;
			case PARSE_VAR:
				if (*c == '"') {
					state = SEARCH;
				} else {
					val.insert(val.begin(), *c);
				}
				break;
			case PARSE_ATTR:
				if (*c == ' ' || *c == '\t' || *c == '\n' || *c == '\r') {
					state = SEARCH;
					fill_pair(val, attr, p);
				} else {
					attr.insert(attr.begin(), *c);
				}
				break;
			}
		}

		print_parsed();
		return p;
	}

	inline std::string get_node_name(const std::string& line)
	{
		if (line.empty())
			return ""s;
		size_t start = 0;
		if (start = line.find("<"); start != std::string::npos) {
			start += 1;
			std::string res("");
			for (auto c = line.begin() + start; c != line.end(); ++c) {
				if (*c != ' ' && *c != '>' && *c != '/' && *c != '\t' && *c != '\r' && *c != '\n')
					res += *c;
				else
					return res;
			}
		}
		return ""s;
	}

	inline std::string pop_node_name(std::string& str)
	{
		if (str.empty())
			return ""s;
		auto nodename = utils::get_node_name(str);
		if (nodename.empty())
			return ""s;
		size_t start = str.find("<" + nodename);
		if (start == std::string::npos)
			return ""s;
		str.erase(start + 1, nodename.length());
		return nodename;
	};

	inline bool change_node_name(std::string& str, const std::string& new_node_name)
	{
		auto node = pop_node_name(str);
		if (node.empty())
			return false;
		size_t f = str.find('<');
		++f;
		str.insert(str[f], new_node_name);
		if (auto c = str[f + new_node_name.length() + 1]; c != ' ' || c != '\t')
			str.insert(str[f + new_node_name.length() + 1], " "s);
		return true;
	}

	inline std::vector<std::pair<std::string, std::string>> get_all_attributes(const std::string* line)
	{
		if (line)
			return get_all_attributes(*line);
		return get_all_attributes(""s);
	}

	inline std::vector<std::pair<std::string, std::string>> pop_all_attributes(std::string& line)
	{
		std::vector<std::pair<std::string, std::string>> attrs;

		if (line.empty()) {
			return attrs;  // Возвращаем пустой вектор, если строка пуста
		}
		// Регулярное выражение для поиска атрибутов в формате name="value"
		std::regex attribute_regex(R"(([\w\-:]+)\s*=\s*\"([^\"]*)\")");
		std::smatch match;

		// Используем регулярное выражение для поиска атрибутов и их значений
		while (std::regex_search(line, match, attribute_regex)) {
			attrs.emplace_back(match[1].str(), match[2].str());  // Сохраняем атрибут и его значение

			// Удаляем найденный атрибут из строки
			line.erase(match.position(0), match.length(0));
		}
		// Удаляем пробелы и табуляции внутри угловых скобок
		line = std::regex_replace(line, std::regex(R"(\s*(<\s*|\s*>|\s*/\s*>)\s*)"), "$1");
		return attrs;  // Возвращаем список атрибутов
	}

	inline std::vector<std::pair<std::string, std::string>> pop_all_attributes(std::string* line)
	{
		if (line)
			return pop_all_attributes(*line);
		std::vector<std::pair<std::string, std::string>> empty;
		return empty;
	}

	inline bool add_attribute(std::string& line, const std::string& attr, const std::string& value)
	{
		if (line.empty() || attr.empty())
			return false;

		auto get_tag_end = [](const std::string& line) {
			if (size_t start = line.find("<"); start != std::string::npos) {
				start += 1;
				for (auto c = line.begin() + start; c != line.end(); ++c, ++start) {
					if (*c == ' ' || *c == '>' || *c == '/' || *c == '\t' || *c == '\r' || *c == '\n')
						return start;
				}
			}
			return std::string::npos;
		};

		auto ins = " "s + attr + "=\"" + value + "\"";
		line.insert(get_tag_end(line), ins);
		size_t f = line.find(ins);
		if (auto c = line.begin() + f + ins.length(); *c != ' ' && *c != '\t' && *c != '\r' && *c != '\n' && *c != '/' && *c != '>')
			line.insert(c, ' ');
		return true;
	}

	inline bool add_attribute(std::string* line, const std::string& attr, const std::string& value)
	{
		if (line)
			return add_attribute(*line, attr, value);
		return false;
	}

	inline std::string get_node_name(const std::string* line)
	{
		if (line)
			return get_node_name(*line);
		return ""s;
	}

	inline void remove_new_line_symbols(std::string& str)
	{
		for (auto p = str.begin(); p != str.end();)
			if (*p == '\n' || *p == '\r') {
				str.erase(p);
			} else {
				++p;
			}
	}

	inline std::string to_lower_case(std::string str)
	{
		if (str.empty())
			return str;
		std::transform(str.begin(), str.end(), str.begin(),
			[](unsigned char c) { return (char)std::tolower(c); });
		return str;
	}

	template <typename T>
	inline std::string to_hex_string(T i)
	{
		std::stringstream stream;
		stream << std::hex << i;
		return stream.str();
	}
}
