#include "Scene/SceneBase.h"
#include "Scene/SceneManager.h"
#include "Misc/GameUtil.h"
#pragma once

namespace Menu::NAF
{
	class ManageScenesHandler : public BindableMenu<ManageScenesHandler, SUB_MENU_TYPE::kManageScenes>
	{
	public:
		enum Stage
		{
			kSelectScene,
			kManageScene,
			kManageWalkInstance,
			kSelectPosition
		};

		Stage currentStage = Stage::kSelectScene;
		uint64_t selectionId = 0;

		float cachedSpeed = 0.0f;
		std::string cachedAnimId = "N/A";
		Scene::SyncState cachedSyncStatus = Scene::Synced;

		std::vector<std::string> cachedPositions;

		using BindableMenu::BindableMenu;

		virtual ~ManageScenesHandler()
		{
		}

		using Events = Data::Events;

		virtual void InitSubmenu() override
		{
			BindableMenu::InitSubmenu();
			RegisterUIListener({
				Events::SCENE_START,
				Events::SCENE_END,
				Events::SCENE_SYNC_STATUS_CHANGE,
				Events::SCENE_SPEED_CHANGE,
				//Events::SCENE_ANIM_LOOP,
				Events::SCENE_ANIM_CHANGE
			});

			auto d = PersistentMenuState::SceneData::GetSingleton();
			if (d->pendingSceneId > 0) {
				selectionId = d->pendingSceneId;
				currentStage = d->isWalkInstance ? kManageWalkInstance : kManageScene;
				d->pendingSceneId = 0;
			}
		}

		virtual void OnUIEvent(Events::event_type evnt, const Events::EventData& data) override {
			switch (evnt) {
			case Events::SCENE_START:
				OnSceneStart(data);
				break;
			case Events::SCENE_END:
				OnSceneEnd(data);
				break;
			case Events::SCENE_SYNC_STATUS_CHANGE:
				RefreshIfMatchPair<Scene::SyncState>(data);
				break;
			case Events::SCENE_SPEED_CHANGE:
				RefreshIfMatchPair<float>(data);
				break;
			case Events::SCENE_ANIM_LOOP:
				OnSceneAnimLoop(data);
				break;
			case Events::SCENE_ANIM_CHANGE:
				RefreshIfMatchPair<std::string>(data);
				break;
			}
		}

		void OnSceneStart(const Events::EventData& data) {
			if (auto uid = std::any_cast<uint64_t>(&data); currentStage == Stage::kManageWalkInstance && uid && (*uid) == selectionId) {
				currentStage = Stage::kManageScene;
				manager->RefreshList(false);
			} else if (currentStage == Stage::kSelectScene) {
				manager->RefreshList(false);
			}
		}

		void OnSceneEnd(const Events::EventData& data)
		{
			if (auto d = std::any_cast<Events::SceneData>(&data); currentStage != Stage::kSelectScene && d && d->id == selectionId) {
				currentStage = Stage::kSelectScene;
				manager->RefreshList(true);
				manager->ShowNotification("Scene ended.");
			} else if (currentStage == Stage::kSelectScene) {
				manager->RefreshList(false);
			}
		}

		template <typename T>
		void RefreshIfMatchPair(const Events::EventData& data)
		{
			if (auto info = std::any_cast<std::pair<uint64_t, T>>(&data); currentStage == Stage::kManageScene && info && info->first == selectionId) {
				manager->RefreshList(false);
			}
		}

		void OnSceneAnimLoop(const Events::EventData& data) {
			if (auto info = std::any_cast<uint64_t>(&data); currentStage == Stage::kManageScene && info && (*info) == selectionId) {
				manager->ShowNotification("Looped!", 0.5f);
			}
		}

		std::string GetSceneName(std::vector<SerializableActorHandle> actors) {
			std::string result = "";
			for (auto& h : actors) {
				auto a = h.get();
				if (a != nullptr) {
					result += GameUtil::GetActorName(a.get()) + " x ";
				}
			}
			if (result.size() > 3) {
				result = result.substr(0, result.size() - 3);
			} else {
				//Scene has no actors, or the only actor in the scene has an empty name
				result = "<possibly_delinquent_scene>";
			}
			return result;
		}

		virtual BindingsVector GetBindings() override
		{
			manager->SetPanelShown(false);
			SetSearchWidgetEnabled(currentStage == kSelectPosition);

			switch (currentStage) {
				case kSelectScene:
				{
					BindingsVector result;
					std::vector<SerializableActorHandle> actors;

					Scene::SceneManager::VisitAllScenes([&](Scene::IScene* scn) {

						actors.clear();
						for (auto& a : scn->actors) {
							actors.push_back(a.first);
						}

						result.push_back({ GetSceneName(actors), MENU_BINDING_WARG(ManageScenesHandler::SelectScene, scn->uid), false, 0, 0, 0, std::nullopt, false, nullptr,
							MENU_BINDING_WARG(ManageScenesHandler::SceneHovered, scn->uid) });
					});

					Scene::SceneManager::VisitAllWalkInstances([&](Scene::SceneManager::WalkInfo& info, uint64_t instanceId) {

						actors.clear();
						for (auto& a : info.completionState) {
							actors.push_back(a.first);
						}

						result.push_back({ GetSceneName(actors), MENU_BINDING_WARG(ManageScenesHandler::SelectWalkInstance, instanceId), false, 0, 0, 0, std::nullopt, false, nullptr,
							MENU_BINDING_WARG(ManageScenesHandler::WalkInstanceHovered, instanceId) });
					});

					manager->SetMenuTitle("Active Scenes [" + std::to_string(result.size()) + "]");
					return result;
				}
				case kManageScene:
				{
					CacheSceneInfo();
					return {
						{ "Scene Status: Active", std::nullopt },
						{ std::format("Sync Status: {}", GetSyncStateString()), std::nullopt },
						{ "Speed (%): ", MENU_BINDING(ManageScenesHandler::AdjustSceneSpeed), true, 1, 500, static_cast<int>(cachedSpeed) },
						{ std::format("Current Animation: {}", cachedAnimId), std::nullopt },
						{ "Change Position...", MENU_BINDING(ManageScenesHandler::ChangePosition) },
						{ "Actor Inventories...", MENU_BINDING_WARG(ManageScenesHandler::GotoInventoriesMenu, selectionId) },
						{ "Stop Scene", MENU_BINDING(ManageScenesHandler::StopScene) }						
					};
				}
				case kManageWalkInstance:
				{
					return {
						{ "Scene Status: Walking to Scene..." },
						{ "Skip Walk", MENU_BINDING(ManageScenesHandler::SkipWalk) },
						{ "Stop Scene", MENU_BINDING(ManageScenesHandler::StopScene) }
					};
				}
				case kSelectPosition:
				{
					ConfigurePanel();

					BindingsVector result;
					result.reserve(cachedPositions.size());

					for (auto& p : cachedPositions) {
						std::string strPos(p);
						result.push_back({ strPos, MENU_BINDING_WARG(ManageScenesHandler::SetPosition, strPos) });
					}

					return result;
				}
				default:
				{
					return {
						{ "No options." }
					};
				}
			}
		}

