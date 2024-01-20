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

		inline static float transitionTime = 1.0f;

		std::function<void(int)> BindGoto(SUB_MENU_TYPE ty) {
			return Bind(&MainMenuHandler::Goto, ty);
		}

		virtual BindingsVector GetBindings() override {
			manager->SetMenuTitle("NAF");
			return {
				{ "New Scene", BindGoto(kNewScene) },
				{ "Active Scenes", BindGoto(kManageScenes) },
				{ "Creator", BindGoto(kCreator) },
				{ "Item Explorer", BindGoto(kItemExplorer) },
				{ "Settings", BindGoto(kSettings) },
				//{ GetRecordingAnim() ? "Stop Recording" : "Start Recording", Bind(&MainMenuHandler::ToggleRecording) },
				//{ "Play Data/NAF/animOutput.nanim", Bind(&MainMenuHandler::Start) },
				//{ "Stop Animation", Bind(&MainMenuHandler::Stop) },
				//{ "File Browser", Bind(&MainMenuHandler::DoFileBrowserWidget) }
			};
		}

		bool GetRecordingAnim() {
			bool result = false;
			BodyAnimation::GraphHook::VisitGraph(RE::PlayerCharacter::GetSingleton(), [&](BodyAnimation::NodeAnimationGraph* g) {
				result = g->GetRecording();
			});
			return result;
		}

		void ToggleRecording(int) {
			BodyAnimation::GraphHook::VisitGraph(RE::PlayerCharacter::GetSingleton(), [&](BodyAnimation::NodeAnimationGraph* g) {
				g->SetRecording(!g->GetRecording());
				if (!g->GetRecording()) {
					g->recorder.SaveRecording("Data\\NAF\\animOutput.nanim", g->nodeMap);
					g->flags.set(BodyAnimation::NodeAnimationGraph::kTemporary);
				} else {
					g->flags.reset(BodyAnimation::NodeAnimationGraph::kTemporary);
				}
			});

			manager->RefreshList(false);
		}

		void ChangeTransTime(float alter, int) {
			transitionTime += alter;
			manager->RefreshList(false);
		}

		void Start(int) {
			BodyAnimation::GraphHook::LoadAndPlayAnimation(RE::PlayerCharacter::GetSingleton(), "Data\\NAF\\animOutput.nanim", transitionTime);
		}

		void Stop(int) {
			BodyAnimation::GraphHook::StopAnimation(RE::PlayerCharacter::GetSingleton(), transitionTime);
		}

		void LogNode(int) {
			auto _3d = RE::PlayerCharacter::GetSingleton()->Get3D();
			LogTransform(_3d->GetObjectByName("Root"));
			LogTransform(_3d->GetObjectByName("COM"));
			LogTransform(_3d->GetObjectByName("Pelvis"));
		}

		void LogTransform(RE::NiAVObject* n) {
			if (!n)
				return;
			logger::info("{}:", n->name);
			logger::info("world: {:.3f}, {:.3f}, {:.3f}", n->world.translate.x, n->world.translate.y, n->world.translate.z);
			logger::info("local: {:.3f}, {:.3f}, {:.3f}", n->local.translate.x, n->local.translate.y, n->local.translate.z);
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
