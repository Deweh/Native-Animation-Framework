#pragma once
#include "EventProxy.h"
#include "FaceAnimation/FaceUpdateHook.h"
#include "Functors.h"
#include "IScene.h"
#include "Misc/MathUtil.h"

#include "Bridge/NewData/OffsetClass.h"


#define SCNSYNC_DELAY_FUNCTOR(delName)                 \
	namespace Scene                                    \
	{                                                  \
		class delName##Delegate : public SceneDelegate \
		{                                              \
			using SceneDelegate::SceneDelegate;        \
			virtual void Run() override                \
			{                                          \
				Process##delName##Functor(sceneId);    \
			}                                          \
		};                                             \
	}                                                  \
	CEREAL_REGISTER_TYPE(Scene::DelegateFunctor<Scene::delName##Delegate>);

namespace Scene
{
	class SystemTimerFunctor : public Tasks::TaskFunctor
	{
	public:
		uint64_t sceneId = 0;
		uint16_t timerId = 0;

		SystemTimerFunctor() {}

		SystemTimerFunctor(uint64_t _sceneId, uint16_t _timerId) :
			sceneId(_sceneId), timerId(_timerId) {}

		virtual void Run() override
		{
			F4SE::GetTaskInterface()->AddTask([sceneId = sceneId, timerId = timerId]() {
				SceneManager::VisitScene(sceneId, [timerId = timerId](IScene* scn) {
					scn->controlSystem->OnTimer(timerId);
				});
			});
		}

		template <class Archive>
		void serialize(Archive& ar, const uint32_t)
		{
			ar(cereal::base_class<Tasks::TaskFunctor>(this), sceneId, timerId);
		}
	};

	void ProcessScaleFunctor(uint64_t sceneId)
	{
		if (!Data::Settings::Values.bDisableRescaler) {
			SceneManager::VisitScene(sceneId, [](IScene* scn) {
				scn->ForEachActor([&](RE::Actor* currentActor, ActorPropertyMap& props) {
					float targetScale = 1.0f;
					if (auto customScale = GetProperty<float>(props, kScale); customScale.has_value()) {
						targetScale = customScale.value();
					} else if (auto npc = currentActor->GetNPC(); currentActor != player && npc != nullptr) {
						targetScale = player->GetScale();
					}

					targetScale = GameUtil::CalcActorScale(currentActor, targetScale);
					currentActor->SetScale(targetScale * 0.999f);
					currentActor->SetScale(targetScale);
				});
			});
		}
	}
}

SCNSYNC_DELAY_FUNCTOR(Scale);

namespace Scene
{
	void ProcessPostStartFunctor(uint64_t sceneId)
	{
		SceneManager::VisitScene(sceneId, [](IScene* scn) {
			scn->ForEachActor([&](RE::Actor* currentActor, ActorPropertyMap&) {
				RE::BGSAnimationSystemUtils::InitializeActorInstant(currentActor, false);

				if (currentActor == player) {
					CamHook::LookAt(player->Get3D());
					GameUtil::SetFlyCam(true);
					player->SetAIControlled(true);
					player->DisableInputForPlayer("NAF_Scene", GetInputsToDisableForScene());
				}

				currentActor->SetGraphVariable("bHumanoidFootIKDisable", true);
			});

			scn->ApplyEquipmentSet(scn->startEquipSet);
			scn->controlSystem->OnBegin(scn, "");
			scn->tasks.StartWithRepeats<DelegateFunctor<ScaleDelegate>>(50, 30, scn->uid);
		});
	}

	void ProcessStopFunctor(uint64_t sceneId)
	{
		SceneManager::StopScene(sceneId);
	}

	void ProcessPostStopFunctor(uint64_t sceneId)
	{
		SceneManager::VisitScene(
			sceneId, [](IScene* scn) {
				bool hasPlayer = false;
				scn->ForEachActor([&](RE::Actor* currentActor, ActorPropertyMap&) {
					hasPlayer = hasPlayer || currentActor == player;
					currentActor->SetGraphVariable("bHumanoidFootIKDisable", false);
				},
					false);
				scn->TransitionToAnimation(nullptr);
				scn->ApplyEquipmentSet(scn->stopEquipSet);
				if (hasPlayer) {
					CamHook::SetActive(false);
					GameUtil::SetFlyCam(false);
				}
				scn->detachQueued = true;
			},
			true);
	}
}

SCNSYNC_DELAY_FUNCTOR(PostStart);
SCNSYNC_DELAY_FUNCTOR(Stop);
SCNSYNC_DELAY_FUNCTOR(PostStop);

namespace Scene
{
	static std::unique_ptr<IControlSystem> GetControlSystem(std::shared_ptr<const Data::Position> position, bool excludeTrees = false);
}

#include "ControlSystems.h"

namespace Scene
{
	// For some reason, player animations are offset from all other NPC animations, even though the sync
	// information reports the same exact times. This offset is added to the sync information to correct
	// for it.
	// A value of 10ms makes the difference imperceptible in my tests, although it could possibly use
	// further tweaking, so I'm leaving it here for future configuration.
	inline static const float playerSyncOffset = 0.010f;

	static std::unique_ptr<IControlSystem> GetControlSystem(std::shared_ptr<const Data::Position> position, bool excludeTrees)
	{
		std::unique_ptr<IControlSystem> sys = nullptr;
		auto sysInfo = position->GetControlSystemInfo();
		switch (sysInfo->type) {
		case Data::ControlSystemType::kAnimationGroupSystem:
			sys = std::make_unique<AnimationGroupControlSystem>();
			break;
		case Data::ControlSystemType::kPositionTreeSystem:
			if (!excludeTrees)
				sys = std::make_unique<PositionTreeControlSystem>();
			break;
		case Data::ControlSystemType::kAnimationSystem:
		default:
			sys = std::make_unique<BaseControlSystem>();
		}
		if (sys)
			sys->SetInfo(sysInfo.get());
		return sys;
	}

	class Scene : public IScene
	{
	public:
		struct LocalSyncInfo
		{
			RE::NiPointer<RE::Actor> actor;
			float currentAnimTime = 0.0f;
			float totalAnimTime = 0.0f;
		};

		using IScene::IScene;

		std::vector<LocalSyncInfo> syncInfoVec;
		GameUtil::GraphTime cachedSyncInfo;
		float diffLimit = 100.0f * 0.05f;
		float basicallyFullSpeed = 100.0f * 0.9999f;
		RE::NiPoint3 baseLocation;
		RE::NiPoint3 baseAngle;

		virtual ~Scene()
		{
			logger::trace("Scene ID#{} deleted.", uid);
		}

		virtual void StartTimer(uint16_t id, double durationMs) override
		{
			tasks.StartNumbered<SystemTimerFunctor>(id, durationMs, uid, id);
		}

		virtual void StopTimer(uint16_t id) override
		{
			tasks.StopNumbered<SystemTimerFunctor>(id);
		}

		virtual bool HasPlayer() override
		{
			bool result = false;
			ForEachActor([&](RE::Actor* a, ActorPropertyMap&) {
				if (a == player)
					result = true;
			});
			return result;
		}

		virtual uint64_t QUID() override
		{
			return uid;
		}

		virtual bool QAutoAdvance() override
		{
			return settings.autoAdvance;
		}

		virtual std::vector<std::string> QCachedHKXStrings() override
		{
			std::vector<std::string> result;
			auto order = GetActorHandlesInOrder(actors);
			result.resize(order.size());

			for (size_t i = 0; i < order.size(); i++) {
				const auto& hndl = order[i];
				if (auto iter = cachedIdlesMap.find(hndl); iter != cachedIdlesMap.end()) {
					result[i] = Utility::StringToLower(iter->second.idle.GetFilePath());
				}
			}

			return result;
		}

		virtual bool PushQueuedControlSystem() override
		{
			if (queuedSystem != nullptr) {
				auto lastId = controlSystem->QAnimationID();
				controlSystem.reset(queuedSystem.release());
				queuedSystem = nullptr;
				controlSystem->OnBegin(this, lastId);
				return true;
			} else {
				return false;
			}
		}

		void QueueControlSystem(std::unique_ptr<IControlSystem> sys)
		{
			queuedSystem = std::move(sys);
			std::string nextId = queuedSystem->QAnimationID();
			controlSystem->OnEnd(this, nextId);
		}

		////NAF Bridge offset
		//virtual void UpdatePositionOffsetInScene(const Data::Position* position)
		//{
		//	offset.clear();
		//	if (position) {
		//		switch (position->posType) {
		//		case Data::Position::Type::kAnimation:
		//			{
		//				if (position->offset.size() > 0 && position->offset.at(0).has_value()) {
		//					auto rit = position->offset.rbegin();
		//					while (rit != position->offset.rend()) {
		//						offset.push_back(*rit++);
		//					}
		//				}
		//			}
		//		case Data::Position::Type::kAnimationGroup:
		//			{
		//				if (position->offset.size() > 0 && position->offset.at(0).has_value()) {
		//					auto rit = position->offset.rbegin();
		//					while (rit != position->offset.rend()) {
		//						offset.push_back(*rit++);
		//					}
		//				}
		//			}
		//		case Data::Position::Type::kPositionTree:
		//			{
		//				offset.clear();
		//				auto treePos = Data::GetPositionTree(position->id).get();
		//				if (treePos && treePos->tree->offset.size() > 0 && treePos->tree->offset.at(0).has_value()) {
		//					auto rit = treePos->tree->offset.rbegin();
		//					while (rit != treePos->tree->offset.rend()) {
		//						offset.push_back(*rit++);
		//					}
		//				}
		//			}
		//		}
		//	}
		//}

		//virtual void UpdatePositionOffsetInScene(const Data::PositionTree* positionTree)
		//{
		//	offset.clear();
		//	if (positionTree && positionTree->tree->offset.size() > 0 && positionTree->tree->offset.at(0).has_value()) {
		//		offset = positionTree->tree->offset;
		//	}
		//}

		//virtual void UpdatePositionOffsetInScene(std::vector<offset_optional> new_offset)
		//{
		//	offset.clear();
		//	offset = new_offset;
		////NAF Bridge end

		virtual bool Init(std::shared_ptr<const Data::Position> position) override
		{
			startEquipSet = position->startEquipSet;
			stopEquipSet = position->stopEquipSet;
			controlSystem = GetControlSystem(position);
			baseLocation = location;
			baseAngle = angle;
			
			// AnimationGroupSystem and PositionTreeSystem manage their own timing
			// Only set duration for regular animations
			/*if (position->posType == Data::Position::kAnimation) {
				SetDuration(settings.duration);
			}*/

			bool shouldSetDuration = (position->posType == Data::Position::kAnimation);
			if (position->posType == Data::Position::kAnimationGroup) {
				auto group = Data::GetAnimationGroup(position->idForType);
				if (group && group->sequential == false) {
					shouldSetDuration = true;
				}
			}

			if (shouldSetDuration) {
				SetDuration(settings.duration);
			}

			return true;
		}

		virtual bool Begin() override
		{
			if (status != SceneState::Initializing) {
				return false;
			}

			// Only keep rotation around the Z axis.
			angle.x = 0;
			angle.y = 0;
			SetAnimMult(100);

			std::vector<RE::NiPointer<RE::Actor>> actorList;

			size_t actor_count = 0; //NAFBridge offset
			ForEachActor([&](RE::Actor* currentActor, ActorPropertyMap& props) {
				currentActor->StopInteractingQuick();
				//logger::info{ "pos : {}, {}, {}\t actor : {} Begin start",
				//	currentActor->data.location.x, currentActor->data.location.y, currentActor->data.location.z, currentActor->GetDisplayFullName() };
				if (currentActor != player) {
					if (currentActor->HasKeyword(Data::Forms::TeammateReadyWeaponKW)) {
						props[kHadReadyWeapon].value = true;
						currentActor->ModifyKeyword(Data::Forms::TeammateReadyWeaponKW, false);
					}

					if (currentActor->GetCanDoFavor()) {
						props[kWasCommandable].value = true;
						currentActor->SetCanDoFavor(false);
					}

					currentActor->ModifyKeyword(Data::Forms::BlockActivationKW, true);
					currentActor->StopCombat();
					currentActor->StopAlarmOnActor();
					currentActor->SetRestrained(true);

					if (currentActor->currentProcess != nullptr) {
						currentActor->currentProcess->ignoringCombat = true;
					}

					PackageOverride::Set(currentActor->GetActorHandle(), Data::Forms::NAFLockPackage, true);
				} else {
					if (RE::UI::GetSingleton()->GetMenuOpen("PipboyMenu"))
						RE::UIMessageQueue::GetSingleton()->AddMessage("PipboyMenu", RE::UI_MESSAGE_TYPE::kHide);
					RE::PlayerCamera::GetSingleton()->Force3rdPerson();
					if (!(RE::PlayerCamera::GetSingleton()->currentState.get()->STATE & RE::CameraState::kFree)) {
						RE::PlayerCamera::GetSingleton()->ToggleFreeCameraMode(false);
					}
					player->StopMoving(5.0f);
				}

				//NAFBridge offset
				RE::NiPoint3 actorLoc = location;
				if (currentPosition) {
					auto& currentPositionOffset = currentPosition->offset;
					if (currentPositionOffset.size() > 0 && currentPositionOffset[0].has_value()) {
						size_t c = ((currentPositionOffset.size() > actor_count) && (currentPositionOffset[actor_count].has_value())) ? actor_count : 0;
						auto os = currentPositionOffset[c];
						MathUtil::ApplyOffsetToLocalSpace(actorLoc, os.value(), os.valueA());
					}
				}

				actorList.push_back(RE::NiPointer<RE::Actor>(currentActor));
				++actor_count;
				//NAFBridge end

				currentActor->SetPosition(actorLoc, true);
				currentActor->SetAngleOnReference(angle);
				currentActor->ClearLookAtTarget();
				currentActor->TurnOffHeadtracking();
					
				//currentActor->SetPosition(location, true); NAFBridge offset off
				//logger::info{ "pos : {}, {}, {}\t actor : {} Begin end",
				//	currentActor->data.location.x, currentActor->data.location.y, currentActor->data.location.z, currentActor->GetDisplayFullName() };
			});

			if (actorList.size() > 0) {
				fannyAnim.StartTracking(actorList);
			}

			status = SceneState::Active;
			Data::Events::Send(Data::Events::SCENE_START, uid);
			tasks.Start<DelegateFunctor<PostStartDelegate>>(50, uid);
			return true;
		}

		virtual void SoftEnd() override
		{
			if (status == SceneState::PendingDeletion || status == SceneState::Ending)
				return;

			status = SceneState::Ending;
			QueueControlSystem(std::make_unique<EndingControlSystem>());
		}

		virtual bool End() override
		{
			if (status == SceneState::PendingDeletion) {
				return false;
			}

			tasks.StopAll();
			SetAnimMult(100);

			ForEachActor([&](RE::Actor* currentActor, ActorPropertyMap& props) {
				currentActor->EnableCollision();
				currentActor->SetNoCollision(false);
				FaceAnimation::FaceUpdateHook::StopAnimation(currentActor->GetActorHandle());

				if (currentActor == player) {
					player->SetAIControlled(false);
					player->ReenableInputForPlayer();
				} else {
					if (auto hadReadyWeap = GetProperty<bool>(props, kHadReadyWeapon); hadReadyWeap.has_value()) {
						currentActor->ModifyKeyword(Data::Forms::TeammateReadyWeaponKW, true);
					}

					if (auto wasCmdable = GetProperty<bool>(props, kWasCommandable); wasCmdable.has_value()) {
						currentActor->SetCanDoFavor(true);
					}

					if (currentActor->currentProcess != nullptr) {
						currentActor->currentProcess->ignoringCombat = false;
					}

					currentActor->SetRestrained(false);
					currentActor->ModifyKeyword(Data::Forms::BlockActivationKW, false);
					PackageOverride::Clear(currentActor->GetActorHandle(), true);
					currentActor->EvaluatePackage(false, true);
				}

				// NAF Bridge fix scale after scene if scale was overridden
				// Look up initial scale by actor's FormID
				uint32_t actorFormID = currentActor->formID;
				auto scaleIt = settings.initialScales.find(actorFormID);
				
				if (scaleIt == settings.initialScales.end()) {
					// No saved scale for this actor - use default behavior
					if (!Data::Settings::Values.bDisableRescaler) {
						currentActor->SetScale(1.0f);
					}
				} else {
					float targetScale = scaleIt->second;
					constexpr float EPSILON = 0.001f;
					
					// First attempt to set the scale
					currentActor->SetScale(targetScale);
					float actualScale = currentActor->GetScale();
					
					// GetScale() returns refScale * baseScale, but SetScale() sets refScale only.
					// If actual scale doesn't match target, we need to compensate for baseScale.
					// actualScale = setScale * baseScale
					// We want: targetScale = newSetScale * baseScale
					// So: newSetScale = targetScale / baseScale = targetScale / (actualScale / setScale)
					// Since we set setScale = targetScale: newSetScale = targetScale / (actualScale / targetScale) = targetScale^2 / actualScale
					if (std::fabs(actualScale - targetScale) > EPSILON && std::fabs(actualScale) > EPSILON) {
						float baseScale = actualScale / targetScale;  // baseScale = actualScale / refScale (where refScale was set to targetScale)
						float compensatedScale = targetScale / baseScale;
						currentActor->SetScale(compensatedScale);
					}
				}
				// NAF Bridge fix scale end
			},
				false);

			status = SceneState::PendingDeletion;
			Data::Events::Send(Data::Events::SCENE_END, Data::Events::SceneData{ uid, GetActorsInOrder(actors) });
			tasks.Start<DelegateFunctor<PostStopDelegate>>(50, uid);
			return true;
		}

		virtual void OnActorDeath(RE::Actor*) override
		{
			ProcessCanNotContinue();
		}
	
		virtual void OnActorHit(RE::Actor*, const RE::TESHitEvent& hit) override
		{
			//Bridge end scene on radiation hit fix object->As<RE::TESObjectARMO>() if (auto bObj = targetItem.object->As<RE::TESObjectARMO>(); bObj)
			// Проверка sourceFormID на валидность
			
			if (hit.sourceFormID != 0) {
				if (auto sourceForm = RE::TESForm::GetFormByID(hit.sourceFormID); sourceForm) {
					if (auto spell = sourceForm->As<RE::SpellItem>(); spell) {
						if (spell->HasKeyword(static_cast<RE::BGSKeyword*>(RE::TESForm::GetFormByID(0x4B25C))))
							return;
						for (auto& mgef : spell->listOfEffects) {
							if (mgef->effectSetting->HasKeyword(static_cast<RE::BGSKeyword*>(RE::TESForm::GetFormByID(0x4B25C))))  //DamageTypeRadiation [KYWD:0004B25C]
								return;
						}
					}
				}
			}
			
			if (!settings.ignoreCombat) {
				ProcessCanNotContinue();
			}
		}

		virtual void OnActorLocationChange(RE::Actor*, RE::BGSLocation* newLocation) override
		{
			auto refr = settings.locationRefr.get();
			if (refr != nullptr && refr->parentCell != nullptr) {
				auto loc = refr->parentCell->GetLocation();
				if (loc == newLocation) {
					return;
				}
			}
			ProcessCanNotContinue();
		}

		void ProcessCanNotContinue()
		{
			noUpdate = true;
			F4SE::GetTaskInterface()->AddTask([uid = uid]() {
				SceneManager::StopScene(uid);
			});
		}

		virtual double QDuration() override
		{
			return settings.duration;
		}

		virtual void SetDuration(float dur) override
		{
			if (settings.duration != dur)
				settings.duration = dur;

			if (dur < 0) {
				tasks.Stop<DelegateFunctor<StopDelegate>>();
			} else {
				if (dur < 0.5) {
					settings.duration = static_cast<float>(Data::Settings::Values.iDefaultSceneDuration);
				}

				tasks.Start<DelegateFunctor<StopDelegate>>(settings.duration * 1000, uid);
			}
		}

		virtual float GetRemainingDuration() override
		{
			return static_cast<float>(tasks.GetRemainingTime<DelegateFunctor<StopDelegate>>());
		}

		virtual void StopSmoothSync() override
		{
			SetAnimMult(animMult);
			SetSyncState(Synced);
		}

		virtual void ApplyMorphSet(const std::string& set) override
		{
			if (auto morphSet = Data::GetMorphSet(set); morphSet != nullptr) {
				ForEachActor([&](RE::Actor* currentActor, ActorPropertyMap&) {
					morphSet->Apply(currentActor);
				},
					false);
			}
		}

		virtual void ApplyEquipmentSet(const std::string& set) override
		{
			OrderedActionQueue::InsertDelay(10);
			if (auto equipSet = Data::GetEquipmentSet(set); equipSet != nullptr) {
				ForEachActor([&](RE::Actor* currentActor, ActorPropertyMap&) {
					equipSet->Apply(currentActor);
				},
					false);
			}
		}

		void ClearAnimObjects()
		{
			const auto t_intfc = F4SE::GetTaskInterface();
			for (auto& p : actors) {
				t_intfc->AddTask([hndl = p.first]() {
					if (auto a = hndl.get(); a != nullptr) {
						RE::BSAnimationGraphEvent evnt{ a.get(), "AnimObjUnequip", "" };
						RE::BGSAnimationSystemUtils::NotifyGraphSources(a.get(), evnt);
					}
				});
			}
		}

		virtual void TransitionToAnimation(std::shared_ptr<const Data::Animation> anim)
		{
			StopSmoothSync();
			SetTrackAnimTime(false);
			ForEachActor([&](RE::Actor* currentActor, ActorPropertyMap& props) {
				if (auto actions = GetProperty<Data::ActionSet>(props, kAction); actions.has_value()) {
					actions->RunStop(currentActor);
				}
				if (auto stopEquipSet = GetProperty<std::string>(props, kStopEquipSet); stopEquipSet.has_value()) {
					Data::ApplyEquipmentSet(currentActor, stopEquipSet.value());
				}
				bool requiresReset = false;
				std::optional<std::string> stopEvent = std::nullopt;
				if (auto r = Data::GetRace(currentActor); r) {
					requiresReset = r->requiresReset;
					stopEvent = r->stopEvent;
				}
				if (requiresReset || anim == nullptr) {
					BodyAnimation::SmartIdle::Stop(currentActor, stopEvent);
				}
			});

			if (anim == nullptr)
				return;

			anim->SetActorInfo(actors);

			ForEachActor([&](RE::Actor* currentActor, ActorPropertyMap& props) {
				if (auto actions = GetProperty<Data::ActionSet>(props, kAction); actions.has_value()) {
					actions->RunStart(currentActor);
				}
			});

			ClearAnimObjects();
			PlayAnimations();
			Data::Events::Send(Data::Events::SCENE_ANIM_CHANGE, std::pair<uint64_t, std::string>{ uid, anim->id });
		}

		virtual void PlayAnimations() override
		{
			cachedIdlesMap.clear();
			ForEachActor([&](RE::Actor* a, ActorPropertyMap& props) {
				auto hndl = a->GetActorHandle();
				//FaceAnimation::FaceUpdateHook::StopAnimation(hndl);
				// NAF Bridge: Only stop face animation if NOT managed by Action's mfgSet
				bool hasMfgSetFromAction = false;
				if (auto actions = GetProperty<Data::ActionSet>(props, kAction); actions.has_value()) {
					for (auto& actionId : *actions) {
						if (auto action = Data::GetAction(actionId); action != nullptr && action->mfgSet.has_value()) {
							hasMfgSetFromAction = true;
							break;
						}
					}
				}

				// Only stop face animation if not managed by Action
				if (!hasMfgSetFromAction) {
					FaceAnimation::FaceUpdateHook::StopAnimation(hndl);
				}

				if (auto idl = GetProperty<SerializableIdle>(props, kIdle); idl.has_value()) {
					cachedIdlesMap[hndl].idle.SetIdleForm(idl.value());
				} else if (auto dynIdl = GetProperty<std::string>(props, kDynIdle); dynIdl.has_value()) {
					if (Utility::StringEndsWith(dynIdl.value(), ".nanim")) {
						auto idleId = GetProperty<std::string>(props, kDynIdleID);
						cachedIdlesMap[hndl].idle.SetNAFPath(dynIdl.value(), idleId.has_value() ? idleId.value() : "default");
					} else {
						cachedIdlesMap[hndl].idle.SetHKXPath(dynIdl.value());
					}
				}

				if (auto faceAnim = GetProperty<std::string>(props, kFaceAnim); faceAnim.has_value()) {
					auto doLoop = GetProperty<bool>(props, kLoopFaceAnim);
					FaceAnimation::FaceUpdateHook::LoadAndPlayAnimation(hndl, faceAnim.value(), (doLoop.has_value() ? doLoop.value() : false), true);
				}

				if (auto startEquipSet = GetProperty<std::string>(props, kStartEquipSet); startEquipSet.has_value()) {
					Data::ApplyEquipmentSet(a, startEquipSet.value());
				}
			});

			SetSyncState(SettingUp);
		}

		virtual bool SetPosition(const std::string& id) override
		{
			auto targetPos = Data::GetPosition(id);

			if (targetPos == nullptr) {
				return false;
			}

			//NAFBridge offset
			currentPosition = targetPos;

			RE::NiPoint3 actorLoc = location;
			size_t actor_count = 0;

			ForEachActor([&](RE::Actor* currentActor, ActorPropertyMap&) {
				//logger::info{ "pos : {}, {}, {}\t actor : {} SetPosition start",
				//	currentActor->data.location.x, currentActor->data.location.y, currentActor->data.location.z, currentActor->GetDisplayFullName() };
				if (currentPosition) {
					auto& currentPositionOffset = currentPosition->offset;
					if (currentPositionOffset.size() > 0 && currentPositionOffset[0].has_value()) {
						size_t c = ((currentPositionOffset.size() > actor_count) && (currentPositionOffset[actor_count].has_value())) ? actor_count : 0;
						auto os = currentPositionOffset[c];
						MathUtil::ApplyOffsetToLocalSpace(actorLoc, os.value(), os.valueA());
					}
				}

				if (!MathUtil::CoordsWithinError(currentActor->data.location, actorLoc)) {
					currentActor->SetPosition(actorLoc, true);
					currentActor->DisableCollision();
					currentActor->SetNoCollision(true);
				}
				//logger::info{ "pos : {}, {}, {}\t actor :  {} SetPosition end",
				//	currentActor->data.location.x, currentActor->data.location.y, currentActor->data.location.z, currentActor->GetDisplayFullName() };
				++actor_count;
			});
			//NAFBridge end

			QueueControlSystem(GetControlSystem(targetPos));

			Data::Events::Send(Data::Events::SCENE_POS_CHANGE, Data::Events::ScenePositionData{ uid, id, true });
			return true;
		}

		virtual void SetOffset(const RE::NiPoint3& a_location, RE::NiPoint3 a_angle, bool angleIsDegrees = false)
		{
			if (angleIsDegrees) {
				a_angle = MathUtil::DegreesToRadians(a_angle);
			}

			location.x = baseLocation.x + a_location.x;
			location.y = baseLocation.y + a_location.y;
			location.z = baseLocation.z + a_location.z;

			angle.x = baseAngle.x + a_angle.x;
			angle.y = baseAngle.y + a_angle.y;
			angle.z = baseAngle.z + a_angle.z;

			MathUtil::ConstrainRadians(angle);
		}

		virtual void SetAnimMult(float mult) override
		{
			animMult = mult;
			diffLimit = animMult * 0.05f;
			basicallyFullSpeed = animMult * 0.9999f;
			ForEachActor([&](RE::Actor* currentActor, ActorPropertyMap&) {
				GameUtil::SetAnimMult(currentActor, mult);
			});
			Data::Events::Send(Data::Events::SCENE_SPEED_CHANGE, std::pair<uint64_t, float>{ uid, mult });
		}

		virtual void SetSyncState(SyncState s) override
		{
			if (syncStatus != s) {
				syncStatus = s;
				Data::Events::Send(Data::Events::SCENE_SYNC_STATUS_CHANGE, std::pair<uint64_t, SyncState>{ uid, s });
			}
		}

		void SetTrackAnimTime(bool track)
		{
			if (track != trackAnimTime) {
				trackAnimTime = track;
				animTime = 0.0f;
				lastAnimTime = 0.0f;
			}
		}

		void PerformSync()
		{
			bool noActorsReady = true;
			bool allActorsReady = true;
			float minTime = std::numeric_limits<float>::infinity();
			size_t i = 0;
			size_t readyCount = 0;
			std::optional<bool> playerReady = std::nullopt;

			// NAF Bridge fix for player sync issue
			// In some kind of reason player can be not ready while other actors are ready.
			// Despite this, the player still participates correctly in the scene. Therefore, we use this workaround.
			// Fill syncInfoVec entries only for actors that returned valid times
			ForEachActor([&](RE::Actor* currentActor, ActorPropertyMap&) {
				if (BodyAnimation::SmartIdle::GetGraphTime(currentActor, cachedSyncInfo) &&
					cachedSyncInfo.current >= 0.0f) {
					if (currentActor == player) {
						if (cachedSyncInfo.current <= playerSyncOffset) {
							// Player reported time but it's below the sync offset threshold -> treat as not ready
							playerReady = false;
							allActorsReady = false;
							return;
						}
						playerReady = true;
					}

					// Accept this actor's timing
					auto& ele = syncInfoVec[i];
					ele.actor.reset(currentActor);
					ele.currentAnimTime = cachedSyncInfo.current + ((currentActor == player) * playerSyncOffset);
					ele.totalAnimTime = cachedSyncInfo.total;

					//If this is the first actor, set minTime to its anim time to kick off the process.
					//Use arithmetic here instead of a conditional to avoid an unneccesary branch.
					
					// minTime += ele.currentAnimTime * noActorsReady; //Bridge removed
					noActorsReady = false;
					minTime = std::min(minTime, ele.currentAnimTime);
					++i;
					++readyCount;
				} else {
					// Mark actor (and possibly player) as not ready
					allActorsReady = false;
					if (currentActor == player) {
						playerReady = false;
					}
				}
			});

			// If only the player is not ready, consider all actors ready
			if (playerReady.has_value() && playerReady.value() == false && actors.size() - readyCount == 1) {
				allActorsReady = true;
				//Force sync even if only player
				readyCount += 1;
			}

			if (minTime > 0 && allActorsReady && readyCount > 0) {
				for (size_t idx = 0; idx < i; ++idx) {
					auto& info = syncInfoVec[idx];
					if (minTime < info.totalAnimTime) {
						BodyAnimation::SmartIdle::SetGraphTime(info.actor.get(), minTime - ((info.actor.get() == player) * playerSyncOffset));
					}
				}

				SetSyncState(Synced);
				SetTrackAnimTime(true);
			}
		}

		virtual void Update() override
		{
			switch (syncStatus) {
			case Synced:
				break;
			case SettingUp:
				{
					bool allReady = true;
					for (auto& idl : cachedIdlesMap) {
						auto a = idl.first.get();
						if (a != nullptr && RE::BGSAnimationSystemUtils::IsActiveGraphInTransition(a.get())) {
							GameUtil::SetAnimMult(a.get(), GameUtil::GetAnimMult(a.get()) + 5.0f);
							allReady = false;
						}
					}

					if (allReady) {
						for (auto& idl : cachedIdlesMap) {
							auto a = idl.first.get().get();
							idl.second.idle.Play(a);
						}

						SetAnimMult(animMult);
						SetSyncState(WaitingForLoad);
					}
				}
			case WaitingForLoad:
				{
					bool oneLoading = false;
					std::string idlePath;
					for (auto& actorIdle : cachedIdlesMap) {
						auto a = actorIdle.first.get().get();
						if (a != nullptr && (actorIdle.second.idle.IsIdleLoading(a) || RE::BGSAnimationSystemUtils::IsActiveGraphInTransition(a))) {
							oneLoading = true;
							break;
						}
					}

					if (!oneLoading) {
						syncInfoVec.clear();
						syncInfoVec.resize(actors.size());
						SetSyncState(SyncingTimes);
					}
					break;
				}
			case SyncingTimes:
				{
					PerformSync();
					break;
				}
			}

			if (trackAnimTime) {
				// NAF Bridge fix for player sync issue
				// In some kind of reason player can be not ready while other actors are ready.
				// Despite this, the player still participates correctly in the scene. Therefore, we use this workaround.
				auto trackingActor = actors.begin()->first.get();
				if (trackingActor.get() == RE::PlayerCharacter::GetSingleton() && actors.size() > 1) {
					trackingActor = (++actors.begin())->first.get();
				}
				if (trackingActor != nullptr &&
					BodyAnimation::SmartIdle::GetGraphTime(trackingActor.get(), cachedSyncInfo) &&
					cachedSyncInfo.current >= 0.0f) {
					animTime = cachedSyncInfo.current;
					if (animTime < lastAnimTime) {
						controlSystem->OnAnimationLoop(this);
						Data::Events::Send(Data::Events::SCENE_ANIM_LOOP, uid);
					}
					lastAnimTime = animTime;
				}
			}

			fannyAnim.Update(0.32f);  //0.16f ~ 60 FPS

			size_t actor_count = 0; //NAFBridge offset
			ForEachActor([&](RE::Actor* currentActor, ActorPropertyMap& props) {
				if (currentActor == player) {
					player->UpdatePlayer3D();
				}
				RE::NiPoint3 actorLoc = location;
				RE::NiPoint3 actorAngle = angle;
				//NAFBridge offset
				//logger::info{ "pos : {}, {}, {}\t actor : {} Update start",
				//	currentActor->data.location.x, currentActor->data.location.y, currentActor->data.location.z, currentActor->GetDisplayFullName() };
				if (currentPosition) {
					auto& currentPositionOffset = currentPosition->offset;
					if (currentPositionOffset.size() > 0 && currentPositionOffset[0].has_value()) {
						size_t c = ((currentPositionOffset.size() > actor_count) && currentPositionOffset[actor_count].has_value()) ? actor_count : 0;
						auto os = currentPositionOffset[c];
						MathUtil::ApplyOffsetToLocalSpace(actorLoc, os.value(), os.valueA());
					}
				}
				++actor_count;
				//NAFBridge end
				if (auto offset = GetProperty<std::pair<RE::NiPoint3, float>>(props, kOffset); offset.has_value()) {
					MathUtil::ApplyOffsetToLocalSpace(actorLoc, offset->first, actorAngle.z);
					actorAngle.z += offset->second;
					MathUtil::ConstrainRadian(actorAngle.z);
				}
				if (!MathUtil::CoordsWithinError(currentActor->data.angle, actorAngle)) {
					currentActor->SetAngleOnReference(actorAngle);
				}

				if (!MathUtil::CoordsWithinError(currentActor->data.location, actorLoc)) {
					currentActor->SetPosition(actorLoc, true);
					currentActor->DisableCollision();
					currentActor->SetNoCollision(true);
				}
				//logger::info{ "pos : {}, {}, {}\t actor : {} Update end",
				//	currentActor->data.location.x, currentActor->data.location.y, currentActor->data.location.z, currentActor->GetDisplayFullName() };
			});
		}

		template <class Archive>
		void serialize(Archive& ar, const uint32_t)
		{
			ar(cereal::base_class<IScene>(this), baseLocation, baseAngle);
			diffLimit = animMult * 0.05f;
			basicallyFullSpeed = animMult * 0.9999f;
			if (syncInfoVec.size() < actors.size()) {
				syncInfoVec.resize(actors.size());
			}
		}

		virtual void Update3dPos() //NAF Bridge offset
		{
			//NAFBridge offset
			RE::NiPoint3 actorLoc = location;
			size_t actor_count = 0;

			ForEachActor([&](RE::Actor* currentActor, ActorPropertyMap&) {
				//logger::info{ "pos : {}, {}, {}\t actor : {} SetPosition start",
				//	currentActor->data.location.x, currentActor->data.location.y, currentActor->data.location.z, currentActor->GetDisplayFullName() };
				if (currentPosition) {
					auto& currentPositionOffset = currentPosition->offset;
					if (currentPositionOffset.size() > 0 && currentPositionOffset[0].has_value()) {
						size_t c = ((currentPositionOffset.size() > actor_count) && (currentPositionOffset[actor_count].has_value())) ? actor_count : 0;
						auto os = currentPositionOffset[c];
						MathUtil::ApplyOffsetToLocalSpace(actorLoc, os.value(), os.valueA());
					}
				}

				if (!MathUtil::CoordsWithinError(currentActor->data.location, actorLoc)) {
					currentActor->SetPosition(actorLoc, true);
					currentActor->DisableCollision();
					currentActor->SetNoCollision(true);
				}
				//logger::info{ "pos : {}, {}, {}\t actor :  {} SetPosition end",
				//	currentActor->data.location.x, currentActor->data.location.y, currentActor->data.location.z, currentActor->GetDisplayFullName() };
				++actor_count;
			});
			//NAFBridge end
		}
		//NAFBridge
	};

	extern RE::BSScript::IVirtualMachine* g_VM;

	StartResult SceneManager::StartScene(const SceneSettings& settings, uint64_t& sceneIdInOut, bool overrideId, bool ignoreInScene)
	{
		auto locationRef = settings.locationRefr.get();
		auto actors = settings.QActors();
		auto position = settings.startPosition;

		if (auto res = ValidateStartSceneArgs(settings, ignoreInScene); !res) {
			Data::Events::Send(Data::Events::SCENE_FAILED, overrideId ? sceneIdInOut : 0ui64);
			return res;
		}

		std::shared_ptr<IScene> newScene = std::make_shared<Scene>();
		newScene->location = locationRef->data.location;
		newScene->angle = locationRef->data.angle;
		newScene->settings = settings;
		newScene->settings.ClearPreStartInfo();
		//NAF Bridge offset
		newScene->currentPosition = position;
		if (newScene->currentPosition.get()->posType == newScene->currentPosition.get()->kPositionTree)
		{
			auto node = Data::GetPositionTree(newScene->currentPosition.get()->idForType);
			if (node) {
				newScene->currentPosition = Data::GetPosition(node->tree->position);
			}
		}
		//NAF Bridge offset
		for (size_t i = 0; i < actors.size(); i++) {
			newScene->actors.insert({ actors[i]->GetActorHandle(), { { kOrder, ActorProperty{ i } } } });
		}

		if (overrideId) {
			newScene->uid = sceneIdInOut;
		} else {
			newScene->uid = Data::Uid::Get();
			sceneIdInOut = newScene->uid;
		}
		newScene->Init(position);

		for (auto& a : actors) {
			if (!a)
				continue;

			if (a.get() == player) {
				if (static_cast<size_t>(RE::PlayerCamera::GetSingleton()->currentState->id.get()) != RE::CameraState::k3rdPerson) {
					RE::PlayerCamera::GetSingleton()->ForceVATSMode();
					RE::PlayerCamera::GetSingleton()->Force3rdPerson();
				}
				player->DisableCollision();
				/*if (!(RE::PlayerCamera::GetSingleton()->currentState.get()->STATE & RE::CameraState::kFree)) {
					RE::PlayerCamera::GetSingleton()->ToggleFreeCameraMode(false);
				}*/
			}
			//NAFBridge end
			//NAF Bridge scale fix : save initial scale by FormID
			newScene->settings.initialScales[a->formID] = a->GetScale();
			//NAF Bridge scale fix end
			if (a->parentCell != locationRef->parentCell)
				a->WarpToRef(locationRef.get());
		}

		if (locationRef->IsFurniture()) {
			locationRef->FreeUpAllMarkers();
		}

		AttachScene(newScene);
		std::unique_lock l{ newScene->lock };
		newScene->Begin();
		return { kNone };
	}
}

CEREAL_REGISTER_TYPE(Scene::Scene);
CEREAL_REGISTER_TYPE(Scene::SystemTimerFunctor);
