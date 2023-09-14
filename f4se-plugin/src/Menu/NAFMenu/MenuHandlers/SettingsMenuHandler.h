#pragma once
#include "Menu/BindableMenu.h"

#define BIND_SETTING_BOOL(name, var) \
	{ std::format("{}: {}", name, var ? "ON" : "OFF"), MENU_BINDING_WARG(SettingsMenuHandler::SetBool, &var) }

#define BIND_SETTING_STRING(name, var, ...) \
	{ std::format("{}: {}", name, var), MENU_BINDING_WARG(SettingsMenuHandler::SetString, StringSettingInfo(&var, __VA_ARGS__)) }

#define BIND_SETTING_UINT(name, var, minVal, maxVal) \
	{ std::format("{}:", name), MENU_BINDING_WARG(SettingsMenuHandler::SetUInt, &var), true, minVal, maxVal, static_cast<int>(var) }

namespace Menu::NAF
{
	class SettingsMenuHandler : public BindableMenu<SettingsMenuHandler, SUB_MENU_TYPE::kSettings>
	{
	public:
		using BindableMenu::BindableMenu;

		struct StringSettingInfo
		{
			template <class... Args>
			StringSettingInfo(std::string* _s, Args... a)
			{
				s = _s;
				possibleValues = { a... };
			}

			std::string* s;
			std::vector<std::string> possibleValues;
		};

		virtual ~SettingsMenuHandler() {}

		Data::Settings::UnsafeSettingValues tempValues;
		bool isDirty = false;

		virtual void InitSubmenu() override {
			RegisterUIListener(Data::Events::SETTINGS_CHANGED);

			manager->SetMenuTitle("Settings");
			BindableMenu::InitSubmenu();
			ConfigurePanel({
				{ "Reload Config Data", Button, MENU_BINDING(SettingsMenuHandler::ReloadXMLs) },
				{ "Reset to Default", Button, MENU_BINDING(SettingsMenuHandler::Reset) },
				{ "Apply Settings", Button, MENU_BINDING(SettingsMenuHandler::ApplySettings) },
			});

			tempValues = Data::Settings::Values;
		}

		virtual void OnUIEvent(Data::Events::event_type, const Data::Events::EventData&)
		{
			manager->RefreshList(false);
		}

		virtual BindingsVector GetBindings() override
		{
			return {
				BIND_SETTING_BOOL("Use LookAt Cam", tempValues.bUseLookAtCam),
				BIND_SETTING_STRING("LookAt Cam Target", tempValues.sLookAtCamTarget, "HEAD", "Chest", "COM"),
				BIND_SETTING_UINT("Default Scene Duration", tempValues.iDefaultSceneDuration, 1, 9999999),
				BIND_SETTING_BOOL("Disable Actor Rescaling", tempValues.bDisableRescaler)
			};
		}

		void SetBool(bool* val, int) {
			(*val) = !(*val);
			manager->RefreshList(false);
		}

		void SetString(const StringSettingInfo& s, int) {
			std::string res = s.possibleValues[0];

			for (size_t i = 0; i < s.possibleValues.size(); i++) {
				if ((*s.s) == s.possibleValues[i] && (i + 1) < s.possibleValues.size()) {
					res = s.possibleValues[i + 1];
					break;
				}
			}

			(*s.s) = res;
			manager->RefreshList(false);
		}

		void SetUInt(uint32_t* var, int val) {
			(*var) = val;
		}

		void Reset(int) {
			tempValues.ToDefault();
			manager->RefreshList(false);
		}

		void ApplySettings(int) {
			Data::Settings::PushValues(tempValues);
			if (Data::Settings::Save()) {
				manager->ShowNotification("Settings saved.", 1.5f);
			} else {
				manager->ShowNotification("Warning: Failed to save settings. Changes will only take effect for this session.");
			}
		}

		void ReloadXMLs(int) {
			QueueHotReload("Reloading...", "Data reload complete.");
		}

		virtual void Back() override
		{
			manager->GotoMenu(kMain, true);
		}
	};
}
