#include "hooks.h"
#include "DiverseBodiesRedux/Manager/ManagerActorPreset.h"

extern bool IsSerializeFinished();
extern bool IsInActorsMap(uint32_t actorId);
extern bool IsInActorsMap(RE::Actor* actor);
extern dbr_manager::ActorPreset* Find(RE::Actor*);
extern std::optional<bool> IsActorExcluded(RE::Actor* actor);

extern concurrency::concurrent_queue<std::pair<uint32_t, void*>> looksmenu_hooked_queue;

//ProcessingNPC g_processingReset{};
//ProcessingNPC g_processingChangeHeadParts{};

TESObjectLoadedEventHandler g_OriginalReceiveEventObjectLoaded = nullptr;
TESInitScriptEventHandler g_OriginalReceiveEventInitScript = nullptr;
//Reset3DHandler g_OriginalReset3D = nullptr;
ChangeHeadPartRemovePartHandler g_OriginalChangeHeadPartRemovePart = nullptr;
ChangeHeadPartHandler g_OriginalChangeHeadPart = nullptr;
//Update3DModelHandler g_OriginalUpdate3DModel = nullptr;
DoUpdate3DModelHandler g_OriginalDoUpdate3DModel = nullptr;
//Вылеты BSClothInstance
SetTransformSetHandler g_OriginalSetTransformSet = nullptr;
BSClothExtraData_SetSettle_t g_OriginalBSClothExtraData_SetSettle = nullptr;
BSTransformSetHandler g_OriginalBSTransformSet = nullptr;
//hclClothInstance_GetActiveSimClothIndices_t g_OriginalGetActiveSimClothIndices = nullptr;
//BSClothExtraData_TeleportToMatchBoneTransform_t g_OriginalBSClothExtraData_TeleportToMatchBoneTransform = nullptr;
BSClothUtils_BSTransformSet_QClothSupportsLOD_t g_OriginalBSClothUtils_BSTransformSet_QClothSupportsLOD = nullptr;

DetourXS detourObjectLoaded;
DetourXS detourInitScript;
//DetourXS detourReset3D;
DetourXS detourChangeHeadPartRemovePart;
DetourXS detourChangeHeadPart;
//DetourXS detourUpdate3DModel;
DetourXS detourDoUpdate3DModel;
//Вылеты BSClothInstance
DetourXS detourSetTransformSet;
DetourXS detourBSClothExtraData;
DetourXS detourBSTransformSet;
DetourXS detourGetActiveSimClothIndices;
DetourXS detourBSClothTeleport;
DetourXS detourBSClothUtils_QClothSupportsLOD;


