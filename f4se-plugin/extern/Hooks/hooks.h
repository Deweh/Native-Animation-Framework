#pragma once
#include <algorithm>
#include <chrono>
#include <concurrent_queue.h>
#include <concurrent_unordered_set.h>
#include <condition_variable>
#include <format>
#include <functional>
#include <iostream>
#include <iterator>
#include <mutex>
#include <optional>
#include <ppl.h>
#include <string>
#include <thread>
#include <unordered_set>

#include "F4SE/F4SE.h"
#include "RE/Fallout.h"
#include <DiverseBodiesRedux/Globals/Globals.h>
#include <F4SE/API.h>

namespace logger = F4SE::log;

#include <detourxs/DetourXS.h>

#include "LooksMenuInterfaces.h"
//#include "ProcessingSafe.h"

// Объявление функции SetupDetours (один раз)
void SetupDetours(ActorUpdateManager* manager);

using TESObjectLoadedEventHandler = RE::BSEventNotifyControl (*)(ActorUpdateManager*, RE::TESObjectLoadedEvent*, void*);
using TESInitScriptEventHandler = RE::BSEventNotifyControl (*)(ActorUpdateManager*, RE::TESInitScriptEvent*, void*);
//using Reset3DHandler = void (*)(RE::Actor*, bool, RE::RESET_3D_FLAGS, bool, RE::RESET_3D_FLAGS);
using ChangeHeadPartHandler = void (*)(RE::TESNPC*, RE::BGSHeadPart*);
using ChangeHeadPartRemovePartHandler = void (*)(RE::TESNPC*, RE::BGSHeadPart*, bool);
//using Update3DModelHandler = void (*)(RE::AIProcess*, RE::Actor*, RE::TESObjectREFR*, bool);
using DoUpdate3DModelHandler = void(*)(RE::AIProcess*, RE::Actor*, RE::RESET_3D_FLAGS);
//BSFaceGenPendingHeadData
//Вылеты BSClothInstance
using SetTransformSetHandler = void (*)(void*, uint32_t, void*);
using BSClothExtraData_SetSettle_t = void(__fastcall*)(void* thisPtr, bool enable);
using BSTransformSetHandler = bool(__fastcall*)(void* thisPtr);
//using hclClothInstance_GetActiveSimClothIndices_t = void(__fastcall*)(void* instance, uint32_t index, void* outArray);
//using BSClothExtraData_TeleportToMatchBoneTransform_t = void(__fastcall*)(void* thisPtr, void* a2, void* a3);
using BSClothUtils_BSTransformSet_QClothSupportsLOD_t = bool(__fastcall*)(void* thisPtr);

RE::BSEventNotifyControl HookedReceiveEventObjectLoaded(ActorUpdateManager*, RE::TESObjectLoadedEvent*, void*);
RE::BSEventNotifyControl HookedReceiveEventInitScript(ActorUpdateManager*, RE::TESInitScriptEvent*, void*);
//void HookedReset3D(RE::Actor*, bool, RE::RESET_3D_FLAGS, bool, RE::RESET_3D_FLAGS);
void ProcessChangeHeadPart(RE::TESNPC* npc, RE::BGSHeadPart* hpart, bool bRemoveExtraParts, bool isRemove);
void HookedChangeHeadPartRemovePart(RE::TESNPC*, RE::BGSHeadPart*, bool);
void HookedChangeHeadPart(RE::TESNPC*, RE::BGSHeadPart*);
//void HookedUpdate3DModel(RE::AIProcess*, RE::Actor*, RE::TESObjectREFR*, bool);
void HookedDoUpdate3DModel(RE::AIProcess*, RE::Actor*, RE::RESET_3D_FLAGS);
//Вылеты BSClothInstance
void HookedSetTransformSet(void* instance, uint32_t index, void* transformSet);
void Hooked_BSClothExtraData_SetSettle(void* thisPtr, bool enable);
void Hooked_BSTransformSet(void* thisPtr);
//void __fastcall Hooked_hclClothInstance_GetActiveSimClothIndices(void* instance, uint32_t index, void* outArray);
//void __fastcall Hooked_BSClothExtraData_TeleportToMatchBoneTransform(void* thisPtr, void* a2, void* a3);
bool __fastcall Hooked_BSClothUtils_BSTransformSet_QClothSupportsLOD(void* thisPtr);
