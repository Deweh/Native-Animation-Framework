#include "Scene/SceneManager.h"
#include "Scene/SceneBase.h"
#pragma once

namespace Menu::NAF
{
	class NewSceneHandler : public BindableMenu<NewSceneHandler, SUB_MENU_TYPE::kNewScene>
	{
	public:
		enum Stage
		{
			kSelectActors,
			kSelectLocation,
			kSelectPosition
		};

		std::vector<SerializableActorHandle> cachedActors;
		std::vector<SerializableActorHandle> selectedActors;

		Data::Global::ApplicableFurniture cachedFurniture;
		std::vector<RE::ObjectRefHandle> cachedLocations;
		RE::ObjectRefHandle selectedLocation;
		std::vector<std::string> cachedPositions;

		Stage currentStage = Stage::kSelectActors;
		std::string searchTerm = "";

		using BindableMenu::BindableMenu;

		virtual ~NewSceneHandler()
		{
		}

		virtual void InitSubmenu() override {
			complexItemList = false;
		}

		virtual std::vector<std::string> GetItemList() override
		{
			std::vector<std::string> result;

			switch (currentStage) {
				case Stage::kSelectActors:
				{
					ConfigurePanel({
						{ "Confirm", Button, MENU_BINDING(NewSceneHandler::ConfirmActors) }
					});
					manager->SetMenuTitle("New Scene: Select Actors");

					auto distMap = GameUtil::GenerateRefDistMap<RE::Actor>([](RE::Actor* a) {
						return (GameUtil::ActorIsAlive(a) && !a->HasKeyword(Data::Forms::ActorTypeChildKW));
					});

					cachedActors.clear();
					selectedActors.clear();
					cachedActors.reserve(distMap.size());
					result.reserve(distMap.size());

					for (auto p : distMap) {
						if (!Scene::SceneManager::IsActorInScene(p.ref)) {
							cachedActors.push_back(p.ref->GetActorHandle());
							result.push_back(std::format("{} ({:.2f}ft)", p.name, p.distance));
						}
					}

					break;
				}
				case Stage::kSelectLocation:
				{
					manager->SetPanelShown(false);
					manager->SetMenuTitle("Select Location");

					std::vector<RE::TESObjectREFR*> actors;
					actors.reserve(selectedActors.size());
					for (auto& h : selectedActors) {
						auto a = h.get().get();
						if (a != nullptr) {
							actors.push_back(a);
						}
					}

					if (std::find(actors.begin(), actors.end(), player) == actors.end())
					{
						actors.push_back(player);
					}				

					auto distMap = GameUtil::GenerateRefDistMap<RE::TESObjectREFR>([&](RE::TESObjectREFR* r) {
						if (GameUtil::RefIsDisabled(r))
							return false;

						bool isMatch = cachedFurniture.forms.contains(r->data.objectReference);
						if (!isMatch && cachedFurniture.keywords.size() > 0) {
							RE::BSScrapArray<const RE::BGSKeyword*> kws;
							r->CollectAllKeywords(kws, nullptr);
							for (auto kw : kws) {
								if (cachedFurniture.keywords.contains(kw)) {
									isMatch = true;
									break;
								}
							}
						}

						return isMatch;
					}, actors);

					cachedLocations.clear();
					cachedLocations.reserve(distMap.size());

					for (auto p : distMap) {
						cachedLocations.push_back(p.ref->GetHandle());
						result.push_back(std::format("{} ({:.2f}ft)", p.name, p.distance));
					}
					break;
				}
				case Stage::kSelectPosition:
				{
					std::string finalTerm = searchTerm.size() > 0 ? searchTerm : "[None]";

					ConfigurePanel({
						{ "Search", Button, MENU_BINDING(NewSceneHandler::SearchPositions) },
						{ "Search Term: " + finalTerm, Info }
					});
					manager->SetMenuTitle("Select Position");

					for (auto& p : cachedPositions) {
						if (searchTerm.size() < 1 || Utility::StringToLower(p).find(searchTerm) != std::string::npos) {
							result.push_back(std::string(p));
						}
					}
					break;
				}
			}
			
			return result;
		}

