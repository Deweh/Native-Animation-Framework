#pragma once
#include "Menu/BindableMenu.h"
#include "Misc/MathUtil.h"
#include "Scene/SceneBase.h"
#include "Scene/SceneManager.h"
#include "Misc/MenuOpenCloseEventHandler.h"

class MenuOpenCloseEventHandler :
	public RE::BSTEventSink<RE::MenuOpenCloseEvent>,
	public Singleton<MenuOpenCloseEventHandler>,
	public Data::EventListener<MenuOpenCloseEventHandler>
{
public:
	using MenuOpenCloseEventSource = RE::BSTEventSource<RE::MenuOpenCloseEvent>;

	RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent& event, MenuOpenCloseEventSource*)
	{
		//logger::trace("MenuOpenCloseEvent ({}), opening is ({})", event.menuName.c_str(), event.opening);
		//your code here on menu open/close
		if (event.menuName == "ContainerMenu" && event.opening)
		{
			// opening menu
			logger::trace("ContainerMenu opened");
			auto ui = RE::UI::GetSingleton();
			ui->UpdateMenuMode("ContainerMenu", false);
		}
		else if(event.menuName == "ContainerMenu" && !event.opening)
		{
			// closing
			logger::trace("Containermenu closed");

			
			if (auto sceneData = Menu::PersistentMenuState::SceneData::GetSingleton(); sceneData->restoreSubmenu != Menu::SUB_MENU_TYPE::kNone)
			{
				F4SE::GetTaskInterface()->AddTask([]() {
					RE::UIMessageQueue::GetSingleton()->AddMessage("NAFMenu", RE::UI_MESSAGE_TYPE::kShow);
				});
			}
		}

		return RE::BSEventNotifyControl::kContinue;
	}

	void OnGameDataReady(Data::Events::event_type, Data::Events::EventData&)
	{
		RE::UI::GetSingleton()->RegisterSink(this);
	}

protected:
	friend class Singleton;
	MenuOpenCloseEventHandler()
	{
		RegisterListener(Data::Events::GAME_DATA_READY, &MenuOpenCloseEventHandler::OnGameDataReady);
	}
};
