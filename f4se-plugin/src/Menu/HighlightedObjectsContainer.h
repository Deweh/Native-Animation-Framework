#pragma once
#include "Serialization/General.h"
#include <unordered_set>

namespace Menu
{
	using namespace Serialization::General;

	class HighlightedObjectsContainer
	{
	public:
		RE::TESEffectShader* hoverEffect = nullptr;
		std::unordered_set<SerializableRefHandle> refs;

		HighlightedObjectsContainer()
		{
			GetEffect();
		}

		~HighlightedObjectsContainer()
		{
			RemoveAll();
		}

		bool HasEffect() {
			return hoverEffect != nullptr;
		}

		bool GetEffect() {
			//NAF_HoverFXS
			auto hoverForm = RE::TESDataHandler::GetSingleton()->LookupForm(0x1734, "NAF.esp");
			if (hoverForm) {
				hoverEffect = hoverForm->As<RE::TESEffectShader>();
				if (hoverEffect) {
					return true;
				}
			}
			return false;
		}

		void Add(RE::TESObjectREFR* ref) {
			if (ref && HasEffect()) {
				hoverEffect->Play(ref);
				refs.insert(ref->GetHandle());
			}
		}

		void AddSingle(RE::TESObjectREFR* ref)
		{
			RemoveAll();
			Add(ref);
		}

		void Remove(RE::TESObjectREFR* ref)
		{
			if (ref && refs.contains(ref->GetHandle())) {
				hoverEffect->Stop(ref);
			}
		}

		void RemoveAll() {
			if (HasEffect()) {
				for (auto& h : refs) {
					auto r = h.get().get();
					if (r) {
						hoverEffect->Stop(r);
					}
				}
			}
		}
	};
}
