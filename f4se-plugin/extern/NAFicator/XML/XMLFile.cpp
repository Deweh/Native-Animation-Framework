#pragma once
#pragma warning(disable:4996)
#include <IniParser/Ini.h>
#include "NAFicator/XML/XMLFile.h"
#include "NAFicator/XML/utils.h"
#include "utils/utility.h"
#include <fstream>
#include "NAFicator/Version.h"
#include <NAFicator/Objects/objects_map.h>
#include <NAFicator/Version.h>


extern ini::map inimap;

XMLfile::XMLfile(const std::filesystem::path& source_file) :
	source(source_file)
{
	logger::info("Opening file {} ...", source_file.filename().string());

	if (!std::filesystem::exists(source)) {
		source = "";
		return;
	}

	if (std::filesystem::is_directory(source)) {
		source = "";
		return;
	}

	if (source.extension().string() != ".xml"s) {
		source = "";
		return;
	}
	
	std::ifstream read(source);
	if (!read.is_open()) {
		source = "";
		return;
	}

	std::string line;
	while (!read.eof()) {
		line.clear();
		getline(read, line);
		if (!line.empty())
			buffer.push_back(line);
	}
	read.close();
	normalize();

	if (hasValue)
		logger::info("normalize... success\nprepare to make objects...");
	else
		logger::error("{} normalize... failed, file skipped", filename());
}

void XMLfile::for_each_string(std::function<void(std::vector<std::string>::iterator&, XMLfile*)> do_work_with_string)
{
	if (this) {
		for (auto it = buffer.begin(), end = buffer.end(); it != end; ++it) {
			if (!it->empty()) {
				do_work_with_string(it, this);
			} else {
				continue;
			}
		}
	}
}

// Helper method: Clean XML commentaries
void XMLfile::process_clean_commentaries()
{
	bool commentary_state = false;
	std::function<void(std::vector<std::string>::iterator & it, XMLfile * x)> clean_commentaries = [&](std::vector<std::string>::iterator& it, XMLfile* x) {
		if (it->empty())
			return it;
		auto& str = *it;
		if (commentary_state) {
			size_t commentary_end = str.find("-->");
			if (commentary_end != std::string::npos) {
				str.erase(0, (commentary_end + 3));
				commentary_state = false;
			} else {
				str.clear();
				commentary_state = true;
			}
		} else {
			size_t commentary_start = str.find("<!--");
			if (commentary_start != std::string::npos) {
				size_t commentary_end = str.find("-->");
				if (commentary_end != std::string::npos) {
					str.erase(commentary_start, ((commentary_end + 3) - commentary_start));
					commentary_state = false;
				} else {
					str.erase(commentary_start, str.length() - commentary_start);
					commentary_state = true;
				}
			} else {
				commentary_state = false;
			}
		}
		if (size_t commentary_start = str.find("<!--") != std::string::npos)
			clean_commentaries(it, x);
		return it;
	};

	for_each_string([&](std::vector<std::string>::iterator& it, XMLfile* x) { 
		if (it == buffer.begin())
			*it = std::move(utils::string::clean_string_from_BOM(*it));
		clean_commentaries(it, this);
		if (!it->empty())
			*it = utils::xml::trim(*it);
		if (!it->empty())
			utils::xml::remove_inline_spaces(*it);
	});
}

// Helper method: Remove whitespaces
void XMLfile::process_whitespace_removal()
{
	auto remove_whitespaces = [&, this](std::vector<std::string>::iterator& it, XMLfile*) {
		utils::xml::remove_inline_spaces(*it);
		utils::string::trim(*it, " \t\n\r"s);
	};

	for_each_string(remove_whitespaces);
}

// Helper method: Apply specific file fixes
void XMLfile::apply_specific_file_fixes(std::vector<std::string>::iterator& it)
{
	// rxl_bp70_anims_positionData.xml "tags= fix
	if (this->filename() == "rxl_bp70_anims_positionData.xml")
	{
		if (it->find("\"tags="))
		{
			utils::replace(*it, "\"tags="s, "tags=\""s);
		}
	}
}

// Helper method: Check if attribute is known empty
bool XMLfile::is_known_empty_attribute(std::string_view attr, std::unordered_set<std::string>& cache, bool& parsed)
{
	if (!parsed) {
		parsed = true;
		auto str = inimap.get<std::string>("sSkipEmptyAttributesInLog"s, "General"s);
		if (!str.empty()) {
			std::vector<std::string> attributes;
			utils::delim(str, ","s, attributes);
			cache.insert(attributes.begin(), attributes.end());
		}
	}
	return cache.find(std::string(attr)) != cache.end();
}

