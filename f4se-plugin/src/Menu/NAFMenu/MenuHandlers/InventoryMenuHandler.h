#pragma once
#include "Menu/BindableMenu.h"
#include "Misc/MathUtil.h"
#include "Scene/SceneBase.h"
#include "Scene/SceneManager.h"

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

				RE::ContainerMenuNAF::OpenContainerMenu(a, 3, false);

				if (!hadSWIKeyword)
				{
					a->ModifyKeyword(Data::Forms::ShowWornItemsKW, false);
				}
			}
		}

		void Goto(SUB_MENU_TYPE menuType, int)
		{
			manager->GotoMenu(menuType, true);
		}

		void StowPlayerWeapon(int)
		{
			RE::PlayerCharacter::GetSingleton()->DrawWeaponMagicHands(false);
		}

		virtual void BackImpl() override
		{
			manager->CloseMenu();
		}
	};
}
