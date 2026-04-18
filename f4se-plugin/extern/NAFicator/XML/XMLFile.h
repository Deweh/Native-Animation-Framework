#pragma once
#include <filesystem>
#include <string>
#include <utility>
#include <unordered_set>
#include <vector>
#include <list>
#include <functional>
#include <stack>

#include "F4SE/F4SE.h"
#include "F4SE/Logger.h"

namespace logger = F4SE::log;
using namespace std::string_literals;
using namespace std::string_view_literals;
typedef std::list<std::string_view>::iterator buf_iterator;
typedef std::pair<std::string, std::pair<buf_iterator, buf_iterator>> NODE;
typedef std::unordered_multimap<std::string, std::pair<buf_iterator, buf_iterator>> content_map;

class XMLfile
{
	std::filesystem::path source = "";
	std::string_view root_node = ""s;
	bool hasValue = false;
	bool normalize();
	void for_each_string(std::function<void(std::vector<std::string>::iterator&, XMLfile*)>);
	std::vector<std::string> buffer;

	// Helper methods for normalize()
	void process_clean_commentaries();
	void process_whitespace_removal();
	void process_lexicography_check();
	void process_duplicate_attributes();
	void process_line_gluing();
	std::string_view process_root_node_extraction();
	void fix_GrayUserBP_animationGroupData();

	// Helper methods for lexicography check
	void apply_specific_file_fixes(std::vector<std::string>::iterator& it);
	bool is_known_empty_attribute(std::string_view attr, std::unordered_set<std::string>& cache, bool& parsed);
	void parse_xml_line_state_machine(std::vector<std::string>::iterator& it, std::unordered_set<std::string>& empty_attrs, bool& parsed_flag);

public:
	
	static inline const std::unordered_map<std::string_view, std::pair<std::string_view, std::string_view>> valid_root_nodes = {
		{ "animationData"sv, std::pair("animation"sv, "id"sv) },
		{ "raceData"sv, std::pair("race"s, "skeleton"sv) },
		{ "positionData"sv, std::pair("position"sv, "id"sv) },
		{ "morphSetData"sv, std::pair("morphSet"sv, "id"sv) },
		{ "equipmentSetData"sv, std::pair("equipmentSet"sv, "id"sv) },
		{ "actionData"sv, std::pair("action"sv, "id"sv) },
		{ "animationGroupData"sv, std::pair("animationGroup"sv, "id"sv) },
		{ "furnitureData"sv, std::pair("group"sv, "id"sv) },
		{ "positionTreeData"sv, std::pair("tree"sv, "id"sv) },
		{ "tagData"sv, std::pair("tag"sv, "position"sv) },
		{ "mfgSetData"sv, std::pair("mfgSet"sv, "id"sv) },
		{ "overlayData"sv, std::pair("overlaySet"sv, "id"sv) },
		{ "protectedEquipmentData"sv, std::pair("condition"sv, ""sv) }
	};

	static inline std::vector<std::pair<std::string_view, std::pair<std::string_view, std::string_view>>> removed_nodes;
	static inline bool is_valid_root(const std::string& root_node_name)
	{
		return valid_root_nodes.contains(root_node_name);
	}

	XMLfile(const std::filesystem::path& source_file);

	bool has_value() const { return hasValue; }
	void set_critical_error(std::string_view error);
	void send_warning(std::string_view warn);
	const std::string_view get_root() const { return root_node; }
	std::string filename() const { return source.filename().string(); }
	std::shared_ptr<std::stringstream> make_stringstream() const;
	void print_to_file() const;
};

std::unordered_map<std::string_view, std::string_view> parse_attributes(std::string_view str, const std::vector<std::string_view>& attributes, const std::unordered_map<std::string_view, std::string_view>& defaults);
