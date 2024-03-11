#pragma once

namespace Data
{
	class Condition
	{
	public:
		std::optional<bool> isFemale;
		std::optional<bool> isPlayer;
		std::optional<std::string> name;
		bool nameTrue = true;
		std::optional<std::string> rootBehavior;
		bool rootBehaviorTrue = true;
		std::optional<std::string> keyword;
		bool keywordTrue = true;
		std::optional<bool> isCompanion;
		bool isOverride = false;

		static std::string GetActorName(RE::Actor* targetActor)
		{
			std::string emptyName = "";

			if (!targetActor)
				return emptyName;

			auto* npc = targetActor->GetNPC();

			if (!npc)
				return emptyName;

			auto* fullName = npc->GetFullName();

			if (!fullName)
				return emptyName;

			return std::string(fullName);
		}

		bool IsTrue(RE::Actor* a) const {
			if (!a) {
				return false;
			}

			if (isFemale.has_value() && (a->GetSex() == 1) != isFemale.value()) {
				return false;
			}

			if (isPlayer.has_value() && (a == RE::PlayerCharacter::GetSingleton()) != isPlayer.value()) {
				return false;
			}

			if (name.has_value() && (GetActorName(a) == name.value()) != nameTrue) {
				return false;
			}

			if (rootBehavior.has_value() && (a->race->rootBehaviorGraphName->c_str() == rootBehavior.value()) != rootBehaviorTrue) {
				return false;
			}

			if (keyword.has_value()) {
				if (auto kw = RE::TESForm::GetFormByEditorID<RE::BGSKeyword>(keyword.value()); (kw && a->HasKeyword(kw)) != keywordTrue) {
					return false;
				}
			}

			if (isCompanion.has_value() && a->boolFlags.any(RE::Actor::BOOL_FLAGS::kDoNotShowOnStealthMeter) != isCompanion.value()) {
				return false;
			}

			return true;
		}

		void SkeletonToRootBehavior(const SkeletonMapType& map)
		{
			if (rootBehavior.has_value()) {
				if (auto iter = map.find(rootBehavior.value()); iter != map.end()) {
					rootBehavior = iter->second;
				} else {
					logger::warn("Condition uses invalid skeleton! Skeleton attribute will be ignored.");
					rootBehavior = std::nullopt;
				}
			}
		}

		static bool GetTrueFalseString(std::optional<std::string>& s) {
			if (!s.has_value()) {
				return true;
			}

			if (s.value().rfind("!", 0) != std::string::npos) {
				s = s.value().erase(0, 1);
				return false;
			} else {
				return true;
			}
		}

		void ParseNode(XMLUtil::Mapper& m) {
			m(&isFemale, std::optional<bool>(std::nullopt), false, false, "", "isFemale");
			m(&isPlayer, std::optional<bool>(std::nullopt), false, false, "", "isPlayer");

			m(&name, std::optional<std::string>(std::nullopt), false, false, "", "name");
			nameTrue = GetTrueFalseString(name);

			m(&rootBehavior, std::optional<std::string>(std::nullopt), false, false, "", "skeleton");
			rootBehaviorTrue = GetTrueFalseString(rootBehavior);

			m(&keyword, std::optional<std::string>(std::nullopt), false, false, "", "hasKeyword");
			keywordTrue = GetTrueFalseString(keyword);

			m(&isCompanion, std::optional<bool>(std::nullopt), false, false, "", "isCompanion");
			m(&isOverride, false, false, false, "", "isOverride");
		}
	};

	template <typename T>
	struct ConditionSet : public std::vector<std::pair<Condition, T>>
	{
		void Apply(RE::Actor* a) const
		{
			std::vector<size_t> passedConditions;
			for (size_t i = 0; i < std::vector<std::pair<Condition, T>>::size(); i++) {
				auto& data = std::vector<std::pair<Condition, T>>::at(i);
				if (data.first.IsTrue(a)) {
					if (!data.first.isOverride) {
						passedConditions.push_back(i);
					} else {
						passedConditions = { i };
						break;
					}
				}
			}

			for (auto& i : passedConditions) {
				auto& data = std::vector<std::pair<Condition, T>>::at(i);
				data.second.Apply(a);
			}
		}

		void SkeletonToRootBehavior(const SkeletonMapType& map)
		{
			for (auto iter = std::vector<std::pair<Condition, T>>::begin(); iter != std::vector<std::pair<Condition, T>>::end(); iter++) {
				iter->first.SkeletonToRootBehavior(map);
			}
		}

		void ParseNodes(XMLUtil::Mapper& m) {
			m.GetArray([&](XMLUtil::Mapper& m) {
				std::pair<Condition, T> newPair;

				newPair.first.ParseNode(m);
				newPair.second.ParseConditionNode(m);
				
				if (m)
					std::vector<std::pair<Condition, T>>::push_back(newPair);
				return m;
			}, "condition", "", false);
		}
	};

	class IDCondition : public IdentifiableObject, public Condition
	{
		static bool Parse(XMLUtil::Mapper& m, IDCondition& out) {
			out.ParseID(m);
			out.ParseNode(m);
			return m;
		}
	};
}
