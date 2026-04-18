#pragma once
#include "ParsedObject.h"

namespace NAFicator
{
	class Action : 
		public ParsedObject
	{
	public:
		Action(Data::XMLUtil::Mapper& m) :
			ParsedObject(kAction) { parse(m); };

		bool parse(Data::XMLUtil::Mapper& m) override;
		Data::XMLUtil::Mapper& parse_id(Data::XMLUtil::Mapper&) override;
		std::ostringstream& ssprint(std::ostringstream& ss) const override;
		bool is_valid() override;

		std::optional<std::string> startEquipmentSet;
		std::optional<std::string> stopEquipmentSet;
		std::optional<std::string> mfgSet;
	};
}
