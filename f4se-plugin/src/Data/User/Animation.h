#pragma once
#include "Misc/MathUtil.h"

namespace Data
{
	class Animation : public IdentifiableObject, public TagHolder
	{
	public:
		struct Slot
		{
			template <typename T>
			struct integrated_optional
			{
				void operator=(const T& v) {
					val = v;
					hasVal = true;
				}

				void operator=(const std::nullopt_t&) {
					hasVal = false;
				}

				bool has_value() const {
					return hasVal;
				}

				T& value() {
					return val;
				}

				const T& value() const {
					return val;
				}

				void set_has_value(bool v) {
					hasVal = v;
				}

				template <class Archive>
				void serialize(Archive& ar, const uint32_t)
				{
					ar(hasVal, val);
				}
			private:
				bool hasVal;
				T val;
			};
			
			ActorGender gender;
			bool behaviorRequiresConvert = false;
			bool idleRequiresConvert = false;
			bool dynamicIdle = false;
			bool loopFaceAnim = false;
			std::string rootBehavior;
			std::string idle;
			integrated_optional<std::string> faceAnim;
			integrated_optional<Morphs> morphs;
			integrated_optional<std::string> startEquipSet;
			integrated_optional<std::string> stopEquipSet;
			integrated_optional<ActionSet> actions;
			integrated_optional<std::pair<RE::NiPoint3, float>> offset;
			integrated_optional<float> customScale;

			static Slot FromActorAndHKX(RE::Actor* a, const std::string& hkxPath) {
				Slot result;
				result.rootBehavior = a->race->behaviorGraphProjectName->c_str();
				result.gender = static_cast<ActorGender>(a->GetSex());
				result.idle = hkxPath;
				result.dynamicIdle = true;
				return result;
			}

			template <typename T>
			static void OptionalSet(Scene::ActorPropertyMap& m, Scene::PropType pTy, integrated_optional<T> val) {
				if (val.has_value()) {
					m[pTy].value = val.value();
				} else {
					m.erase(pTy);
				}
			}

			template <typename T>
			static void OptionalApply(RE::Actor* a, integrated_optional<T> val)
			{
				if (val.has_value()) {
					val.value().Apply(a);
				}
			}

			void Apply(RE::Actor* a, Scene::ActorPropertyMap& m) const {
				if (!a) {
					return;
				}

				if (dynamicIdle) {
					m[Scene::PropType::kDynIdle].value = idle;
					m.erase(Scene::PropType::kIdle);
				} else {
					m[Scene::PropType::kIdle].value = GetIdle();
					m.erase(Scene::PropType::kDynIdle);
				}

				m[Scene::PropType::kLoopFaceAnim].value = loopFaceAnim;
				OptionalSet(m, Scene::kFaceAnim, faceAnim);
				OptionalApply(a, morphs);
				OptionalSet(m, Scene::kStartEquipSet, startEquipSet);
				OptionalSet(m, Scene::kStopEquipSet, stopEquipSet);
				OptionalSet(m, Scene::kAction, actions);
				OptionalSet(m, Scene::kOffset, offset);
				OptionalSet(m, Scene::kScale, customScale);
			}

			RE::TESIdleForm* GetIdle() const {
				RE::TESIdleForm* result = RE::TESForm::GetFormByEditorID<RE::TESIdleForm>(idle);
				if (!result)
					result = Data::Forms::LooseIdleStop;
				return result;
			}

			template <class Archive>
			void serialize(Archive& ar, const uint32_t)
			{
				ar(rootBehavior, gender, idle, dynamicIdle, faceAnim, morphs, startEquipSet, stopEquipSet, actions);
			}
		};

		static ActorGender ActorSexToGender(RE::Actor::Sex s)
		{
			switch (s) {
			case RE::Actor::Sex::Male:
				return ActorGender::Male;
			case RE::Actor::Sex::Female:
				return ActorGender::Female;
			default:
				return ActorGender::Any;
			}
		}

