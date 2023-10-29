#include "NAFMenuStateManager.h"
#pragma once

namespace Menu
{
	class NAFMenu : public RE::GameMenuBase
	{
	public:
		std::unique_ptr<NAFMenuStateManager> stateManager;
		bool inTextEntry = false;

		static RE::IMenu* Create(const RE::UIMessage&) {
			return new NAFMenu();
		}

		NAFMenu() {
			menuFlags.set(
				RE::UI_MENU_FLAGS::kUsesCursor,
				RE::UI_MENU_FLAGS::kDisablePauseMenu,
				RE::UI_MENU_FLAGS::kUsesMovementToDirection
			);

			depthPriority.set(RE::UI_DEPTH_PRIORITY::kTerminal);

			const auto ScaleformManager = RE::BSScaleformManager::GetSingleton();
			if (!ScaleformManager->LoadMovieEx(*this, "Interface/NAFMenu.swf", "root.Menu_mc", RE::Scaleform::GFx::Movie::ScaleModeType::kShowAll)) {
				RE::SendHUDMessage::ShowHUDMessage("Failed to load menu.", "UIMenuCancel", false, true);
				return;
			}

			filterHolder = RE::msvc::make_unique<RE::BSGFxShaderFXTarget>(*uiMovie, "root.Menu_mc");
			if (filterHolder) {
				filterHolder->CreateAndSetFiltersToHUD(RE::HUDColorTypes::kGameplayHUDColor);
				shaderFXObjects.push_back(filterHolder.get());
			}
		}

		virtual void MapCodeObjectFunctions() override {
			MapCodeMethodToASFunction("CloseMenu", 0);
			MapCodeMethodToASFunction("PlaySound", 1);
			MapCodeMethodToASFunction("SetTextEntry", 2);
			MapCodeMethodToASFunction("NotifyLoaded", 3);
			MapCodeMethodToASFunction("ItemSelected", 4);
			MapCodeMethodToASFunction("ItemHoverChanged", 5);
			MapCodeMethodToASFunction("ButtonClicked", 6);
			MapCodeMethodToASFunction("TextEntryCompleted", 7);
			MapCodeMethodToASFunction("TickerSelected", 8);
			MapCodeMethodToASFunction("TickerDynamicSelected", 9);
			MapCodeMethodToASFunction("ItemRightClicked", 10);
		}

		virtual void Call(const Params& a_params) override {
			switch ((*((std::uint32_t*)&(a_params.userData)))) {
				case 0:
				{
					CloseMenu();
					break;
				}
				case 1:
				{
					if (a_params.argCount == 1 && a_params.args[0].IsString()) {
						RE::UIUtils::PlayMenuSound(a_params.args[0].GetString());
					}
					break;
				}
				case 2:
				{
					if (a_params.argCount == 1 && a_params.args[0].IsBoolean()) {
						inTextEntry = a_params.args[0].GetBoolean();
						auto ControlMap = RE::ControlMap::GetSingleton();
						auto UI = RE::UI::GetSingleton();
						ControlMap->SetTextEntryMode(inTextEntry);
						UI->UpdateMenuMode("NAFMenu", inTextEntry);
					}
					break;
				}
				case 3:
				{
					stateManager = std::make_unique<NAFMenuStateManager>(this);
					stateManager->InitMenu();
					break;
				}
				case 4:
				{
					if (a_params.argCount == 1 && a_params.args[0].IsInt()) {
						if (stateManager) {
							stateManager->ItemSelected(a_params.args[0].GetInt());
						}
					}
					break;
				}
				case 5:
				{
					if (a_params.argCount == 2 && a_params.args[0].IsInt() && a_params.args[1].IsBoolean()) {
						if (stateManager) {
							stateManager->ItemHoverChanged(a_params.args[0].GetInt(), a_params.args[1].GetBoolean());
						}
					}
					break;
				}
				case 6:
				{
					if (a_params.argCount == 1 && a_params.args[0].IsInt()) {
						if (stateManager) {
							stateManager->ButtonClicked(a_params.args[0].GetInt());
						}
					}
					break;
				}
				case 7:
				{
					if (a_params.argCount == 2 && a_params.args[0].IsBoolean() && a_params.args[1].IsString()) {
						if (stateManager) {
							stateManager->TextEntryCompleted(a_params.args[0].GetBoolean(), a_params.args[1].GetString());
						}
					}
					break;
				}
				case 8:
				{
					if (a_params.argCount == 2 && a_params.args[0].IsInt() && a_params.args[1].IsInt()) {
						if (stateManager) {
							stateManager->TickerSelected(a_params.args[0].GetInt(), a_params.args[1].GetInt());
						}
					}
					break;
				}
				case 9:
				{
					if (a_params.argCount == 2 && a_params.args[0].IsInt() && a_params.args[1].IsInt()) {
						if (stateManager) {
							stateManager->DynamicTickerSelected(a_params.args[0].GetInt(), a_params.args[1].GetInt());
						}
					}
					break;
				}
				case 10:
				{
					if (a_params.argCount == 1 && a_params.args[0].IsInt()) {
						if (stateManager) {
							stateManager->ItemRightClicked(a_params.args[0].GetInt());
						}
					}
					break;
				}
				default:
					break;
			}
		}

