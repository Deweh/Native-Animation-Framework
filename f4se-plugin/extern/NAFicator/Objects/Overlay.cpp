#pragma once
#include "Overlay.h"
namespace NAFicator
{
	bool Overlay::is_valid()
	{
		bool v = (valid && !id.empty() && !overlays.empty());
		if (valid && !v)
			set_invalid("Failed object validation!");
		return valid;
	}
	
	Data::XMLUtil::Mapper& Overlay::parse_id(Data::XMLUtil::Mapper& m)
	{
		return ParsedObject::parse_id(m);
	}

	bool Overlay::parse(Data::XMLUtil::Mapper& m)
	{
		parse_id(m);
		std::string tmp("");

		m.GetArray([&](Data::XMLUtil::Mapper& m) {
			ConditionsSet cond(m);

			std::vector<overlay_group> vector_og;
			m.GetArray([&](Data::XMLUtil::Mapper& m) { //std::pair<overlay_group_setting, std::unordered_map<std::string, overlay_settings>>
				overlay_group og;
				auto& [olaygroup_settings, olay] = og;
				m(&olaygroup_settings.duration, 0, true, false, "", "duration");
				m(&olaygroup_settings.quantity, 1, true, false, "", "quantity");

				m.GetArray([&tmp, &olay](Data::XMLUtil::Mapper& m) {
					overlay_settings o;
					m(&tmp, Data::XMLUtil::Mapper::emptyStr, false, false, "overlay node has no 'template'", "template");
					m(&o.alpha, 100, false, false, "overlay node has no 'alpha'", "alpha");
					m(&o.isFemale, true, false, false, "overlay node has no 'isFemale'", "isFemale");
					olay.emplace(tmp, o);
					tmp.clear();
					return m;
				},
					"overlay", "overlayGroup has no 'overlay' nodes", true);
				if (olay.size())
					vector_og.push_back(og);

				return m;
			},
				"overlayGroup", "condition has no 'overlayGroup' node!", true);

			if (!vector_og.empty())
				overlays.push_back(make_pair(cond, vector_og));

			return m;
		},
			"condition", "overlaySet has no 'condition' node!", false);
		return m;
	}

	std::ostringstream& Overlay::ssprint(std::ostringstream& s) const
	{
		auto print_overlay_group = [](const std::vector<overlay_group>& k, std::ostringstream& stream) {
			std::for_each(k.begin(), k.end(), [&](auto& og) {
				auto& [groupSettings, groupElements] = og;
				if (!groupElements.empty()) {
					stream << "\t\t\t<overlayGroup duration=\"" << groupSettings.duration << "\" quantity=\"" << groupSettings.quantity << "\">\n";
					std::for_each(groupElements.begin(), groupElements.end(), [&](auto& oe) {
						stream << "\t\t\t\t<overlay template=\"" << oe.first << "\" alpha=\""
							   << oe.second.alpha << "\" isFemale=\"" << oe.second.isFemale << "\"/>\n";
					});
					stream << "\t\t\t</overlayGroup>\n";
				}
			});
		};

		s << "\n<!-- FILE : " << filename << (valid ? " -->\n" : "\n");
		s << ((!valid && !log.empty()) ? (log + "\n\n"s) : "");

		if (id.empty()) {
			s << "\t<overlaySet id=\"" << get_unknown_id() << "\">\n";
		} else {
			s << "\t<overlaySet id=\"" << id << "\">\n";
		}
		std::for_each(overlays.begin(), overlays.end(), [&](const std::pair<ConditionsSet, std::vector<overlay_group>>& c) {
			auto& [conditionSet, overlayGroup] = c;
			s << "\t\t<condition";
			conditionSet.print_conditions(s);
			s << ">\n";
			print_overlay_group(overlayGroup, s);
			s << "\t\t</condition>";
		});
		s << "\n\t</overlaySet>" << (valid ? "" : " -->");
		
		return s;
	}

	bool Overlay::overlay_settings::operator == (const overlay_settings& other) const
	{
		return alpha == other.alpha && isFemale == other.isFemale;
	}

	bool Overlay::overlay_settings::operator<(const overlay_settings& other) const
	{
		if (alpha < other.alpha) {
			return true;
		} else if (alpha == alpha) {
			return static_cast<int>(isFemale) < static_cast<int>(other.isFemale);
		} else {
			return false;
		}
	}

	size_t Overlay::overlay_settings::operator()() const
	{
		return std::hash<int>()(alpha);
	}
}
