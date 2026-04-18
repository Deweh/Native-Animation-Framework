#pragma once
#include "ParsedObject.h"

namespace NAFicator
{
	class MfgSet :
		public ParsedObject
	{
	public:
		MfgSet(Data::XMLUtil::Mapper& m) :
			ParsedObject(kMfgSet) { parse(m); };

		struct morph
		{
			int32_t intensity;
			std::optional<bool> lock;
		};

		std::map<uint8_t, morph> morphs;

		Data::XMLUtil::Mapper& parse_id(Data::XMLUtil::Mapper& m) override;
		bool parse(Data::XMLUtil::Mapper& m) override;
		std::ostringstream& ssprint(std::ostringstream& s) const override;
		bool is_valid() override;
	};
}
