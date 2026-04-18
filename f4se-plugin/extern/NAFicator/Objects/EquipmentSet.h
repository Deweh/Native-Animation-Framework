#pragma once
#include "ParsedObject.h"

namespace NAFicator
{
	class EquipmentSet :
		public ParsedObject
	{
	public:
		typedef std::vector<std::pair<std::string, std::string>> attributes_map;
		typedef std::vector<std::pair<std::string, attributes_map>> nodes_map;

		EquipmentSet(Data::XMLUtil::Mapper& m) :
			ParsedObject(kEquipmentSet) { parse(m); };

		std::vector<std::pair<ConditionsSet, nodes_map>> equipmentSet_map;

		Data::XMLUtil::Mapper& parse_id(Data::XMLUtil::Mapper& m) override;
		bool parse(Data::XMLUtil::Mapper& m) override;
		std::ostringstream& ssprint(std::ostringstream& s) const override;
		bool is_valid() override;
	};
}
