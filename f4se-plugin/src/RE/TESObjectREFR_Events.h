#pragma once

namespace RE
{
	struct TESActorLocationChangeEvent
	{
		NiPointer<RE::TESObjectREFR> refr;
		BGSLocation* oldLocation;
		BGSLocation* newLocation;
	};

	struct TESPackageEvent
	{
		enum PackageEventType
		{
			Begin,
			End,
			Change
		};

		NiPointer<TESObjectREFR> refr;
		uint32_t formId;
		PackageEventType type;
	};

	struct DamageImpactData
	{
		NiPoint4 hitPos;
		NiPoint4 hitDirection;
		NiPoint4 projectileDir;
		bhkNPCollisionObject* collisionObj;
	};
	static_assert(sizeof(DamageImpactData) == 0x38);

	class VATSCommand;
	class HitData
	{
	public:
		void SetAllDamageToZero()
		{
			flags &= 0xFFFFFE07;
			calculatedBaseDamage = 0.f;
			blockedDamage = 0.f;
			reducedDamage = 0.f;
			blockStaggerMult = 0.f;
		}

		DamageImpactData impactData;
		int8_t gap38[8];
		ObjectRefHandle attackerHandle;
		ObjectRefHandle victimHandle;
		ObjectRefHandle sourceHandle;
		int8_t gap4C[4];
		BGSAttackData* attackData;
		BGSObjectInstance source;
		MagicItem* effect;
		SpellItem* spellItem;
		VATSCommand* VATScmd;
		TESAmmo* ammo;
		BSTArray<BSTTuple<TESForm*, BGSTypedFormValuePair::SharedVal>>* damageTypes;
		float calculatedBaseDamage;
		float baseDamage;
		float totalDamage;
		float blockedDamage;
		float blockMult;
		float reducedDamage;
		float field_A8;
		float blockStaggerMult;
		float sneakAttackMult;
		float field_B4;
		float field_B8;
		float field_BC;
		float criticalDamageMult;
		uint32_t flags;
		BGSEquipIndex equipIndex;
		uint32_t materialType;
		int32_t bodypartType;
		int8_t gapD4[4];
	};
	static_assert(sizeof(HitData) == 0xD8);

	class TESHitEvent
	{
	public:
		HitData hitdata;
		int8_t gapD8[8];
		TESObjectREFR* victim;
		TESObjectREFR* attacker;
		BSFixedString matName;
		uint32_t sourceFormID;
		uint32_t projFormID;
		bool hasHitData;
		int8_t gapD1[7];
	};
	static_assert(sizeof(TESHitEvent) == 0x108);

	namespace TESObjectREFR_Events
	{
		void RegisterForPackage(BSTEventSink<TESPackageEvent>* a_sink) {
			using func_t = decltype(&RegisterForPackage);
			REL::Relocation<func_t> func{ REL::ID(1448133) };
			return func(a_sink);
		}

		void UnregisterForPackage(BSTEventSink<TESPackageEvent>* a_sink) {
			using func_t = decltype(&UnregisterForPackage);
			REL::Relocation<func_t> func{ REL::ID(1182113) };
			return func(a_sink);
		}

		void RegisterForHit(BSTEventSink<TESHitEvent>* a_sink)
		{
			using func_t = decltype(&RegisterForHit);
			REL::Relocation<func_t> func{ REL::ID(1240328) };
			return func(a_sink);
		}

		void UnregisterForHit(BSTEventSink<TESHitEvent>* a_sink)
		{
			using func_t = decltype(&UnregisterForHit);
			REL::Relocation<func_t> func{ REL::ID(973940) };
			return func(a_sink);
		}

		void RegisterForDeath(BSTEventSink<TESDeathEvent>* a_sink) {
			using func_t = decltype(&RegisterForDeath);
			REL::Relocation<func_t> func{ REL::ID(33618) };
			return func(a_sink);
		}

		void RegisterForActorLocationChange(BSTEventSink<TESActorLocationChangeEvent>* a_sink)
		{
			using func_t = decltype(&RegisterForActorLocationChange);
			REL::Relocation<func_t> func{ REL::ID(78267) };
			return func(a_sink);
		}
	}
}
