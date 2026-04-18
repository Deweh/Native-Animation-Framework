#pragma once
#include "Animation.h"

namespace NAFicator
{
	bool Animation::is_valid()
	{
		bool v = (valid && !id.empty() && !actors.empty() && [this]() -> bool {
			for (const auto& a : actors) {
				if (!a.hkx.has_value() && !a.formId.has_value())
					return false;
			}
			return true;
		}());

		if (valid && !v)
			set_invalid("Failed object validation!");
		return valid;
	}
	
	Data::XMLUtil::Mapper& Animation::parse_id(Data::XMLUtil::Mapper& m)
	{
		return ParsedObject::parse_id(m);
	}

	std::vector<std::array<float, 4>> Animation::parse_offsets(const std::string_view offsets)
	{
		OffsetParser parser;
		return parser(offsets);
	}
	
	bool Animation::parse(Data::XMLUtil::Mapper& m)
	{
		//Заполняем id
		ParsedObject::parse_id(m);
		//Заполняем tags
		parse_tags(m);

		//парсим offset, заполняется позже, в actors
		std::string string_offset;
		std::vector<std::array<float, 4>> offsets;
		m(&string_offset, Data::XMLUtil::Mapper::emptyStr, true, false, "offset has no 'offset'", "offset");
		if (!string_offset.empty()) {
			offsets = parse_offsets(string_offset);
			string_offset.clear();
		}

		//Заполняем Actor
		m.GetArray([&](Data::XMLUtil::Mapper& m) {
			Actor a;

			//заполняем offset
			if (actors.size() < offsets.size()) {
				a.offset = offsets[actors.size()];
			} else if (offsets.size() == 1) {
				a.offset = offsets[0];  //если указан только один оффсет, то применяем его ко всем актёрам
			}

			//Заполняем hkx или formId
			std::string tmp("");
			m(&tmp, Data::XMLUtil::Mapper::emptyStr, false, false, "", "file");
			if (tmp.empty()) {
				m.GetOptNode(&tmp, Data::XMLUtil::Mapper::emptyStr, "idle", false, false,
					"Animation actor node has no idle form! Probably it is stop idle, then it is okay!", "idleForm", "form");
				if (!tmp.empty()) {
					std::string tmp_source("");
					m.GetOptNode(&tmp_source, Data::XMLUtil::Mapper::emptyStr, "idle", true, false,
						"Animation actor node has no idle source! Probably it is stop idle, then it is okay!", "idleSource", "source");
					if (!tmp_source.empty()) {
						a.formId = std::make_pair(tmp, tmp_source);
						tmp.clear();
					}
				}
			} else {
				a.hkx = tmp;
				tmp.clear();
			}
				
			//Заполняем skeleton
			m(&tmp, Data::XMLUtil::Mapper::emptyStr, true, false, "", "skeleton");
			if (!tmp.empty()) {
				a.skeleton = tmp;
				tmp.clear();
			}
			//Заполняем gender
			m(&tmp, Data::XMLUtil::Mapper::emptyStr, false, false, "", "gender");
			if (!tmp.empty()) {
				a.gender = tmp;
				tmp.clear();
			}

			//Заполняем mfgSet
			m(&tmp, Data::XMLUtil::Mapper::emptyStr, false, false, "", "mfgSet");
			if (!tmp.empty()) {
				a.faceAnim = tmp;
				tmp.clear();
			}

			//Заполняем faceAnim
			m(&tmp, Data::XMLUtil::Mapper::emptyStr, false, false, "", "faceAnim");
			if (!tmp.empty()) {
				a.faceAnim = tmp;
				tmp.clear();
			}

			//Заполняем loopFaceAnim
			m(&tmp, Data::XMLUtil::Mapper::emptyStr, false, false, "", "loopFaceAnim");
			if (!tmp.empty()) {
				a.loopFaceAnim = [](const std::string& b)->std::optional<bool> {
					if (b == "true")
						return true;
					else if (b == "false")
						return false;
					else {
						return std::nullopt;
					}
				}(tmp);
				tmp.clear();
			}
			//Заполняем startEquipmentSet
			m(&tmp, Data::XMLUtil::Mapper::emptyStr, true, false, "", "startEquipmentSet");
			if (!tmp.empty()) {
				a.startEquipmentSet = tmp;
				tmp.clear();
			}
			//Заполняем stopEquipmentSet
			m(&tmp, Data::XMLUtil::Mapper::emptyStr, true, false, "", "stopEquipmentSet");
			if (!tmp.empty()) {
				a.stopEquipmentSet = tmp;
				tmp.clear();
			}

			//Заполняем scale
			m(&tmp, Data::XMLUtil::Mapper::emptyStr, true, false, "", "scale");
			if (!tmp.empty()) {
				try {
					a.scale = std::stof(tmp);
				}
				catch (...) {
					//nothing to do
				}
				tmp.clear();
			}

			//Заполняем actions
			m.GetArray([&](Data::XMLUtil::Mapper& m) {
				std::optional<std::string> tmp;
				m(&tmp, std::optional<std::string>(std::nullopt), false, false, "actions node has no 'id'", "id");
				if (tmp.has_value()) {
					a.actions.push_back(tmp.value());
				}
				return m;
			}, "action", "", false);

			//Заполняем morphs
			m.GetArray([&](Data::XMLUtil::Mapper& m) {
				std::string mid("");
				float mto;
				m(&mid, Data::XMLUtil::Mapper::emptyStr, false, false, "morph node has no 'id'", "id");
				m(&mto, 0.0f, false, false, "morph node has to 'id'", "to");
					
				if (!mid.empty()) {
					if (!a.morphs.has_value()) {
						a.morphs = std::map<std::string, float>();
					}
					a.morphs->emplace(mid, mto);
				}

				return m;
			}, "morph", "", false);

			m.GetArray([&](Data::XMLUtil::Mapper& m) {
				std::string path("");
				std::string mid("");
				float mto;
				m(&path, Data::XMLUtil::Mapper::emptyStr, false, false, "value node has no 'path'", "path");
				m(&mid, Data::XMLUtil::Mapper::emptyStr, false, false, "value node has no 'value'", "value");
				m(&mto, 0.0f, false, false, "value node has no 'to'", "to");

				if (!mid.empty() && path == "morph") {
					if (!a.morphs.has_value()) {
						a.morphs = std::map<std::string, float>();
					}
					a.morphs->emplace(mid, mto);
				}

				return m;
			},
				"value", "", false);

			actors.push_back(a);
			return m; },
		"actor", "Animation node has no actors!", true);

		return m;
	}

