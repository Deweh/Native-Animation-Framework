#pragma once
#include "Papyrus/EventRegistrations.h"
#include "Papyrus/NAF.h"
#include "Papyrus/EventProxy.h"

#include "Bridge/Consts.h"

#define PAPYRUS_BIND(funcName) a_VM->BindNativeMethod(NAF::SCRIPT_NAME, #funcName, NAF::funcName, true)
#define PAPYRUS_BIND_LATENT(funcName, retType) a_VM->BindNativeMethod<retType>(NAF::SCRIPT_NAME, #funcName, NAF::funcName, true, true)

extern RE::BSScript::IVirtualMachine* g_VM;

namespace Papyrus
{
	bool RegisterFunctions(RE::BSScript::IVirtualMachine* a_VM)
	{
		g_VM = a_VM;
		
		PAPYRUS_BIND(ToggleMenu);
		PAPYRUS_BIND(SetDisableRescaler);
		PAPYRUS_BIND(GetDisableRescaler);		
		PAPYRUS_BIND(IsActorUsable);
		PAPYRUS_BIND(SetActorUsable);
		PAPYRUS_BIND(SetPackageOverride);
		PAPYRUS_BIND(ClearPackageOverride);
		PAPYRUS_BIND(PlayDynamicIdle);

		PAPYRUS_BIND(StartScene);
		PAPYRUS_BIND(WalkToAndStartScene);
		PAPYRUS_BIND(StopScene);
		PAPYRUS_BIND(SetScenePosition);
		PAPYRUS_BIND(GetSceneActors);
		PAPYRUS_BIND(GetSceneHKXs);
		PAPYRUS_BIND(GetSceneTags);
		PAPYRUS_BIND(SetSceneSpeed);
		PAPYRUS_BIND(GetSceneSpeed);
		PAPYRUS_BIND(GetSceneFromActor);
		PAPYRUS_BIND(IsSceneRunning);
		PAPYRUS_BIND(GetSceneProperty);
		PAPYRUS_BIND(ValidateSceneParams);
		PAPYRUS_BIND(ReEnablePlayerInput);

		PAPYRUS_BIND(PlayNANIM);
		PAPYRUS_BIND(StopNANIM);
		PAPYRUS_BIND(SetIKChainTarget);
		PAPYRUS_BIND(SetIKChainEnabled);

		PAPYRUS_BIND(SetEyeCoordOverride);
		PAPYRUS_BIND(ClearEyeCoordOverride);
		PAPYRUS_BIND(PlayFaceAnimation);
		PAPYRUS_BIND(StopFaceAnimation);

		PAPYRUS_BIND(DrawRectangle);
		PAPYRUS_BIND(DrawText);
		PAPYRUS_BIND(DrawLine);
		PAPYRUS_BIND(SetElementPosition);
		PAPYRUS_BIND(TranslateElementTo);
		PAPYRUS_BIND(StopElementTranslation);
		PAPYRUS_BIND(RemoveElement);
		PAPYRUS_BIND(GetElementWidth);
		PAPYRUS_BIND(GetElementHeight);
		PAPYRUS_BIND(MoveElementToFront);
		PAPYRUS_BIND(MoveElementToBack);
		PAPYRUS_BIND(SetElementMask);
		PAPYRUS_BIND(AttachElementTo);

		PAPYRUS_BIND(GetLocalTransform);
		PAPYRUS_BIND(SynchronizeAnimations);

//Bridge papyrus
#undef PAPYRUS_BIND
#undef PAPYRUS_BIND_LATENT
#define PAPYRUS_BIND(funcName) a_VM->BindNativeMethod(MODNAME, #funcName, NAFBridge::funcName, true)
#define PAPYRUS_BIND_LATENT(funcName, retType) a_VM->BindNativeMethod<retType>(MODNAME, #funcName, NAFBridge::funcName, true, true)

		PAPYRUS_BIND(ApplyMorphSet);
		PAPYRUS_BIND(CompleteWalkForActor);
		PAPYRUS_BIND(CompleteWalkForScene);
		PAPYRUS_BIND(FindPositionBySceneSettings);
		PAPYRUS_BIND(ApplyEquipmentSet);
		PAPYRUS_BIND(ValidateSceneParamsIgnoreInScene);
		PAPYRUS_BIND(GetPositionInstalled);
		PAPYRUS_BIND(GetFurnitureList);
		PAPYRUS_BIND(GetOverlay);

		return true;
	}
#undef PAPYRUS_BIND
#undef PAPYRUS_BIND_LATENT
}
