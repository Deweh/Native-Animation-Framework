#pragma once

namespace bridge
{
	bool InitLogs();
	bool RegisterConsole();
	void MessageCallback(F4SE::MessagingInterface::Message* a_msg);
	void HudMessage(std::string a_message, std::string a_sound, bool warning);
	void InitializeBridge();
	void EveryTimeLoadGame();
	void AfterGameDataLoaded();
}
