#pragma once

#define NAF_ESP_NAME "NAF.esp"
#define NAF_LOCK_ALIAS_NAME "LockedActors"
#define NAF_HOVER_EFFECT 0x1734
#define NAF_MAIN_QUEST 0xF99
#define NAF_LOCK_ACTOR_PACKAGE 0xF9A
#define NAF_WALK_PACKAGE 0x266A
#define NAF_IN_SCENE_KEYWORD 0x1ECF
#define NAF_DO_NOT_USE_KEYWORD 0x1ED0
#define NAF_WALK_TO_KEYWORD 0x266B
#define DUMMY_STATIC_OBJECT 0xF9D
#define BRIDGE_DO_NOT_USE_KEYWORDS_LIST 0x31752 //NAF Bridge protected keywords list patch
#define MODNAME "NAFBridge"

//NAF Bridge eslfication
#define ESL_NAF_HOVER_EFFECT 0x1
#define ESL_NAF_WALK_PACKAGE 0x4
#define ESL_NAF_IN_SCENE_KEYWORD 0x2
#define ESL_NAF_DO_NOT_USE_KEYWORD 0x3
#define ESL_NAF_WALK_TO_KEYWORD 0x5

#define GET_FORM_EID(type, eid) RE::TESForm::GetFormByEditorID<type>(eid)
#define GET_FORM(type, id) RE::TESForm::GetFormByID<type>(id)
#define GET_MOD_FORM(type, id, modName) RE::TESDataHandler::GetSingleton()->LookupForm<type>(id, modName)

#define NAF_BRIDGE_ESP "AAF.esm" //NAF Bridge protected keywords list patch

namespace Data
{
	namespace Forms
	{
		RE::PlayerCharacter* Player = nullptr;
		RE::TESPackage* NAFWalkPackage = nullptr;
		RE::TESPackage* NAFLockPackage = nullptr;
		RE::BGSKeyword* NAFInSceneKW = nullptr;
		RE::BGSKeyword* NAFDoNotUseKW = nullptr;
		RE::BGSKeyword* NAFWalkToKW = nullptr;
		RE::TESIdleForm* LooseIdleStop = nullptr;
		RE::ActorValueInfo* AnimMultAV = nullptr;
		RE::TESIdleForm* NAFDynIdle = nullptr;
		RE::BGSKeyword* TeammateReadyWeaponKW = nullptr;
		RE::BGSKeyword* BlockActivationKW = nullptr;
		RE::BGSKeyword* ActorTypeChildKW = nullptr;
		RE::BGSKeyword* ShowWornItemsKW = nullptr;
		RE::BGSListForm* protectedKeywords = nullptr;
		RE::TESObjectSTAT* dummy_static = nullptr;

		
		//NAf Bridge eslfication
		//void GetFormPointers() {
		//	if (!RE::TESDataHandler::GetSingleton()->LookupLoadedModByName(NAF_ESP_NAME)) {
		//		logger::critical("NAF.esp is not installed/enabled. The framework will not function correctly without this. Aborting.");
		//		MessageBoxA(nullptr, "NAF.esp is not installed/enabled, NAF will not function correctly without this. If you would like to uninstall NAF, remove NAF.dll from the F4SE/Plugins folder.", "Native Animation Framework", 0x00001000);
		//		std::abort();
		//	}

		//	Player = RE::PlayerCharacter::GetSingleton();
		//	AnimMultAV = GET_FORM(RE::ActorValueInfo, 0x2D2);
		//	NAFInSceneKW = GET_MOD_FORM(RE::BGSKeyword, NAF_IN_SCENE_KEYWORD, NAF_ESP_NAME);
		//	NAFDoNotUseKW = GET_MOD_FORM(RE::BGSKeyword, NAF_DO_NOT_USE_KEYWORD, NAF_ESP_NAME);
		//	LooseIdleStop = GET_FORM_EID(RE::TESIdleForm, "LooseIdleStop");
		//	NAFLockPackage = GET_MOD_FORM(RE::TESPackage, NAF_LOCK_ACTOR_PACKAGE, NAF_ESP_NAME);
		//	NAFWalkPackage = GET_MOD_FORM(RE::TESPackage, NAF_WALK_PACKAGE, NAF_ESP_NAME);
		//	NAFWalkToKW = GET_MOD_FORM(RE::BGSKeyword, NAF_WALK_TO_KEYWORD, NAF_ESP_NAME);
		//	NAFDynIdle = GET_FORM_EID(RE::TESIdleForm, "NAF_DynamicIdle");
		//	TeammateReadyWeaponKW = GET_FORM_EID(RE::BGSKeyword, "TeammateReadyWeapon_DO");
		//	BlockActivationKW = GET_FORM_EID(RE::BGSKeyword, "BlockPlayerActivation");
		//	ActorTypeChildKW = GET_FORM_EID(RE::BGSKeyword, "ActorTypeChild");
		//	ShowWornItemsKW = GET_FORM_EID(RE::BGSKeyword, "ShowWornItemsKeyword");
		//	protectedKeywords = GET_MOD_FORM(RE::BGSListForm, BRIDGE_DO_NOT_USE_KEYWORDS_LIST, NAF_BRIDGE_ESP);  //NAF Bridge protected keywords list patch
		//}

