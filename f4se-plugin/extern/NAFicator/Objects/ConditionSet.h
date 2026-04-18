#pragma once
#include "ObjectsHeader.h"

namespace NAFicator 
{
	struct ConditionsSet
	{
		std::optional<bool> isFemale;
		std::optional<bool> isPlayer;
		std::optional<std::string> name;
		std::optional<std::string> skeleton;
		std::optional<std::string> hasKeyword;
		std::optional<std::string> raceFormID;

		inline void clear()
		{
			isFemale.reset();
			isPlayer.reset();
			name.reset();
			skeleton.reset();
			hasKeyword.reset();
			raceFormID.reset();
		}
		ConditionsSet(Data::XMLUtil::Mapper& m);
		bool operator==(const ConditionsSet& other) const;
		ConditionsSet parse_conditions(Data::XMLUtil::Mapper& m);
		std::ostringstream& print_conditions(std::ostringstream& s) const;
	};

	void parse_conditions(ConditionsSet& cond, Data::XMLUtil::Mapper& m);
	std::ostringstream& print_conditions(const ConditionsSet& c, std::ostringstream& s);

	struct ConditionsSetHasher
	{
		std::size_t operator()(const ConditionsSet& c) const;
	};
}
