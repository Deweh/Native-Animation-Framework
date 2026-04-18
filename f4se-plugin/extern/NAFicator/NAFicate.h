#pragma once
#include <mutex>
#include <thread>
#include <future>
#include <limits.h>
#include <boost/algorithm/string/replace.hpp>

#include <F4SE/F4SE.h>
#include <F4SE/Logger.h>
#include <RE/Fallout.h>
#include <IniParser\Ini.h>
#include <NAFicator/XML/XMLFile.h>
#include <NAFicator/Form/Form.h>

#include <RE/Bethesda/BSResource.h>
#include <NAFicator/Objects/ParsedObject.h>
#include <NAFicator/ThreadPool/ThreadPool.h>

void START(const std::filesystem::path& from);
RE::TESForm* get_form_from_string(const std::string& xFormID, const std::string& plugin);
void parse_files(const std::filesystem::path& from, std::vector<XMLfile>&, std::vector<std::string>&);
void for_each_file(std::vector<XMLfile>& xmls, std::function<void(XMLfile&)> apply, const std::string& rootNode);
bool process_file(const std::string& p_src, std::vector<XMLfile>& xmls, std::vector<std::string>& skipped);
//void patch_raceData(XMLfile& x);
//std::string get_source(XMLfile& x, std::list<std::string>::iterator& it);
//void patch_replace_forms_to_hkx(XMLfile& x);
//void patch_remove_missing_furniture(XMLfile& x);
//void patch_remove_missed_links(XMLfile& x);
//void patch_fix_wrong_offset(XMLfile& x);
//void patch_fix_nofurn_and_bed_tags(XMLfile& x);
//returns XMLfile*, and iterators for node begin, and after node end string. Checks for priority.
//std::shared_ptr<std::pair<XMLfile*, std::pair<buf_iterator, buf_iterator>>> find_key_node(const std::string& root_node, const std::string& attribute_name, const std::string& attribute_value);
