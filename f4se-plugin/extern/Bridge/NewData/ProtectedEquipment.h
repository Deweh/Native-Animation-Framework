#pragma once

namespace Data
{
	class ProtectedKeyword : public IdentifiableObject
	{
	public:

		static inline std::vector<LinkableForm<RE::BGSKeyword>> forms;

		static bool Parse(XMLUtil::Mapper& m, ProtectedKeyword& out)
		{
			int depth = 0;
			
			StringFormInfo info;

			m.GetArray([&](XMLUtil::Mapper& m) {
				info.source.clear();
				info.form.clear();

				m(&info.form, XMLUtil::Mapper::emptyStr, false, true, "ProtectKeyword has no 'form' attribute!", "form");
				logger::info("ProtectedEquipmentData form : {}", info.form);
				m(&info.source, XMLUtil::Mapper::emptyStr, false, true, "ProtectKeyword has no 'source' attribute!", "source");
				logger::info("ProtectedEquipmentData source : {}", info.source);

				if ([](std::string& frm, std::string& src) {
						if (frm.empty()) {
							return false;
						}

						if (src.empty()) {
							return false;
						}

						return true;
					}(info.form, info.source))

				{
					out.forms.emplace_back(info);
				}		
				return m;
			},
				"protectKeyword", "Condition has no protectKeyword!");
			out.id = m.GetFileName();
			out.loadPriority = 0;

			while (depth)
				m.UpNode(),--depth;
			
			return out.forms.size();
		}
	};
}
