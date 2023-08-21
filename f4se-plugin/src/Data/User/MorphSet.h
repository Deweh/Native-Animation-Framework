#pragma once

namespace Data
{
	struct MorphPair
	{
		std::string name;
		float value;

		template <class Archive>
		void serialize(Archive& ar, const uint32_t)
		{
			ar(name, value);
		}
	};

	struct Morphs : public std::vector<MorphPair>
	{
	public:
		void Apply(RE::Actor* a) const;

		void ProcessApply(RE::Actor* a) const {
			if (!a)
				return;

			for (auto iter = std::vector<MorphPair>::begin(); iter != std::vector<MorphPair>::end(); iter++) {
				LooksMenu::SetMorph(a, iter->name, nullptr, iter->value);
			}
			LooksMenu::UpdateMorphs(a);
		}

		void ParseConditionNode(XMLUtil::Mapper& m) {
			m.GetArray([&](XMLUtil::Mapper& m) {
				MorphPair p;
				m(&p.name, ""s, true, true, "morph node has no 'value' attribute!", "value");
				m(&p.value, 0.0f, false, true, "morph node has no 'to' attribute!", "to");
				if (m) {
					std::vector<MorphPair>::push_back(p);
				}
				return m;
			}, "morph", "", false);
		}
	};

	class MorphSet : public IdentifiableObject
	{
	public:
		static bool Parse(XMLUtil::Mapper& m, MorphSet& out)
		{
			out.ParseID(m);
			out.morphs.ParseNodes(m);

			return m;
		}

		void Apply(RE::Actor* a) const {
			morphs.Apply(a);
		}

		ConditionSet<Morphs> morphs;
	};
}