		virtual void ItemHoverChanged(int index, bool hovered) override {
			switch (currentStage) {
			case Stage::kSelectActors:
				{
					auto a = cachedActors[index].get().get();

					if (hovered) {
						manager->highlightedObjects.AddSingle(a);
					} else {
						manager->highlightedObjects.RemoveAll();
					}
					break;
				}
			case Stage::kSelectLocation:
				{
					auto r = cachedLocations[index].get().get();

					if (hovered) {
						manager->highlightedObjects.AddSingle(r);
					} else {
						manager->highlightedObjects.RemoveAll();
					}
					break;
				}
			default:
				break;
			}
		}

		virtual void ItemSelected(int index) override
		{
			switch (currentStage) {
			case Stage::kSelectActors:
				{
					auto& menuItems = manager->menuItems;
					if (!menuItems[index].bold) {
						selectedActors.push_back(cachedActors[index]);
					} else {
						selectedActors.erase(std::remove(selectedActors.begin(), selectedActors.end(), cachedActors[index]), selectedActors.end());
					}
					manager->SetItem(index, menuItems[index].label, (menuItems[index].bold ? false : true));
					break;
				}
			case Stage::kSelectLocation:
				{
					std::vector<RE::Actor*> actors;
					actors.reserve(selectedActors.size());

					for (auto& h : selectedActors) {
						auto a = h.get().get();
						if (!a) {
							manager->ShowNotification("One or more selected actors no longer exist.");
							return;
						} else {
							actors.push_back(a);
						}
					}

					selectedLocation = cachedLocations[index];
					auto loc = selectedLocation.get().get();
					if (!loc) {
						manager->ShowNotification("The selected location no longer exists.");
						return;
					}

					cachedPositions = Data::Global::GetFilteredPositions(actors, true, true, loc);
					if (cachedPositions.size() < 1) {
						manager->ShowNotification("There are no available positions for the selected combination of actors.");
						return;
					}

					currentStage = Stage::kSelectPosition;
					manager->RefreshList(true);
					break;
				}
			case Stage::kSelectPosition:
				{
					for (auto& h : selectedActors) {
						if (!h.get().get()) {
							manager->ShowNotification("One or more selected actors no longer exist.");
							return;
						}
					}

					if (!selectedLocation.get().get()) {
						manager->ShowNotification("The selected location no longer exists.");
						return;
					}

					auto id = manager->menuItems[index].label;
					Scene::SceneSettings settings;
					settings.actors = selectedActors;
					settings.locationRefr = selectedLocation;
					settings.startPosition = Data::GetPosition(id);
					settings.duration = -1.0f;
					uint64_t sceneId = 0;
					if (auto res = Scene::SceneManager::WalkToAndStartScene(settings, sceneId); res) {
						PersistentMenuState::SceneData::GetSingleton()->pendingSceneId = sceneId;
					} else {
						manager->ShowNotification(std::format("Failed to start scene: {}", res.GetErrorMessage()));
						return;
					}
					
					manager->GotoMenu(SUB_MENU_TYPE::kManageScenes, true);
					break;
				}
			}
		}

		void ConfirmActors(int) {
			if (selectedActors.size() > 0) {
				std::vector<RE::Actor*> actors;
				actors.reserve(selectedActors.size());

				for (auto& h : selectedActors) {
					auto a = h.get().get();
					if (!a) {
						manager->ShowNotification("One or more selected actors no longer exist.");
						return;
					} else {
						actors.push_back(a);
					}
				}

				cachedFurniture = Data::Global::GetApplicableFurniture(actors, true);

				manager->highlightedObjects.RemoveAll();
				currentStage = Stage::kSelectLocation;
				manager->RefreshList(true);
			} else {
				manager->ShowNotification("At least one actor must be selected.");
			}
		}

		void SearchPositions(int) {
			GetTextInput([&](bool ok, const std::string& text) {
				if (ok) {
					//Remove new lines.
					auto txtFixed = text;
					txtFixed.erase(std::remove(txtFixed.begin(), txtFixed.end(), '\r'), txtFixed.end());
					txtFixed.erase(std::remove(txtFixed.begin(), txtFixed.end(), '\n'), txtFixed.end());
					searchTerm = Utility::StringToLower(txtFixed);
					manager->RefreshList(true);
				}
			});
		}

		virtual void Back() override
		{
			switch (currentStage) {
			case kSelectPosition:
				currentStage = kSelectLocation;
				manager->RefreshList(true);
				break;
			case kSelectLocation:
				currentStage = kSelectActors;
				manager->RefreshList(true);
				break;
			default:
				manager->GotoMenu(SUB_MENU_TYPE::kMain, true);
				break;
			}
		}
	};
}
