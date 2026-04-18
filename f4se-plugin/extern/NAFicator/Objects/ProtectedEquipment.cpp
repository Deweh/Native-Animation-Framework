#pragma once
#include "ProtectedEquipment.h"

namespace NAFicator
{
	std::size_t ProtectedEquipment::pair_hash::operator()(const std::pair<std::string, std::string>& pair) const
	{
		auto h1 = std::hash<std::string>{}(pair.first);
		auto h2 = std::hash<std::string>{}(pair.second);
		return h1 ^ (h2 << 1);
	}

	bool ProtectedEquipment::pair_equal::operator()(const std::pair<std::string, std::string>& a, const std::pair<std::string, std::string>& b) const
	{
		return a.first == b.first && a.second == b.second;
	}

	ProtectedEquipment& ProtectedEquipment::operator()(Data::XMLUtil::Mapper& m)
	{
		std::string form(""), source("");
		m.GetArray([&](Data::XMLUtil::Mapper& m) {
			m(&form, Data::XMLUtil::Mapper::emptyStr, true, true, "ProtectKeyword has no 'form' attribute!", "form");
			m(&source, Data::XMLUtil::Mapper::emptyStr, true, true, "ProtectKeyword has no 'source' attribute!", "source");

			if (!form.empty() && !source.empty()) {
				protected_keywords.emplace(normalize_form_id(form), normalize_source(source));
			}

			return m;
		},
			"protectKeyword", "Condition has no protectKeyword!");
		return *this;
	}
	
	std::ostringstream& ProtectedEquipment::operator()(std::ostringstream& s)
	{
		auto is_valid_keyword = [](const std::string& form, const std::string& source) {
			auto kwd = get_form_from_string(form, source);
			return (kwd && kwd->formType == RE::ENUM_FORM_ID::kKYWD);
		};
		
		s << "\t<condition>";
		std::for_each(protected_keywords.begin(), protected_keywords.end(), [&](const auto& kwd) {
			if (is_valid_keyword(kwd.first, kwd.second))
				s << "\n\t\t<protectKeyword form=\"" << kwd.first << "\" source=\"" << kwd.second << "\"/>";
		});
		s << "\n\t</condition>";
		return s;
	}
}
