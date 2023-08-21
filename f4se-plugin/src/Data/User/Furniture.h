#pragma once

namespace Data
{
	typedef std::shared_ptr<std::vector<LinkableForm<RE::TESBoundObject>>> FurnitureList;

	class Furniture : public IdentifiableObject
	{
	public:
		static bool Parse(XMLUtil::Mapper& m, Furniture& out) {
			out.ParseID(m);

			std::string source;
			std::string form;
			m.GetArray([&](XMLUtil::Mapper& m) {
				m(&source, ""s, true, true, "Furniture node has no 'source' attribute!", "source");
				m(&form, ""s, true, true, "Furniture node has no 'form' attribute!", "form");

				if (m)
					out.forms.emplace_back(source, form);

				return m;
			}, "furniture", "Furniture group has no 'furniture' nodes!");

			m(&out.startAnim, std::optional<std::string>(std::nullopt), true, false, "", "startAnimation");
			m(&out.stopAnim, std::optional<std::string>(std::nullopt), true, false, "", "stopAnimation");

			return m;
		}

		std::vector<LinkableForm<RE::TESBoundObject>> forms;
		std::optional<std::string> startAnim;
		std::optional<std::string> stopAnim;
	};
}
