#pragma once

namespace Data
{
	class Race : public IdentifiableObject
	{
	public:
		LinkableForm<RE::TESRace> baseForm;
		bool requiresReset = false;
		bool requiresForceLoop = false;
		std::optional<std::string> startEvent = std::nullopt;
		std::optional<std::string> stopEvent = std::nullopt;
		std::optional<std::string> graph = std::nullopt;

		static bool Parse(XMLUtil::Mapper& m, Race& out)
		{
			m(&out.id, ""s, true, true, "Race has no skeleton attribute!", "skeleton");
			m.GetMinMax(&out.loadPriority, 0, true, false, "", INT32_MIN, INT32_MAX, "loadPriority");
			std::string form;
			std::string source;
			m(&form, ""s, true, true, "Race has no form attribute!", "form");
			m(&source, ""s, true, true, "Race has no source attribute!", "source");

			if (m)
				out.baseForm.set(source, form);

			m(&out.requiresReset, false, true, false, "", "requiresReset");
			m(&out.requiresForceLoop, false, true, false, "", "requiresForceLoop");
			m(&out.startEvent, std::optional<std::string>(std::nullopt), true, false, "", "startEvent");
			m(&out.stopEvent, std::optional<std::string>(std::nullopt), true, false, "", "stopEvent");
			m(&out.graph, std::optional<std::string>(std::nullopt), true, false, "", "graph");

			return m;
		}
	};

	
}
