#pragma once

namespace RE
{
	class ActionQueue;

	class __declspec(novtable) TESActionData :
		public ActionInput
	{
	public:
		static constexpr auto RTTI{ RTTI::TESActionData };
		static constexpr auto VTABLE{ VTABLE::TESActionData };

		TESActionData(ACTIONPRIORITY a_priority, TESObjectREFR* a_refr, BGSAction* a_action)
		{
			stl::emplace_vtable(this);
			priority = a_priority;
			ref = RE::NiPointer<RE::TESObjectREFR>(a_refr);
			action = a_action;
		}

		virtual ~TESActionData(){};

		virtual ActorState* GetActorState() { return nullptr; }
		virtual ActionQueue* GetActionQueue() { return nullptr; }
		virtual BGSAnimationSequencer* GetAnimationSequencer() { return nullptr; }
		virtual TESActionData* CreateCopy() { return nullptr; }
		virtual bool DoIt() { return false; }

		BSFixedString unkStr01;
		BSFixedString unkStr02;
		int32_t unk01 = 0;
		int64_t unk02 = 0;
		int64_t unk03 = 0;
		int32_t unk04 = 0;
		int32_t unk05 = 0;
		uint8_t padding[32];
	};
}
