bool g_gameDataReady = false;
#pragma once
#include <string>
#include "Data/Constants.h"
#include "Misc/Utility.h"
#include "Data/Events.h"
#include "RE/ExtraREClasses.h"
#include "Serialization/General.h"
#include "Tasks/TaskFunctor.h"
#include "PackageOverride/EvalHook.h"
#include "LooksMenu/LooksMenu.h"
#include "FaceAnimation/AnimationData.h"
#include "Data/Global.h"
#include "Data/CommandEngine.h"
#include "BodyAnimation/GraphHook.h"
#include "BodyAnimation/SmartIdle.h"
#include "CamHook/CamHook.h"
#include "Misc/GameUtil.h"
#include "Data/Uid.h"
#include "Menu/NAFHUDMenu/SceneHUD.h"
#include "Scene/IScene.h"
#include "Scene/SceneManager.h"
#include "Scene/SceneBase.h"
#include "FaceAnimation/FaceUpdateHook.h"
#include "Tasks/GameLoopHook.h"
#include "Menu/Menu.h"
#include "Scripts/Papyrus.h"
#include "Serialization/Serialization.h"
#include "API/API.h"
//Bridge
#include "Bridge/Bridge.h"
#include "Bridge/Papyrus/Papyrus.h"
#include "Bridge/Papyrus/NAF_Utils.h"
#include <filesystem>
#include "Bridge/IniParser/Ini.h"

RE::BSScript::IVirtualMachine* g_VM;

