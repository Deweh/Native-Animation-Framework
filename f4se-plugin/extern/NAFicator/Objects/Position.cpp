#pragma once
#include "Position.h"

namespace NAFicator
{
	bool Position::is_valid()
	{
		bool v = valid && !id.empty() && !linked_animation.empty();
		if (valid && !v)
			set_invalid("Failed object validation!");
		return v;
	}
	
	Data::XMLUtil::Mapper& Position::parse_id(Data::XMLUtil::Mapper& m)
	{
		return ParsedObject::parse_id(m);
	}

	bool Position::parse(Data::XMLUtil::Mapper& m)
	{	
		std::string offset_str;

		// Чтение других id
		parse_id(m);

		// Чтение offset
		if (m(&offset_str, Data::XMLUtil::Mapper::emptyStr, true, false, "", "offset") && !offset_str.empty()) {
			offset = parse_offsets(offset_str);
		} else {
			auto r = m.GetRoot();
			r.GetArray([&](Data::XMLUtil::Mapper& r) {
				std::string o;
				r(&o, Data::XMLUtil::Mapper::emptyStr, true, false, "", "offset");
				if (!o.empty()) {
					auto tmp = parse_offsets(o);
					if (tmp) {
						offset = tmp;  // Присваиваем значение напрямую
					}
				}
				return r;
			},
				"animationOffset", "", false);
		}

		// Обработка тегов
		m(&offset_str, Data::XMLUtil::Mapper::emptyStr, false, false, "", "tags");
		if (!offset_str.empty()) {
			auto v_tags = utils::string::split(offset_str, ","); 
			for (const auto& t : v_tags) {
				tags.emplace(t);
			}
		}

		// Чтение флагов и наборов
		auto read_optional_string = [&](const std::string& key, std::optional<std::string>& opt) {
			m(&offset_str, Data::XMLUtil::Mapper::emptyStr, true, false, "", key);
			if (!offset_str.empty()) {
				opt = offset_str;
			}
		};

		// Чтение значения isHidden
		std::string hidden_str;
		m(&hidden_str, Data::XMLUtil::Mapper::emptyStr, true, false, "", "isHidden");
		if (!hidden_str.empty()) {
			isHidden = (hidden_str == "true");  // Присваиваем true или false
		}

		read_optional_string("startMorphSet", startMorphSet);
		read_optional_string("stopMorphSet", stopMorphSet);
		read_optional_string("startEquipmentSet", startEquipmentSet);
		read_optional_string("stopEquipmentSet", stopEquipmentSet);

		// Чтение локаций
		m(&offset_str, Data::XMLUtil::Mapper::emptyStr, true, false, "position has no 'location'", "location");
		if (!offset_str.empty()) {
			Utility::ForEachSubstring(offset_str, ",", [&](const std::string_view& s) {
				locations.emplace(s);
			});
		}

		// Чтение анимации
		if (m(&offset_str, Data::XMLUtil::Mapper::emptyStr, true, false, "", "animation")) {
			linked_animation = offset_str;
			linked_animation_type = animation;
		} else if (m(&offset_str, Data::XMLUtil::Mapper::emptyStr, true, false, "", "animationGroup")) {
			linked_animation = offset_str;
			linked_animation_type = group;
		} else if (m(&offset_str, Data::XMLUtil::Mapper::emptyStr, true, false, "", "positionTree")) {
			linked_animation = offset_str;
			linked_animation_type = tree;
		}
		if (linked_animation.empty() && !id.empty()) {
			linked_animation = id;
			linked_animation_type = animation;
		}

		return m;
	}

	std::optional<std::vector<std::array<float, 4>>> Position::parse_offsets(const std::string_view offsets) const
	{
		OffsetParser parser;
		return parser(offsets);
	}

	std::ostringstream& Position::ssprint(std::ostringstream& s) const
	{
		const bool is_effectively_valid = valid && (utils::string::to_lower(static_cast<const std::string>(linked_animation)) != "null");

		auto print_tags = [this](std::ostringstream& stream) {
			if (!tags.empty()) {
				stream << " tags=\"";
				bool first = true;
				std::for_each(tags.begin(), tags.end(), [&](const auto& t) {
					first ? (stream << t) : (stream << "," << t);
					first = false;
				});
				stream << "\"";
			}
		};

		auto print_locations = [this](std::ostringstream& stream) {
			if (!locations.empty()) {
				stream << " location=\"";
				bool first = true;
				std::for_each(locations.begin(), locations.end(), [&](const auto& t) {
					first ? (stream << t) : (stream << "," << t);
					first = false;
				});
				stream << "\"";
			}
		};

		auto print_offset = [this](std::ostringstream& stream) {
			auto prnt_offset = [](const std::array<float, 4>& a, std::ostringstream& str) -> std::ostringstream& {
				for (size_t i = 0; i < a.size(); ++i) {
					(i == 0) ? (str << a[i]) : (str << "," << a[i]);
				}
				return str;
			};

			if (offset.has_value()) {
				stream << " offset=\"";
				bool first = true;
				std::for_each(offset.value().begin(), offset.value().end(), [&](const auto& t) {
					if (first) {
						prnt_offset(t, stream);
					} else {
						stream << ":";
						prnt_offset(t, stream);
					}
					first = false;
				});
				stream << "\"";
			}
		};

		auto print_animation = [](Position::type l_type, const std::string& l_animation, std::ostringstream& stream) {
			switch (l_type) {
			case animation:
				stream << " animation=\"";
				break;
			case group:
				stream << " animationGroup=\"";
				break;
			case tree:
				stream << " positionTree=\"";
				break;
			}
			stream << l_animation << "\"";
		};

		// Header comment with file + invalid reason (if any)
		s << "\n<!-- FILE : " << filename;
		if (is_effectively_valid) {
			s << " -->\n";
		} else {
			s << "\n";
			if (!log.empty()) {
				s << log << "\n";
			}
			s << "-->\n";
		}

		const char* line_prefix = is_effectively_valid ? "\t" : "\t<!-- ";
		const char* line_suffix = is_effectively_valid ? "" : " -->";

		s << line_prefix;
		if (id.empty()) {
			s << "<position id=\"" << get_unknown_id() << "\"";
		} else {
			s << "<position id=\"" << id << "\"";
		}
		print_animation(linked_animation_type, linked_animation, s);
		if (startMorphSet)
			s << " startMorphSet=\"" << startMorphSet.value() << "\"";
		if (stopMorphSet)
			s << " stopMorphSet=\"" << stopMorphSet.value() << "\"";
		if (startEquipmentSet)
			s << " startEquipmentSet=\"" << startEquipmentSet.value() << "\"";
		if (stopEquipmentSet)
			s << " stopEquipmentSet=\"" << stopEquipmentSet.value() << "\"";
		s << " isHidden=\"" << (isHidden ? "true" : "false") << "\"";
		print_tags(s);
		print_locations(s);
		print_offset(s);
		s << "/>" << line_suffix;

		return s;
	}
}
