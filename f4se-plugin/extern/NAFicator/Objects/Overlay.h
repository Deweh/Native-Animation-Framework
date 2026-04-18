#pragma once
#include "ParsedObject.h"
namespace NAFicator
{
	class Overlay :
		public ParsedObject
	{
	public:
		Overlay(Data::XMLUtil::Mapper& m) :
			ParsedObject(kOverlay) { parse(m); };

		struct overlay_group_setting
		{
			int duration;
			int quantity;
		};
		
		struct overlay_settings
		{
			int alpha;
			bool isFemale;
			/*std::optional<int> red;
			std::optional<int> green;
			std::optional<int> blue;
			std::optional<int> offset_u;
			std::optional<int> offset_v;
			std::optional<int> scale_u;
			std::optional<int> scale_v;
			std::optional<int> priority;*/

			bool operator==(const overlay_settings& other) const;
			bool operator<(const overlay_settings& other) const;
			size_t operator()() const;
		};

		typedef std::pair<overlay_group_setting, std::unordered_map<std::string, overlay_settings>> overlay_group;

		std::vector<std::pair<ConditionsSet, std::vector<overlay_group>>> overlays;

		Data::XMLUtil::Mapper& parse_id(Data::XMLUtil::Mapper& m) override;
		bool parse(Data::XMLUtil::Mapper& m) override;
		std::ostringstream& ssprint(std::ostringstream& s) const override;
		bool is_valid() override;
	};
}
