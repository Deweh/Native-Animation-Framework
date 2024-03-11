#include <shared_mutex>
#include "Misc/Utility.h"
#include "Data/Forms.h"
#include "Tasks/TimerThread.h"
#include "Serialization/General.h"
#include "IScene.h"
#include "Menu/NAFHUDMenu/HUDManager.h"
#include <unordered_set>
#pragma once
#undef DrawText

using namespace Tasks;

namespace Scene
{
	typedef std::shared_mutex DataLock;
	typedef std::unique_lock<DataLock> DataWriteLock;
	typedef std::shared_lock<DataLock> DataReadLock;

	using namespace Serialization::General;

	class SceneManager
	{
	public:
		struct WalkInfo
		{
			SceneSettings settings;
			std::unordered_map<SerializableActorHandle, bool> completionState;

			template <class Archive>
			void serialize(Archive& ar, const uint32_t)
			{
				ar(settings, completionState);
			}
		};

		struct PersistentState
		{
			std::unordered_map<uint64_t, WalkInfo> actorsWalkingToScene;
			std::unordered_map<uint64_t, std::shared_ptr<IScene>> scenes;

			std::atomic<uint32_t> skipWalkBox = 0;
			std::atomic<uint32_t> skipWalkText = 0;
			std::atomic<uint64_t> playerWalkInstance = 0;

			template <class Archive>
			void serialize(Archive& ar, const uint32_t)
			{
				ar(scenes, actorsWalkingToScene, skipWalkBox, skipWalkText, playerWalkInstance);
			}
		};

		static SceneManager* GetSingleton()
		{
			static SceneManager singleton;
			return &singleton;
		}

		inline static DataLock scenesMapLock;
		inline static std::mutex actorsWalkingLock;
		inline static std::unique_ptr<PersistentState> state = std::make_unique<PersistentState>();

		static bool GetScene(uint64_t uid, std::shared_ptr<IScene>& sceneOut) {
			DataReadLock l1{ scenesMapLock };
			if (state->scenes.contains(uid)) {
				sceneOut = state->scenes[uid];
				return true;
			} else {
				sceneOut = nullptr;
				return false;
			}
		}

		static void VisitScene(uint64_t uid, std::function<void(IScene*)> visitFunc, bool ignorePendingDeletion = false, std::function<void()> notAttachedFunc = nullptr)
		{
			std::shared_ptr<IScene> targetScene;
			if (GetScene(uid, targetScene)) {
				std::unique_lock l{ targetScene->lock };
				if (targetScene->status != SceneState::PendingDeletion || ignorePendingDeletion) {
					visitFunc(targetScene.get());
				}
			} else if(notAttachedFunc != nullptr) {
				notAttachedFunc();
			}
		}

		static void VisitAllScenes(std::function<void(IScene*)> visitFunc, bool ignorePendingDeletion = false)
		{
			const auto localScenes = GetSceneMapSnapshot();
			
			for (auto& s : localScenes) {

				std::unique_lock l{ s->lock };
				if (s->status != SceneState::PendingDeletion || ignorePendingDeletion) {
					visitFunc(s.get());
				}
			}
		}

		static void VisitWalkInstance(uint64_t instanceId, std::function<void(WalkInfo&)> visitFunc) {
			std::unique_lock l{ actorsWalkingLock };
			auto iter = state->actorsWalkingToScene.find(instanceId);
			if (iter != state->actorsWalkingToScene.end()) {
				visitFunc(iter->second);
			}
		}

		static void VisitAllWalkInstances(std::function<void(WalkInfo&, uint64_t)> visitFunc) {
			std::unique_lock l{ actorsWalkingLock };
			for (auto& iter : state->actorsWalkingToScene) {
				visitFunc(iter.second, iter.first);
			}
		}

		class WalkFinishedFunctor : public PackageOverride::PackageEndFunctor
		{
		public:
			uint64_t instanceId;

			WalkFinishedFunctor() {}

			WalkFinishedFunctor(uint64_t _id) :
				instanceId(_id) {}

			virtual void Run() override {
				std::unique_lock l{ actorsWalkingLock };
				auto iter = state->actorsWalkingToScene.find(instanceId);
				if (iter != state->actorsWalkingToScene.end() && iter->second.completionState.contains(data)) {
					iter->second.completionState[data] = true;
				} else {
					StopWalkPackage(data);
					return;
				}

				bool allReady = true;
				for (auto& info : iter->second.completionState) {
					if (!info.second)
						allReady = false;
				}

				if (allReady) {
					l.unlock();
					CompleteWalk(instanceId);
				}
			}

			template <class Archive>
			void serialize(Archive& ar, const uint32_t)
			{
				ar(cereal::base_class<PackageOverride::PackageEndFunctor>(this), instanceId);
			}
		};

