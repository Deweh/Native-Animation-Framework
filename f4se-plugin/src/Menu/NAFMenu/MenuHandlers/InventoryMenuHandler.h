#pragma once
#include "Menu/BindableMenu.h"
#include "Misc/MathUtil.h"
#include "Scene/SceneBase.h"
#include "Scene/SceneManager.h"
#include "Misc/MenuOpenCloseEventHandler.h"

namespace Menu::NAF
{
	class InventoryMenuHandler : public BindableMenu<InventoryMenuHandler, SUB_MENU_TYPE::kInventories>
	{
	public:
		using BindableMenu::BindableMenu;

		// current scene ID
		uint64_t curSceneId = 0;

		virtual ~InventoryMenuHandler() {}

		virtual void InitSubmenu() override
		{
			// setup this submenu
			BindableMenu::InitSubmenu();
			auto sceneData = PersistentMenuState::SceneData::GetSingleton();
			curSceneId = sceneData->pendingSceneId;
			sceneData->pendingSceneId = 0;
		}

		virtual BindingsVector GetBindings() override
		{
			// menu title
			manager->SetMenuTitle("Actor Inventories");
			
			// populate menu with actors list and showactorinventory actions
			BindingsVector result;
			Scene::SceneManager::VisitScene(curSceneId, [&](Scene::IScene* scn) {
				scn->ForEachActor([&](RE::Actor* currentActor, Scene::ActorPropertyMap&) {
					result.push_back({ GameUtil::GetActorName(currentActor),
						Bind(&InventoryMenuHandler::ShowActorInventory, currentActor->GetActorHandle()) });
				});
			});

			return result;
		}

		void ShowActorInventory(RE::ActorHandle h, int)
		{
			RE::Actor* a = h.get().get();
			if (a)
			{
				// preserve keyword state and add keyword
				bool hadSWIKeyword = a->HasKeyword(Data::Forms::ShowWornItemsKW);
				if (!hadSWIKeyword)
				{
					a->ModifyKeyword(Data::Forms::ShowWornItemsKW, true);
				}

				// persistent state
				auto sceneData = PersistentMenuState::SceneData::GetSingleton();
				sceneData->pendingSceneId = curSceneId;
				sceneData->restoreSubmenu = kInventories;							
				if (!hadSWIKeyword)
				{
					sceneData->removeShowWornItemsKWActorHandle = h;
				}
				else
				{
					sceneData->removeShowWornItemsKWActorHandle = std::nullopt;
				}
				
				// close NAF menu
				Menu::IStateManager::activeInstance->CloseMenu();

				// open actor container menu
				RE::ContainerMenuNAF::OpenContainerMenu(a, 3, false);

			}
		}

		void Goto(SUB_MENU_TYPE menuType, int)
		{
			// navigate to specified menu
			manager->GotoMenu(menuType, true);
		}

		virtual void BackImpl() override
		{
			// navigate back to scenes menu
			manager->GotoMenu(kManageScenes, false, false);
		}
	};
}
