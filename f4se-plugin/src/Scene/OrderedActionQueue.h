#pragma once

namespace Scene
{
	using namespace Serialization::General;

	class OrderedActionQueue
	{
	public:
		struct EquipInformation
		{
			std::unordered_map<uint8_t, SerializableBaseForm> storedEquipment;
			std::vector<SerializableBaseForm> addedEquipment;

			template <class Archive>
			void serialize(Archive& ar, const uint32_t)
			{
				ar(storedEquipment, addedEquipment);
			}
		};

		struct PersistentState
		{
			std::unordered_map<SerializableActorHandle, EquipInformation> equipment;

			template <class Archive>
			void serialize(Archive& ar, const uint32_t)
			{
				ar(equipment);
			}
		};

		inline static std::unique_ptr<PersistentState> state = std::make_unique<PersistentState>();
		inline static std::queue<std::function<bool()>> equipQueue;
		inline static uint16_t delayCounter = 0;
		inline static safe_mutex lock;

		static void Update() {
			std::unique_lock l{ lock };
			if (delayCounter > 0) {
				delayCounter--;
				return;
			}
			while (!equipQueue.empty()) {
				auto& func = equipQueue.front();
				func();
				equipQueue.pop();

				if (delayCounter > 0) {
					break;
				}
			}
		}

		static void InsertCustom(const std::function<bool()>& func) {
			std::unique_lock l{ lock };
			equipQueue.push(func);
		}

		static void InsertDelay(uint8_t frames) {
			std::unique_lock l{ lock };
			equipQueue.push([frames = frames]() { delayCounter += frames; return true; });
		}

		static void UnEquip(RE::Actor* a, uint8_t slot) {
			std::unique_lock l{ lock };
				if (!a || !a->biped)
					return;
			
			equipQueue.push(std::function<bool()>(std::bind(&ProcessUnEquip, RE::NiPointer<RE::Actor>(a), slot)));
		}

		static void ReEquip(RE::Actor* a, std::optional<uint8_t> slot = std::nullopt) {
			std::unique_lock l{ lock };
				if (!a || !a->biped)
					return;
			
			equipQueue.push(std::function<bool()>(std::bind(&ProcessReEquip, RE::NiPointer<RE::Actor>(a), slot)));
		}

		static void Reset() {
			std::unique_lock l{ lock };
			state->equipment.clear();
			equipQueue = std::queue<std::function<bool()>>();
			delayCounter = 0;
		}

		static void AddEquipment(RE::Actor* a, RE::TESBoundObject* equipForm) {
			std::unique_lock l{ lock };
			if (!a || !a->biped || !equipForm)
				return;

			equipQueue.push(std::function<bool()>(std::bind(&ProcessAddEquipment, RE::NiPointer<RE::Actor>(a), equipForm)));
		}

		static void RemoveEquipment(RE::Actor* a, RE::TESBoundObject* equipForm)
		{
			std::unique_lock l{ lock };
			if (!a || !a->biped || !equipForm)
				return;

			equipQueue.push(std::function<bool()>(std::bind(&ProcessRemoveEquipment, RE::NiPointer<RE::Actor>(a), equipForm)));
		}

		static void ApplyMorphs(RE::Actor* a, const Data::Morphs& m) {
			std::unique_lock l{ lock };
			if (!a)
				return;

			equipQueue.push(std::function<bool()>(std::bind(&ProcessApplyMorphs, RE::NiPointer<RE::Actor>(a), m)));
		}