		static StartResult WalkToAndStartScene(SceneSettings settings, uint64_t& sceneIdOut) {
			auto actors = settings.QActors();
			auto locationRef = settings.locationRefr.get();

			if (auto res = ValidateStartSceneArgs(settings); !res)
				return res;

			uint64_t uid = Data::Uid::Get();
			WalkInfo info{ settings };
			info.completionState.reserve(actors.size());

			std::unique_lock l{ actorsWalkingLock };
			bool allNoProcess = true;
			for (auto& a : actors) {
				auto hndl = a->GetActorHandle();

				if (StartWalkPackage(a.get(), locationRef.get(), uid)) {
					allNoProcess = false;
					info.completionState[hndl] = false;
				} else {
					info.completionState[hndl] = true;
				}
			}

			state->actorsWalkingToScene[uid] = info;

			if (allNoProcess) {
				l.unlock();
				CompleteWalk(uid);
			}

			sceneIdOut = uid;
			return { kNone };
		}

		static bool CompleteWalk(uint64_t instanceId) {
			std::unique_lock l{ actorsWalkingLock };
			auto iter = state->actorsWalkingToScene.find(instanceId);

			if (iter == state->actorsWalkingToScene.end()) {
				return false;
			}

			auto info = *iter;
			state->actorsWalkingToScene.erase(iter);
			l.unlock();

			for (auto& i : info.second.completionState) {
				StopWalkPackage(i.first);
			}

			uint64_t uid = info.first;

			StartScene(info.second.settings, uid, true, true);
			return true;
		}

		static StartResult StartScene(const SceneSettings& settings, uint64_t& sceneIdOut, bool overrideId = false, bool ignoreInScene = false);

		static bool IsActorInScene(RE::Actor* actor, bool ignoreInSceneKW = false) {
			if (!Data::Forms::NAFInSceneKW || !Data::Forms::NAFDoNotUseKW) {
				return false;
			}
			
			return (actor->HasKeyword(Data::Forms::NAFInSceneKW) && !ignoreInSceneKW) || actor->HasKeyword(Data::Forms::NAFDoNotUseKW);
		}

		static void SetActorInScene(RE::Actor* actor, bool inScene)
		{
			if (!Data::Forms::NAFInSceneKW) {
				return;
			}

			actor->ModifyKeyword(Data::Forms::NAFInSceneKW, inScene);
		}

		static void AttachScene(std::shared_ptr<IScene> scn) {
			std::unique_lock l{ scn->lock };
			scn->ForEachActor([&](RE::Actor* currentActor, ActorPropertyMap&) {
				SetActorInScene(currentActor, true);
			}, false);

			InsertSceneIntoMap(scn);
		}

		static bool StopScene(uint64_t sceneId, bool forceStop = false) {
			std::shared_ptr<IScene> targetScene;
			if (!GetScene(sceneId, targetScene)) {
				std::unique_lock l{ actorsWalkingLock };
				auto iter = state->actorsWalkingToScene.find(sceneId);

				if (iter != state->actorsWalkingToScene.end()) {
					for (auto& info : iter->second.completionState) {
						StopWalkPackage(info.first);
					}
					state->actorsWalkingToScene.erase(iter);
					Data::Events::Send(Data::Events::SCENE_END, Data::Events::SceneData{ sceneId });
					return true;
				} else {
					return false;
				}
			}

			std::unique_lock l{ targetScene->lock };
			bool res = false;
			if (targetScene->status == SceneState::Ending || forceStop) {
				res = targetScene->End();
			} else {
				targetScene->SoftEnd();
			}
			return res;
		}

		static void DetachScene(uint64_t sceneId) {
			std::shared_ptr<IScene> scn = ReleaseSceneFromMap(sceneId);

			if (scn != nullptr) {
				std::unique_lock l{ scn->lock };
				scn->ForEachActor([&](RE::Actor* currentActor, ActorPropertyMap&) {
					SetActorInScene(currentActor, false);
				}, false);
			}
		}

		static const std::vector<std::shared_ptr<IScene>> GetSceneMapSnapshot() {
			DataReadLock l{ scenesMapLock };
			std::vector<std::shared_ptr<IScene>> result;
			result.reserve(state->scenes.size());
			for (const auto& p : state->scenes) {
				result.push_back(p.second);
			}
			return result;
		}

		static void UpdateScenes() {
			const auto localScenes = GetSceneMapSnapshot();

			for (auto& scn : localScenes) {
				std::unique_lock l{ scn->lock };
				//If scene is queued for detach and its only references are the scene map and this function's
				//local copy of the scene map, process the detach.
				if (scn->detachQueued && scn.use_count() <= 2) {
					DetachScene(scn->uid);
					continue;
				}
				if (scn->status != SceneState::PendingDeletion && !scn->noUpdate && scn->actors.size() > 0) {
					auto firstActor = scn->actors.begin()->first.get().get();
					if (firstActor != nullptr && firstActor->parentCell != nullptr && firstActor->parentCell->loadedData != nullptr) {
						scn->Update();
					}
				}
			}
		}

		static void Reset() {
			const auto localScenes = GetSceneMapSnapshot();

			for (auto scn : localScenes) {
				std::unique_lock l{ scn->lock };
				scn->status = SceneState::PendingDeletion;
			}

			std::scoped_lock l{ scenesMapLock, actorsWalkingLock };
			state = std::make_unique<PersistentState>();
		}
		
