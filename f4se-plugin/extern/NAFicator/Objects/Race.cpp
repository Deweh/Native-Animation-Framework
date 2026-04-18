#pragma once
#include "Race.h"

namespace NAFicator
{
	bool Race::is_valid()
	{
		bool v = (valid && !id.empty() && !form.empty() && !source.empty());
		if (valid && !v)
			set_invalid("Failed object validation!");
		return valid;
	}
	
	Data::XMLUtil::Mapper& Race::parse_id(Data::XMLUtil::Mapper& m)
	{
		m(&id, ""s, true, true, "Node has no 'id' attribute!", "skeleton");
		m.GetMinMax(&loadPriority, 0, true, false, "", INT32_MIN, INT32_MAX, "loadPriority");
		filename = m.GetFileName();
		return m;
	}

	bool Race::parse(Data::XMLUtil::Mapper& m)
	{
		parse_id(m);
		std::string tmp("");

		m(&form, Data::XMLUtil::Mapper::emptyStr, true, true, "Race has no form attribute!", "form");
		if (!form.empty()) {
			m(&source, Data::XMLUtil::Mapper::emptyStr, true, true, "Race has no source attribute!", "source");
		}
		
		m(&tmp, Data::XMLUtil::Mapper::emptyStr, true, false, "", "requiresReset");
		if (!tmp.empty()) {
			requiresReset = tmp == "true" ? true : false;
			tmp.clear();
		}

		m(&tmp, Data::XMLUtil::Mapper::emptyStr, true, false, "", "requiresForceLoop");
		if (!tmp.empty()) {
			requiresForceLoop = tmp == "true" ? true : false;
			tmp.clear();
		}

		m(&startEvent, std::optional<std::string>(std::nullopt), true, false, "Race has no startEvent attribute!", "startEvent");
		m(&stopEvent, std::optional<std::string>(std::nullopt), true, false, "Race has no stopEvent attribute!", "stopEvent");
		m(&graph, std::optional<std::string>(std::nullopt), true, false, "", "graph");

		return m;
	}

	std::ostringstream& Race::ssprint(std::ostringstream& s) const
	{
		
		s << "\n<!-- FILE : " << filename << (valid ? " -->\n" : "\n");
		s << ((!valid && !log.empty()) ? (log + "\n\n"s) : "");

		if (id.empty()) {
			s << "\t<race skeleton=\"" << get_unknown_id() << "\"";
		} else {
			s << "\t<race skeleton=\"" << id << "\"";
		}

		s << " form=\"" << form << "\" source=\"" << source << "\"";

		if (requiresReset.has_value()) {
			s << " requiresReset=\"" << (requiresReset.value() ? "true" : "false") << "\"";
		}
		if (requiresForceLoop.has_value()) {
			s << " requiresForceLoop=\"" << (requiresForceLoop.value() ? "true" : "false") << "\"";
		}
		if (startEvent.has_value()) {
			s << " startEvent=\"" << startEvent.value() << "\"";
		}
		if (stopEvent.has_value()) {
			s << " stopEvent=\"" << stopEvent.value() << "\"";
		}
		if (graph.has_value()) {
			s << " graph=\"" << graph.value() << "\"";
		}
		s << "/>" << (valid ? "" : " -->");

		return s;
	}
}
