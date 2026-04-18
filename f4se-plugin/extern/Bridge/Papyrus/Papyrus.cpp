#pragma once

#include "F4SE/F4SE.h"
#include "RE/Fallout.h"

#include "Bridge/Consts.h"
#include "Bridge/Papyrus/Papyrus.h"
#include <random>

namespace logger = F4SE::log;

#define PAPYRUS_BIND(funcName) a_VM->BindNativeMethod("NAFBridge", #funcName, funcName, true)
#define PAPYRUS_BIND_LATENT(funcName, retType) a_VM->BindNativeMethod<retType>("NAFBridge", #funcName, funcName, true, true)

extern int PRINT_LOG;

namespace Papyrus
{
	bool RegisterBridgeFunctions(RE::BSScript::IVirtualMachine* a_VM)
	{
		//PAPYRUS_BIND(GetFurnitureList);
		PAPYRUS_BIND(MovePlayerToPosition);
		PAPYRUS_BIND(UpdateReference3D);
		PAPYRUS_BIND(Update3dFlags);
		PAPYRUS_BIND(ClearAll3DUpdateFlags);
		PAPYRUS_BIND(Is3rdPersonVisible);
		PAPYRUS_BIND(Load3d);
		PAPYRUS_BIND(Update3DPosition);
		PAPYRUS_BIND(CutFormList);
		PAPYRUS_BIND(IsPreCulled);

		logger::info("NAFBridge functions registered.");
		return true;
	}
	
	//RE::BGSListForm* GetFurnitureList(std::monostate, RE::BGSListForm* formlist)
	//{
	//	if (formlist == nullptr)
	//		return nullptr;
	//	formlist->ClearData();
	//	/*if (formlist->scriptAddedTempForms == nullptr) {
	//		formlist->scriptAddedTempForms = new RE::BSTArray<std::uint32_t>;
	//	}*/ 
	//	return _Furnitures->ImportData(formlist);
	//}

	void MovePlayerToPosition(std::monostate, float x, float y, float z)
	{
		RE::Actor* PlayerRef = static_cast<RE::Actor*>(RE::TESForm::GetFormByID(20));
		RE::NiPoint3 Pos(x, y, z);
		PlayerRef->SetPosition(Pos, true);
	}

	void UpdateReference3D(std::monostate, RE::TESObjectREFR* obj)
	{
		if (obj == nullptr)
			return;
		obj->UpdateReference3D();
	}

	void Update3dFlags(std::monostate, RE::TESObjectREFR* obj, size_t flag)
	{
		if (obj == nullptr)
			return;
		obj->Set3DUpdateFlag(static_cast<RE::RESET_3D_FLAGS>(1u << flag));
		/*kModel = 1u << 0,
		kSkin = 1u << 1,
		kHead = 1u << 2,
		kFace = 1u << 3,
		kScale = 1u << 4,
		kSkeleton = 1u << 5,
		kInitDefault = 1u << 6,
		kSkyCellSkin = 1u << 7,
		kHavok = 1u << 8,
		kDontAddOutfit = 1u << 9,
		kKeepHead = 1u << 10,
		kDismemberment = 1u << 11*/
	}

	void ClearAll3DUpdateFlags(std::monostate, RE::TESObjectREFR* obj)
	{
		if (obj == nullptr)
			return;
		obj->ClearAll3DUpdateFlags();
	}

	bool Is3rdPersonVisible(std::monostate, RE::TESObjectREFR* obj)
	{
		if (obj == nullptr)
			return false;
		return obj->Is3rdPersonVisible();
	}

	void Load3d(std::monostate, RE::TESObjectREFR* obj, bool backgroundLoad)
	{
		if (obj == nullptr)
			return;
		obj->Load3D(backgroundLoad);
	}

	void Update3DPosition(std::monostate, RE::TESObjectREFR* obj, bool warp)
	{
		if (obj == nullptr)
			return;
		obj->Update3DPosition(warp);
	}

	int GetLoaded3dFlags(std::monostate, RE::TESObjectREFR* obj)
	{
		if (obj == nullptr)
			return 0;
		return obj->GetFullyLoaded3D()->GetFlags();
	}

	RE::BGSListForm* CutFormList(std::monostate, RE::BGSListForm* flist, RE::TESForm* cut_to_form)
	{
		size_t count = 0;
		for (auto f : flist->arrayOfForms) {
			++count;
			if (f == cut_to_form)
				break;
		}
		if (flist->arrayOfForms.size() == count) {
			flist->arrayOfForms.clear();
			return flist;
		}

		RE::BSTArray<RE::TESForm*> copy;
		for (size_t i = 0; i < flist->arrayOfForms.size(); ++i) {
			if (i <= count)
				continue;
			copy.push_back(flist->arrayOfForms[i]);
		}
		flist->arrayOfForms = copy;
		return flist;
	}

	bool IsPreCulled(std::monostate, RE::TESObjectREFR* obj)
	{
		if (!obj)
			return false;
		/*if (obj->IsDisabled())
			return false;*/
		/*auto data = obj->loadedData;
		if (!data)
			return true;*/
		auto niav_obj = obj->Get3D();
		if (niav_obj ? !niav_obj->GetAppCulled() : false)
			return true;

		auto preCulled = RE::BSPreCulledObjects::Get3DForID(obj->formID);
		return preCulled ? true : false;
	}
}

#undef PAPYRUS_BIND
#undef PAPYRUS_BIND_LATENT
