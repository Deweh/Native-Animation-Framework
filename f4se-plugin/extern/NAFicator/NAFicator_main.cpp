#include "F4SE/F4SE.h"
#include "RE/Fallout.h"
#include <IniParser/Ini.h>
#include <filesystem>
#include <optional>
//#include <format>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <sstream>
#include <string>
#include <NAFicator/NAFicate.h>
//#include <PCH.h>
#include <NAFicator/Version.h>

#define DLLEXPORT __declspec(dllexport)

namespace logger = F4SE::log;
using namespace std::literals;

ini::map inimap(std::filesystem::current_path().string() + "\\Data\\Naficator.ini"s);
std::string cachedMessage;    // Кэш для сообщения
bool isErrorMessage = false;  // Флаг для определения типа сообщения
std::atomic<bool> voodoo_ready{ false };

std::filesystem::path Where = "";

extern void AfterGameDataReady();

void outputCachedMessage()
{
	if (!cachedMessage.empty()) {
		RE::MessageMenuManager::GetSingleton()->Create(
			isErrorMessage ? "NAFicator: something goes wrong" : "NAFicator end",
			cachedMessage.c_str(), nullptr, RE::WARNING_TYPES::kSystem);
		cachedMessage.clear();  // Очищаем кэш после вывода
	}
}

void start_up()
{
	std::string noexist = "Folder doesn't exist: ";
	std::string warn = "You should close the game and disable NAFICATOR before you can proceed. You can disable it using ..Fallout 4/Data/Naficator.ini option iNaficator enable to 0, or removing Fallout 4/Data/F4SE/Plugins/Naficator.dll."s;

	try {
		inimap.get<bool>("bNaficatorEnable"s, "General"s);
		inimap.get<std::string>("sFolderFrom"s, "General"s);
		inimap.get<std::string>("sFolderWhere"s, "General"s);
		inimap.get<int>("iRemoveFixWrongZOffsets"s, "General"s);
		inimap.get<float>("fzOffsetMax"s, "General"s);
		inimap.get<int>("iNaficatorFilesPriority"s, "General"s);
		inimap.get<std::string>("sSkipEmptyAttributesInLog"s, "General"s);
		inimap.get<bool>("bPrintDebugXMLs"s, "General"s);
	} catch (...) {
		logger::critical("💥 Bad ini settings, cancel NAFicator start...");
		cachedMessage = "Bad ini settings, cancel NAFicator start...";
		isErrorMessage = true;  // Устанавливаем флаг ошибки
		return;                 // Выход из функции, если произошла ошибка
	}

	if (inimap.get<bool>("bNaficatorEnable"s, "General"s)) {
		isErrorMessage = false;  // Устанавливаем флаг успеха

		std::string FROM = inimap.get<std::string>("sFolderFrom"s, "General"s);
		if (FROM[0] == '/' || FROM[0] == '\\') {
			FROM = std::filesystem::current_path().string() + FROM;
		}
		if (!std::filesystem::exists(FROM)) {
			noexist += FROM;
			cachedMessage = noexist;  // Кэшируем сообщение об ошибке
			isErrorMessage = true;    // Устанавливаем флаг ошибки
			return;                   // Выход из функции, если папка не существует
		}

		auto WHERE = inimap.get<std::string>("sFolderWhere"s, "General"s);
		if (WHERE[0] == '/' || WHERE[0] == '\\') {
			WHERE = std::filesystem::current_path().string() + WHERE;
		}
		if (!std::filesystem::exists(WHERE)) {
			noexist += WHERE;
			cachedMessage = noexist;  // Кэшируем сообщение об ошибке
			isErrorMessage = true;    // Устанавливаем флаг ошибки
			return;                   // Выход из функции, если папка не существует
		}

		logger::info("🤪 NAFicator starts\nFROM: {}\nTO: {}", FROM, WHERE);
		std::filesystem::path From(FROM);
		From.make_preferred();
		Where = WHERE;
		Where.make_preferred();

		START(From);
		logger::info("🤪 NAFicator finished xmls processing! Wait for game data loaded...");
	} else {
		logger::info("🛑 NAFicator is disabled in ini, skip start.");
		cachedMessage = "";
		isErrorMessage = false;  // Устанавливаем флаг успеха
		return;
	}
	
	cachedMessage = warn;    // Кэшируем сообщение об успешном завершении
	isErrorMessage = false;  // Устанавливаем флаг успеха
}

void MessageHandler(F4SE::MessagingInterface::Message* a_msg)
{
	if (!a_msg) {
		return;
	}

	switch (a_msg->type) {
	case F4SE::MessagingInterface::kGameDataReady:
		if (voodoo_ready.exchange(true) == false) {
			return;
		}
		AfterGameDataReady();
		logger::info("🧐 NAFicator finished!");
		break;
	default:
		break;
	}
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info)
{
	//auto path = logger::log_directory();
	std::optional<std::filesystem::path> path("Data/NAFicator/"s);
	path->make_preferred();
	if (!path) {
		return false;
	}

	*path /= std::format("{}.log", ver::PROJECT);
	path->make_preferred();
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%d/%m/%Y - %T] [%^%l%$] %v"s);

	logger::info("{}", std::string(ver::NAME.begin(), ver::NAME.end()));
	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = std::string(ver::NAME.begin(), ver::NAME.end()).c_str();
	a_info->version = ver::computeVersionInt(ver::MAJOR, ver::MINOR, ver::PATCH);

	if (a_f4se->IsEditor()) {
		logger::critical("💥 Loaded in editor! Abort.");
		return false;
	}

	const auto ver = a_f4se->RuntimeVersion();
	if (ver != F4SE::RUNTIME_1_10_162 && ver != F4SE::RUNTIME_1_10_163) {
		logger::critical("💥 unsupported runtime v{} ! Abort.", ver.string());
		return false;
	}

	logger::info("🤖 READY");

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se);

	const auto messaging = F4SE::GetMessagingInterface();
	if (!messaging || !messaging->RegisterListener(MessageHandler)) {
		logger::critical("💥 Failed to get F4SE messaging interface, marking as incompatible. Abort.");
		return false;
	} else {
		logger::info("✅ Registered with F4SE messaging interface.");
		logger::info("🤪 Starting...");
		start_up();
	}

	return true;
}
