#pragma once
#include "AnimationGroup.h"

namespace NAFicator
{
	bool AnimationGroup::is_valid()
	{
		bool v = (valid && !id.empty() && !stages.empty() && [this] {
			for (const auto& s : stages) {
				if (s.animation.empty())
					return false;
			}
			return true;
		}());

		if (valid && !v)
			set_invalid("Failed object validation!");
		return valid;
	}
	
	Data::XMLUtil::Mapper& AnimationGroup::parse_id(Data::XMLUtil::Mapper& m)
	{
		ParsedObject::parse_id(m);
		std::string tmp("");
		m(&tmp, Data::XMLUtil::Mapper::emptyStr, false, false, "", "sequential");
		if (!tmp.empty()) {
			sequential = tmp == "true" ? true : false;
			tmp.clear();
		}
		return m;
	}
	
	bool AnimationGroup::parse(Data::XMLUtil::Mapper& m)
	{
		parse_id(m);
		
		m.GetArray([&](Data::XMLUtil::Mapper& m) -> bool {
			stage s;
			m(&s.animation, Data::XMLUtil::Mapper::emptyStr, false, true, "stage node has no 'animation'", "animation");

			std::string loops("");
			m(&loops, Data::XMLUtil::Mapper::emptyStr, false, false, "", "loops");
			if (!loops.empty()) {
				try {
					s.loops = stoul(loops);
				}
				catch (...) {
					//nothing
				}
			}

			m(&s.weight, uint32_t(1), false, false, "", "weight");

			stages.push_back(s);

			return true;
		},
			"stage"sv, "AnimationGroup has no 'stage' nodes", false);

		return true;
	}

	std::ostringstream& AnimationGroup::ssprint(std::ostringstream& s) const
	{
		auto print_stage_node = [](const stage& e, std::ostringstream& s) {
			s << "\t\t<stage animation=\"" << e.animation << "\" weight=\"" << e.weight << "\"";
			if (e.loops.has_value()) {
				s << " loops=\"" << e.loops.value() << "\"";
			}
			s << "/>\n";
		};

		s << "\n<!-- FILE : " << filename << (valid ? " -->\n" : "\n");
		s << ((!valid && !log.empty()) ? (log + "\n\n"s) : "");

		if (id.empty()) {
			s << "\t<animationGroup id=\"" << get_unknown_id() << "\"";
		} else {
			s << "\t<animationGroup id=\"" << id << "\"";
		}
		if (sequential.has_value())
			s << " sequential=\"" << (sequential.value() ? "true" : "false") << "\"";
		s << ">\n";
		std::for_each(stages.begin(), stages.end(), [&](const auto& stg) {
			print_stage_node(stg, s);
		});
		s << "\t</animationGroup>" << (valid ? "" : " -->");
		return s;
	}

	std::vector<AnimationGroup::stage>::const_iterator AnimationGroup::find_animation(const std::string& animationId) const
	{
		return std::find_if(stages.begin(), stages.end(), [&animationId](const stage& a) {
			return a.animation == animationId;
		});
	}

	std::vector<AnimationGroup::stage>::iterator AnimationGroup::find_animation(const std::string& animationId)
	{
		return std::find_if(stages.begin(), stages.end(), [&animationId](const stage& a) {
			return a.animation == animationId;
		});
	}
				
}
