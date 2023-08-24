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
			std::string kw;
			m.GetArray([&](XMLUtil::Mapper& m) {
				source.clear();
				form.clear();
				kw.clear();
				bool hasOne = false;

				m(&source, ""s, true, false, "", "source");
				m(&form, ""s, true, false, "", "form");

				if (!source.empty() || !form.empty()) {
					out.forms.emplace_back(source, form);
					hasOne = true;
				}

				m(&kw, ""s, true, false, "", "keyword");

				if (!kw.empty()) {
					out.keywords.emplace_back(kw);
					hasOne = true;
				}

				if (!hasOne) {
					m.CustomFail("Furniture node has no applicable attributes!");
				}

				return m;
			}, "furniture", "Furniture group has no 'furniture' nodes!");

			m(&out.startAnim, std::optional<std::string>(std::nullopt), true, false, "", "startAnimation");
			m(&out.stopAnim, std::optional<std::string>(std::nullopt), true, false, "", "stopAnimation");

			return m;
		}

		std::vector<std::string> keywords;
		std::vector<LinkableForm<RE::TESBoundObject>> forms;
		std::optional<std::string> startAnim;
		std::optional<std::string> stopAnim;
	};
}
