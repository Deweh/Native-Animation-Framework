#pragma once
#include "ParsedObject.h"
#include "Tag.h"
#include "OffsetParser.hpp"

namespace NAFicator
{
	class Animation :
		public ParsedObject, public TagHolder
	{
	public:
		struct Actor
		{
			std::optional<bool> loopFaceAnim;
			std::optional<float> scale;
			std::optional<std::string> gender;
			std::optional<std::string> hkx;
			std::optional<std::pair<std::string, std::string>> formId;
			std::optional<std::array<float, 4>> offset;
			std::optional<std::string> skeleton;
			std::optional<std::string> faceAnim;
			std::optional<std::map<std::string, float>> morphs;
			std::optional<std::string> startEquipmentSet;
			std::optional<std::string> stopEquipmentSet;
			std::vector<std::optional<std::string>> actions;
		};

		Animation(Data::XMLUtil::Mapper& m) :
			ParsedObject(kAnimation) { parse(m); };

		std::vector<Actor> actors;

		bool parse(Data::XMLUtil::Mapper& m) override;	
		Data::XMLUtil::Mapper& parse_id(Data::XMLUtil::Mapper& m) override;
		std::ostringstream& ssprint(std::ostringstream& ss) const override;
		bool is_valid() override;

		std::vector<std::array<float, 4>> parse_offsets(const std::string_view offsets);
	};
}
