#pragma once

#define DEF_STR(n, def)												\
	std::string n = def;											\
	namespace detail {												\
		bool n##_init = ([]() { StrMap[#n] = &n; return true; })(); \
	}

struct CaseInsensitiveStringHash
{
	std::size_t operator()(const std::string& str) const
	{
		std::size_t hash = std::_FNV_offset_basis;
		for (char c : str) {
			hash ^= static_cast<std::size_t>(std::tolower(c));
			hash *= std::_FNV_prime;
		}

		return hash;
	}
};

struct CaseInsensitiveStringEqual
{
	bool operator()(const std::string& lhs, const std::string& rhs) const
	{
		return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
			[](char a, char b) {
				return std::tolower(a) == std::tolower(b);
			});
	}
};

namespace Strings
{
	namespace detail
	{
		std::unordered_map<std::string, std::string*, CaseInsensitiveStringHash, CaseInsensitiveStringEqual> StrMap;
	}

	std::string* Find(const std::string& a_id) {
		if (auto iter = detail::StrMap.find(a_id); iter != detail::StrMap.end()) {
			return iter->second;
		}
		return nullptr;
	}

	DEF_STR(ScnErr_BadLocation, "No location ref, or location ref has no parent cell.");
	DEF_STR(ScnErr_NoActors, "Actor array must have at least one actor.");
	DEF_STR(ScnErr_InvalidActor, "One or more provided actors are None/Disabled/Dead.");
	DEF_STR(ScnErr_InScene, "One or more provided actors are already in a scene and/or marked as unusable.");
	DEF_STR(ScnErr_WrongWorldspace, "Player must be in the same worldspace as the scene location.");
	DEF_STR(ScnErr_NullPosition, "The provided position is no longer available.");
	DEF_STR(ScnErr_AlreadyStarted, "The scene has already been started.");
	DEF_STR(ScnErr_NoAvailablePositions, "No available positions for provided combination of actors.");
	DEF_STR(ScnErr_SpecifiedPositionNotAvailable, "Provided position ID is not installed or not available for the provided combination of actors.");
}

#undef DEF_STR
