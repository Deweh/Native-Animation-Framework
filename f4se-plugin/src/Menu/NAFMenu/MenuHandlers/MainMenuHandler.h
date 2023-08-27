#pragma once
#include "Menu/BindableMenu.h"
#include "Scene/SceneBase.h"
#include "Misc/MathUtil.h"

namespace Menu::NAF
{
	class MainMenuHandler : public BindableMenu<MainMenuHandler, SUB_MENU_TYPE::kMain>
	{
	public:
		using BindableMenu::BindableMenu;

		virtual ~MainMenuHandler() {}

		std::function<void(int)> BindGoto(SUB_MENU_TYPE ty) {
			return Bind(&MainMenuHandler::Goto, ty);
		}

		virtual BindingsVector GetBindings() override {
			manager->SetMenuTitle("NAF");
			return {
				{ "New Scene", BindGoto(kNewScene) },
				{ "Active Scenes", BindGoto(kManageScenes) },
				{ "Creator", BindGoto(kCreator) },
				{ "Settings", BindGoto(kSettings) },				
				//{ "Test", Bind(&MainMenuHandler::Test) },
				//{ "File Browser", Bind(&MainMenuHandler::DoFileBrowserWidget) }
			};
		}

		void DoFileBrowserWidget(int) {
			ShowFileBrowser("Data\\Meshes", ".hkx", [&](bool ok, const std::string& filename) {
				if (ok) {
					manager->ShowNotification(std::format("Selected {}", filename));
				} else {
					manager->ShowNotification("Canceled.");
				}
				manager->RefreshList(true);
			});
		}

		void Test(int) {
			auto newAnim = std::make_shared<Data::Animation>();
			newAnim->id = "AAA_Test_AAA";
			newAnim->loadPriority = 10'000;
			
			Data::Animation::Slot slot;
			slot.rootBehavior = "RaiderProject";
			slot.gender = ActorGender::Male;
			slot.dynamicIdle = true;
			slot.idle = "Test_M1.hkx";

			newAnim->slots.push_back(slot);

			slot.gender = ActorGender::Female;
			slot.idle = "Test_F1.hkx";

			newAnim->slots.push_back(slot);

			Data::Global::Animations.insert({ newAnim->id, { {}, newAnim } });

			auto newPos = std::make_shared<Data::Position>();
			newPos->hidden = false;
			newPos->id = "AAA_Test_AAA";
			newPos->idForType = "AAA_Test_AAA";
			newPos->posType = Data::Position::kAnimation;
			newPos->loadPriority = 10'000;
			newPos->startEquipSet = "unEquip";
			newPos->stopEquipSet = "reEquip";
			newPos->startMorphSet = "ready";
			newPos->stopMorphSet = "unReady";

			Data::Global::Positions.insert({ newPos->id, { {}, newPos } });

			manager->ShowNotification("Done");
		}

		void Goto(SUB_MENU_TYPE menuType, int) {
			manager->GotoMenu(menuType, true);
		}

		void StowPlayerWeapon(int) {
			RE::PlayerCharacter::GetSingleton()->DrawWeaponMagicHands(false);
		}

		virtual void BackImpl() override {
			manager->CloseMenu();
		}
	};
}
