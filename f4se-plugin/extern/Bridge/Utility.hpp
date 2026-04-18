#pragma once
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std::string_literals;

namespace ini
{

	inline std::vector<std::string> SplitString(const std::string& fullString, const std::string& delimiter)
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

	inline std::string& RemoveAllSpaces(std::string& str)
	{
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

	inline std::string& RemoveAllSpaces(const std::string& str)
	{
		std::string copy = str;
		return RemoveAllSpaces(copy);
	}

	inline std::string& RemoveEscapeChars(std::string& str)
	{
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

	inline std::string& RemoveSideSpaces(std::string& str)
	{
		while (str.begin() != str.end() && (*str.begin() == '\t' || *str.begin() == ' '))
			str.erase(*str.begin());
		while (str.rbegin() != str.rend() && (*str.rend() == '\t' || *str.rbegin() == ' '))
			str.erase(*str.rbegin());
		return str;
	}

	inline std::string RemoveSideSpaces(const std::string& str)
	{
		std::string copy = str;
		return RemoveSideSpaces(copy);
	}

	inline std::string& RemoveComments(std::string& str)
	{
		size_t f = str.find(';');
		return f == std::string::npos ? str : str.erase(f, std::string::npos);
	}

	inline std::string RemoveComments(const std::string& str)
	{
		return RemoveComments(std::string(str));
	}

	inline std::string& GetIniLine(std::ifstream& file, std::string& str, size_t* line_counter = nullptr)
	{
		str.clear();
		getline(file, str);
		RemoveComments(str);
		RemoveSideSpaces(str);

		if (line_counter)
			++*line_counter;

		return str;
	}

	inline void RemoveNewLineSymbols(std::string& str)
	{
		for (auto p = str.begin(); p != str.end();)
			if (*p == '\n' || *p == '\r') {
				str.erase(p);
			} else {
				++p;
			}
	}

	inline bool IsUnused(const std::string& str)
	{
		std::string copy(str);
		RemoveComments(copy);
		RemoveSideSpaces(copy);
		RemoveNewLineSymbols(copy);
		return copy.empty();
	}

	inline std::string GetSection(const std::string& str)
	{
		if (*str.begin() == '[' && *(str.end() - 1) == ']')
			return std::string(str.begin() + 1, str.end() - 1);
		return std::string("");
	}

	inline std::vector<std::string> GetKeyValuePair(const std::string& str)
	{
		return SplitString(str, "=");
	}
}
