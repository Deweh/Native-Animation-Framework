#pragma once

namespace Data
{
	class EquipmentSet : public IdentifiableObject
	{
	public:
		inline static const std::unordered_map<std::string, uint8_t> bipedSlotNames{
			{ "Hair Top", 0ui8 },
			{ "Hair Long", 1ui8 },
			{ "FaceGen Head", 2ui8 },
			{ "BODY", 3ui8 },
			{ "L Hand", 4ui8 },
			{ "R Hand", 5ui8 },
			{ "[U] Torso", 6ui8 },
			{ "[U] L Arm", 7ui8 },
			{ "[U] R Arm", 8ui8 },
			{ "[U] L Leg", 9ui8 },
			{ "[U] R Leg", 10ui8 },
			{ "[A] Torso", 11ui8 },
			{ "[A] L Arm", 12ui8 },
			{ "[A] R Arm", 13ui8 },
			{ "[A] L Leg", 14ui8 },
			{ "[A] R Leg", 15ui8 },
			{ "Headband", 16ui8 },
			{ "Eyes", 17ui8 },
			{ "Beard", 18ui8 },
			{ "Mouth", 19ui8 },
			{ "Neck", 20ui8 },
			{ "Ring", 21ui8 },
			{ "Scalp", 22ui8 },
			{ "Decapitation", 23ui8 },
			{ "Unnamed1", 24ui8 },
			{ "Unnamed2", 25ui8 },
			{ "Unnamed3", 26ui8 },
			{ "Unnamed4", 27ui8 },
			{ "Unnamed5", 28ui8 },
			{ "Shield", 29ui8 },
			{ "Pipboy", 30ui8 },
			{ "FX", 31ui8 },
			{ "Possibly Weapons1", 32ui8 },
			{ "Possibly Weapons2", 33ui8 },
			{ "Possibly Weapons3", 34ui8 },
			{ "Possibly Weapons4", 35ui8 },
			{ "Possibly Weapons5", 36ui8 },
			{ "Possibly Weapons6", 37ui8 },
			{ "Possibly Weapons7", 38ui8 },
			{ "Possibly Weapons8", 39ui8 },
			{ "Possibly Weapons9", 40ui8 },
			{ "Possibly Weapons10", 41ui8 },
			{ "Possibly Weapons11", 42ui8 },
			{ "Possibly Weapons12", 43ui8 }
		};

		struct ReEquipData
		{
			bool resetAll = false;
			std::optional<uint8_t> slot = std::nullopt;
		};

		struct EquipmentObject
		{
			std::string form;
			std::string source;
			bool locked = false;
		};

		struct EquipManagerData
		{
			void Apply(RE::Actor* a) const {
				if (!a) {
					return;
				}

				for (auto& s : unEquips) {
					Scene::OrderedActionQueue::UnEquip(a, s);
				}

				for (auto& s : reEquips) {
					if (s.slot.has_value()) {
						Scene::OrderedActionQueue::ReEquip(a, s.slot.value());
					} else if (s.resetAll) {
						Scene::OrderedActionQueue::ReEquip(a);
					}
				}

				for (auto& obj : addEquipments) {
					Scene::OrderedActionQueue::AddEquipment(a, IdentifiableObject::StringsToForm<RE::TESObjectARMO>(obj.source, obj.form));
				}

				for (auto& obj : removeEquipments) {
					Scene::OrderedActionQueue::RemoveEquipment(a, IdentifiableObject::StringsToForm<RE::TESObjectARMO>(obj.source, obj.form));
				}
			}

			static std::optional<uint8_t> SlotNameToIndex(const std::string& slot) {
				std::optional<uint8_t> result = std::nullopt;
				auto iter = bipedSlotNames.find(slot);
				if (iter != bipedSlotNames.end()) {
					result = iter->second;
				}
				return result;
			}

			void ParseConditionNode(XMLUtil::Mapper& m) {
				m.GetArray([&](XMLUtil::Mapper& m) {
					std::string slot;
					m(&slot, ""s, false, false, "", "bipedSlot");
					auto uSlot = SlotNameToIndex(slot);
					if (uSlot.has_value()) {
						unEquips.insert(uSlot.value());
					}

					return m;
				}, "unEquip", "", false);

				m.GetArray([&](XMLUtil::Mapper& m) {
					ReEquipData data;

					m(&data.resetAll, false, false, false, "", "resetAll");

					if (data.resetAll) {
						reEquips.push_back(data);
						return m;
					}

					std::string slot;
					m(&slot, ""s, false, false, "", "bipedSlot");
					auto uSlot = SlotNameToIndex(slot);
					if (uSlot.has_value()) {
						data.slot = uSlot;
						reEquips.push_back(data);
					}

					return m;
				}, "reEquip", "", false);

				m.GetArray([&](XMLUtil::Mapper& m) {
					EquipmentObject obj;

					m(&obj.form, ""s, true, false, ""s, "form");
					if (obj.form.size() < 1) {
						return m;
					}
					m(&obj.source, ""s, true, false, ""s, "source");
					if (obj.source.size() < 1) {
						return m;
					}

					addEquipments.push_back(obj);

					return m;
				}, "addEquipment", "", false);

				m.GetArray([&](XMLUtil::Mapper& m) {
					EquipmentObject obj;

					m(&obj.form, ""s, true, false, ""s, "form");
					if (obj.form.size() < 1) {
						return m;
					}
					m(&obj.source, ""s, true, false, ""s, "source");
					if (obj.source.size() < 1) {
						return m;
					}

					removeEquipments.push_back(obj);

					return m;
				}, "removeEquipment", "", false);
			}

			std::unordered_set<uint8_t> unEquips;
			std::vector<ReEquipData> reEquips;
			std::vector<EquipmentObject> addEquipments;
			std::vector<EquipmentObject> removeEquipments;
		};

		static bool Parse(XMLUtil::Mapper& m, EquipmentSet& out)
		{
			out.ParseID(m);
			out.datas.ParseNodes(m);

			return m;
		}

		void Apply(RE::Actor* a) const
		{
			datas.Apply(a);
		}

		ConditionSet<EquipManagerData> datas;
	};
}