		private:
		static bool ProcessUnEquip(RE::NiPointer<RE::Actor> a, uint8_t slot)
		{
			auto targetItem = a->biped->object[slot].parent;
			if (!targetItem.object)
				return true;

			if (auto bObj = targetItem.object->As<RE::TESObjectARMO>(); bObj) {
				if (bObj->HasKeyword(Data::Forms::NAFDoNotUseKW)) {
					return true;
				}
			}

			if (targetItem.instanceData != nullptr) {
				if (auto kwForm = targetItem.instanceData->GetKeywordData(); kwForm && kwForm->HasKeyword(Data::Forms::NAFDoNotUseKW)) {
					return true;
				}
			}
			//NAFBridge protectedKeywords

			auto Obj = targetItem.object->As<RE::TESObjectARMO>();
			auto kwForm = targetItem.instanceData ? targetItem.instanceData->GetKeywordData() : nullptr;

			if (Data::Forms::protectedKeywords) {
				for (auto& item : Data::Forms::protectedKeywords->arrayOfForms) {
					if (auto kwd = item->As<RE::BGSKeyword>(); kwd) {
						if (Obj && Obj->HasKeyword(kwd)) {
							return true;
						}
						if (kwForm && kwForm->HasKeyword(kwd)) {
								return true;
						}
					}
				}
			}
			//NAFBridge end
			state->equipment[a->GetActorHandle()].storedEquipment.insert({ slot, targetItem.object });
			DoUnequip(a, targetItem.object, targetItem.instanceData.get());
			
			return true;
		}

		static bool ProcessReEquip(RE::NiPointer<RE::Actor> a, std::optional<uint8_t> slot)
		{
			auto iter = state->equipment.find(a->GetActorHandle());
			if (iter == state->equipment.end() || iter->second.storedEquipment.size() < 1)
				return true;

			if (!slot.has_value()) {
				for (auto& f : iter->second.storedEquipment) {
					DoEquip(a, f.second);
				}

				iter->second.storedEquipment.clear();
			} else {
				if (!iter->second.storedEquipment.contains(slot.value()))
					return true;

				RE::TESForm* equipForm = iter->second.storedEquipment[slot.value()];
				DoEquip(a, equipForm);

				iter->second.storedEquipment.erase(slot.value());
			}

			if (iter->second.storedEquipment.size() < 1 && iter->second.addedEquipment.size() < 1)
				state->equipment.erase(iter);

			return true;
		}

		static bool ProcessAddEquipment(RE::NiPointer<RE::Actor> a, RE::TESBoundObject* equipForm)
		{
			if (a->GetInventoryObjectCount(equipForm) < 1) {
				a->AddInventoryItem(equipForm, nullptr, 1, nullptr, nullptr, nullptr);
				state->equipment[a->GetActorHandle()].addedEquipment.push_back(equipForm);
			}

			if (a.get() == player) {
				RE::SendHUDMessage::ClearMessages();
			}

			DoEquip(a, equipForm);

			return true;
		}

		static bool ProcessRemoveEquipment(RE::NiPointer<RE::Actor> a, RE::TESBoundObject* equipForm)
		{
			DoUnequip(a, equipForm);

			auto iter = state->equipment.find(a->GetActorHandle());
			if (iter != state->equipment.end()) {
				for (auto n = iter->second.addedEquipment.begin(); n != iter->second.addedEquipment.end();) {
					if ((*n) != nullptr && (*n)->formID == equipForm->formID) {
						RE::TESObjectREFR::RemoveItemData removeData(equipForm, 1);
						a->RemoveItem(removeData);
						n = iter->second.addedEquipment.erase(n);
						break;
					} else {
						n++;
					}
				}

				if (iter->second.storedEquipment.size() < 1 && iter->second.addedEquipment.size() < 1)
					state->equipment.erase(iter);
			}

			if (a.get() == player) {
				RE::SendHUDMessage::ClearMessages();
			}

			return true;
		}

		static bool ProcessApplyMorphs(RE::NiPointer<RE::Actor> a, Data::Morphs m)
		{
			m.ProcessApply(a.get());
			return true;
		}

		static void DoEquip(RE::NiPointer<RE::Actor> a, RE::TESForm* obj, RE::TBO_InstanceData* instData = nullptr) {
			auto inst = RE::BGSObjectInstance(obj, instData);
			RE::ActorEquipManager::GetSingleton()->EquipObject(a.get(), inst, 0, 1, nullptr, true, false, true, true, false);
		}

		static void DoUnequip(RE::NiPointer<RE::Actor> a, RE::TESForm* obj, RE::TBO_InstanceData* instData = nullptr) {
			auto inst = RE::BGSObjectInstance(obj, instData);
			RE::ActorEquipManager::GetSingleton()->UnequipObject(a.get(), &inst, 0xFFFFFFFF, nullptr, 0, true, true, true, true, nullptr);
		}
	};
}