RE::BSEventNotifyControl HookedReceiveEventObjectLoaded(ActorUpdateManager* manager, RE::TESObjectLoadedEvent* evn, void* dispatcher)
{
	return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl HookedReceiveEventInitScript(ActorUpdateManager* manager, RE::TESInitScriptEvent* evn, void* dispatcher)
{
	return RE::BSEventNotifyControl::kContinue;  // Отключаем бодиген
}

std::mutex mapMutex;
std::condition_variable mapCV;

//void HookedReset3D(RE::Actor* actor, bool a_reloadAll, RE::RESET_3D_FLAGS a_additionalFlags, bool a_queueReset, RE::RESET_3D_FLAGS a_excludeFlags)
//{
//	if (!actor) {
//		logger::warn("HookedReset3D: Actor is null.");
//		return;
//	}
//
//	auto check_flags = [&]()->bool {
//		return (static_cast<uint16_t>(a_additionalFlags & f3D::kDiverseBodiesFlag) || static_cast<uint16_t>(a_additionalFlags) || (actor->currentProcess && 
//			(static_cast<uint16_t>(actor->currentProcess->GetAll3DUpdateFlags()) & ~static_cast<uint16_t>(a_excludeFlags))));
//	};
//
//	auto npc = get_leveled_TESNPC(actor->GetNPC());
//	if (!npc) {
//		logger::info("HookedReset3D: NPC is null for actor {:x}.", actor->formID);
//	}
//
//	if (extended_log) {
//		logger::info("Reset3D requested for actor {:x}, npc template {}, AIProcess state {}, args(reloadAll {}, addFlags {}, queueReset {}, excludeFlags {})",
//			actor->formID,
//			npc ? std::format("{:x}", npc->formID) : "NULL",
//			actor->currentProcess ? std::to_string(actor->currentProcess->GetAll3DUpdateFlags()) : "no process",
//			a_reloadAll ? "true" : "false",
//			static_cast<uint16_t>(a_additionalFlags),
//			a_queueReset ? "true" : "false",
//			static_cast<uint16_t>(a_excludeFlags));
//	}
//
//	/*if (npc && g_processingReset.contains(npc->formID)) {
//		std::thread([=] {
//			while (g_processingReset.contains(npc->formID))
//				std::this_thread::sleep_for(std::chrono::milliseconds(100));
//			HookedReset3D(actor, a_reloadAll, a_additionalFlags, a_queueReset, a_excludeFlags);
//		}).detach();
//		return;
//	}*/
//
//	if (check_flags())
//		if (auto preset = Find(actor); preset && !preset->empty()) {
//			if (!npc) {
//				logger::info("HookedReset3D: NPC is null for actor {:x}.", actor->formID);
//			} else if (npc) {
//				//g_processingReset.insert(npc->formID);
//				a_additionalFlags |= f3D::kDiverseBodiesFlag;
//
//				preset->apply(true, a_reloadAll, a_additionalFlags, a_queueReset, a_excludeFlags);
//				return;
//			}	
//		}
//
//	if (extended_log)
//		logger::info("{} : Reset 3d ({}, {}, {}, {}", std::format("{:x}", actor->formID),
//			a_reloadAll ? "true" : "false",
//			std::to_string(static_cast<uint16_t>(a_additionalFlags)),
//			a_queueReset ? "true" : "false",
//			std::to_string(static_cast<uint16_t>(a_excludeFlags)));
//
//	g_OriginalReset3D(actor, a_reloadAll, a_additionalFlags, a_queueReset, a_excludeFlags);
//}

//void HookedUpdate3DModel(RE::AIProcess* process, RE::Actor* actor, RE::TESObjectREFR* ref, bool force)
//{
//	logger::info("HookedUpdate3DModel {:x} {:x} {}", actor ? actor->formID : 0x0, ref ? ref->formID : 0x0, force ? "true" : "false");
//	g_OriginalUpdate3DModel(process, actor, ref, force);
//}

void HookedSetTransformSet(void* instance, uint32_t index, void* transformSet)
{
	if (!transformSet) {  // Проверка на nullptr
		logger::warn("HookedSetTransformSet: transformSet is null. Skipping...");
		return;
	}
	// Вызов оригинальной функции, если проверка пройдена
	g_OriginalSetTransformSet(instance, index, transformSet);
}

//void __fastcall Hooked_BSClothExtraData_SetSettle(void* thisPtr, bool enable)
//{
//	__try {
//		uint8_t* rcx_ptr = static_cast<uint8_t*>(thisPtr);
//		uint32_t edx_val = *reinterpret_cast<uint32_t*>(rcx_ptr + 0x70);
//		uint8_t** rax_ptr = reinterpret_cast<uint8_t**>(rcx_ptr + 0x60);
//		uint8_t** r8_ptr = rax_ptr + edx_val;
//
//		while (rax_ptr < r8_ptr) {
//			uint8_t* current = *rax_ptr;
//			if (*reinterpret_cast<uint32_t*>(current + 0x48) > 0) {
//				uint8_t* inner_rcx = *reinterpret_cast<uint8_t**>(current + 0x40);
//				if (inner_rcx) {
//					uint8_t* rdx_val = *reinterpret_cast<uint8_t**>(inner_rcx);
//					if (rdx_val) {
//						*reinterpret_cast<bool*>(rdx_val + 0x1B7) = enable;
//					}
//				}
//			}
//			rax_ptr++;
//		}
//	} __except (EXCEPTION_EXECUTE_HANDLER) {
//		// Логирование ошибки
//		logger::error("Memory access violation in BSClothExtraData hook");
//	}
//}

void __fastcall Hooked_BSClothExtraData_SetSettle(void* thisPtr, bool enable)
{
	__try {
		// Проверка thisPtr
		if (!thisPtr || IsBadReadPtr(thisPtr, sizeof(void*))) {
			logger::warn("Hooked_BSClothExtraData_SetSettle: Invalid thisPtr (0x{:x})", reinterpret_cast<uintptr_t>(thisPtr));
			return;
		}

		uint8_t* rcx_ptr = static_cast<uint8_t*>(thisPtr);

		// Проверка смещений внутри структуры
		if (IsBadReadPtr(rcx_ptr + 0x60, sizeof(uint8_t**)) || IsBadReadPtr(rcx_ptr + 0x70, sizeof(uint32_t))) {
			logger::warn("Hooked_BSClothExtraData_SetSettle: Corrupted structure at 0x{:x}", reinterpret_cast<uintptr_t>(thisPtr));
			return;
		}

		uint32_t edx_val = *reinterpret_cast<uint32_t*>(rcx_ptr + 0x70);
		uint8_t** rax_ptr = reinterpret_cast<uint8_t**>(rcx_ptr + 0x60);
		uint8_t** r8_ptr = rax_ptr + edx_val;

		// Обработка каждого элемента в массиве
		while (rax_ptr < r8_ptr) {
			// Проверка указателя rax_ptr
			if (IsBadReadPtr(rax_ptr, sizeof(uint8_t*))) {
				logger::warn("Hooked_BSClothExtraData_SetSettle: Invalid array pointer");
				break;
			}

			uint8_t* current = *rax_ptr;
			// Проверка current
			if (!current || IsBadReadPtr(current, sizeof(uint8_t))) {
				logger::warn("Hooked_BSClothExtraData_SetSettle: Skipping invalid cloth instance");
				rax_ptr++;
				continue;
			}

			// Проверка current + 0x48
			if (IsBadReadPtr(current + 0x48, sizeof(uint32_t))) {
				logger::warn("Hooked_BSClothExtraData_SetSettle: Invalid count field");
				rax_ptr++;
				continue;
			}

			if (*reinterpret_cast<uint32_t*>(current + 0x48) > 0) {
				// Проверка current + 0x40
				if (IsBadReadPtr(current + 0x40, sizeof(uint8_t*))) {
					logger::warn("Hooked_BSClothExtraData_SetSettle: Invalid transform array");
					rax_ptr++;
					continue;
				}

				uint8_t* inner_rcx = *reinterpret_cast<uint8_t**>(current + 0x40);
				// Проверка inner_rcx
				if (!inner_rcx || IsBadReadPtr(inner_rcx, sizeof(uint8_t*))) {
					logger::warn("Hooked_BSClothExtraData_SetSettle: Invalid transform data");
					rax_ptr++;
					continue;
				}

				uint8_t* rdx_val = *reinterpret_cast<uint8_t**>(inner_rcx);
				// Проверка rdx_val и целевого адреса
				if (!rdx_val || IsBadReadPtr(rdx_val + 0x1B7, sizeof(bool))) {
					logger::warn("Hooked_BSClothExtraData_SetSettle: Invalid settle flag address");
					rax_ptr++;
					continue;
				}

				*reinterpret_cast<bool*>(rdx_val + 0x1B7) = enable;
			}
			rax_ptr++;
		}
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		logger::error("Hooked_BSClothExtraData_SetSettle: Critical error at 0x{:x}", reinterpret_cast<uintptr_t>(_ReturnAddress()));
	}
}

void __fastcall Hooked_BSTransformSet(void* thisPtr)
{
	if (!thisPtr || IsBadReadPtr(thisPtr, sizeof(void*))) {
		logger::warn("Hooked_BSTransformSet: Invalid thisPtr (0x{:x}). Skipping...", reinterpret_cast<uintptr_t>(thisPtr));
		return;
	}

	/*__try {
		g_OriginalBSTransformSet(thisPtr);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		logger::error(
			"Possible memory access violation in BSTransformSet hook. "
			"thisPtr: 0x{:x}, Return Address: 0x{:x}",
			reinterpret_cast<uintptr_t>(thisPtr),
			reinterpret_cast<uintptr_t>(_ReturnAddress()));
	}*/
	return;
}

//void __fastcall Hooked_hclClothInstance_GetActiveSimClothIndices(void* instance, uint32_t index, void* outArray)
//{
//	if (!instance || !outArray) {
//		logger::error("Invalid arguments in hclClothInstance::getActiveSimClothIndices.");
//		return;
//	}
//
//	__try {
//		// Проверка, что индекс не превышает разумные пределы (например, 0xFFFF)
//		if (index > 0xFFFF) {
//			logger::error("Suspicious index value: {}", index);
//			return;
//		}
//
//		// Вызов оригинальной функции
//		g_OriginalGetActiveSimClothIndices(instance, index, outArray);
//	} __except (EXCEPTION_EXECUTE_HANDLER) {
//		logger::error("Access violation in hclClothInstance::getActiveSimClothIndices (Address: 0x{:x}).",
//			reinterpret_cast<uintptr_t>(_ReturnAddress()));
//	}
//}
//
//void __fastcall Hooked_BSClothExtraData_TeleportToMatchBoneTransform(void* thisPtr, void* a2, void* a3)
//{
//	__try {
//		// Проверка всех аргументов на nullptr
//		if (!thisPtr || !a2 || !a3) {
//			logger::warn("Invalid arguments in BSClothExtraData::TeleportToMatchBoneTransform (thisPtr: 0x{:x}, a2: 0x{:x}, a3: 0x{:x})",
//				reinterpret_cast<uintptr_t>(thisPtr),
//				reinterpret_cast<uintptr_t>(a2),
//				reinterpret_cast<uintptr_t>(a3));
//			return;
//		}
//
//		// Проверка структуры BSClothExtraData
//		auto clothData = reinterpret_cast<uint8_t*>(thisPtr);
//		if (IsBadReadPtr(clothData + 0x60, sizeof(void*))) {
//			logger::warn("Corrupted BSClothExtraData structure at 0x{:x}", reinterpret_cast<uintptr_t>(thisPtr));
//			return;
//        }
//
//        // Дополнительная проверка валидности
//		if (IsBadReadPtr(thisPtr, sizeof(void*))) {
//			logger::warn("Invalid thisPtr pointer in BSClothExtraData::TeleportToMatchBoneTransform");
//			return;
//		}
//
//        if (IsBadReadPtr(a2, sizeof(void*))) {
//			logger::warn("Invalid a2 pointer in BSClothExtraData::TeleportToMatchBoneTransform");
//			return;
//        }
//
//		if (IsBadReadPtr(a3, sizeof(void*))) {
//			logger::warn("Invalid a3 pointer in BSClothExtraData::TeleportToMatchBoneTransform");
//			return;
//		}
//
//        // Вызов оригинальной функции
//        g_OriginalBSClothExtraData_TeleportToMatchBoneTransform(thisPtr, a2, a3);
//
//	} __except (EXCEPTION_EXECUTE_HANDLER) {
//		logger::error("Critical error in BSClothExtraData::TeleportToMatchBoneTransform (RIP: 0x{:x})",
//			reinterpret_cast<uintptr_t>(_ReturnAddress()));
//	}
//}

bool __fastcall Hooked_BSClothUtils_BSTransformSet_QClothSupportsLOD(void* thisPtr)
{
	if (!thisPtr || IsBadReadPtr(thisPtr, sizeof(void*))) {
		logger::warn("Hooked_BSClothUtils_BSTransformSet_QClothSupportsLOD: Invalid thisPtr (0x{:x})", reinterpret_cast<uintptr_t>(thisPtr));
		return false;
	}

	__try {
		return g_OriginalBSClothUtils_BSTransformSet_QClothSupportsLOD(thisPtr);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		//logger::error("Critical error in BSClothUtils::BSTransformSet::QClothSupportsLOD (RIP: 0x{:x})", reinterpret_cast<uintptr_t>(_ReturnAddress()));
		return false;
	}
}

void HookedDoUpdate3DModel(RE::AIProcess* process, RE::Actor* actor, RE::RESET_3D_FLAGS flags)
{	
	bool log = iniSettings::getInstance().getExtendedLogs();
	RE::TESNPC* npc = get_face_TESNPC(actor->GetNPC());
	auto npcFormID = npc ? npc->formID : 0;
	if (!actor || !process)
		return;

	if (log) {
		logger::info("DoUpdate3DModel started {:0x} : {:0x}, flags {}", actor->formID, npcFormID, static_cast<int>(flags));
	}
	
	if (!iniSettings::getInstance().getDisableChangeHeadparts()) {
		if (!process /*|| !process->middleHigh || !actor || !actor->currentProcess || !actor->currentProcess->middleHigh*/) {
			if (log) {
				/*if (!process) {*/
				logger::info("DoUpdate3DModel no process {:0x} : {:0x}, flags {}", actor->formID, npcFormID, static_cast<int>(flags));
				/*} else if (!process->middleHigh) {
					logger::info("DoUpdate3DModel no middleHigh process {:0x} : {:0x}, flags {}", actor->formID, npc ? npc->formID : 0, static_cast<int>(flags));
				} else {
					logger::info("DoUpdate3DModel should never happens {:0x} : {:0x}, flags {}", actor->formID, npc ? npc->formID : 0, static_cast<int>(flags));
				}*/
			}
		} else {
			bool have_dbr_flag = bool(flags & f3D::kDiverseBodiesFlag);
			if (have_dbr_flag) {
				flags &= static_cast<f3D>(~static_cast<uint16_t>(f3D::kDiverseBodiesFlag));
			}

			constexpr auto headflags = static_cast<f3D>(static_cast<uint16_t>(f3D::kHead) | static_cast<uint16_t>(f3D::kFace));

			if (log) {
				npc = get_face_TESNPC(actor->GetNPC());
				logger::info("DoUpdate3DModel processing {:0x} : {:0x}, have_dbr_flag {}, headflags {}",
					actor->formID,
					npcFormID,
					static_cast<int>(flags),
					have_dbr_flag ? "true" : "false",
					bool(flags & headflags) ? "true" : "false");
			}

			if (bool(flags & headflags)) {
				if (!have_dbr_flag && flags != f3D::kNone) {
					if (auto preset = Find(actor); preset) {
						if (auto presetFlags = preset->get_flags(); actor && bool(presetFlags & headflags)) {
							flags |= presetFlags;
							std::thread([preset, flags] {
								std::this_thread::sleep_for(std::chrono::seconds(iniSettings::getInstance().getDelayTimer()));
								auto& actor = preset->actor;
								if (actor && actor->currentProcess /*&& actor->currentProcess->middleHigh*/) {
									if (iniSettings::getInstance().getExtendedLogs()) {
										auto npc = get_face_TESNPC(actor->GetNPC());
										logger::info("DoUpdate3DModel call preset->update {:0x} : {:0x} - {}",
											actor->formID,
											npc ? npc->formID : 0, static_cast<int>(flags));
									}
									preset->update(flags);
								}
							}).detach();
							return;
						}
					}
				}
			}
		}
	}

	
	/*if (npc && g_processingChangeHeadParts.contains(npcFormID)) {
		std::thread([process, actor, flags, npc, log, npcFormID] {
			int count{};
			while (g_processingChangeHeadParts.contains(npcFormID)) {
				std::this_thread::sleep_for(std::chrono::microseconds(1));
				++count;
			}
			HookedDoUpdate3DModel(process, actor, flags);
			if (log && count) {
				logger::info("HookedDoUpdate3DModel TESNPC* {:0x} waited for {} microseconds", npcFormID, count);
			}
		}).detach();
		return;
	}

	if (npcFormID) {
		g_processingChangeHeadParts.insert(npcFormID);
		if (log)
			logger::info("HookedDoUpdate3DModel block TESNPC* {:0x}", npcFormID);
	}*/

	if (log) {
		logger::info("DoUpdate3DModel call original process {:0x} : {:0x}, flags {}", actor->formID, npcFormID, static_cast<int>(flags));
	}

	g_OriginalDoUpdate3DModel(process, actor, flags);

	/*if (log)
		logger::info("HookedDoUpdate3DModel free TESNPC* {:0x}", npcFormID);
	g_processingChangeHeadParts.erase(npcFormID);*/
}

void remove_with_extra(RE::TESNPC* npc, RE::BGSHeadPart* hpart) {
	for (auto& extra : hpart->extraParts) {
		if (!extra->extraParts.empty())
			remove_with_extra(npc, extra);
		else
			g_OriginalChangeHeadPartRemovePart(npc, extra, false);
	}
	g_OriginalChangeHeadPartRemovePart(npc, hpart, false);
}

void ProcessChangeHeadPart(RE::TESNPC* npc, RE::BGSHeadPart* hpart, bool bRemoveExtraParts, bool isRemove)
{
	if (!npc || !hpart)
		return;

	npc = get_face_TESNPC(npc);
	auto formID{0};
	if (npc) {
		formID = npc->formID;
	}

	bool log = iniSettings::getInstance().getExtendedLogs();
	if (log)
		logger::info("Try to start processing changeHeadPart for {:0x}", formID);

	/*if (g_processingChangeHeadParts.contains(formID)) {
		std::thread([npc, hpart, bRemoveExtraParts, isRemove, formID, log] {
			int count{};
			while (g_processingChangeHeadParts.contains(formID)) {
				std::this_thread::sleep_for(std::chrono::microseconds(1));
				++count;
			}
			ProcessChangeHeadPart(npc, hpart, bRemoveExtraParts, isRemove);
			if (log && count)
				logger::info("TESNPC* {:0x} waited for {} microseconds", formID, count);
		}).detach();
		return;
	}

	g_processingChangeHeadParts.insert(formID);
	if (log)
		logger::info("ProcessChangeHeadPart block TESNPC* {:0x}", formID);*/
	if (isRemove) {
		if (bRemoveExtraParts)
			remove_with_extra(npc, hpart);
		else
			g_OriginalChangeHeadPartRemovePart(npc, hpart, false);

	} else {
		g_OriginalChangeHeadPart(npc, hpart);
	}

	/*if (log)
		logger::info("ProcessChangeHeadPart free TESNPC* {:0x}", formID);
	g_processingChangeHeadParts.erase(formID);*/
}

void HookedChangeHeadPartRemovePart(RE::TESNPC* npc, RE::BGSHeadPart* hpart, bool bRemoveExtraParts)
{
	if (!npc || !hpart) {
		return g_OriginalChangeHeadPartRemovePart(npc, hpart, bRemoveExtraParts);
	}
	ProcessChangeHeadPart(npc, hpart, bRemoveExtraParts, true);
	//g_OriginalChangeHeadPartRemovePart(npc, hpart, bRemoveExtraParts);
}

void HookedChangeHeadPart(RE::TESNPC* npc, RE::BGSHeadPart* hpart)
{
	if (!npc || !hpart) {
		return g_OriginalChangeHeadPart(npc, hpart);
	}
	ProcessChangeHeadPart(npc, hpart, false, false);
	//g_OriginalChangeHeadPart(npc, hpart);
}

void SetupDetours(ActorUpdateManager* manager)
{
	uintptr_t baseAddr = reinterpret_cast<uintptr_t>(GetModuleHandleA("f4ee.dll"));

	if (!detourObjectLoaded.Create(reinterpret_cast<LPVOID>(baseAddr + 0x3DE0), reinterpret_cast<LPVOID>(HookedReceiveEventObjectLoaded))) {
		logger::error("Failed to create detour for ReceiveEventObjectLoaded");
	} else {
		g_OriginalReceiveEventObjectLoaded = reinterpret_cast<TESObjectLoadedEventHandler>(detourObjectLoaded.GetTrampoline());
		logger::info("Detour for ReceiveEventObjectLoaded created successfully.");
	}

	if (!detourInitScript.Create(reinterpret_cast<LPVOID>(baseAddr + 0x3C40), reinterpret_cast<LPVOID>(HookedReceiveEventInitScript))) {
		logger::error("Failed to create detour for ReceiveEventInitScript");
	} else {
		g_OriginalReceiveEventInitScript = reinterpret_cast<TESInitScriptEventHandler>(detourInitScript.GetTrampoline());
		logger::info("Detour for ReceiveEventInitScript created successfully.");
	}

	/*if (!detourReset3D.Create(reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL)) + REL::ID(302888).offset()), reinterpret_cast<LPVOID>(HookedReset3D))) {
		logger::error("Failed to create detour for Reset3DHandler");
	} else {
		g_OriginalReset3D = reinterpret_cast<Reset3DHandler>(detourReset3D.GetTrampoline());
		logger::info("Detour for Reset3DHandler created successfully.");
	}*/

	if (!detourChangeHeadPartRemovePart.Create(reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL)) + 5985632), reinterpret_cast<LPVOID>(HookedChangeHeadPartRemovePart))) {
		logger::error("Failed to create detour for ChangeHeadPartRemovePart");
	} else {
		g_OriginalChangeHeadPartRemovePart = reinterpret_cast<ChangeHeadPartRemovePartHandler>(detourChangeHeadPartRemovePart.GetTrampoline());
		logger::info("Detour for ChangeHeadPartRemovePart created successfully.");
	}

	if (!detourChangeHeadPart.Create(reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL)) + 6003504), reinterpret_cast<LPVOID>(HookedChangeHeadPart))) {
		logger::error("Failed to create detour for ChangeHeadPart");
	} else {
		g_OriginalChangeHeadPart = reinterpret_cast<ChangeHeadPartHandler>(detourChangeHeadPart.GetTrampoline());
		logger::info("Detour for ChangeHeadPart created successfully.");
	}

	//if (!detourUpdate3DModel.Create(reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL)) + 0xE3C9C0), reinterpret_cast<LPVOID>(HookedUpdate3DModel))) {
	//	logger::error("Failed to create detour for Update3DModel");
	//} else {
	//	g_OriginalUpdate3DModel = reinterpret_cast<Update3DModelHandler>(detourUpdate3DModel.GetTrampoline());
	//	logger::info("Detour for Update3DModel created successfully.");
	//}

	if (!detourDoUpdate3DModel.Create(reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL)) + 0xE60860), reinterpret_cast<LPVOID>(HookedDoUpdate3DModel))) {
		logger::error("Failed to create detour for DoUpdate3DModel");
	} else {
		g_OriginalDoUpdate3DModel = reinterpret_cast<DoUpdate3DModelHandler>(detourDoUpdate3DModel.GetTrampoline());
		logger::info("Detour for DoUpdate3DModel created successfully.");
	}

	//Вылеты BSClothInstance
	if (!detourSetTransformSet.Create(
			reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL)) + 0x1DDC7A0),
			reinterpret_cast<LPVOID>(HookedSetTransformSet))) {
		logger::error("Failed to create detour for SetTransformSet");
	} else {
		g_OriginalSetTransformSet = reinterpret_cast<SetTransformSetHandler>(detourSetTransformSet.GetTrampoline());
		logger::info("Detour for SetTransformSet created successfully.");
	}

	if (!detourBSClothExtraData.Create(
			reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL)) + 0x1DA7A40),
			reinterpret_cast<LPVOID>(Hooked_BSClothExtraData_SetSettle))) {
		logger::error("Failed to create detour for BSClothExtraData");
	} else {
		g_OriginalBSClothExtraData_SetSettle =
			reinterpret_cast<BSClothExtraData_SetSettle_t>(
				detourBSClothExtraData.GetTrampoline());
		logger::info("Detour for BSClothExtraData created successfully.");
	}

	//с этим вылетает 0304E698 DLC03_Armor_Trapper_Suit_Hunter (equipitem 0304E698)
	if (!detourBSTransformSet.Create(
			reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL)) + 0x1DAF3B0),
			reinterpret_cast<LPVOID>(Hooked_BSTransformSet))) {
		logger::error("Failed to create detour for BSTransformSet");
	} else {
		g_OriginalBSTransformSet =
			reinterpret_cast<BSTransformSetHandler>(
				detourBSTransformSet.GetTrampoline());
		logger::info("Detour for BSTransformSet created successfully.");
	}

	//if (!detourGetActiveSimClothIndices.Create(
	//		reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL)) + 0x1DDC960),
	//		reinterpret_cast<LPVOID>(Hooked_hclClothInstance_GetActiveSimClothIndices))) {
	//	logger::error("Failed to create detour for GetActiveSimClothIndices");
	//} else {
	//	g_OriginalGetActiveSimClothIndices =
	//		reinterpret_cast<hclClothInstance_GetActiveSimClothIndices_t>(
	//			detourGetActiveSimClothIndices.GetTrampoline());
	//	logger::info("Detour for GetActiveSimClothIndices created successfully.");
	//}

	//if (!detourBSClothTeleport.Create(
	//		reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL)) + /*0x1DA7000*/ 0x1DA6EB0),
	//		reinterpret_cast<LPVOID>(Hooked_BSClothExtraData_TeleportToMatchBoneTransform))) {
	//	logger::error("Failed to create detour for BSClothExtraData::TeleportToMatchBoneTransform");
	//} else {
	//	g_OriginalBSClothExtraData_TeleportToMatchBoneTransform =
	//		reinterpret_cast<BSClothExtraData_TeleportToMatchBoneTransform_t>(detourBSClothTeleport.GetTrampoline());
	//	logger::info("Detour for BSClothExtraData::TeleportToMatchBoneTransform created successfully.");
	//}

	if (!detourBSClothUtils_QClothSupportsLOD.Create(
			reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL)) + 0x1DAF3B0),
			reinterpret_cast<LPVOID>(Hooked_BSClothUtils_BSTransformSet_QClothSupportsLOD))) {
		logger::error("Failed to create detour for BSClothUtils::BSTransformSet::QClothSupportsLOD");
	} else {
		g_OriginalBSTransformSet =
			reinterpret_cast<BSTransformSetHandler>(
				detourBSClothUtils_QClothSupportsLOD.GetTrampoline());
		logger::info("Detour for BSClothUtils::BSTransformSet::QClothSupportsLOD created successfully.");
	}
}
