#pragma once
#include "ParsedObject.h"

namespace NAFicator
{
	class AnimationGroup :
		public ParsedObject
	{
	public:
		struct stage
		{
			uint32_t weight;
			std::optional<size_t> loops;
			std::string animation;
		};

		AnimationGroup(Data::XMLUtil::Mapper& m) :
			ParsedObject(kAnimationGroup) { parse(m); };

		std::optional<bool> sequential;
		std::vector<stage> stages;

		bool parse(Data::XMLUtil::Mapper& m) override;
		Data::XMLUtil::Mapper& parse_id(Data::XMLUtil::Mapper& m) override;
		std::ostringstream& ssprint(std::ostringstream& s) const override;
		bool is_valid() override;

		std::vector<AnimationGroup::stage>::const_iterator find_animation(const std::string& animationId) const;
		std::vector<AnimationGroup::stage>::iterator find_animation(const std::string& animationId);
	};
}
