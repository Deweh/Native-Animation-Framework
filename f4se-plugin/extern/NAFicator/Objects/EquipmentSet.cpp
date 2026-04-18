#pragma once
#include "EquipmentSet.h"

namespace NAFicator
{
	bool EquipmentSet::is_valid()
	{
		bool v = valid && !id.empty() && !equipmentSet_map.empty();
		if (valid && !v)
			set_invalid("Failed object validation!");
		return valid;
	}
	
	Data::XMLUtil::Mapper& EquipmentSet::parse_id(Data::XMLUtil::Mapper& m)
	{
		return ParsedObject::parse_id(m);
	}

	bool EquipmentSet::parse(Data::XMLUtil::Mapper& m)
	{
		parse_id(m);
		std::string tmp("");
		m.GetArray([&](Data::XMLUtil::Mapper& m) {		
			ConditionsSet cond(m);

			//typedef std::vector<std::string, std::string> attributes_map;
			//typedef std::vector<std::string, attributes_map> nodes_map;

			nodes_map node;
			
			auto push_conds = [&] {
				if (!node.empty())
					equipmentSet_map.push_back(std::make_pair(cond, node));
			}; 

			m.GetArray([&](Data::XMLUtil::Mapper& m) {
				m(&tmp, ""s, false, false, "unEquip has no 'bipedSlot' nodes", "bipedSlot");
				attributes_map attrs;
				if (!tmp.empty()) {
					attrs.push_back(std::make_pair("bipedSlot"s, tmp));
					tmp.clear();

					node.push_back(std::make_pair("unEquip", attrs));
				}
				return m;
			},
				"unEquip", "", false);

			if (!node.empty()) {
				push_conds();
				return m;
			}

			m.GetArray([&](Data::XMLUtil::Mapper& m) {
				attributes_map attrs;
				if (m(&tmp, Data::XMLUtil::Mapper::emptyStr, false, false, "", "resetAll"); !tmp.empty()) {
					attrs.push_back(std::make_pair("resetAll", tmp));
					tmp.clear();
				} else if (m(&tmp, Data::XMLUtil::Mapper::emptyStr, false, false, "", "bipedSlot"); !tmp.empty()) {
					attrs.push_back(std::make_pair("bipedSlot", tmp));
					tmp.clear();
				}

				if (!attrs.empty()) {
					node.push_back(std::make_pair("reEquip", attrs));
				}
				return m;
			},
				"reEquip", "", false);

			if (!node.empty()) {
				push_conds();
				return m;
			}

			m.GetArray([&](Data::XMLUtil::Mapper& m) {
				attributes_map attrs;
				m(&tmp, Data::XMLUtil::Mapper::emptyStr, false, false, "addEquipment has no 'form'", "form");
				if (!tmp.empty()) {
					std::string source("");
					m(&source, Data::XMLUtil::Mapper::emptyStr, true, false, "addEquipment has no 'source'", "source");
					if (!source.empty()) {
						attrs.push_back(std::make_pair("form", tmp));
						attrs.push_back(std::make_pair("source", source));
						tmp.clear();

						node.push_back(std::make_pair("addEquipment", attrs));
					}
				}

				return m;
			},
				"addEquipment", "", false);

			if (!node.empty()) {
				push_conds();
				return m;
			}

			m.GetArray([&](Data::XMLUtil::Mapper& m) {
				attributes_map attrs;
				m(&tmp, Data::XMLUtil::Mapper::emptyStr, false, false, "removeEquipment has no 'form'", "form");
				if (!tmp.empty()) {
					std::string source("");
					m(&source, Data::XMLUtil::Mapper::emptyStr, true, false, "removeEquipment has no 'source'", "source");
					if (!source.empty()) {
						attrs.push_back(std::make_pair("form", tmp));
						attrs.push_back(std::make_pair("source", source));
						tmp.clear();
						
						node.push_back(std::make_pair("removeEquipment", attrs));
					}
				}

				return m;
			},
				"removeEquipment", "", false);

			if (!node.empty()) {
				push_conds();
			}

			return m;
		},
			"condition", "no condition nodes");

		return true;
	}

	std::ostringstream& EquipmentSet::ssprint(std::ostringstream& s) const
	{	
		auto print_equipment_actions = [](const nodes_map& nodes, std::ostringstream& stream) {
			std::for_each(nodes.begin(), nodes.end(), [&](const std::pair<std::string, attributes_map>& node) {
				auto& [node_name, attributes] = node;
				stream << "\t\t\t<" << node_name;
				std::for_each(attributes.begin(), attributes.end(), [&](const std::pair<std::string, std::string>& v) {
					auto& [attr_name, attr_value] = v;
					stream << " " << attr_name << "=\"" << attr_value << "\"";
				});
				stream << "/>\n";
			});
		};

		s << "\n<!-- FILE : " << filename << (valid ? " -->\n" : "\n");
		s << ((!valid && !log.empty()) ? (log + "\n\n"s) : "");

		if (id.empty()) {
			s << "\t<equipmentSet id=\"" << get_unknown_id() << "\">\n";
		} else {
			s << "\t<equipmentSet id=\"" << id << "\">\n";
		}
		std::for_each(equipmentSet_map.begin(), equipmentSet_map.end(), [&](const std::pair<ConditionsSet, nodes_map>& c) {
			auto& [conditionSet, nodes] = c;
			s << "\t\t<condition";
			conditionSet.print_conditions(s);
			s << ">\n";
			print_equipment_actions(nodes, s);
			s << "\t\t</condition>\n";
		});
		s << "\t</equipmentSet>" << (valid ? "" : " -->");
		
		return s;
	}
}
