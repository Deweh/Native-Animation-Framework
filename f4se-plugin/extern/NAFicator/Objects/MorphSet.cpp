#pragma once
#include "MorphSet.h"

namespace NAFicator
{
	bool MorphSet::is_valid()
	{
		bool v = (valid && !id.empty() && !morphs.empty());
		if (valid && !v)
			set_invalid("Failed object validation!");
		return valid;
	}
	
	Data::XMLUtil::Mapper& MorphSet::parse_id(Data::XMLUtil::Mapper& m)
	{
		return ParsedObject::parse_id(m);
	}

	bool MorphSet::parse(Data::XMLUtil::Mapper& m)
	{
		parse_id(m);
		std::string tmp("");
		m.GetArray([&](Data::XMLUtil::Mapper& m) {
			ConditionsSet cond(m);

			pair_key_kv_pair pairs;
			m.GetArray([&](Data::XMLUtil::Mapper& m) {
				std::pair<std::string, float> p;
				m(&p.first, Data::XMLUtil::Mapper::emptyStr, false, false, "morph has no 'morphID'", "value");
				m(&p.second, 0.f, false, false, "morph has no 'to'", "to");
				if (!p.first.empty()) {
					pairs.emplace(p.first, p.second);
				}
				return m;
			},
				"morph", "", false);
			if (!pairs.empty())
				morphs.push_back(std::make_pair(cond, pairs));

			return m;
		},
			"condition", "no condition nodes");
		return true;
	}

	std::ostringstream& MorphSet::ssprint(std::ostringstream& s) const
	{
		auto print_morphs = [](const std::map<std::string, float>& k, std::ostringstream& stream) {
			std::for_each(k.begin(), k.end(), [&](const std::pair<std::string, float>& kp) {
				stream << "\t\t\t<morph value=\"" << kp.first << "\" to=\"" << kp.second << "\"/>\n";
			});
		};

		s << "\n<!-- FILE : " << filename << (valid ? " -->\n" : "\n");
		s << ((!valid && !log.empty()) ? (log + "\n\n"s) : "");

		if (id.empty()) {
			s << "\t<morphSet id=\"" << get_unknown_id() << "\">\n";
		} else {
			s << "\t<morphSet id=\"" << id << "\">\n";
		}
		std::for_each(morphs.begin(), morphs.end(), [&](const std::pair<ConditionsSet, std::map<std::string, float>>& c) {
			auto& [conditionSet, morphSet] = c;
			s << "\t\t<condition";
			conditionSet.print_conditions(s);
			s << ">\n";
			print_morphs(morphSet, s);
			s << "\n\t\t</condition>";
		});
		s << "\t</morphSet>" << (valid ? "" : " -->");
		
		return s;
	}
}