	std::ostringstream& Animation::ssprint(std::ostringstream& s) const
	{
		auto print_tags = [this](std::ostringstream& s) {
			if (!tags.empty()) {
				s << " tags=\"";
				bool first = true;
				std::for_each(tags.begin(), tags.end(), [&](const auto& t) {
					first ? (s << t) : (s << "," << t);
					first = false;
				});
				s << "\"";
			}
		};

		/*auto print_offsets = [](const std::vector<Actor>& actors, std::ostringstream& stream) {
			if (actors.size() && actors[0].offset) {
				stream << " offset=\"";
				bool first = true;
				std::for_each(actors.begin(), actors.end(), [&first, &stream](const Actor& a) {
					auto& offset = a.offset.value();
					if (!first) {
						stream << ":";
					}
					stream << offset[0] << "," << offset[1] << "," << offset[2] << "," << offset[3];
					first = false;
				});
				stream << "\"";
			}
		};*/

		auto print_offsets = [](const std::vector<Actor>& actors, std::ostringstream& stream) {
			if (!actors.empty() && actors[0].offset) {
				stream << " offset=\"";
				bool first = true;
				std::for_each(actors.begin(), actors.end(), [&first, &stream, &actors](const Actor& a) {
					if (!first) {
						stream << ":";
					}
					if (a.offset) {
						auto& offset = a.offset.value();
						stream << offset[0] << "," << offset[1] << "," << offset[2] << "," << offset[3];
						
					} else if (actors[0].offset) {
						auto& offset = actors[0].offset.value();
						stream << offset[0] << "," << offset[1] << "," << offset[2] << "," << offset[3];
					}
					else {
						stream << "0,0,0,0";
					}
					first = false;
				});
				stream << "\"";
			}
		};
		
		auto print_actor_node = [](const Actor& a, std::ostringstream& s) {
			s << "\t\t<actor";
			if (a.hkx.has_value()) {
				s << " file=\"" << a.hkx.value() << "\"";
			} else if (a.formId.has_value()) {
				s << " form=\"" << a.formId.value().first << "\" source=\"" << a.formId.value().second << "\"";
			}
			if (a.loopFaceAnim.has_value()) {
				s << " loopFaceAnim=\"" << (a.loopFaceAnim.value() ? "true" : "false") << "\"";
			}
			if (a.scale.has_value()) {
				s << " scale=\"" << a.scale.value() << "\"";
			}
			if (a.gender.has_value()) {
				s << " gender=\"" << a.gender.value() << "\"";
			}
			if (a.skeleton.has_value()) {
				s << " skeleton=\"" << a.skeleton.value() << "\"";
			}
			if (a.faceAnim.has_value()) {
				s << " faceAnim=\"" << a.faceAnim.value() << "\"";
			}
			if (a.startEquipmentSet.has_value()) {
				s << " startEquipmentSet=\"" << a.startEquipmentSet.value() << "\"";
			}
			if (a.stopEquipmentSet.has_value()) {
				s << " stopEquipmentSet=\"" << a.stopEquipmentSet.value() << "\"";
			}
			s << ((a.morphs.has_value() || !a.actions.empty()) ? ">\n" : "/>\n");
			if (a.morphs.has_value()) {
				std::for_each(a.morphs.value().begin(), a.morphs.value().end(), [&](const std::pair<std::string, float>& m) {
					s << "\t\t\t<morph id=\"" << m.first << "\" to=\"" << std::to_string(m.second) << "\"/>\n";
				});
			}
			std::for_each(a.actions.begin(), a.actions.end(), [&](const auto& act) {
				if (act.has_value())
					s << "\t\t\t<action id=\"" << act.value() << "\"/>\n";
			});
			if (a.morphs.has_value() || !a.actions.empty())
				s << "\t\t</actor>\n";
		};

		s << "\n<!-- FILE : " << filename << (valid ? " -->\n" : "\n");
		s << ((!valid && !log.empty()) ? (log + "\n\n"s) : "");

		if (id.empty()) {
			s << "\t<animation id=\"" << get_unknown_id() << "\"";
		} else {
			s << "\t<animation id=\"" << id << "\"";
		}
		print_tags(s);
		print_offsets(actors, s);
		s << ">\n";
		std::for_each(actors.begin(), actors.end(), [&](const auto& a) {
			print_actor_node(a, s);
			s << "\n";
		});
		s << "\t</animation>" << (valid ? "" : " -->");
		return s;
	}
}
