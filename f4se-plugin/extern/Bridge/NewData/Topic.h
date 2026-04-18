#pragma once

namespace Data
{
	class Topic : public IdentifiableObject
	{
	public:
		static bool Parse(XMLUtil::Mapper& m, Action& out)
		{
			out.ParseID(m);

			if (!m.DownNode("self", "", false))
				return false;

			m(&out.startEquipSet, std::optional<std::string>(std::nullopt), true, false, "", "startEquipmentSet");
			m(&out.stopEquipSet, std::optional<std::string>(std::nullopt), true, false, "", "stopEquipmentSet");

			return m;
		}

		std::optional<std::string> startEquipSet;
		std::optional<std::string> stopEquipSet;

		void RunStart(RE::Actor* a) const;
		void RunStop(RE::Actor* a) const;
	};

	struct ActionSet : public std::vector<std::string>
	{
		bool Parse(XMLUtil::Mapper& m)
		{
			std::optional<std::string> id;
			bool hasData = false;

			size_t i = 0;
			m.GetArray([&](XMLUtil::Mapper& m) {
				m(&id, std::optional<std::string>(std::nullopt), false, false, "", "id");
				if (id.has_value()) {
					std::vector<std::string>::at(i) = id.value();
				}

				hasData = true;
				i++;
				return m; }, "action", "", false, [&](size_t size) { std::vector<std::string>::resize(size); });

			return hasData;
		}

		void RunStart(RE::Actor* a) const;
		void RunStop(RE::Actor* a) const;
	};
}
