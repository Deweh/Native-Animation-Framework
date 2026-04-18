#pragma once
#include "ParsedObject.h"

namespace NAFicator
{
	class MorphSet :
		public ParsedObject
	{
	public:	
		MorphSet(Data::XMLUtil::Mapper& m) :
			ParsedObject(kMorphSet) { parse(m); };

		typedef std::map<std::string, float> pair_key_kv_pair;

		std::vector<std::pair<ConditionsSet, pair_key_kv_pair>> morphs;

		Data::XMLUtil::Mapper& parse_id(Data::XMLUtil::Mapper& m) override;
		bool parse(Data::XMLUtil::Mapper& m) override;
		std::ostringstream& ssprint(std::ostringstream& s) const override;
		bool is_valid() override;
	};
}
