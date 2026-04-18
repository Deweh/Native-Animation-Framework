#pragma once
#include "MfgSet.h"

namespace NAFicator
{
	bool MfgSet::is_valid()
	{
		bool v = (valid && !id.empty() && !morphs.empty());
		if (valid && !v)
			set_invalid("Failed object validation!");
		return valid;
	}
	
	Data::XMLUtil::Mapper& MfgSet::parse_id(Data::XMLUtil::Mapper& m)
	{
		return ParsedObject::parse_id(m);
	}

	bool MfgSet::parse(Data::XMLUtil::Mapper& m)
	{
		parse_id(m);

		uint8_t i = 0xFF;
		m.GetArray([&](Data::XMLUtil::Mapper& m) {
			morph mrph;
			
			m(&i, (uint8_t)0xFF, false, true, "MfgSet morphID has no 'morphID' attribute!", "morphID");

			m(&mrph.intensity, 0, false, true, "MfgSet intensity has no 'intensity' attribute!", "intensity");

			std::string tmp;
			m(&tmp, Data::XMLUtil::Mapper::emptyStr, true, false, "", "lock");
			if (!tmp.empty())
				mrph.lock = tmp == "true" ? true : false;

			if (i < 49)
				morphs.emplace(i, mrph);
			return m;
		},
			"setting", "MfgSet has no setting!");

		return m;
	}

	std::ostringstream& MfgSet::ssprint(std::ostringstream& s) const
	{
		auto print_setting = [](const std::pair<uint8_t, morph>& m, std::ostringstream& stream) {
			stream << " intensity=\"" << m.second.intensity << "\"";
			if (m.second.lock.has_value()) 
				stream << " lock=\"" << (m.second.lock.value() ? "true" : "false") << "\"";
		};

		s << "\n<!-- FILE : " << filename << (valid ? " -->\n" : "\n");
		s << ((!valid && !log.empty()) ? (log + "\n\n"s) : "");

		if (id.empty()) {
			s << "\t<mfgSet id=\"" << get_unknown_id() << "\">\n";
		} else {
			s << "\t<mfgSet id=\"" << id << "\">\n";
		}
		std::for_each(morphs.begin(), morphs.end(), [&](const std::pair<uint8_t, morph>& m) {
			s << "\t\t<setting morphID=\"" << std::to_string(m.first) << "\"";
			print_setting(m, s);
			s << "/>\n";
		});
		s << "\t</mfgSet>" << (valid ? "" : " -->");
		
		return s;
	}
}
