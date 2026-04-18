#pragma once
#include "ParsedObject.h"

namespace NAFicator
{
	class Furniture :
		public ParsedObject
	{
	public:
		Furniture(Data::XMLUtil::Mapper& m) :
			ParsedObject(kFurniture) { parse(m); };

		std::optional<std::string> startAnimation;
		std::optional<std::string> stopAnimation;
		std::list<std::unordered_map<std::string, std::string>> furn;

		bool parse(Data::XMLUtil::Mapper& m) override;
		Data::XMLUtil::Mapper& parse_id(Data::XMLUtil::Mapper& m) override;
		std::ostringstream& ssprint(std::ostringstream& s) const override;
		bool is_valid() override;
		void check_links_to_forms_and_clear_badlinks();
	};
}
