#pragma once

class RE::ReferenceEffect : public RE::NiObject
{
public:
	static RE::ReferenceEffect* InstantiateTransientHitEffect(TESObjectREFR* a_refr, TESEffectShader* a_shader, float a_f = -1.0f, TESObjectREFR* a_refr2 = nullptr, bool a_b1 = false, bool a_b2 = false, NiAVObject* a_obj = nullptr, bool a_b3 = false)
	{
		using func_t = decltype(&RE::ReferenceEffect::InstantiateTransientHitEffect);
		REL::Relocation<func_t> func{ REL::ID(652173) };
		return func(a_refr, a_shader, a_f, a_refr2, a_b1, a_b2, a_obj, a_b3);
	}
};

RE::ReferenceEffect* RE::TESEffectShader::Play(RE::TESObjectREFR* a_refr)
{
	if (a_refr != nullptr && a_refr->parentCell != nullptr && a_refr->parentCell->loadedData != nullptr && a_refr->GetFullyLoaded3D() != nullptr) {
		return RE::ReferenceEffect::InstantiateTransientHitEffect(a_refr, this);
	} else {
		return nullptr;
	}
}

void RE::TESEffectShader::Stop(RE::TESObjectREFR* a_refr)
{
	RE::ProcessLists::GetSingleton()->FinishShaderEffect(a_refr, this);
}
