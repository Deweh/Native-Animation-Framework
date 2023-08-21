#pragma once

namespace RE
{
	struct BSAnimationGraphEvent
	{
		TESObjectREFR* refr;
		BSFixedString animEvent;
		BSFixedString argument;  //Something like SoundPlay.>>Descriptorname<<
	};

	namespace BGSAnimationSystemUtils
	{
		struct ActiveSyncInfo
		{
			BSTObjectArena<BSTTuple<BSFixedString, float>> otherSyncInfo;
			float currentAnimTime;
			float animSpeedMult;
			float totalAnimTime;
		};

		inline bool GetActiveSyncInfo(const IAnimationGraphManagerHolder* a_graphHolder, ActiveSyncInfo& a_infoOut)
		{
			using func_t = decltype(&GetActiveSyncInfo);
			REL::Relocation<func_t> func{ REL::ID(1349978) };
			return func(a_graphHolder, a_infoOut);
		}

		inline bool IsActiveGraphInTransition(const TESObjectREFR* a_refr)
		{
			using func_t = decltype(&IsActiveGraphInTransition);
			REL::Relocation<func_t> func{ REL::ID(839650) };
			return func(a_refr);
		}

		inline bool InitializeActorInstant(RE::Actor* a_actor, bool a_initializeNodes) {
			using func_t = decltype(&InitializeActorInstant);
			REL::Relocation<func_t> func{ REL::ID(672857) };
			return func(a_actor, a_initializeNodes);
		}

		inline bool IsIdleLoading(RE::TESObjectREFR* a_ref, const BSFixedString& a_idl)
		{
			using func_t = decltype(&IsIdleLoading);
			REL::Relocation<func_t> func{ REL::ID(686648) };
			return func(a_ref, a_idl);
		}

		inline bool WillEventChangeState(const TESObjectREFR& ref, const BSFixedString& evn)
		{
			using func_t = decltype(&WillEventChangeState);
			REL::Relocation<func_t> func{ REL::ID(35074) };
			return func(ref, evn);
		}

		inline float GetAnimationMaxDuration(TESObjectREFR* a_refr) {
			using func_t = decltype(&GetAnimationMaxDuration);
			REL::Relocation<func_t> func{ REL::ID(177755) };
			return func(a_refr);
		}

		inline bool GetEventSourcePointersFromGraph(const TESObjectREFR* a_refr, BSScrapArray<BSTEventSource<RE::BSAnimationGraphEvent>*>& a_sourcesOut) {
			using func_t = decltype(&GetEventSourcePointersFromGraph);
			REL::Relocation<func_t> func{ REL::ID(897074) };
			return func(a_refr, a_sourcesOut);
		}

		inline void NotifyGraphSources(const TESObjectREFR* a_refr, RE::BSAnimationGraphEvent& a_event) {
			RE::BSScrapArray<RE::BSTEventSource<RE::BSAnimationGraphEvent>*> sources;
			if (GetEventSourcePointersFromGraph(a_refr, sources)) {
				for (const auto& s : sources) {
					if (s != nullptr)
						s->Notify(a_event);
				}
			}
		}

		inline BGSAction* GetDefaultObjectForActionInitializeToBaseState() {
			using func_t = decltype(&GetDefaultObjectForActionInitializeToBaseState);
			REL::Relocation<func_t> func{ REL::ID(639576) };
			return func();
		}

		inline bool RunActionOnActor(Actor* a_actor, TESActionData& a_action, bool a_unk = false) {
			using func_t = decltype(&RunActionOnActor);
			REL::Relocation<func_t> func{ REL::ID(22408) };
			return func(a_actor, a_action, a_unk);
		}
	};
}
