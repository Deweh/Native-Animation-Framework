#pragma once
#include "ParsedObject.h"
#include "OffsetParser.hpp"

namespace NAFicator
{
	class Position :
		public ParsedObject
	{
	public:
		enum type
		{
			animation,
			group,
			tree
		};

		bool isHidden = false;
		type linked_animation_type = animation;

		std::string linked_animation;
		std::set<std::string> tags;
		std::optional<std::string> startMorphSet;
		std::optional<std::string> stopMorphSet;
		std::optional<std::string> startEquipmentSet;
		std::optional<std::string> stopEquipmentSet;
		std::set<std::string> locations;
		std::optional<std::vector<std::array<float, 4>>> offset;

		Position(Data::XMLUtil::Mapper& m) :
			ParsedObject(kPosition) { parse(m); };

		Data::XMLUtil::Mapper& parse_id(Data::XMLUtil::Mapper& m) override;
		bool parse(Data::XMLUtil::Mapper& m) override;
		std::ostringstream& ssprint(std::ostringstream& s) const override;
		bool is_valid() override;

		std::optional<std::vector<std::array<float, 4>>> parse_offsets(const std::string_view offsets) const;
	};
}