namespace
{
	void MessageHandler(F4SE::MessagingInterface::Message* a_msg)
	{
		if (!a_msg) {
			return;
		}

		switch (a_msg->type) {
			case F4SE::MessagingInterface::kGameDataReady:
			{
				if (static_cast<bool>(a_msg->data)) {
					if (GetModuleHandleA("NAFBridge.dll")) {
						RE::MessageMenuManager::GetSingleton()->Create("NAF BRIDGE WARNING!",
							"It seems you've been updated from older Bridge's version, but didn't delete NAFBridge.dll. You should delete it before you can proceed. It is in GameFolder/Data/F4SE/Plugins/NAFBridge.dll. \nIf you're using MO2 it will be placed in MO2/Mods/Bridge-mod-folder/F4SE/Plugins/NAFBridge.dll", nullptr, RE::WARNING_TYPES::kSystem);
					}
					
					logger::info("Game data finished loading, registering NAF menu...");

					auto ui = RE::UI::GetSingleton();
					if (ui) {
						ui->RegisterSink(Tasks::TimerThread::GetSingleton());
						ui->RegisterSink(Menu::HUDManager::GetSingleton());
						Menu::Register(ui);
					}
					
					logger::info("Linking references...");
					Utility::StartPerformanceCounter();

					Data::Global::InitGameData();

					double performanceSeconds = Utility::GetPerformanceCounter();
					logger::info("Finished linking in {:.3f}s", performanceSeconds);
					logger::info("Ready!");

					g_gameDataReady = true;

					Data::Events::Send(Data::Events::GAME_DATA_READY);

					std::string file("");  
					if (std::filesystem::exists(MCM_INI_PATH))
						file = MCM_INI_PATH;

					if (!file.empty()) {
						ini::map map(file);

						if (map.get<bool>("bdebugAnimations"s, "Debug"s))
						{
							[&]() {
								for (auto ref : Data::Global::Animations) {
									auto anim = ref.second.second.get();
									std::string res("\n");

									res += "ANIMATION id : ", res += anim->id;
									res += "\n\tloadPriority : ", res += std::to_string(anim->loadPriority);
									res += "\n\tactors count : ", res += std::to_string(anim->slots.size());
									res += "\n\ttags : ";
									for (auto& tag : anim->tags) {
										res += "\n\t\t"s + tag.data();
									}

									logger::info("{}\n", res);
								}
							}();
						}

						if (map.get<bool>("bdebugPositions"s, "Debug"s)) {
							auto getPosTypeString = [](size_t type) {
								switch (type) {
								case 0u:
									return "kAnimation"s;
								case 1u:
									return "kAnimationGroup"s;
								case 2u:
									return "kPositionTree"s;
								default:
									std::string unknown("unknown type ");
									unknown += type;
									return unknown;
								}
							};

							[&]() {
								for (auto ref : Data::Global::Positions) {
									auto pos = ref.second.second.get();
									std::string res("\n");

									res += "POSITION id : ", res += pos->id;
									res += "\n\tBaseAnimation : ", res += pos->GetBaseAnimation().get()->id;
									res += "\n\tHidden : ", res += pos->hidden ? "true" : "false";
									res += "\n\tloadPriority : ", res += std::to_string(pos->loadPriority);
									res += "\n\ttype : ", res += getPosTypeString(pos->posType);
									res += "\n\tstartEquipSet : ", pos->startEquipSet;
									res += "\n\tstopEquipSet : ", res += pos->stopEquipSet;
									res += "\n\tstartMorphSet : ", res += pos->startMorphSet;
									res += "\n\toffset : ", res += Scene::offset_to_string(pos->offset);

									res += "\nTags : ";
									for (auto t : pos->tags) {
										res += "\n\t", res += t;
									}

									res += "Locations : ";
									for (auto loc : pos->locations) {
										res += "\n\t", res += loc;
									}

									logger::info("{}\n", res);
								}
							}();
						}

						if (map.get<bool>("bdebugFaceAnims"s, "Debug"s)) {
							[&]() {
								for (auto& ref : Data::Global::FaceAnims) {
									auto fanim = ref.second.second.get();

									std::string res("\n");
									res += "FACE ANIM id : ", res += fanim->id;
									res += "\n\tloadPriority : ", res += std::to_string(fanim->loadPriority);

									logger::info("{}\n", res);
								}
							}();
						}

						if (map.get<bool>("bdebugMorphSets"s, "Debug"s)) {
							[&]() {
								for (auto ref : Data::Global::MorphSets) {
									auto m = ref.second.second.get();
									std::string res("\n");

									res += "MORPH SET id : ", res += m->id;
									res += "\n\tloadPriority : ", res += std::to_string(m->loadPriority);
									res += "\n\tmorphs : ";
									for (auto& mrph : m->morphs) {
										res += "\n\t\tname : "s + mrph.second.data()->name + "\tvalue : "s + std::to_string(mrph.second.data()->value);
									}

									logger::info("{}\n", res);
								}
							}();
						}

						if (map.get<bool>("bdebugEquipmentSets"s, "Debug"s)) {
							[&]() {
								for (auto ref : Data::Global::EquipmentSets) {
									auto m = ref.second.second.get();
									std::string res("\n");

									res += "EQUIPMENT SET id : ", res += m->id;
									res += "\n\tloadPriority : ", res += std::to_string(m->loadPriority);
									/*res += "\n\tbipedSlots : ";
							for (auto& es : m->bipedSlotNames) {
								res += "\n\t\t["s + std::to_string(es.second) + "]"s);
							}
						 res += "\n\tequipmentData : ";
						 for (auto& ed : m->datas) {
							 res += "\n\t\t["s + std::to_string(ed.first.) + "]"s);
							}*/

									logger::info("{}\n", res);
								}
							}();
						}

						if (map.get<bool>("bdebugActions"s, "Debug"s)) {
							[&]() {
								for (auto ref : Data::Global::Actions) {
									auto m = ref.second.second.get();
									std::string res("\n");

									res += "ACTION id : ", res += m->id;
									res += "\n\tloadPriority : ", res += std::to_string(m->loadPriority);

									logger::info("{}\n", res);
								}
							}();
						}

						if (map.get<bool>("bdebugAnimationGroups"s, "Debug"s)) {
							[&]() {
								for (auto ref : Data::Global::AnimationGroups) {
									auto m = ref.second.second.get();
									std::string res("\n");

									res += "ANIMATION GROUP id : ", res += m->id;
								 res += "\n\tloadPriority : ", res += std::to_string(m->loadPriority);

									logger::info("{}\n", res);
								}
							}();
						}

						if (map.get<bool>("bdebugPositionTrees"s, "Debug"s)) {
							[&]() {
								for (auto ref : Data::Global::PositionTrees) {
									auto m = ref.second.second.get();
									std::string res("\n");

									res += "POSITION TREE id : ", res += m->id;
									res += "\n\tloadPriority : ", res += std::to_string(m->loadPriority);

									logger::info("{}\n", res);
								}
							}();
						}

						if (map.get<bool>("bdebugRaces"s, "Debug"s)) {
							[&]() {
								for (auto ref : Data::Global::Races) {
									auto m = ref.second.second.get();
								 std::string res("\n");

									res += "RACE id : ", res += m->id;
								 res += "\n\tloadPriority : ", res += std::to_string(m->loadPriority);
								 res += "\n\tbaseForm : ", res += m->baseForm.get()->formEditorID;
								 if (m->startEvent.has_value())
									 res += "\n\tstartEvent : ", res += m->startEvent.value();
								 if (m->stopEvent.has_value())
									 res += "\n\tstopEvent : ", res += m->stopEvent.value();
								 if (m->graph.has_value())
									 res += "\n\tgraph : ", res += m->graph.value();
								 res += "\n\trequiresReset : ", res += m->requiresReset ? "true" : "false";
								 res += "\n\trequiresForceLoop : ", res += m->requiresForceLoop ? "true" : "false";

									logger::info("{}\n", res);
								}
							}();
						}

						if (map.get<bool>("bdebugGraphInfos"s, "Debug"s)) {
							[&]() {
								for (auto ref : Data::Global::GraphInfos) {
									auto m = ref.second.second.get();
									std::string res("\n");

									res += "GRAPH INFO id : ", res += m->id;
									res += "\n\tloadPriority : ", res += std::to_string(m->loadPriority);

									logger::info("{}\n", res);
								}
							}();
						}

						if (map.get<bool>("bdebugOverlays"s, "Debug"s)) {
							[&]() {
								for (auto ref : Data::Global::Overlays) {
									auto set = ref.second.second.get();
									std::string res("\n");

									res += "OVERLAY id : ", res += set->id;
									res += "\n\tloadPriority : ", res += std::to_string(set->loadPriority);
									res += "\n\tfileName : ", res += set->fileName;
									res += "\n\tduration : ", res += std::to_string(set->duration);
									res += "\n\tquantity : ", res += std::to_string(set->quantity);

									res += "\noverlays : ";
									for (auto o : set->overlays) {
										res += "\n\ttemplate : ", res += o.Template;
										res += "\talpha : ", res += std::to_string(o.alpha);
										res += "\tisFemale : ", res += o.isFemale ? "true" : "false";
									}

									logger::info("{}\n", res);
								}
							}();
						}

						if (map.get<bool>("bdebugProtectedKeywords"s, "Debug"s)) {
							[&]() {
								std::string res("\n");
								res += "PROTECTED KEYWORD : ";
								auto formlist = RE::TESDataHandler::GetSingleton()->LookupForm<RE::BGSListForm>(0x31752, "AAF.esm"s);
								for (auto& ProtectedKwds : formlist->arrayOfForms) {
									res += "\n\tid : 0x", res += std::format("{:x}", ProtectedKwds->formID), res += "\t", res += ProtectedKwds->GetFormEditorID();
								}
								logger::info("{}\n", res);
							}();
						}

						if (map.get<bool>("bdebugFurnitures"s, "Debug"s)) {
							[&]() {
								std::string res("\n");
								auto formlist = RE::TESDataHandler::GetSingleton()->LookupForm<RE::BGSListForm>(0x2E7D, "AAF.esm"s);
								if (formlist == nullptr)
									return;
								formlist->ClearData();

								for (auto& el : Data::Global::Furnitures) {
									auto furniture = el.second.second;
									for (auto& form : furniture.get()->forms) {
										formlist->arrayOfForms.push_back(form.get());
									}
								}

								res += "PARSED FURNITURE : ";
								for (auto form : RE::TESDataHandler::GetSingleton()->LookupForm<RE::BGSListForm>(0x2E7D, "AAF.esm"s)->arrayOfForms) {
									if (form)
										res += "\n\tid : 0x", res += std::format("{:x}", form->formID), res += "\t", res += form->GetFormEditorID();
								}
								logger::info("{}\n", res);
							}();
						}
					}
				}

				if (!Data::FannyAnimationConfigManager::LoadConfig(std::string{ FANNY_ANIMATE_JSON_PATH }, std::string{ FANNY_ANIMATE_FALLBACK_JSON_PATH })) {
					logger::error("Failed to load FannyAnimation configuration!");
				} else {
					logger::info("FannyAnimation configuration loaded successfully.");
				}

				break;
			}
			case F4SE::MessagingInterface::kPostLoad:
			{
				LooksMenu::Init();
			}
			default:
				break;
		}
	}

