#include "Bridge/Consts.h"
#include "RE/Fallout.h"

namespace Papyrus
{
	bool RegisterBridgeFunctions(RE::BSScript::IVirtualMachine* a_VM);
	void MovePlayerToPosition(std::monostate, float x, float y, float z);
	void UpdateReference3D(std::monostate, RE::TESObjectREFR* obj);
	void Update3dFlags(std::monostate, RE::TESObjectREFR* obj, size_t flag);
	void ClearAll3DUpdateFlags(std::monostate, RE::TESObjectREFR* obj);
	bool Is3rdPersonVisible(std::monostate, RE::TESObjectREFR* obj);
	void Load3d(std::monostate, RE::TESObjectREFR* obj, bool backgroundLoad);
	void Update3DPosition(std::monostate, RE::TESObjectREFR* obj, bool warp);
	int GetLoaded3dFlags(std::monostate, RE::TESObjectREFR* obj);
	RE::BGSListForm* CutFormList(std::monostate, RE::BGSListForm* flist, RE::TESForm* cut_to_form);
	void PatchOffsets(std::monostate);
	bool IsPreCulled(std::monostate, RE::TESObjectREFR* obj);
}
