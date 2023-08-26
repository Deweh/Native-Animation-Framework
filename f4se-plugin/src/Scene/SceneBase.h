#pragma once
#include "IScene.h"
#include "Functors.h"
#include "FaceAnimation/FaceUpdateHook.h"
#include "Misc/MathUtil.h"
#include "EventProxy.h"

#define SCNSYNC_DELAY_FUNCTOR(delName)	           \
namespace Scene{								   \
	class delName##Delegate : public SceneDelegate \
	{											   \
		using SceneDelegate::SceneDelegate;		   \
		virtual void Run() override				   \
		{										   \
			Process##delName##Functor(sceneId);	   \
		}										   \
	};											   \
}												   \
CEREAL_REGISTER_TYPE(Scene::DelegateFunctor<Scene::delName##Delegate>); \

namespace Scene
{
	class SystemTimerFunctor : public Tasks::TaskFunctor
	{
	public:
		uint64_t sceneId = 0;
		uint16_t timerId = 0;

		SystemTimerFunctor(){}

		SystemTimerFunctor(uint64_t _sceneId, uint16_t _timerId) :
			sceneId(_sceneId), timerId(_timerId) {}

		virtual void Run() override {
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
		// NIS: Disable rescaler setting
		if (!Data::Settings::Values.bDisableRescaler) {
			SceneManager::VisitScene(sceneId, [](IScene* scn) {
				auto player = RE::PlayerCharacter::GetSingleton();

				scn->ForEachActor([&](RE::Actor* currentActor, ActorPropertyMap& props) {
					float targetScale = 1.0f;
					if (auto customScale = GetProperty<float>(props, kScale); customScale.has_value()) {
						targetScale = customScale.value();
					} else if (auto npc = currentActor->GetNPC(); currentActor != player && npc != nullptr) {
						targetScale = player->GetScale() / npc->GetHeight(currentActor, currentActor->race);
					}

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
			auto player = RE::PlayerCharacter::GetSingleton();

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
				auto player = RE::PlayerCharacter::GetSingleton();
				bool hasPlayer = false;
				scn->ForEachActor([&](RE::Actor* currentActor, ActorPropertyMap&) {
					hasPlayer = hasPlayer || currentActor == player;
					currentActor->SetGraphVariable("bHumanoidFootIKDisable", false);
				}, false);
				scn->TransitionToAnimation(nullptr);
				scn->ApplyEquipmentSet(scn->stopEquipSet);
				if (hasPlayer) {
					CamHook::SetActive(false);
					GameUtil::SetFlyCam(false);
				}
			},
			true);
		SceneManager::DetachScene(sceneId);
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
		RE::BGSAnimationSystemUtils::ActiveSyncInfo cachedSyncInfo;
		float diffLimit = 100.0f * 0.05f;
		float basicallyFullSpeed = 100.0f * 0.9999f;
		RE::NiPoint3 baseLocation;
		RE::NiPoint3 baseAngle;

		virtual ~Scene()
		{
		}

		virtual void StartTimer(uint16_t id, double durationMs) override {
			tasks.StartNumbered<SystemTimerFunctor>(id, durationMs, uid, id);
		}

		virtual void StopTimer(uint16_t id) override {
			tasks.StopNumbered<SystemTimerFunctor>(id);
		}

		virtual bool HasPlayer() override {
			auto player = RE::PlayerCharacter::GetSingleton();
			bool result = false;
			ForEachActor([&](RE::Actor* a, ActorPropertyMap&) {
				if (a == player)
					result = true;
			});
			return result;
		}

		virtual uint64_t QUID() override {
			return uid;
		}

		virtual bool QAutoAdvance() override {
			return settings.autoAdvance;		
		}

		virtual std::vector<std::string> QCachedHKXStrings() override {
			std::vector<std::string> result;
			auto order = GetActorHandlesInOrder(actors);
			result.resize(order.size());

			for (size_t i = 0; i < order.size(); i++) {
				const auto& hndl = order[i];
				if (auto iter = cachedIdlesMap.find(hndl); iter != cachedIdlesMap.end()) {
					if (iter->second.dynIdle.has_value()) {
						result[i] = Utility::StringToLower(iter->second.dynIdle.value());
					} else if (auto idl = iter->second.regularIdle.get(); idl != nullptr) {
						result[i] = Utility::StringToLower(idl->animFileName);
					}
				}
			}

			return result;
		}

		virtual bool PushQueuedControlSystem() override {
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

		void QueueControlSystem(std::unique_ptr<IControlSystem> sys) {
			queuedSystem = std::move(sys);
			std::string nextId = queuedSystem->QAnimationID();
			controlSystem->OnEnd(this, nextId);
		}

		virtual bool Init(std::shared_ptr<const Data::Position> position) override
		{
			IScene::Init(position);
			startEquipSet = position->startEquipSet;
			stopEquipSet = position->stopEquipSet;
			controlSystem = GetControlSystem(position);
			baseLocation = location;
			baseAngle = angle;
			SetDuration(settings.duration);
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
			auto player = RE::PlayerCharacter::GetSingleton();

			ForEachActor([&](RE::Actor* currentActor, ActorPropertyMap& props) {
				currentActor->StopInteractingQuick();

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
					player->StopMoving(5.0f);
				}

				currentActor->SetPosition(location, true);
				currentActor->SetAngleOnReference(angle);
				currentActor->ClearLookAtTarget();
				currentActor->TurnOffHeadtracking();
			});

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

			auto player = RE::PlayerCharacter::GetSingleton();

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

				currentActor->SetScale(1.0f);
			}, false);

			status = SceneState::PendingDeletion;
			Data::Events::Send(Data::Events::SCENE_END, uid);
			tasks.Start<DelegateFunctor<PostStopDelegate>>(50, uid);
			return true;
		}

		virtual void OnActorDeath(RE::Actor*) override {
			ProcessCanNotContinue();
		}

		virtual void OnActorHit(RE::Actor*, const RE::TESHitEvent&) override {
			if (!settings.ignoreCombat) {
				ProcessCanNotContinue();
			}
		}

		virtual void OnActorLocationChange(RE::Actor*, RE::BGSLocation* newLocation) override {
			auto refr = settings.locationRefr.get();
			if (refr != nullptr && refr->parentCell != nullptr) {
				auto loc = refr->parentCell->GetLocation();
				if (loc == newLocation) {
					return;
				}
			}
			ProcessCanNotContinue();
		}

		void ProcessCanNotContinue() {
			noUpdate = true;
			F4SE::GetTaskInterface()->AddTask([uid = uid]() {
				SceneManager::StopScene(uid);
			});
		}

		virtual double QDuration() override {
			return settings.duration;
		}

		virtual void SetDuration(float dur) override {
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

		virtual void StopSmoothSync() override
		{
			SetAnimMult(animMult);
			SetSyncState(Synced);
		}

		virtual void ApplyMorphSet(const std::string& set) override {
			if (auto morphSet = Data::GetMorphSet(set); morphSet != nullptr) {
				ForEachActor([&](RE::Actor* currentActor, ActorPropertyMap&) {
					morphSet->Apply(currentActor);
				}, false);
			}
		}

		virtual void ApplyEquipmentSet(const std::string& set) override {
			OrderedActionQueue::InsertDelay(10);
			if (auto equipSet = Data::GetEquipmentSet(set); equipSet != nullptr) {
				ForEachActor([&](RE::Actor* currentActor, ActorPropertyMap&) {
					equipSet->Apply(currentActor);
				}, false);
			}
		}

		void ClearAnimObjects() {
			const auto t_intfc = F4SE::GetTaskInterface();
			for(auto& p : actors) {
				t_intfc->AddTask([hndl = p.first]() {
					if (auto a = hndl.get(); a != nullptr) {
						RE::BSAnimationGraphEvent evnt{ a.get(), "AnimObjUnequip", "" };
						RE::BGSAnimationSystemUtils::NotifyGraphSources(a.get(), evnt);
					}
				});
			}
		}

		virtual void TransitionToAnimation(std::shared_ptr<const Data::Animation> anim) {
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
					RE::BGSAction* resetGraph = RE::BGSAnimationSystemUtils::GetDefaultObjectForActionInitializeToBaseState();
					RE::TESActionData action(RE::ActionInput::ACTIONPRIORITY::kTry, currentActor, resetGraph);
					RE::BGSAnimationSystemUtils::RunActionOnActor(currentActor, action);
					if (stopEvent.has_value())
						currentActor->NotifyAnimationGraphImpl(stopEvent.value());
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

		virtual void PlayAnimations() override {
			cachedIdlesMap.clear();
			ForEachActor([&](RE::Actor* a, ActorPropertyMap& props) {
				auto hndl = a->GetActorHandle();
				FaceAnimation::FaceUpdateHook::StopAnimation(hndl);

				if (auto idl = GetProperty<SerializableIdle>(props, kIdle); idl.has_value()) {
					cachedIdlesMap[hndl] = { std::nullopt, idl.value() };
				} else if (auto dynIdl = GetProperty<std::string>(props, kDynIdle); dynIdl.has_value()) {
					cachedIdlesMap[hndl] = { dynIdl.value(), nullptr };
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

		virtual bool SetPosition(const std::string& id) override {
			auto targetPos = Data::GetPosition(id);

			if (targetPos == nullptr) {
				return false;
			}

			QueueControlSystem(GetControlSystem(targetPos));

			Data::Events::Send(Data::Events::SCENE_POS_CHANGE, Data::Events::ScenePositionData{ uid, id, true });
			return true;
		}

		virtual void SetOffset(const RE::NiPoint3& a_location, RE::NiPoint3 a_angle, bool angleIsDegrees = false) {
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

		virtual void SetAnimMult(float mult) override {
			animMult = mult;
			diffLimit = animMult * 0.05f;
			basicallyFullSpeed = animMult * 0.9999f;
			ForEachActor([&](RE::Actor* currentActor, ActorPropertyMap&) {
				GameUtil::SetAnimMult(currentActor, mult);
			});
			Data::Events::Send(Data::Events::SCENE_SPEED_CHANGE, std::pair<uint64_t, float>{ uid, mult });
		}

		virtual void SetSyncState(SyncState s) override {
			if (syncStatus != s) {
				syncStatus = s;
				Data::Events::Send(Data::Events::SCENE_SYNC_STATUS_CHANGE, std::pair<uint64_t, SyncState>{ uid, s });
			}
		}

		void SetTrackAnimTime(bool track) {
			if (track != trackAnimTime) {
				trackAnimTime = track;
				animTime = 0.0f;
				lastAnimTime = 0.0f;
			}
		}

		void UpdateSmoothSync() {
			auto player = RE::PlayerCharacter::GetSingleton();
			bool allActorsReady = true;
			bool noActorsReady = true;
			int i = 0;

			//Keep track of the earliest anim time amongst all actors to know if any actors are behind.
			float minTime = 0.0f;
			//Keep track of the first total time to see if any are different.
			float baseTotalTime = 0.0f;

			ForEachActor([&](RE::Actor* currentActor, ActorPropertyMap&) {
				cachedSyncInfo.otherSyncInfo.clear();
				//We don't want to make any changes until all actors are finished transitioning
				//to the animation and have their active sync info available.
				if (RE::BGSAnimationSystemUtils::GetActiveSyncInfo(currentActor, cachedSyncInfo) &&
					cachedSyncInfo.currentAnimTime >= 0.0f) {
					auto& ele = syncInfoVec[i];
					ele.actor.reset(currentActor);
					ele.currentAnimTime = std::clamp(cachedSyncInfo.currentAnimTime + (playerSyncOffset * (currentActor == player)), 0.0f, cachedSyncInfo.totalAnimTime);
					ele.totalAnimTime = cachedSyncInfo.totalAnimTime;

					//If this is the first actor, set minTime to its anim time to kick off the process.
					//Use arithmetic here instead of a conditional to avoid an unneccesary branch.
					minTime += ele.currentAnimTime * noActorsReady;
					baseTotalTime += ele.totalAnimTime * noActorsReady;
					noActorsReady = false;
					minTime = min(minTime, ele.currentAnimTime);
					i++;
				} else {
					allActorsReady = false;
				}
			});

			if (minTime > 0 && allActorsReady) {
				bool allFullSpeed = true;
				for (auto& info : syncInfoVec) {
					//Avoid synchronizing animations that have different total times.
					if (info.totalAnimTime != baseTotalTime) {
						allFullSpeed = true;
						break;
					}

					float oldMult = GameUtil::GetAnimMult(info.actor.get());
					//Adjust each actor's animation speed to nudge their current time towards the minTime.
					//Over time this results in an ease-out function, where anim speed is changed by a large amount
					//at first then gradually tapers off until all current times align with the min time.
					//To keep the function from suddenly going out of sync when one actor's current time loops
					//back to 0 before others, the anim speed change is clamped to +/- 5% of the scene's base anim speed.
					float mult = std::clamp(animMult * (minTime / info.currentAnimTime), oldMult - diffLimit, oldMult + diffLimit);
					GameUtil::SetAnimMult(info.actor.get(), mult);

					//basicallyFullSpeed = scene's base anim speed * 0.9999
					if (mult < basicallyFullSpeed) {
						allFullSpeed = false;
					}
				}

				//Once all anim speeds are within floating point error of the scene's base anim speed,
				//the anim times are as synchronized as they're going to get and smooth sync updates can
				//be stopped for this scene.
				if (allFullSpeed) {
					SetAnimMult(animMult);
					SetSyncState(Synced);
				}
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
							if (idl.second.dynIdle.has_value()) {
								std::optional<std::string> startEvent;
								std::optional<std::string> graph;
								if (auto r = Data::GetRace(a); r) {
									startEvent = r->startEvent;
									graph = r->graph;
								}
								DynamicIdle::Play(a, idl.second.dynIdle.value(), startEvent, graph);
							} else if (a != nullptr && a->currentProcess != nullptr) {
								a->currentProcess->PlayIdle(a, static_cast<uint32_t>(RE::DEFAULT_OBJECT::kActionIdle), idl.second.regularIdle, false);
							}
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
						if (actorIdle.second.dynIdle.has_value()) {
							idlePath = actorIdle.second.dynIdle.value();
						} else {
							idlePath = actorIdle.second.regularIdle->animFileName;
						}
						if (a != nullptr && (RE::BGSAnimationSystemUtils::IsIdleLoading(a, idlePath) || RE::BGSAnimationSystemUtils::IsActiveGraphInTransition(a))) {
							oneLoading = true;
							break;
						}
					}

					if (!oneLoading) {
						syncInfoVec.clear();
						syncInfoVec.resize(actors.size());
						SetSyncState(SyncingTimes);
						SetTrackAnimTime(true);
					}
					break;
				}
			case SyncingTimes:
				{
					UpdateSmoothSync();
					break;
				}
			}

			if (trackAnimTime) {
				cachedSyncInfo.otherSyncInfo.clear();
				auto trackingActor = actors.begin()->first.get();
				if (trackingActor != nullptr &&
					RE::BGSAnimationSystemUtils::GetActiveSyncInfo(trackingActor.get(), cachedSyncInfo) &&
					cachedSyncInfo.currentAnimTime >= 0.0f) {
					animTime = cachedSyncInfo.currentAnimTime;
					if (animTime < lastAnimTime) {
						controlSystem->OnAnimationLoop(this);
						Data::Events::Send(Data::Events::SCENE_ANIM_LOOP, uid);
					}
					lastAnimTime = animTime;
				}
			}

			auto player = RE::PlayerCharacter::GetSingleton();
			ForEachActor([&](RE::Actor* currentActor, ActorPropertyMap& props) {
				if (currentActor == player) {
					player->UpdatePlayer3D();
				}
				RE::NiPoint3 actorLoc = location;
				RE::NiPoint3 actorAngle = angle;
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
	};

	StartResult SceneManager::StartScene(const SceneSettings& settings, uint64_t& sceneIdInOut, bool overrideId, bool ignoreInScene)
	{
		auto locationRef = settings.locationRefr.get();
		auto actors = settings.QActors();
		auto position = settings.startPosition;

		if (auto res = ValidateStartSceneArgs(settings, ignoreInScene); !res)
			return res;

		std::shared_ptr<IScene> newScene = std::make_shared<Scene>();
		newScene->location = locationRef->data.location;
		newScene->angle = locationRef->data.angle;
		newScene->settings = settings;
		newScene->settings.ClearPreStartInfo();

		for (size_t i = 0; i < actors.size(); i++) {
			newScene->actors.insert({ actors[i]->GetActorHandle(), { { kOrder, ActorProperty{ i } } } });
		}

		newScene->Init(position);
		if (overrideId) {
			newScene->uid = sceneIdInOut;
		} else {
			sceneIdInOut = newScene->uid;
		}

		for (auto& a : actors) {
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
