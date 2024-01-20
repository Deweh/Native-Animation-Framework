#include "MenuHandlers/MainMenuHandler.h"
#include "MenuHandlers/NewSceneHandler.h"
#include "MenuHandlers/ManageScenesHandler.h"
#include "MenuHandlers/CreatorMenuHandler.h"
#include "MenuHandlers/FaceAnimCreatorHandler.h"
#include "MenuHandlers/SettingsMenuHandler.h"
#include "MenuHandlers/InventoryMenuHandler.h"
#include "MenuHandlers/BodyCreatorHandler.h"
#include "MenuHandlers/ItemExplorerHandler.h"
#pragma once

namespace Menu
{
	class NAFMenuStateManager : public IStateManager
	{
	public:
		NAFMenuStateManager(RE::GameMenuBase* menu) :
			IStateManager(menu) {}

		virtual ~NAFMenuStateManager() { activeInstance = nullptr; }

		void InitMenu()
		{
			menuItems.clear();
			auto state = PersistentMenuState::GetSingleton(); 
			if (NAFStudioMenu::IsActive()) {
				GotoMenu(SUB_MENU_TYPE::kBodyAnimCreator, true);
			} else if (state->restoreSubmenu != kNone) {
				SUB_MENU_TYPE navTarget = state->restoreSubmenu;
				GotoMenu(navTarget, true);
			} else {
				GotoMenu(SUB_MENU_TYPE::kMain, true);
			}
			state->restoreSubmenu = kNone;
			activeInstance = this;
		}

		virtual void GotoMenu(SUB_MENU_TYPE menu, bool resetScroll, bool resetHighlight = true) override
		{
			if (!menuInstance) {
				return;
			}

			SetPanelShown(false);

			if (resetHighlight)
				highlightedObjects.RemoveAll();

			auto iter = menuTypes.find(menu);
			if (iter != menuTypes.end()) {
				currentMenu = iter->second(this);
			} else {
				return;
			}

			currentMenu->InitSubmenu();
			
			if (!currentMenu->complexItemList) {
				ConstructItems(currentMenu->GetItemList());
			} else {
				menuItems = currentMenu->GetComplexItemList();
			}

			PushItemsToMenu(resetScroll);
		}

		virtual void CloseMenu() override {
			if (auto UIMessageQueue = RE::UIMessageQueue::GetSingleton(); UIMessageQueue) {
				UIMessageQueue->AddMessage("NAFMenu", RE::UI_MESSAGE_TYPE::kHide);
			}
		}

		virtual void RefreshList(bool resetScroll) override {
			if (!menuInstance || !currentMenu) {
				return;
			}

			if (!currentMenu->complexItemList) {
				ConstructItems(currentMenu->GetItemList());
			} else {
				menuItems = currentMenu->GetComplexItemList();
			}
			
			PushItemsToMenu(resetScroll);
		}

		void ItemSelected(int index)
		{
			if (menuItems.size() <= index || !currentMenu) {
				return;
			}

			currentMenu->ItemSelected(index);
		}

		void Back()
		{
			if (!menuInstance) {
				return;
			}

			RE::UIUtils::PlayMenuSound("UIMenuCancel");

			if (!currentMenu) {
				CloseMenu();
			}

			currentMenu->Back();
		}

		void ItemHoverChanged(int index, bool hovered) {
			if (menuItems.size() <= index || !currentMenu) {
				return;
			}

			currentMenu->ItemHoverChanged(index, hovered);
		}

		virtual bool SetItem(int index, std::string label, bool bold) override
		{
			if (!menuInstance || menuItems.size() <= index) {
				return false;
			}
			menuItems[index].label = label;
			menuItems[index].bold = bold;

			RE::Scaleform::GFx::Value args[3];
			args[0] = index;
			args[1] = label.c_str();
			args[2] = bold;

			menuInstance->menuObj.Invoke("SetSingleDataItem", nullptr, args, 3);
			return true;
		}

		void ButtonClicked(int index) {
			if (!currentMenu) {
				return;
			}

			currentMenu->ButtonClicked(index);
		}