		void GetFormPointers() {
			if (RE::TESDataHandler::GetSingleton()->LookupLoadedModByName(NAF_ESP_NAME)) {
				NAFInSceneKW = GET_MOD_FORM(RE::BGSKeyword, NAF_IN_SCENE_KEYWORD, NAF_ESP_NAME);
				NAFDoNotUseKW = GET_MOD_FORM(RE::BGSKeyword, NAF_DO_NOT_USE_KEYWORD, NAF_ESP_NAME);
				NAFWalkPackage = GET_MOD_FORM(RE::TESPackage, NAF_WALK_PACKAGE, NAF_ESP_NAME);
				NAFWalkToKW = GET_MOD_FORM(RE::BGSKeyword, NAF_WALK_TO_KEYWORD, NAF_ESP_NAME);	
			} else if (RE::TESDataHandler::GetSingleton()->LookupLoadedLightModByName(NAF_ESP_NAME)) {
				NAFInSceneKW = GET_MOD_FORM(RE::BGSKeyword, ESL_NAF_IN_SCENE_KEYWORD, NAF_ESP_NAME);
				NAFDoNotUseKW = GET_MOD_FORM(RE::BGSKeyword, ESL_NAF_DO_NOT_USE_KEYWORD, NAF_ESP_NAME);
				NAFWalkPackage = GET_MOD_FORM(RE::TESPackage, ESL_NAF_WALK_PACKAGE, NAF_ESP_NAME);
				NAFWalkToKW = GET_MOD_FORM(RE::BGSKeyword, ESL_NAF_WALK_TO_KEYWORD, NAF_ESP_NAME);		
			} else {
				logger::critical("NAF.esp is not installed/enabled. The framework will not function correctly without this. Aborting.");
				MessageBoxA(nullptr, "NAF.esp is not installed/enabled, NAF will not function correctly without this. If you would like to uninstall NAF, remove NAF.dll from the F4SE/Plugins folder.", "Native Animation Framework", 0x00001000);
				std::abort();
			}

			Player = RE::PlayerCharacter::GetSingleton();
			AnimMultAV = GET_FORM(RE::ActorValueInfo, 0x2D2);
			LooseIdleStop = GET_FORM_EID(RE::TESIdleForm, "LooseIdleStop");
			NAFLockPackage = GET_MOD_FORM(RE::TESPackage, NAF_LOCK_ACTOR_PACKAGE, NAF_ESP_NAME);
			NAFDynIdle = GET_FORM_EID(RE::TESIdleForm, "NAF_DynamicIdle");
			TeammateReadyWeaponKW = GET_FORM_EID(RE::BGSKeyword, "TeammateReadyWeapon_DO");
			BlockActivationKW = GET_FORM_EID(RE::BGSKeyword, "BlockPlayerActivation");
			ActorTypeChildKW = GET_FORM_EID(RE::BGSKeyword, "ActorTypeChild");
			ShowWornItemsKW = GET_FORM_EID(RE::BGSKeyword, "ShowWornItemsKeyword");
			protectedKeywords = GET_MOD_FORM(RE::BGSListForm, BRIDGE_DO_NOT_USE_KEYWORDS_LIST, NAF_BRIDGE_ESP);  //NAF Bridge protected keywords list patch
			dummy_static = GET_MOD_FORM(RE::TESObjectSTAT, DUMMY_STATIC_OBJECT, NAF_BRIDGE_ESP); //NAF Bridge dummy static object

			
		}
	}
}

#define player Data::Forms::Player
