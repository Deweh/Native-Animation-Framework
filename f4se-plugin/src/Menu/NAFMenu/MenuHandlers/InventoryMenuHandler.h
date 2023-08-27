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

		uint64_t curSceneId = 0;

		virtual ~InventoryMenuHandler() {}

		virtual void InitSubmenu() override
		{
			BindableMenu::InitSubmenu();
			auto sceneData = PersistentMenuState::SceneData::GetSingleton();
			curSceneId = sceneData->pendingSceneId;
			sceneData->pendingSceneId = 0;
		}

		virtual BindingsVector GetBindings() override
		{			
			manager->SetMenuTitle("Actor Inventories");
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
				bool hadSWIKeyword = a->HasKeyword(Data::Forms::ShowWornItemsKW);
				if (!hadSWIKeyword)
				{
					a->ModifyKeyword(Data::Forms::ShowWornItemsKW, true);
				}

				auto sceneData = PersistentMenuState::SceneData::GetSingleton();
				sceneData->pendingSceneId = curSceneId;
				sceneData->restoreSubmenu = kInventories;			

				Menu::IStateManager::activeInstance->CloseMenu();
				Sleep(100);


				RE::ContainerMenuNAF::OpenContainerMenu(a, 3, false);

			}
		}

		void Goto(SUB_MENU_TYPE menuType, int)
		{
			manager->GotoMenu(menuType, true);
		}

		virtual void BackImpl() override
		{
			manager->GotoMenu(kManageScenes, false, false);
		}
	};
}
