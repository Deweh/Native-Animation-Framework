#pragma once

namespace Menu
{
	class IStateManager;

	class IMenuHandler
	{
	public:
		IStateManager* manager;
		bool complexItemList = false;
		bool registered = false;

		IMenuHandler(IStateManager* _stateInstance) {
			manager = _stateInstance;
		}

		virtual ~IMenuHandler() {}

		virtual void InitSubmenu() {}
		virtual std::vector<std::string> GetItemList() { return std::vector<std::string>(); }
		virtual std::vector<MenuItemData> GetComplexItemList() { return std::vector<MenuItemData>(); }
		virtual void ItemSelected(int){};
		virtual void ItemRightClicked(int){};
		virtual void ItemHoverChanged(int, bool){};
		virtual void ButtonClicked(int){};
		virtual void TextEntryCompleted(bool, std::string){};
		virtual void TickerSelected(int, TickerEventType){};
		virtual void DynamicTickerSelected(int, TickerEventType){};
		virtual void Back() = 0;

	protected:

	};
}
