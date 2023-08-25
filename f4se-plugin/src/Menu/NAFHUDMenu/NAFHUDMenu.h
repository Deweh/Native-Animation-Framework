#pragma once
#include "HUDManager.h"

namespace Menu
{
	class NAFHUDMenu : public RE::GameMenuBase
	{
	public:
		static RE::IMenu* Create(const RE::UIMessage&) {
			return new NAFHUDMenu();
		}

		NAFHUDMenu() {
			menuFlags.set(
				RE::UI_MENU_FLAGS::kAllowSaving,
				RE::UI_MENU_FLAGS::kRequiresUpdate,
				RE::UI_MENU_FLAGS::kDontHideCursorWhenTopmost,
				RE::UI_MENU_FLAGS::kCompanionAppAllowed
			);

			depthPriority.set(RE::UI_DEPTH_PRIORITY::kBook);

			const auto ScaleformManager = RE::BSScaleformManager::GetSingleton();
			if (!ScaleformManager->LoadMovieEx(*this, "Interface/NAFHUDMenu.swf", "root.Menu_mc", RE::Scaleform::GFx::Movie::ScaleModeType::kShowAll)) {
				RE::SendHUDMessage::ShowHUDMessage("Failed to load NAF HUD menu.", "UIMenuCancel", false, true);
				return;
			}

			filterHolder = RE::msvc::make_unique<RE::BSGFxShaderFXTarget>(*uiMovie, "root.Menu_mc");
			if (filterHolder) {
				filterHolder->CreateAndSetFiltersToHUD(RE::HUDColorTypes::kGameplayHUDColor);
				shaderFXObjects.push_back(filterHolder.get());
			}

			logger::debug("NAF HUD menu loaded.");
		}

		virtual void AdvanceMovie(float a_timeDelta, [[maybe_unused]] std::uint64_t a_time) override
		{
			HUDManager::Update(a_timeDelta);
			GameMenuBase::AdvanceMovie(a_timeDelta, a_time);
		}

		virtual void MapCodeObjectFunctions() override {
			MapCodeMethodToASFunction("CloseMenu", 0);
		}

		virtual void Call(const Params& a_params) override {
			switch ((*((std::uint32_t*)&(a_params.userData)))) {
				case 0:
				{
					CloseMenu();
					break;
				}
				default:
					break;
			}
		}

		void CloseMenu() {
			if (auto UIMessageQueue = RE::UIMessageQueue::GetSingleton(); UIMessageQueue) {
				RE::UIUtils::PlayMenuSound("UIMenuCancel");
				UIMessageQueue->AddMessage("NAFHUDMenu", RE::UI_MESSAGE_TYPE::kHide);
			}
		}

		virtual void HandleEvent(const RE::ButtonEvent* a_event) override {
			if (inputEventHandlingEnabled && a_event->strUserEvent != "NextFocus") {
				if (a_event->QJustPressed()) {
					switch (a_event->GetBSButtonCode()) {
					case RE::BS_BUTTON_CODE::kUp:
						Data::Events::Send(Data::Events::HUD_UP_KEY_DOWN);
						break;
					case RE::BS_BUTTON_CODE::kDown:
						Data::Events::Send(Data::Events::HUD_DOWN_KEY_DOWN);
						break;
					case RE::BS_BUTTON_CODE::kLeft:
						Data::Events::Send(Data::Events::HUD_LEFT_KEY_DOWN);
						break;
					case RE::BS_BUTTON_CODE::kRight:
						Data::Events::Send(Data::Events::HUD_RIGHT_KEY_DOWN);
						break;
					case RE::BS_BUTTON_CODE::kQ:
						Data::Events::Send(Data::Events::HUD_Q_KEY_DOWN);
						break;
					case RE::BS_BUTTON_CODE::kE:
						Data::Events::Send(Data::Events::HUD_E_KEY_DOWN);
						break;
					};
				} else {
					switch (a_event->GetBSButtonCode()) {
					case RE::BS_BUTTON_CODE::kUp:
						Data::Events::Send(Data::Events::HUD_UP_KEY_UP);
						break;
					case RE::BS_BUTTON_CODE::kDown:
						Data::Events::Send(Data::Events::HUD_DOWN_KEY_UP);
						break;
					case RE::BS_BUTTON_CODE::kLeft:
						Data::Events::Send(Data::Events::HUD_LEFT_KEY_UP);
						break;
					case RE::BS_BUTTON_CODE::kRight:
						Data::Events::Send(Data::Events::HUD_RIGHT_KEY_UP);
						break;
					case RE::BS_BUTTON_CODE::kQ:
						Data::Events::Send(Data::Events::HUD_Q_KEY_UP);
						break;
					case RE::BS_BUTTON_CODE::kE:
						Data::Events::Send(Data::Events::HUD_E_KEY_UP);
						break;
					};
				}
			}
		}

		virtual RE::UI_MESSAGE_RESULTS ProcessMessage(RE::UIMessage& a_message) override
		{
			switch (*a_message.type) {
			case RE::UI_MESSAGE_TYPE::kShow:
				{
					HUDManager::SetMenuInstance(this);
					return RE::UI_MESSAGE_RESULTS::kPassOn;
				}
			case RE::UI_MESSAGE_TYPE::kHide:
				{
					HUDManager::SetMenuInstance(nullptr);
					return RE::UI_MESSAGE_RESULTS::kPassOn;
				}
			default:
				return RE::IMenu::ProcessMessage(a_message);
			}
		}
	};
}