// Helper method: Parse XML line using state machine
void XMLfile::parse_xml_line_state_machine(std::vector<std::string>::iterator& it, 
	std::unordered_set<std::string>& empty_attributes, bool& parsed_known_empty_attribute)
{
	auto fill_pair = [&, this](std::string& val, std::string& attr, std::vector<std::pair<std::string, std::string>>& p) {
		if (!attr.empty() && !val.empty()) {
			p.emplace_back(std::make_pair(attr, val));
		} else {
			if (!attr.empty() && !is_known_empty_attribute(attr, empty_attributes, parsed_known_empty_attribute)) {
				std::ostringstream oss;
				oss << "'check_lexicography': no attribute/value in:\n"
					<< *it << ",\nfile: " << source.filename().string();
				send_warning(oss.str());
			}
		}
		attr.clear();
		val.clear();
	};

	auto check_missed_space = [](std::string::iterator it, std::string& str) {
		if (*it == '\"' && std::next(it) != str.end() && std::isalpha(static_cast<unsigned char>(*(std::next(it))))) {
			it = str.insert(std::next(it), ' ');
			--it;
		}
	};

	auto isSpace = [](const char& ch) {
		return (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');
	};

	auto get_prev_symb_without_spaces = [isSpace](std::string& str, std::string::iterator it) -> std::string::iterator {
		if (it == str.begin()) {
			return it;
		}
		--it;
		while (it != str.begin() && isSpace(*it)) {
			--it;
		}
		return it;
	};

	auto get_next_symb_without_spaces = [isSpace](std::string& str, std::string::iterator it) -> std::string::iterator {
		if (it == str.end()) {
			return it;
		}
		++it;
		while (it != str.end() && isSpace(*it)) {
			++it;
		}
		return it;
	};

	auto try_to_replace_slash = [get_prev_symb_without_spaces, get_next_symb_without_spaces](std::string& str, std::string::iterator& it) -> std::string::iterator {
		if (*it == '/') {
			auto prev = get_prev_symb_without_spaces(str, it);
			auto next = get_next_symb_without_spaces(str, it);

			if (prev == str.begin() || next == str.end()) {
				return it;
			}

			if (*prev != '<' && *next != '>') {
				return it = str.erase(it);
			}
		}
		return it;
	};

	enum S
	{
		NODE,
		ATTR,
		VALUE,
		SEARCH
	};
	S state = SEARCH;

	std::string node("");
	std::vector<std::pair<std::string, std::string>> pairs;
	std::string val("");
	std::string attr("");
	bool close_this_node = false;

	for (auto c = it->begin(); c != it->end(); ++c) {
		switch (state) {
		case SEARCH:
			if (*c == '<') {
				if (node.empty()) {
					state = NODE;
					if (std::next(c) != it->end() && *(std::next(c)) == '/') {
						close_this_node = true;
					}
				}
			} else if (isSpace(*c) && std::next(c) != it->end() && std::isalpha(static_cast<unsigned char>(*(std::next(c))))) {
				state = ATTR;
			} else if (*c == '\"' && c != it->begin() && *(std::prev(c)) == '=') {
				state = VALUE;
			}
			if (*c == '/') {
				try_to_replace_slash(*it, c);
			}
			break;

		case VALUE:
			if (*c == '\"') {
				state = SEARCH;
				fill_pair(val, attr, pairs);
			} else {
				val += *c;
			}
			check_missed_space(c, *it);
			break;

		case ATTR:
			if (isSpace(*c) || *c == '=') {
				state = SEARCH;
			} else {
				attr += *c;
			}
			if (*c == '/') {
				try_to_replace_slash(*it, c);
			}
			break;

		case NODE:
			if (isSpace(*c)) {
				if (std::next(c) != it->end() && std::isalpha(static_cast<unsigned char>(*(std::next(c))))) {
					state = ATTR;
				} else {
					state = SEARCH;
				}
			} else if (*c == '>') {
				close_this_node = true;
				fill_pair(val, attr, pairs);
				return;
			} else {
				node += *c;
			}
			if (*c == '/') {
				try_to_replace_slash(*it, c);
			}
			break;
		}
	}
}

// Helper method: Check lexicography (refactored)
void XMLfile::process_lexicography_check()
{
	std::unordered_set<std::string> empty_attributes;
	bool parsed_known_empty_attribute = false;
	
	auto check_lexicography = [&, this](std::vector<std::string>::iterator& it, XMLfile*) {
		apply_specific_file_fixes(it);
		parse_xml_line_state_machine(it, empty_attributes, parsed_known_empty_attribute);
	};

	for_each_string(check_lexicography);
}

// Helper method: Fix duplicate attributes
void XMLfile::process_duplicate_attributes()
{
	auto fix_duplicate_attributes = [&](std::vector<std::string>::iterator& it, XMLfile* x) {
		auto attrs = utils::pop_all_attributes(*it);
		std::unordered_set<std::string> store;

		for (auto i = attrs.begin(); i != attrs.end();) {
			if (!store.insert(i->first).second) {
				logger::info("'fix_duplicate_attributes' removed: {}=\"{}\"", i->first, i->second);
				i = attrs.erase(i);
			} else {
				++i;
			}
		}

		for (const auto& i : attrs) {
			utils::xml::add_attribute(*it, i.first, i.second);
		}
	};

	for_each_string(fix_duplicate_attributes);
}

// Helper method: Glue lines
void XMLfile::process_line_gluing()
{
	auto glue_lines = [&, this](std::vector<std::string>::iterator& it, XMLfile*) {
		auto has_end = [](const std::string& str) {
			return !str.empty() && str.back() == '>';
		};

		auto has_begin = [](const std::string& str) {
			return !str.empty() && str.front() == '<';
		};

		if (auto has_b = has_begin(*it), has_e = has_end(*it); has_b && has_e) {
			// All good
		} else if (has_b && !has_e) {
			auto save_it = it;

			while (!has_end(*save_it) && it != buffer.end()) {
				do {
					++it;
				} while (it != buffer.end() && it->empty());

				if (it == buffer.end()) {
					set_critical_error("'glue_lines' : couldn't glue, error in lines : \n" + *save_it + "\n, file : " + source.filename().string());
					return;
				}

				if (has_begin(*it)) {
					set_critical_error("'glue_lines' : couldn't glue, error in lines : \n" + *save_it + "\n, file : " + source.filename().string());
					return;
				}

				*save_it += ' ' + *it;
				it->clear();
			}

			if (!has_end(*save_it)) {
				set_critical_error("'glue_lines' : couldn't glue, error in lines : \n" + *save_it + "\n, file : " + source.filename().string());
				return;
			}

			it = save_it;
		} else if (!has_b && has_e){
			set_critical_error("'glue_lines' : line has no begin '<' symbol : \n" + *it + "\n, file : " + source.filename().string());
			return;
		} else {
			set_critical_error("'glue_lines' : line has no begin '<' symbol, line has no end '>' symbol : \n" + *it + "\n, file : " + source.filename().string());
		}
	};

	for_each_string(glue_lines);
}

// Helper method: Get root node and add filename
std::string_view XMLfile::process_root_node_extraction()
{
	auto it = std::find_if(buffer.begin(), buffer.end(), [](const auto& str) { return !str.empty(); });
	if (it == buffer.end()) {
		set_critical_error(filename() + " Buffer is empty");
		return root_node;
	}

	auto get_valid_root_key_generic = [](std::string_view k, bool ignore_case = false) -> std::optional<std::string_view> {
		for (auto it = valid_root_nodes.begin(), end = valid_root_nodes.end(); it != end; ++it) {
			if (ignore_case) {
				if (utils::string::to_lower(k) == utils::string::to_lower(it->first))
					return it->first;
			} else {
				if (it->first == k)
					return it->first;
			}
		}
		return std::nullopt;
	};

	std::string node_name = utils::xml::get_node_name(*it);
	root_node = get_valid_root_key_generic(node_name).value_or("");

	if (root_node.empty()) {
		auto dataSet = utils::xml::get_attribute_value(std::string_view(*it), "dataSet");
		if (!dataSet.empty()) {
			std::string dataSetData = std::string(dataSet) + "Data";
			root_node = get_valid_root_key_generic(dataSetData).value_or(get_valid_root_key_generic(dataSetData, true).value_or(""sv));
			if (root_node.empty()) {
				set_critical_error(filename() + " has no root node");
			} else {
				utils::xml::add_attribute(*it, "filename", filename());
			}
		}
	} else {
		utils::xml::add_attribute(*it, "filename", filename());
	}

	if (!root_node.empty()) {
		if (auto node = utils::xml::get_node_name(*it); node != root_node)
		{
			utils::xml::change_node_name(*it, root_node);
			if (auto p = it->rfind("/>")) {
				it->replace(p, 2, ">");
				auto i = buffer.end();
				while (i != buffer.begin() && std::prev(i)->empty()) i--;
				if (i == buffer.begin()) {
					logger::warn("No nodes in xml file {}", filename());
					return root_node = "";
				}
				if (i == buffer.end())	{
					buffer.push_back("</"s + std::string(root_node) + ">");
				} else {
					*i = "</"s + std::string(root_node) + ">";
				}
			}
		}
	}
	return root_node;
}

// Helper method: Fix GrayUserBP_animationGroupData
void XMLfile::fix_GrayUserBP_animationGroupData()
{
	if (filename() != "GrayUserBP_animationGroupData.xml" || root_node != "animationData") {
		return;
	}

	auto fix_animationGroupData_for_GrayUserBP_animationGroupData = [&]() {
		auto open = std::find_if(buffer.begin(), buffer.end(), [](const std::string& str) {
			return !str.empty();
		});
		if (open != buffer.end()) {
			utils::xml::change_node_name(*open, "animationGroupData"s);
			auto rev_open = open == buffer.begin() ? buffer.rend() : std::make_reverse_iterator(open);
			auto close = std::find_if(buffer.rbegin(), rev_open, [](std::string& str) {
				if (auto p = str.find("</animationData>"); p != std::string::npos) {
					str.replace(p + 2, 13, "animationGroupData");
					return true;
				} else
					return false;
			});
		}
	};
	fix_animationGroupData_for_GrayUserBP_animationGroupData();
}

bool XMLfile::normalize()
{
	if (this == nullptr)
		return false;

	hasValue = true;

	// Stage 1: Clean commentaries and remove whitespaces
	process_clean_commentaries();

	// Stage 2: Glue lines, check lexicography and fix duplicate attributes
	process_line_gluing();
	process_lexicography_check();
	process_duplicate_attributes();

	// Stage 3: Get root node and add filename attribute
	process_root_node_extraction();

	// Stage 4: Apply specific fixes for known files
	fix_GrayUserBP_animationGroupData();

	// Debug output
	if (inimap.get<bool>("bPrintDebugXMLs"s, "General"s))
		print_to_file();

	// Final validation
	if (hasValue) {
		bool hasContent = std::any_of(buffer.begin(), buffer.end(),
			[](const std::string& s) { return !s.empty(); });

		if (!hasContent) {
			set_critical_error("Buffer is empty after normalization");
			hasValue = false;
		}
	}

	return hasValue;
}

void XMLfile::send_warning(std::string_view warn)
{
	logger::warn("File : {}, warn : {}", source.filename().string(), warn);
}

void XMLfile::set_critical_error(std::string_view error)
{
	hasValue = false;
	logger::error("File : {}, error : {}", source.filename().string(), error);
}

std::shared_ptr<std::stringstream> XMLfile::make_stringstream() const
{
	auto s = std::make_shared<std::stringstream>();
	std::for_each(buffer.begin(), buffer.end(), [&](const std::string& str) {
		if (!str.empty()) {
			*s << str << "\n";
		}
	});
	return s;
}


std::unordered_map<std::string_view, std::string_view> parse_attributes(std::string_view str, const std::vector<std::string_view>& attributes, const std::unordered_map<std::string_view, std::string_view>& defaults)
{
	std::unordered_map<std::string_view, std::string_view> attrs;
	for (const auto& a : attributes) {
		auto val = utils::xml::get_attribute_value(str, a);
		if (!val.empty())
			attrs.emplace(make_pair(a, val));
		else if (defaults.contains(a)) {
			try {
				val = defaults.at(a);
				attrs.emplace(make_pair(a, val));
			}
			catch (...) {
				continue;
			}
		}
	}
	return attrs;
};

void XMLfile::print_to_file() const
{
	// Определяем путь к файлу
	std::filesystem::path output_path = std::filesystem::current_path() / "Data" / "NAFicator" / "Debug" / filename();

	// Создаем поток для записи в файл
	std::ofstream output_file(output_path);
	if (!output_file.is_open()) {
		logger::critical("Failed to open file for writing: {}", output_path.string());
		return;
	}

	// Получаем строковый поток и записываем его содержимое в файл
	auto stringstream = make_stringstream();
	output_file << stringstream->str();

	// Закрываем файл
	output_file.close();

	logger::info("Successfully written to file: {}", output_path.string());
}
