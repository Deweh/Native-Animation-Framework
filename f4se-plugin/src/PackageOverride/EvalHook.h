#pragma once
#include "detourxs/DetourXS.h"
#include "Manager.h"

namespace PackageOverride::EvalHook
{
	typedef void(EvaluatePackageSig)(RE::Actor*, bool, bool);
	typedef bool(CheckPackageSig)(RE::AIProcess*, RE::Actor*, bool);

	REL::Relocation<EvaluatePackageSig> OriginalEvaluatePackage;
	REL::Relocation<CheckPackageSig> OriginalCheckPackage;
	DetourXS evalHook;
	DetourXS checkHook;
	
	void HookedEvaluatePackage(RE::Actor* a_actor, bool a_isCommand, bool a_resetAI) {
		if (!a_resetAI || IsEvalAllowed(a_actor))
			OriginalEvaluatePackage(a_actor, a_isCommand, a_resetAI);
	}

	bool HookedCheckPackage(RE::AIProcess* a_process, RE::Actor* a_actor, bool a_unk) {
		return IsEvalAllowed(a_actor) ? OriginalCheckPackage(a_process, a_actor, a_unk) : false;
	}

	void RegisterHook() {
		REL::Relocation<EvaluatePackageSig> EvalPkgLoc{ REL::ID(1395257) };
		REL::Relocation<CheckPackageSig> CheckPkgLoc{ REL::ID(609985) };

		if (!evalHook.Create(reinterpret_cast<LPVOID>(EvalPkgLoc.address()), &HookedEvaluatePackage)) {
			logger::warn("Failed to create EvaluatePackage hook!");
		} else {
			OriginalEvaluatePackage = reinterpret_cast<uintptr_t>(evalHook.GetTrampoline());
		}

		if (!checkHook.Create(reinterpret_cast<LPVOID>(CheckPkgLoc.address()), &HookedCheckPackage)) {
			logger::warn("Failed to create CheckForNewPackage hook!");
		} else {
			OriginalCheckPackage = reinterpret_cast<uintptr_t>(checkHook.GetTrampoline());
		}
	}
}