	void InitializeHooking() {
		auto& trampoline = F4SE::GetTrampoline();
		trampoline.create(64);
		Tasks::GameLoopHook::RegisterHook(trampoline);
		BodyAnimation::GraphHook::RegisterHook();
		FaceAnimation::FaceUpdateHook::RegisterHook(trampoline);
		Menu::HUDManager::RegisterHook(trampoline);
		PackageOverride::EvalHook::RegisterHook();
		CamHook::RegisterHook();
	}
}

void ReadIni();

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info)
{
	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::PATCH;

	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= std::format("{}.log", Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
	log->set_level(spdlog::level::trace);
	log->flush_on(spdlog::level::trace);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%m/%d/%Y - %T] [%^%l%$] %v"s);

	//logger::info("{} v{}", Version::PROJECT, Version::NAME);
	logger::info("{}", PLUGINVERSTR);

	if (a_f4se->IsEditor()) {
		return false;
	}

	const auto ver = a_f4se->RuntimeVersion();
	if (ver < F4SE::RUNTIME_1_10_162) {
		//logger::critical("{} does not support runtime v{}", Version::PROJECT, ver.string());
		logger::critical("{} does not support runtime v{}", PLUGINVERSTR, ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se);

	ReadIni();

	InitializeHooking();

	const auto messaging = F4SE::GetMessagingInterface();
	if (!messaging || !messaging->RegisterListener(MessageHandler)) {
		logger::critical("Failed to get F4SE messaging interface, marking as incompatible.");
		return false;
	} else {
		logger::info("Registered with F4SE messaging interface.");
	}

	const auto serialization = F4SE::GetSerializationInterface();
	if (!serialization) {
		logger::critical("Failed to get F4SE serialization interface, marking as incompatible.");
		return false;
	} else {
		serialization->SetUniqueID('NAF');
		serialization->SetSaveCallback(Serialization::SaveCallback);
		serialization->SetLoadCallback(Serialization::LoadCallback);
		serialization->SetRevertCallback(Serialization::RevertCallback);
		logger::info("Registered with F4SE serialization interface.");
	}

	const auto papyrus = F4SE::GetPapyrusInterface();
	if (!papyrus || 
		!papyrus->Register(Papyrus::RegisterFunctions) 
		|| !papyrus->Register(Papyrus::RegisterBridgeFunctions) 
		|| !papyrus->Register(Papyrus::RegisterNAFUtilsFunctions)) 
	{
		logger::critical("Failed to register Papyrus functions!");
	} else {
		logger::info("Registered Papyrus functions.");
	}

	Data::Global::Init();

	bridge::InitializeBridge();

	//logger::info("{:s} initialization successful, waiting for game data load.", Version::PROJECT);
	logger::info("{:s} initialization successful, waiting for game data load.", PLUGINVERSTR);

	return true;
}

void ReadIni()
{
	auto AllKeys = []() {
		std::vector<std::tuple<std::string, std::string, std::string>> m;

		// [Settings]
		m.push_back(std::tuple("5"s, "iMaxFurnitureSearchTimeSeconds"s, "Settings"s));
		m.push_back(std::tuple("50.0"s, "fOccupiedFurnitureSearchRadius"s, "Settings"s));
		m.push_back(std::tuple("1"s, "bSwapFemaleActorInArray"s, "Settings"s));
		m.push_back(std::tuple("1"s, "bHideHud"s, "Settings"s));
		m.push_back(std::tuple("1"s, "bSlowDrying"s, "Settings"s));
		m.push_back(std::tuple("2"s, "iCheckFor3dLoaded"s, "Settings"s));
		m.push_back(std::tuple("0"s, "bOverrideEmptyInclTags"s, "Settings"s));
		m.push_back(std::tuple("1"s, "bAnimateFannies"s, "Settings"s));
		m.push_back(std::tuple("3"s, "bDebugLevel"s, "Settings"s));
		m.push_back(std::tuple("pose,utility"s, "sDefaultExcludeTags"s, "Settings"s));
		m.push_back(std::tuple("Belly,Belly_Mutant,Anal,Breasts,Breasts_Mutant,DP,M_Back,M_Back_Mutant,M_Chest,M_Chest_Mutant"s, "sSlowDryingOverlayIds"s, "Settings"s));
		m.push_back(std::tuple("171"s, "iCustomAAFVersion"s, "Settings"s));
		m.push_back(std::tuple("0"s, "bTryRunSceneIfPrepareFailed"s, "Settings"s));

		// [SceneDefaults]
		m.push_back(std::tuple("60.000000"s, "fSceneDuration"s, "SceneDefaults"s));
		m.push_back(std::tuple("100"s, "iFurniturePreference"s, "SceneDefaults"s));
		m.push_back(std::tuple("3000.0"s, "fFurnitureScanRadius"s, "SceneDefaults"s));
		m.push_back(std::tuple("0"s, "bIgnoreCombat"s, "SceneDefaults"s));
		m.push_back(std::tuple("0"s, "bSkipWalk"s, "SceneDefaults"s));
		m.push_back(std::tuple("0"s, "bForceNPCControll"s, "SceneDefaults"s));

		// [SceneOverrides]
		m.push_back(std::tuple("-1.000000"s, "fSceneDuration"s, "SceneOverrides"s));
		m.push_back(std::tuple("-1"s, "iFurniturePreference"s, "SceneOverrides"s));
		m.push_back(std::tuple("-1.000000"s, "fFurnitureScanRadius"s, "SceneOverrides"s));
		m.push_back(std::tuple("0"s, "iIgnoreCombat"s, "SceneOverrides"s));
		m.push_back(std::tuple("0"s, "iSkipWalk"s, "SceneOverrides"s));
		m.push_back(std::tuple("0"s, "iForceNPCControll"s, "SceneOverrides"s));

		// [Debug]
		m.push_back(std::tuple("0"s, "bdebugAnimations"s, "Debug"s));
		m.push_back(std::tuple("0"s, "bdebugPositions"s, "Debug"s));
		m.push_back(std::tuple("0"s, "bdebugFaceAnims"s, "Debug"s));
		m.push_back(std::tuple("0"s, "bdebugMorphSets"s, "Debug"s));
		m.push_back(std::tuple("0"s, "bdebugEquipmentSets"s, "Debug"s));
		m.push_back(std::tuple("0"s, "bdebugActions"s, "Debug"s));
		m.push_back(std::tuple("0"s, "bdebugAnimationGroups"s, "Debug"s));
		m.push_back(std::tuple("0"s, "bdebugPositionTrees"s, "Debug"s));
		m.push_back(std::tuple("0"s, "bdebugRaces"s, "Debug"s));
		m.push_back(std::tuple("0"s, "bdebugGraphInfos"s, "Debug"s));
		m.push_back(std::tuple("0"s, "bdebugOverlays"s, "Debug"s));
		m.push_back(std::tuple("0"s, "bdebugProtectedKeywords"s, "Debug"s));
		m.push_back(std::tuple("0"s, "bdebugFurnitures"s, "Debug"s));

		return m;
	};

	// Get all default keys
	std::vector<std::tuple<std::string, std::string, std::string>> allSettings = AllKeys();
	
	// Map to store final values: section -> (key -> value)
	std::unordered_map<std::string, std::unordered_map<std::string, std::string>> finalValues;
	
	// Initialize with defaults from AllKeys
	for (const auto& setting : allSettings) {
		const auto& defaultValue = std::get<0>(setting);
		const auto& key = std::get<1>(setting);
		const auto& section = std::get<2>(setting);
		finalValues[section][key] = defaultValue;
	}

	// Try to load settings from MCM_INI_PATH_ALT if it exists
	if (std::filesystem::exists(MCM_INI_PATH_ALT)) {
		try {
			ini::map altMap(MCM_INI_PATH_ALT);
			// Override defaults with values from ALT path
			for (const auto& setting : allSettings) {
				const auto& key = std::get<1>(setting);
				const auto& section = std::get<2>(setting);
				if (auto value = altMap.get<std::string>(key, section); value.has_value()) {
					finalValues[section][key] = value.value();
				}
			}
		} catch (...) {
			logger::warn("Failed to load settings from {}, using defaults", MCM_INI_PATH_ALT);
		}
	}

	// Try to load settings from MCM_INI_PATH if it exists
	if (std::filesystem::exists(MCM_INI_PATH)) {
		try {
			ini::map existingMap(MCM_INI_PATH);
			// Override with values from main path
			for (const auto& setting : allSettings) {
				const auto& key = std::get<1>(setting);
				const auto& section = std::get<2>(setting);
				if (auto value = existingMap.get<std::string>(key, section); value.has_value()) {
					finalValues[section][key] = value.value();
				}
			}
		} catch (...) {
			logger::warn("Failed to load settings from {}, will create new file", MCM_INI_PATH);
		}
	}

	// Create directory if it doesn't exist
	try {
		std::filesystem::path iniPath(MCM_INI_PATH);
		if (iniPath.has_parent_path()) {
			std::filesystem::create_directories(iniPath.parent_path());
		}
	} catch (std::exception& ex) {
		logger::error("Failed to create directory for {}: {}", MCM_INI_PATH, ex.what());
		return;
	}

	// Create/recreate the ini file with all settings
	try {
		std::ofstream outFile(MCM_INI_PATH, std::ios::trunc);
		if (!outFile.is_open()) {
			logger::error("Failed to open {} for writing", MCM_INI_PATH);
			return;
		}

		// Write settings grouped by section
		std::unordered_map<std::string, bool> sectionsWritten;
		
		for (const auto& setting : allSettings) {
			const auto& key = std::get<1>(setting);
			const auto& section = std::get<2>(setting);
			
			// Write section header if this is the first key in this section
			if (sectionsWritten.find(section) == sectionsWritten.end()) {
				if (!sectionsWritten.empty()) {
					outFile << "\n";  // Add blank line between sections
				}
				outFile << "[" << section << "]\n";
				sectionsWritten[section] = true;
			}
			
			// Write key=value
			outFile << key << "=" << finalValues[section][key] << "\n";
		}

		outFile.close();
		logger::info("Successfully saved settings to {}", MCM_INI_PATH);
	} catch (...) {
		logger::error("Failed to save settings to {}", MCM_INI_PATH);
	}
}
