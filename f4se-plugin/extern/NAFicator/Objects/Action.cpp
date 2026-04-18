#pragma once
#include "Action.h"

namespace NAFicator
{
	bool Action::is_valid()
	{
		bool v = (valid && !id.empty() && (startEquipmentSet.has_value() || stopEquipmentSet.has_value() || mfgSet.has_value()));
		if (valid && !v)
			set_invalid("Failed object validation!");
		return valid;
	}

	Data::XMLUtil::Mapper& Action::parse_id(Data::XMLUtil::Mapper& m) 
	{
		return ParsedObject::parse_id(m);
	}
	
	bool Action::parse(Data::XMLUtil::Mapper& m)
	{
		parse_id(m);

		if (!m.DownNode("self", "", false))
			return false;
		m(&startEquipmentSet, std::optional<std::string>(std::nullopt), true, false, "", "startEquipmentSet");
		m(&stopEquipmentSet, std::optional<std::string>(std::nullopt), true, false, "", "stopEquipmentSet");
		m(&mfgSet, std::optional<std::string>(std::nullopt), true, false, "", "mfgSet");
		return m;
	};

	std::ostringstream& Action::ssprint(std::ostringstream& s) const
	{
		auto print_self_node = [this](std::ostringstream& s) {
			s << "\t\t<self";
			if (startEquipmentSet.has_value()) {
				s << " startEquipmentSet=\"" << startEquipmentSet.value() << "\"";
			}
			if (stopEquipmentSet.has_value()) {
				s << " stopEquipmentSet=\"" << stopEquipmentSet.value() << "\"";
			}
			if (mfgSet.has_value()) {
				s <<" mfgSet=\"" << mfgSet.value() << "\"";
			}
			s << "/>\n";
				
		};
		
		s << "\n<!-- FILE : " << filename << (valid ? " -->\n" : "\n");
		s << ((!valid && !log.empty()) ? (log + "\n\n"s) : "");

		if (id.empty()) {
			s << "\t<action id=\"" << get_unknown_id() << "\">\n";
		} else {
			s << "\t<action id=\"" << id << "\">\n";
		}
		print_self_node(s);
		s << "\t</action>" << (valid ? "" : " -->");
		return s;
	}
}