		bool SetActorInfo(Scene::SceneActorsMap& mapIn) const
		{
			return SetActorInfo(mapIn, slots);
		}

		static bool SetActorInfo(Scene::SceneActorsMap& mapIn, const std::vector<Slot>& slots)
		{
			auto actors = Scene::GetActorsInOrder(mapIn);

			for (auto& a : actors) {
				if (a == nullptr) {
					return false;
				}
			}

			std::vector<size_t> anyIndices;

			for (size_t i = 0; i < slots.size(); i++) {
				auto& s = slots[i];
				//First try to fill all gendered slots to avoid incorrectly placing a gendered actor into a non-gendered slot.
				if (s.gender != ActorGender::Any) {
					for (auto it = actors.begin(); it != actors.end(); it++) {
						ActorGender g = ActorSexToGender((*it)->GetSex());

						//If this actor has the same root behavior & gender as the corresponding slot, fill in the slot info for this actor.
						if (s.rootBehavior == (*it)->race->behaviorGraphProjectName[0] && (g == s.gender)) {
							s.Apply(it->get(), mapIn[it->get()->GetActorHandle()]);
							//Remove actor from the temporary vector once its map info has been filled in.
							it = actors.erase(it);
							break;
						}
					}
				} else {
					//Keep track of which slots are non-gendered so that we don't have to loop through them all a second time.
					anyIndices.push_back(i);
				}
			}

			//After filling all gendered slots, fill any non-gendered slots using the same procedure.
			for (auto& i : anyIndices) {
				auto& s = slots[i];

				for (auto it = actors.begin(); it != actors.end(); it++) {
					if (s.rootBehavior == (*it)->race->behaviorGraphProjectName[0]) {
						s.Apply(it->get(), mapIn[it->get()->GetActorHandle()]);
						it = actors.erase(it);
						break;
					}
				}
			}

			return true;
		}

		static std::vector<std::pair<RE::NiPoint3, float>> ParseOffsets(const std::string_view offsets) {
			std::vector<std::pair<RE::NiPoint3, float>> result;

			if (offsets.size() < 1)
				return result;

			Utility::ForEachSubstring(offsets, ":", [&](const std::string_view& off) {
				size_t i = 0;
				auto& ele = result.emplace_back();
				Utility::ForEachSubstring(off, ",", [&](const std::string_view& part) {
					float f = Utility::StringToFloat(part);
					switch (i) {
					case 0:
						ele.first.x = f;
						break;
					case 1:
						ele.first.y = f;
						break;
					case 2:
						ele.first.z = f;
						break;
					case 3:
						ele.second = MathUtil::DegreeToRadian(f);
						break;
					}
					i++;
				});
			},
			[&](size_t size) {
				result.reserve(size);
			});

			return result;
		}