		virtual void SetPanelShown(bool shown, bool leftAlign = false) override {
			RE::Scaleform::GFx::Value args[2];
			args[0] = shown;
			args[1] = leftAlign;
			menuInstance->menuObj.Invoke("SetPanelShown", nullptr, args, 2);
		}

		virtual void SetPanelElements(std::vector<PanelElementData> eles) override {
			if (!menuInstance) {
				return;
			}

			auto movie = menuInstance->uiMovie.get();

			RE::Scaleform::GFx::Value args[1];
			movie->CreateArray(&args[0]);

			for (size_t i = 0; i < eles.size(); i++) {
				RE::Scaleform::GFx::Value entry;
				movie->CreateObject(&entry);
				entry.SetMember("label", eles[i].label.c_str());
				entry.SetMember("itemType", eles[i].itemType);

				args[0].PushBack(entry);
			}

			menuInstance->menuObj.Invoke("SetPanelElements", nullptr, args, 1);
		}

		virtual void ShowNotification(std::string text, float time = 5.0f) override {
			RE::Scaleform::GFx::Value args[2];
			args[0] = text.c_str();
			args[1] = time;
			menuInstance->menuObj.Invoke("ShowNotification", nullptr, args, 2);
		}

		virtual void SetMenuTitle(std::string text) override {
			RE::Scaleform::GFx::Value args[1];
			args[0] = text.c_str();
			menuInstance->menuObj.Invoke("SetMenuTitle", nullptr, args, 1);
		}

		virtual void ShowTextEntry() override
		{
			menuInstance->menuObj.Invoke("ShowTextEntry", nullptr, nullptr, 0);
		}

		virtual void TextEntryCompleted(bool successful, std::string text) override {
			if (!currentMenu) {
				return;
			}

			currentMenu->TextEntryCompleted(successful, text);
		}

		virtual void TickerSelected(int index, int type) override {
			if (!currentMenu || type < 0 || type > 2) {
				return;
			}

			currentMenu->TickerSelected(index, static_cast<TickerEventType>(type));
		}

		virtual void SetTickerText(std::string text, int index) override {
			RE::Scaleform::GFx::Value args[2];
			args[0] = text.c_str();
			args[1] = index;
			menuInstance->menuObj.Invoke("SetTickerText", nullptr, args, 2);
		}

		virtual void DynamicTickerSelected(int index, int type) {
			if (!currentMenu || type < 0 || type > 2) {
				return;
			}

			currentMenu->DynamicTickerSelected(index, static_cast<TickerEventType>(type));
		}

		virtual void SetElementText(std::string text, int index) {
			RE::Scaleform::GFx::Value args[2];
			args[0] = text.c_str();
			args[1] = index;
			menuInstance->menuObj.Invoke("SetElementText", nullptr, args, 2);
		}

		void ItemRightClicked(int index) {
			if (!currentMenu) {
				return;
			}

			currentMenu->ItemRightClicked(index);
		}

	private:
		void ConstructItems(std::vector<std::string> items)
		{
			menuItems.clear();
			menuItems.reserve(items.size());

			MenuItemData data;
			for (size_t i = 0; i < items.size(); i++) {
				data.bold = false;
				data.ticker = false;
				data.label = items[i];
				menuItems.push_back(data);
			}
		}

		bool PushItemsToMenu(bool resetScroll)
		{
			if (!menuInstance) {
				return false;
			}

			auto movie = menuInstance->uiMovie.get();

			RE::Scaleform::GFx::Value args[2];
			movie->CreateArray(&args[0]);

			for (size_t i = 0; i < menuItems.size(); i++) {
				RE::Scaleform::GFx::Value entry;
				movie->CreateObject(&entry);
				entry.SetMember("label", menuItems[i].label.c_str());
				entry.SetMember("bold", menuItems[i].bold);
				entry.SetMember("ticker", menuItems[i].ticker);
				entry.SetMember("value", menuItems[i].value.c_str());

				args[0].PushBack(entry);
			}

			args[1] = resetScroll;
			menuInstance->menuObj.Invoke("SetDataItems", nullptr, args, 2);
			return true;
		}
	};
}
