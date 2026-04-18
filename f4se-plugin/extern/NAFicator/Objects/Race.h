#pragma once
#include "ParsedObject.h"

namespace NAFicator
{
	class Race :
		public ParsedObject
	{
	public:
		std::string form;
		std::string source;
		std::optional<bool> requiresReset;
		std::optional<bool> requiresForceLoop;
		std::optional<std::string> startEvent;
		std::optional<std::string> stopEvent;
		std::optional<std::string> graph;

		Race(Data::XMLUtil::Mapper& m) :
			ParsedObject(kRace) { parse(m); };

		bool parse(Data::XMLUtil::Mapper& m) override;
		Data::XMLUtil::Mapper& parse_id(Data::XMLUtil::Mapper& m) override;
		std::ostringstream& ssprint(std::ostringstream& s) const override;
		bool is_valid() override;
	};
}