		static bool Parse(XMLUtil::Mapper& m, Animation& out)
		{
			out.ParseID(m);
			out.ParseTags(m);
			std::string offset;
			m(&offset, ""s, true, false, "", "offset");
			std::vector<std::pair<RE::NiPoint3, float>> offsetPairs = ParseOffsets(offset);
			
			size_t i = 0;
			m.GetArray([&](XMLUtil::Mapper& m) {
				auto& s = out.slots[i];
				s.behaviorRequiresConvert = true;
				s.idleRequiresConvert = true;

				if (i < offsetPairs.size()) {
					s.offset = offsetPairs[i];
				}

				std::string tmp;
				m(&tmp, ""s, false, false, "", "file");
				if (tmp.size() > 0) {
					s.idleRequiresConvert = false;
					s.dynamicIdle = true;
					s.idle = tmp;
				} else {
					m.GetOptNode(&tmp, ""s, "idle", true, true, "Animation actor node has no idle source!", "idleSource", "source");
					s.idle.append(tmp);
					s.idle.append(IDLE_DELIMITER);
					m.GetOptNode(&tmp, ""s, "idle", true, true, "Animation actor node has no idle form!", "idleForm", "idle", "form");
					s.idle.append(tmp);
				}
				
				m(&s.rootBehavior, "Human"s, true, false, "", "skeleton");
				m(&s.gender, Any, true, false, "", "gender");
				s.faceAnim.set_has_value(m(&s.faceAnim.value(), ""s, true, false, "", "faceAnim"));
				m(&s.loopFaceAnim, false, true, false, "", "loopFaceAnim");
				s.startEquipSet.set_has_value(m(&s.startEquipSet.value(), ""s, true, false, "", "startEquipmentSet"));
				s.stopEquipSet.set_has_value(m(&s.stopEquipSet.value(), ""s, true, false, "", "stopEquipmentSet"));
				s.customScale.set_has_value(m(&s.customScale.value(), 1.0f, true, false, "", "scale"));
				
				s.actions.set_has_value(s.actions.value().Parse(m));

				bool hasMorphs = false;
				auto& actorMorphs = s.morphs.value();
				m.GetArray([&](XMLUtil::Mapper& m) {
					auto& p = actorMorphs.emplace_back();
					m(&p.name, ""s, true, true, "morph node has no 'id' attribute!", "id");
					m(&p.value, 0.0f, false, true, "morph node has no 'to' attribute!", "to");

					if (m)
						hasMorphs = true;

					return m;
				}, "morph", "", false);

				s.morphs.set_has_value(hasMorphs);

				i++;
				return m;
			},
			"actor", "Animation node has no actors!", true,
			[&](size_t size) {
				out.slots.resize(size);
			});

			return m;
		}

		std::vector<Slot> slots;

		template <class Archive>
		void serialize(Archive& ar, const uint32_t)
		{
			ar(id, slots);
		}
	};

	class AnimationFilter
	{
	public:
		struct ActorInfo
		{
			uint32_t maleCount = 0;
			uint32_t femaleCount = 0;
			uint32_t noGenderCount = 0;

			bool SubtractGender(ActorGender g)
			{
				switch (g) {
				case ActorGender::Any:
					if (noGenderCount > 0) {
						noGenderCount -= 1;
					} else if (femaleCount > 0) {
						femaleCount -= 1;
					} else if (maleCount > 0) {
						maleCount -= 1;
					} else {
						return false;
					}
					break;
				case ActorGender::Male:
					if (maleCount > 0) {
						maleCount -= 1;
					} else {
						return false;
					}
					break;
				case ActorGender::Female:
					if (femaleCount > 0) {
						femaleCount -= 1;
					} else {
						return false;
					}
					break;
				}

				return true;
			}
		};

		typedef std::unordered_map<std::string_view, ActorInfo> InfoMap;

		AnimationFilter()
		{
		}

		void operator=(std::vector<RE::Actor*> other)
		{
			for (auto& a : other) {
				AddActor(a);
			}
		}

		AnimationFilter(std::vector<RE::Actor*> actors)
		{
			for (auto& a : actors) {
				AddActor(a);
			}
		}

		AnimationFilter(std::vector<RE::NiPointer<RE::Actor>> actors)
		{
			for (auto& a : actors) {
				AddActor(a.get());
			}
		}

		uint32_t numTotalActors = 0;
		InfoMap iMap;

		bool AddActor(RE::Actor* a)
		{
			if (!a) {
				return false;
			}

			std::string_view graph = a->race->behaviorGraphProjectName[0];

			switch (a->GetSex()) {
			case RE::Actor::Sex::None:
				iMap[graph].noGenderCount += 1;
				break;
			case RE::Actor::Sex::Male:
				iMap[graph].maleCount += 1;
				break;
			case RE::Actor::Sex::Female:
				iMap[graph].femaleCount += 1;
				break;
			}

			numTotalActors += 1;

			return true;
		}

		void CopyToMap(InfoMap& m)
		{
			for (auto& i : iMap) {
				m[i.first].maleCount = i.second.maleCount;
				m[i.first].femaleCount = i.second.femaleCount;
				m[i.first].noGenderCount = i.second.noGenderCount;
			}
		}
	};
}
