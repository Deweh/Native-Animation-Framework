#pragma once
#include "Tag.h"

namespace NAFicator
{	
	void TagHolder::parse_tags(Data::XMLUtil::Mapper& m)
	{
		std::string combinedTags;
		m(&combinedTags, Data::XMLUtil::Mapper::emptyStr, false, false, "", "tags");
		if (combinedTags.size() > 0) {
			Utility::TransformStringToLower(combinedTags);
			Utility::ForEachSubstring(combinedTags, ",", [&](const std::string_view& strv) {
				tags.emplace(strv);
			});
		}
	}

	void TagHolder::merge_tags(const TagHolder& other)
	{
		tags.insert(other.tags.begin(), other.tags.end());
	}

	void TagHolder::merge_and_clear_tags(TagHolder& other)
	{
		merge_tags(other);
		other.tags.clear();
	}

	bool Tag::is_valid()
	{
		bool v = (valid && !id.empty() && !tags.empty());
		if (valid && !v)
			set_invalid("Failed object validation!");
		return valid;
	}

	Data::XMLUtil::Mapper& Tag::parse_id(Data::XMLUtil::Mapper& m)
	{
		m(&id, Data::XMLUtil::Mapper::emptyStr, false, true, "tag node has no 'position' attribute!", "position");
		m.GetMinMax(&loadPriority, 0, true, false, "", INT32_MIN, INT32_MAX, "loadPriority");
		filename = m.GetFileName();
		return m;
	}

	bool Tag::parse(Data::XMLUtil::Mapper& m)
	{
		parse_id(m);
		m(&replace, false, true, false, "", "replace");

		if (id.empty()) {
			return m;
		}

		parse_tags(m);
		return m;
	}

	std::ostringstream& Tag::ssprint(std::ostringstream& s) const
	{

		s << "\n<!-- FILE : " << filename << (valid ? " -->\n" : "\n");
		s << ((!valid && !log.empty()) ? (log + "\n\n"s) : "");

		if (id.empty()) {
			s << "<tag position=\"" << get_unknown_id() << "\" tags=\"";
		} else {
			s << "<tag position=\"" << id << "\" tags=\"";
		}
		std::for_each(tags.begin(), tags.end(), [&](const auto& t) {
			if (*tags.begin() != t)
				s << ",";
			s << t;
		});
		s << "\"/>" << (valid ? "" : " -->");
		return s;
	}
}
