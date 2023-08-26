#pragma once
#include "InventoryMenuHandler.h"
#include "Menu/BindableMenu.h"
#include "Misc/MathUtil.h"
#include "Scene/SceneBase.h"

namespace Menu::NAF
{
	class InventoryMenuHandler : public BindableMenu<InventoryMenuHandler, SUB_MENU_TYPE::kInventories>
	{
	public:
		using BindableMenu::BindableMenu;

		virtual ~InventoryMenuHandler() {}

		std::function<void(int)> BindGoto(SUB_MENU_TYPE ty)
		{
			return Bind(&InventoryMenuHandler::Goto, ty);
		}

		virtual BindingsVector GetBindings() override
		{
			manager->SetMenuTitle("Inventories");
			BindingsVector result;

			SceneManager::VisitScene(sceneId, [&](IScene* scn) {
				scn->ForEachActor([&](RE::Actor* currentActor, ActorPropertyMap& props) {
					  result.push_back({ GameUtil::GetActorName(currentActor),					  
						Bind(&InventoryMenuHandler::ShowActorInventory, currentActor->GetActorHandle()) });
				});
			}

			return result;
		}

		void ShowActorInventory(RE::ActorHandle a)
		{
			bool hadSWIKeyword = a->HasKeyword(Data::Forms::ShowWornItemsKW);
			If(!hadSWIKeyword)
			{
				a->AddKeyword(Data::Forms::ShowWornItemsKW)
			}

			RE::ContainerMenuNAF::OpenContainerMenu(a, 3, false)

			If(!hadSWIKeyword)
			{
				a->RemoveKeyword(Data::Forms::ShowWornItemsKW)
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
