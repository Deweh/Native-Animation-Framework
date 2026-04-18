#include "ConditionSet.h"

namespace NAFicator
{
	void parse_conditions(ConditionsSet& cond, Data::XMLUtil::Mapper& m) {
		std::string tmp;

		m(&tmp, Data::XMLUtil::Mapper::emptyStr, true, false, "", "isFemale");
		if (!tmp.empty()) {
			cond.isFemale = tmp == "true" ? true : false;
			tmp.clear();
		}

		m(&tmp, Data::XMLUtil::Mapper::emptyStr, true, false, "", "isPlayer");
		if (!tmp.empty()) {
			cond.isPlayer = tmp == "true" ? true : false;
			tmp.clear();
		}

		m(&tmp, Data::XMLUtil::Mapper::emptyStr, true, false, "", "name");
		if (!tmp.empty()) {
			cond.name = tmp;
			tmp.clear();
		}

		m(&tmp, Data::XMLUtil::Mapper::emptyStr, true, false, "", "skeleton");
		if (!tmp.empty()) {
			cond.skeleton = tmp;
			tmp.clear();
		}

		m(&tmp, Data::XMLUtil::Mapper::emptyStr, true, false, "", "hasKeyword");
		if (!tmp.empty()) {
			cond.hasKeyword = tmp;
			tmp.clear();
		}

		m(&tmp, Data::XMLUtil::Mapper::emptyStr, true, false, "", "raceFormID");
		if (!tmp.empty()) {
			cond.raceFormID = tmp;
			tmp.clear();
		}
	}
	
	ConditionsSet ConditionsSet::parse_conditions(Data::XMLUtil::Mapper& m)
	{
		NAFicator::parse_conditions(*this, m);
		return *this;
	}

	ConditionsSet::ConditionsSet(Data::XMLUtil::Mapper& m)
	{
		NAFicator::parse_conditions(*this, m);
	}

	bool ConditionsSet::operator==(const ConditionsSet& other) const
	{
		return std::tie(isFemale, isPlayer, name, skeleton, hasKeyword, raceFormID) ==
		       std::tie(other.isFemale, other.isPlayer, other.name, other.skeleton, other.hasKeyword, other.raceFormID);
	}

	std::ostringstream& print_conditions(const ConditionsSet& c, std::ostringstream& s)
	{
		if (c.isFemale.has_value()) {
			s << " isFemale=\"" << (c.isFemale.value() ? "true" : "false") << "\"";
		}
		if (c.isPlayer.has_value()) {
			s << " isPlayer=\"" << (c.isPlayer.value() ? "true" : "false") << "\"";
		}
		if (c.name.has_value()) {
			s << " name=\"" << c.name.value() << "\"";
		}
		if (c.skeleton.has_value()) {
			s << " skeleton=\"" << c.skeleton.value() << "\"";
		}
		if (c.hasKeyword.has_value()) {
			s << " hasKeyword=\"" << c.hasKeyword.value() << "\"";
		}
		if (c.raceFormID.has_value()) {
			s << " raceFormID=\"" << c.raceFormID.value() << "\"";
		}
		return s;
	}

	std::ostringstream& ConditionsSet::print_conditions(std::ostringstream& s) const
	{
		return NAFicator::print_conditions(*this, s);
	}

	std::size_t ConditionsSetHasher::operator()(const ConditionsSet& c) const
	{
		std::size_t h = 0;
		auto hashOptional = [](const auto& opt) {
			return opt.has_value() ? std::hash<typename std::decay<decltype(*opt)>::type>()(*opt) : 0;
		};
		h ^= hashOptional(c.isFemale) << 1;
		h ^= hashOptional(c.isPlayer) << 2;
		h ^= hashOptional(c.name) << 3;
		h ^= hashOptional(c.skeleton) << 4;
		h ^= hashOptional(c.hasKeyword) << 5;
		h ^= hashOptional(c.raceFormID) << 6;

		return h;
	}
}