		void GotoInventoriesMenu(uint64_t sceneId, int) {
			auto sceneData = PersistentMenuState::SceneData::GetSingleton();
			sceneData->pendingSceneId = sceneId;
			manager->GotoMenu(kInventories, true);
		}

		void ChangePosition(int) {
			RE::NiPointer<RE::TESObjectREFR> locRefr = nullptr;
			std::vector<RE::NiPointer<RE::Actor>> actors;
			Scene::SceneManager::VisitScene(selectionId, [&](Scene::IScene* scn) {
				actors = Scene::GetActorsInOrder(scn->actors);
				locRefr = scn->settings.locationRefr.get();
			});

			if (actors.size() < 1) {
				manager->ShowNotification("Error: Failed to get scene actors.");
				return;
			}

			cachedPositions = Data::Global::GetFilteredPositions(actors, true, true, locRefr.get());

			if (cachedPositions.size() < 1) {
				manager->ShowNotification("No available positions for combination of actors.");
				return;
			}

			currentStage = Stage::kSelectPosition;
			manager->RefreshList(true);
		}

		void SetPosition(std::string pos, int) {
			Scene::SceneManager::VisitScene(selectionId, [&](Scene::IScene* scn) {
				scn->SetPosition(pos);
			});

			currentStage = kManageScene;
			manager->RefreshList(true);
		}

		void AdjustSceneSpeed(int amount) {
			Scene::SceneManager::VisitScene(selectionId, [&](Scene::IScene* scn) {
				scn->SetAnimMult(static_cast<float>(amount));
			});
		}

		void StopScene(int) {
			Scene::SceneManager::StopScene(selectionId);
			Back();
		}

		void SkipWalk(int) {
			Scene::SceneManager::CompleteWalk(selectionId);
		}

		void CacheSceneInfo() {
			cachedSpeed = 0.0f;
			cachedSyncStatus = Scene::Synced;
			cachedAnimId = "N/A";
			Scene::SceneManager::VisitScene(selectionId, [&](Scene::IScene* scn) {
				cachedSpeed = scn->animMult;
				cachedSyncStatus = scn->syncStatus;
				cachedAnimId = scn->controlSystem->QAnimationID();
				if (cachedAnimId.size() < 1)
					cachedAnimId = "N/A";
			});
		}

		std::string GetSyncStateString() {
			switch (cachedSyncStatus) {
			case Scene::WaitingForLoad:
				return "Animations Loading...";
			case Scene::SyncingTimes:
				return "Synchronizing Animations...";
			default:
				return "Synced";
			}
		}

		void WalkInstanceHovered(uint64_t instanceId, int hovered) {
			manager->highlightedObjects.RemoveAll();
			if (hovered) {
				Scene::SceneManager::VisitWalkInstance(instanceId, [&](Scene::SceneManager::WalkInfo& info) {
					for (auto& h : info.completionState) {
						manager->highlightedObjects.Add(h.first.get().get());
					}
				});
			}
		}

		void SelectWalkInstance(uint64_t instanceId, int index) {
			selectionId = instanceId;
			currentStage = Stage::kManageWalkInstance;
			manager->highlightedObjects.RemoveAll();
			manager->SetMenuTitle(manager->menuItems[index].label);
			manager->RefreshList(true);
		}

		void SceneHovered(uint64_t uid, int hovered) {
			manager->highlightedObjects.RemoveAll();
			if (hovered) {
				Scene::SceneManager::VisitScene(uid, [&](Scene::IScene* scn) {
					for (auto& h : scn->actors) {
						manager->highlightedObjects.Add(h.first.get().get());
					}
				});
			}
		}

		void SelectScene(uint64_t uid, int index) {
			selectionId = uid;
			currentStage = Stage::kManageScene;
			manager->highlightedObjects.RemoveAll();
			manager->SetMenuTitle(manager->menuItems[index].label);
			manager->RefreshList(true);
		}

		virtual void Back() override
		{
			switch (currentStage) {
			case kSelectPosition:
				currentStage = Stage::kManageScene;
				manager->RefreshList(true);
				break;
			case kManageWalkInstance:
			case kManageScene:
				currentStage = Stage::kSelectScene;
				selectionId = 0;
				manager->RefreshList(true);
				break;
			default:
				manager->GotoMenu(SUB_MENU_TYPE::kMain, true);
			}
		}
	};
}