	protected:
		inline static Data::Events::EventRegistration keyReg;

		inline static std::shared_ptr<IScene> ReleaseSceneFromMap(uint64_t sceneId)
		{
			std::shared_ptr<IScene> result;

			DataWriteLock l{ scenesMapLock };
			if (state->scenes.contains(sceneId)) {
				result = state->scenes[sceneId];
				state->scenes.erase(sceneId);
			}

			return result;
		}

		inline static void InsertSceneIntoMap(std::shared_ptr<IScene> scn) {
			DataWriteLock l{ scenesMapLock };
			state->scenes.insert({ scn->uid, scn });
		}

		inline static void ClearSceneMap() {
			DataWriteLock l{ scenesMapLock };
			state->scenes.clear();
		}

		static void OnHudUpKey(Data::Events::event_type, Data::Events::EventData&) {
			if (state->playerWalkInstance > 0) {
				CompleteWalk(state->playerWalkInstance);
			}

			Data::Events::Unsubscribe(Data::Events::HUD_UP_KEY_DOWN, keyReg);
		}

		static bool StartWalkPackage(RE::Actor* a, const RE::TESObjectREFR* destRefr, uint64_t uid) {
			if (!a) {
				return false;
			}

			if (a == player) {
				player->DisableInputForPlayer("NAF_Scene", GetInputsToDisableForScene());
				player->SetAIControlled(true);
				state->playerWalkInstance = uid;
				keyReg = Data::Events::Subscribe(Data::Events::HUD_UP_KEY_DOWN, &OnHudUpKey);

				F4SE::GetTaskInterface()->AddUITask([]() {
					std::unique_lock l{ actorsWalkingLock };
					using HUD = Menu::HUDManager;
					auto& txt = state->skipWalkText;

					txt = HUD::DrawText("Press [Up Arrow] to skip walk", 0, 0, 18.0f);
					auto txtX = static_cast<float>(960 - (HUD::GetElementWidth(txt) * 0.5));
					HUD::SetElementPosition(txt, txtX, 1100);
					auto txtPos = HUD::GetElementPosition(txt);
					state->skipWalkBox = HUD::DrawRectangle(txtPos.x - 5.0f, txtPos.y - 5.0f, HUD::GetElementWidth(txt) + 10.0f, HUD::GetElementHeight(txt) + 10.0f, 0, 0.8f, 2.0f, 0xFFFFFF, 1.0f);
					HUD::MoveElementToFront(txt);
					HUD::AttachElementTo(state->skipWalkBox, txt);
					HUD::TranslateElementTo(txt, txtX, 800.0f, 1.0f);
				});
			}

			a->StopCombat();
			a->StopAlarmOnActor();
			a->SetLinkedRef(destRefr, Data::Forms::NAFWalkToKW);
			if (PackageOverride::Set(a->GetActorHandle(), Data::Forms::NAFWalkPackage, true, std::make_unique<WalkFinishedFunctor>(uid))) {
				SetActorInScene(a, true);
				return true;
			} else {
				a->SetLinkedRef(nullptr, Data::Forms::NAFWalkToKW);
				return false;
			}
		}

		static void StopWalkPackage(RE::ActorHandle a) {
			PackageOverride::Clear(a, true);
			auto ref = a.get().get();

			if (ref != nullptr) {
				ref->SetLinkedRef(nullptr, Data::Forms::NAFWalkToKW);
			}

			if (ref == player) {
				player->ReenableInputForPlayer();
				player->SetAIControlled(false);
				state->playerWalkInstance = 0;

				F4SE::GetTaskInterface()->AddUITask([]() {
					std::unique_lock l{ actorsWalkingLock };
					Menu::HUDManager::RemoveElement(state->skipWalkText);
					Menu::HUDManager::RemoveElement(state->skipWalkBox);
					state->skipWalkText = 0;
					state->skipWalkBox = 0;
				});
			}

			SetActorInScene(ref, false);
		}

	public:

		static StartResult ValidateStartSceneArgs(const SceneSettings& settings, bool ignoreInScene = false)
		{
			if (!settings.startPosition) {
				return { kNullPosition };
			}

			auto actors = settings.QActors();
			auto locationRef = settings.locationRefr.get();

			if (locationRef == nullptr || locationRef->parentCell == nullptr) {
				return { kBadLocation };
			}

			if (actors.size() < 1) {
				return { kNoActors };
			}

			for (auto& aNi : actors) {
				auto a = aNi.get();
				if (!GameUtil::ActorIsAlive(a, false) || a->HasKeyword(Data::Forms::ActorTypeChildKW)) {
					return { kInvalidActor };
				} else if (IsActorInScene(a, ignoreInScene)) {
					return { kActorInScene };
				} else if (a == player && locationRef->parentCell != nullptr && player->parentCell != nullptr &&
					locationRef->parentCell->GetWorldSpace() != player->parentCell->GetWorldSpace()) {
					return { kPlayerWrongWorldspace };
				}
			}

			return { kNone };
		}
	};
}

CEREAL_REGISTER_TYPE(Scene::SceneManager::WalkFinishedFunctor);
