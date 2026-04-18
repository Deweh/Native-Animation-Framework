#pragma once
#include "ParsedObject.h"

extern RE::TESForm* get_form_from_string(const std::string& xFormID, const std::string& plugin);

namespace NAFicator
{
	class ProtectedEquipment
	{	
		struct pair_hash
		{
			std::size_t operator()(const std::pair<std::string, std::string>& pair) const;
		};

		struct pair_equal
		{
			bool operator()(const std::pair<std::string, std::string>& a, const std::pair<std::string, std::string>& b) const;
		};
		
		static inline std::unordered_set<std::pair<std::string, std::string>,
			pair_hash, pair_equal>
			protected_keywords;

	public:

		ProtectedEquipment& operator()(Data::XMLUtil::Mapper& m);
		std::ostringstream& operator()(std::ostringstream& s);
	};

	

	
}
