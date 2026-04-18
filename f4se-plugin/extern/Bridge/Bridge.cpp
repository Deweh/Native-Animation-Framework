#pragma once
#include "PCH.h"
#include "F4SE/F4SE.h"
#include "F4SE/Logger.h"
//#include "RE/Fallout.h"

#include "Consts.h"

#include "Data/Constants.h"
#include "Bridge/Bridge.h"
#include "Bridge/IniParser/Ini.h"

extern int PRINT_LOG;

namespace bridge
{
	inline bool InitLogs()
	{
		namespace logger = F4SE::log;
		using namespace std::string_literals;
		return true;
	}


	//void ReadDebugSetting()
	//{
	//	std::filesystem::path ini(MCM_INI_PATH);
	//	std::filesystem::path alt(MCM_INI_PATH_ALT);
	//	ini.make_preferred();
	//	alt.make_preferred();
	//}

	void MessageCallback(F4SE::MessagingInterface::Message* a_msg)
	{
		if (!a_msg) {
			return;
		}
		switch (a_msg->type) {
		case F4SE::MessagingInterface::kGameDataReady:
			{
				if (static_cast<bool>(a_msg->data)) {
					logger::info("Game data finished loading");
					AfterGameDataLoaded();
				}

				break;
			}
		case F4SE::MessagingInterface::kPostLoad:
			{
				//nothing
			}
		default:
			break;
		}
	}

	bool RegisterConsole()
	{
		if (F4SE::GetMessagingInterface()->RegisterListener(MessageCallback)) {
			logger::info("Registered with F4SE messaging interface.");
			return true;
		}
		logger::critical("Failed to get F4SE messaging interface, marking as incompatible.");
		return false;
	}

	void HudMessage(std::string a_message, std::string a_sound, bool warning)
	{
		RE::SendHUDMessage::ShowHUDMessage(a_message.c_str(), a_sound.c_str(), false, warning);
	}

	void InitializeBridge()
	{
		RegisterConsole();
		EveryTimeLoadGame();
	}

	void EveryTimeLoadGame()
	{
		/*std::thread([&] {
		}).detach();*/
	}

	void AfterGameDataLoaded()
	{

	}
}