		void CloseMenu() {
			if (auto UIMessageQueue = RE::UIMessageQueue::GetSingleton(); UIMessageQueue) {
				RE::UIUtils::PlayMenuSound("UIMenuCancel");
				UIMessageQueue->AddMessage("NAFMenu", RE::UI_MESSAGE_TYPE::kHide);
			}
		}

		virtual void HandleEvent(const RE::ButtonEvent* a_event) override {
			if (!a_event->disabled && inputEventHandlingEnabled && menuObj.IsObject() && menuObj.HasMember("ProcessButtonEvent")) {
				switch (a_event->GetBSButtonCode()) {
					case RE::BS_BUTTON_CODE::kEscape:
					case RE::BS_BUTTON_CODE::kTab:
					case RE::BS_BUTTON_CODE::kBButton:
						if (a_event->QUserEvent() != "NextFocus" && a_event->QJustPressed()) {
							if (stateManager) {
								stateManager->Back();
							}
						}
						break;

					case RE::BS_BUTTON_CODE::kEnter:
					case RE::BS_BUTTON_CODE::kE:
					case RE::BS_BUTTON_CODE::kAButton:
						if (a_event->QJustPressed()) {
							RE::Scaleform::GFx::Value args[1];
							args[0] = "Accept";
							menuObj.Invoke("ProcessButtonEvent", nullptr, args, 1);
						}
						break;

					case RE::BS_BUTTON_CODE::kUp:
					case RE::BS_BUTTON_CODE::kW:
					case RE::BS_BUTTON_CODE::kDPAD_Up:
						if (a_event->QJustPressed()) {
							RE::Scaleform::GFx::Value args[1];
							args[0] = "Up";
							menuObj.Invoke("ProcessButtonEvent", nullptr, args, 1);
						}
						break;

					case RE::BS_BUTTON_CODE::kDown:
					case RE::BS_BUTTON_CODE::kS:
					case RE::BS_BUTTON_CODE::kDPAD_Down:
						if (a_event->QJustPressed()) {
							RE::Scaleform::GFx::Value args[1];
							args[0] = "Down";
							menuObj.Invoke("ProcessButtonEvent", nullptr, args, 1);
						}
						break;

					case RE::BS_BUTTON_CODE::kLeft:
					case RE::BS_BUTTON_CODE::kA:
					case RE::BS_BUTTON_CODE::kDPAD_Left:
						if (a_event->QJustPressed()) {
							RE::Scaleform::GFx::Value args[1];
							args[0] = "Left";
							menuObj.Invoke("ProcessButtonEvent", nullptr, args, 1);
						}
						break;

					case RE::BS_BUTTON_CODE::kRight:
					case RE::BS_BUTTON_CODE::kD:
					case RE::BS_BUTTON_CODE::kDPAD_Right:
						if (a_event->QJustPressed()) {
							RE::Scaleform::GFx::Value args[1];
							args[0] = "Right";
							menuObj.Invoke("ProcessButtonEvent", nullptr, args, 1);
						}
						break;

					case RE::BS_BUTTON_CODE::kDelete:
					case RE::BS_BUTTON_CODE::kXButton:
						if (a_event->QJustPressed()) {
							RE::Scaleform::GFx::Value args[1];
							args[0] = "Delete";
							menuObj.Invoke("ProcessButtonEvent", nullptr, args, 1);
						}
						break;
				}
			}
		}

		virtual RE::UI_MESSAGE_RESULTS ProcessMessage(RE::UIMessage& a_message) override
		{
			switch (*a_message.type) {
			case RE::UI_MESSAGE_TYPE::kShow:
				{
					auto ControlMap = RE::ControlMap::GetSingleton();
					ControlMap->PushInputContext(RE::UserEvents::INPUT_CONTEXT_ID::kBasicMenuNav);
					ControlMap->PushInputContext(RE::UserEvents::INPUT_CONTEXT_ID::kThumbNav);
					ControlMap->PushInputContext(RE::UserEvents::INPUT_CONTEXT_ID::kVirtualController);
					RE::SendHUDMessage::PushHUDMode("SpecialMode");
					RE::PlayerControls::GetSingleton()->blockPlayerInput = true;

					return RE::UI_MESSAGE_RESULTS::kPassOn;
				}

			case RE::UI_MESSAGE_TYPE::kHide:
				{
					if (inTextEntry) {
						return RE::UI_MESSAGE_RESULTS::kIgnore;
					}

					if (stateManager) {
						stateManager.reset();
					}

					auto ControlMap = RE::ControlMap::GetSingleton();
					ControlMap->PopInputContext(RE::UserEvents::INPUT_CONTEXT_ID::kBasicMenuNav);
					ControlMap->PopInputContext(RE::UserEvents::INPUT_CONTEXT_ID::kThumbNav);
					ControlMap->PopInputContext(RE::UserEvents::INPUT_CONTEXT_ID::kVirtualController);
					
					if (!RE::UI::GetSingleton()->GetMenuOpen("NAFStudioMenu")) {
						RE::SendHUDMessage::PopHUDMode("SpecialMode");
						RE::PlayerControls::GetSingleton()->blockPlayerInput = false;
					}
						

					return RE::UI_MESSAGE_RESULTS::kPassOn;
				}

			default:
				return RE::IMenu::ProcessMessage(a_message);
			}
		}
	};
}
