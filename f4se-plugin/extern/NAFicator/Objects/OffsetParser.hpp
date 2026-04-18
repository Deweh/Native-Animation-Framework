#pragma once
#include "ParsedObject.h"
#include <IniParser/Ini.h>

extern ini::map inimap;

namespace NAFicator
{
	struct OffsetParser
	{
		inline std::vector<std::array<float, 4>> operator()(const std::string_view offsets, const std::string_view filename = "", const std::string_view id = "") const
		{
			std::vector<std::array<float, 4>> result;

			if (offsets.empty())
				return result;

			auto normalize = [&result, &filename, &id]() {
				int setting = inimap.get<int>("iRemoveFixWrongZOffsets"s, "General"s);
				if (setting == 0)
					return;
				int max = inimap.get<float>("fzOffsetMax"s, "General"s);
				for (auto& r : result) {
					if (r[2] > max) {
						if (setting == 1) {
							r[2] = r[3];
						} else if (setting == 2) {
							r[2] = 0;
						}
						logger::info("Offset fixed in file : {}, id : {}", filename, id);
					}
				}
			};

			Utility::ForEachSubstring(offsets, ":", [&](const std::string_view& off) {
				std::array<float, 4> t = { 0.f, 0.f, 0.f, 0.f };
				size_t count = 0;

				Utility::ForEachSubstring(off, ",", [&](const std::string_view& part) {
					if (count < t.size()) {
						auto f = Utility::StringToFloat(part);
						if (f) {
							t[count] = f;
						}
						count++;
					}
				});

				if (count > 0) {
					result.emplace_back(t);
				}
			});

			normalize();
			return result;
		}
	};
}
