#pragma once
#include <fstream>
#include "Hooks/FixedString.h"
#pragma warning(push)
#pragma warning(disable: 4100)

namespace LooksMenu
{
	//NAFBRIDGE for f4ee removeOverlay fix. https://www.nexusmods.com/fallout4/mods/91235
	typedef uint32_t UniqueID;

	typedef std::shared_ptr<F4EEFixedString> StringTableItem;
	typedef std::weak_ptr<F4EEFixedString> WeakTableItem;

	class OverlayInterface : public RE::BSTEventSink<RE::TESObjectLoadedEvent>, public RE::BSTEventSink<RE::TESLoadGameEvent>
	{
	public:
		OverlayInterface() :
			m_highestUID(0) {}

		typedef uint32_t UniqueID;

		enum
		{
			kVersion1 = 1,
			kVersion2 = 2,  // Version 2 now only saves uint32_t FormID instead of UInt64 Handle
			kSerializationVersion = kVersion2,
		};

		class OverlayData
		{
		public:
			enum Flags
			{
				kHasTintColor = (1 << 0),
				kHasOffsetUV = (1 << 1),
				kHasScaleUV = (1 << 2),
				kHasRemapIndex = (1 << 3)
			};

			OverlayData()
			{
				uid = 0;
				flags = 0;
				tintColor.r = 0.0f;
				tintColor.g = 0.0f;
				tintColor.b = 0.0f;
				tintColor.a = 0.0f;
				offsetUV.x = 0.0f;
				offsetUV.y = 0.0f;
				scaleUV.x = 1.0f;
				scaleUV.y = 1.0f;
				remapIndex = 0.50196f;
			}

			UniqueID uid;
			uint32_t flags;
			StringTableItem templateName;
			RE::NiColorA tintColor;
			RE::NiPoint2 offsetUV;
			RE::NiPoint2 scaleUV;
			float remapIndex;

			void UpdateFlags();

			void Save(const F4SE::SerializationInterface* intfc, uint32_t kVersion);
			bool Load(const F4SE::SerializationInterface* intfc, uint32_t kVersion, const std::unordered_map<uint32_t, StringTableItem>& stringTable);
		};
		typedef std::shared_ptr<OverlayData> OverlayDataPtr;

		class PriorityMap : public std::multimap<int32_t, OverlayDataPtr>
		{
		public:
			void Save(const F4SE::SerializationInterface* intfc, uint32_t kVersion);
			bool Load(const F4SE::SerializationInterface* intfc, bool isFemale, uint32_t kVersion, const std::unordered_map<uint32_t, StringTableItem>& stringTable);
		};
		typedef std::shared_ptr<PriorityMap> PriorityMapPtr;

		class OverlayMap : public std::unordered_map<uint32_t, PriorityMapPtr>
		{
		public:
			void Save(const F4SE::SerializationInterface* intfc, uint32_t kVersion);
			bool Load(const F4SE::SerializationInterface* intfc, bool isFemale, uint32_t kVersion, const std::unordered_map<uint32_t, StringTableItem>& stringTable);
		};

		class OverlayTemplate
		{
		public:
			OverlayTemplate() :
				playable(false), sort(0), transformable(false), tintable(false) {}

			typedef std::unordered_map<uint32_t, std::pair<F4EEFixedString, bool>> MaterialMap;

			F4EEFixedString displayName;
			MaterialMap slotMaterial;
			bool playable;
			bool transformable;
			bool tintable;
			int32_t sort;
		};
		typedef std::shared_ptr<OverlayTemplate> OverlayTemplatePtr;

		virtual void Save(const F4SE::SerializationInterface* intfc, uint32_t kVersion);
		virtual bool Load(const F4SE::SerializationInterface* intfc, uint32_t kVersion, const std::unordered_map<uint32_t, StringTableItem>& stringTable);
		virtual void Revert();

		virtual void LoadOverlayMods();
		virtual void ClearMods();
		virtual bool LoadOverlayTemplates(const std::string& filePath);

		virtual UniqueID AddOverlay(RE::Actor* actor, bool isFemale, int32_t priority, const F4EEFixedString& templateName, const RE::NiColorA& tintColor, const RE::NiPoint2& offsetUV, const RE::NiPoint2& scaleUV);
		virtual bool RemoveOverlay(RE::Actor* actor, bool isFemale, UniqueID uid);
		virtual bool RemoveAll(RE::Actor* actor, bool isFemale);
		virtual bool ReorderOverlay(RE::Actor* actor, bool isFemale, UniqueID uid, int32_t newPriority);

		virtual bool ForEachOverlay(RE::Actor* actor, bool isFemale, std::function<void(int32_t, const OverlayDataPtr&)> functor);
		virtual bool ForEachOverlayBySlot(RE::Actor* actor, bool isFemale, uint32_t slotIndex, std::function<void(int32_t, const OverlayDataPtr&, const F4EEFixedString&, bool)> functor);

		virtual void ForEachOverlayTemplate(bool isFemale, std::function<void(const F4EEFixedString&, const OverlayTemplatePtr&)> functor);

		virtual bool UpdateOverlays(RE::Actor* actor);
		virtual bool UpdateOverlay(RE::Actor* actor, uint32_t slotIndex);

		virtual void CloneOverlays(RE::Actor* source, RE::Actor* target);

		virtual UniqueID GetNextUID();

		virtual RE::NiNode* GetOverlayRoot(RE::Actor* actor, RE::NiNode* rootNode, bool createIfNecessary = true);

		virtual const OverlayTemplatePtr GetTemplateByName(bool isFemale, const F4EEFixedString& name);
		virtual const OverlayDataPtr GetOverlayByUID(UniqueID uid);

		std::pair<int32_t, OverlayDataPtr> GetActorOverlayByUID(RE::Actor* actor, bool isFemale, UniqueID uid);

		bool HasSkinChildren(RE::NiAVObject* slot);
		void LoadMaterialData(RE::TESNPC* npc, RE::BSTriShape* shape, const F4EEFixedString& material, bool effect, const OverlayDataPtr& overlayData);

		void DestroyOverlaySlot(RE::Actor* actor, RE::NiNode* overlayHolder, uint32_t slotIndex);
		bool UpdateOverlays(RE::Actor* actor, RE::NiNode* rootNode, RE::NiAVObject* object, uint32_t slotIndex);

	protected:
		friend class OverlayTemplate;
		friend class PriorityMap;
		friend class OverlayData;

		RE::BSSpinLock m_overlayLock;
		OverlayMap m_overlays[2];
		std::vector<UniqueID> m_freeIndices;
		std::unordered_map<UniqueID, OverlayDataPtr> m_dataMap;
		UniqueID m_highestUID;
		std::unordered_map<F4EEFixedString, OverlayTemplatePtr> m_overlayTemplates[2];
		friend bool HookedRemoveOverlay(OverlayInterface* OverlayInterface, RE::Actor* actor, bool isFemale, UniqueID uid);

		// RemoveAll
		friend bool HookedRemoveAll(OverlayInterface* OverlayInterface, RE::Actor* actor, bool isFemale);
	};

	bool HookedRemoveOverlay(OverlayInterface* overlayInterface, RE::Actor* actor, bool isFemale, UniqueID uid)
	{
		RE::BSSpinLock locker(overlayInterface->m_overlayLock);
		auto& overlays = overlayInterface->m_overlays[isFemale ? 1 : 0];
		auto hit = overlays.find(actor->formID);
		if (hit == overlays.end())
			return false;

		OverlayInterface::PriorityMapPtr priorityMap = hit->second;
		if (!priorityMap)
			return false;

		for (auto it = priorityMap->begin(); it != priorityMap->end(); ++it) {
			OverlayInterface::OverlayDataPtr overlayPtr = it->second;
			if (!overlayPtr || overlayPtr->uid != uid)
				continue;

			overlayInterface->m_dataMap.erase(overlayPtr->uid);
			overlayInterface->m_freeIndices.push_back(overlayPtr->uid);
			it = priorityMap->erase(it);
			return true;
		}
		return false;
	}

	//NAFBRIDGE END
	enum LMVersion
	{
		k1_6_20,
		k1_6_18
	};

	class BodyMorphInterface
	{
	public:
		virtual void Save(const F4SE::SerializationInterface*, uint32_t) {}
		virtual bool Load(const F4SE::SerializationInterface*, bool, uint32_t, const std::unordered_map<uint32_t, std::string>&) { return false; }
		virtual void Revert() {}

		virtual void LoadBodyGenSliderMods() {}
		virtual void ClearBodyGenSliders() {}

		virtual bool LoadBodyGenSliders(const std::string& filePath) { return false; }

		virtual void ForEachSlider(uint8_t gender, std::function<void(const std::shared_ptr<uint8_t>& slider)> func) {}

		virtual std::shared_ptr<uint8_t> GetTrishapeMap(const char* relativePath) { return nullptr; }
		virtual std::shared_ptr<uint8_t> GetMorphMap(RE::Actor* actor, bool isFemale) { return nullptr; }

		virtual void SetMorph(RE::Actor* actor, bool isFemale, const RE::BSFixedString& morph, RE::BGSKeyword* keyword, float value) {}
		virtual float GetMorph(RE::Actor* actor, bool isFemale, const RE::BSFixedString& morph, RE::BGSKeyword* keyword) { return 0.0f; }

		virtual void GetKeywords(RE::Actor* actor, bool isFemale, const RE::BSFixedString& morph, std::vector<RE::BGSKeyword*>& keywords) {}
		virtual void GetMorphs(RE::Actor* actor, bool isFemale, std::vector<RE::BSFixedString>& morphs) {}
		virtual void RemoveMorphsByName(RE::Actor* actor, bool isFemale, const RE::BSFixedString& morph) {}
		virtual void RemoveMorphsByKeyword(RE::Actor* actor, bool isFemale, RE::BGSKeyword* keyword) {}
		virtual void ClearMorphs(RE::Actor* actor, bool isFemale) {}
		virtual void CloneMorphs(RE::Actor* source, RE::Actor* target) {}

		virtual void GetMorphableShapes(RE::NiAVObject* node, std::vector<std::shared_ptr<uint8_t>>& shapes) {}
		virtual bool ApplyMorphsToShapes(RE::Actor* actor, RE::NiAVObject* slotNode) { return false; }
		virtual bool ApplyMorphsToShape(RE::Actor* actor, const std::shared_ptr<uint8_t>& morphableShape) { return false; }
		virtual bool UpdateMorphs(RE::Actor* actor) { return false; }
	};

	uint64_t checksum(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::binary);
		if (!file) {
			logger::warn("Error opening file: {}", filename);
			return 0;
		}

		std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(file), {});

		uint64_t sum = 0;
		for (uint8_t byte : buffer) {
			sum += byte;
		}

		return sum;
	}

	bool isInstalled = false;
	LMVersion version;

	namespace detail
	{
		typedef bool(ApplyMorphsToShapes)(BodyMorphInterface*, RE::Actor*, RE::NiAVObject*);
		typedef void(LoadBodyGenSliderMods)(BodyMorphInterface*);
		using RemoveOverlayHandler = bool (*)(OverlayInterface*, bool, UniqueID);  //NAF Bridge F4EE Remove Overlay fix
		using RemoveAllHandler = bool (*)(OverlayInterface*, RE::Actor*, bool);    // Hook for RemoveAll (vtable index 10)

		DetourXS applyRemoveAllHook;  // Hook for RemoveAll
		DetourXS applyMorphsHook;
		DetourXS applyRemoveOverlayHook;  //NAF Bridge F4EE Remove Overlay fix
		ApplyMorphsToShapes* originalApply;

		BodyMorphInterface* g_bodyMorphInterface = nullptr;
		RemoveOverlayHandler originalRemoveOverlay = nullptr;
		RemoveAllHandler originalRemoveAll = nullptr;  // RemoveAll hook


		uint64_t GetVersionOffset(uint64_t off1_6_18, uint64_t off1_6_20)
		{
			switch (version) {
			case k1_6_18:
				return off1_6_18;
			case k1_6_20:
				return off1_6_20;
			default:
				return 0;
			}
		}

		bool HookedRemoveAll(OverlayInterface* overlayInterface, RE::Actor* actor, bool isFemale)
		{
			if (originalRemoveAll) {
				logger::info("HookedRemoveAll called for actor: {}, isFemale: {}", actor ? std::to_string(actor->formID) : "null", isFemale);
				return originalRemoveAll(overlayInterface, actor, isFemale);
			}
			// If trampoline isn't available, fall back to false to avoid undefined behavior.
			return false;
		}

		bool HookedApplyMorphs(BodyMorphInterface* a1, RE::Actor* a2, RE::NiAVObject* a3)
		{
			std::this_thread::sleep_for(std::chrono::microseconds(1));
			return originalApply(a1, a2, a3);
		}

		void RegisterHook()
		{
			HMODULE module = GetModuleHandleA("f4ee.dll");
			if (!module) {
				logger::warn("GetModuleHandleA(\"f4ee.dll\") failed");
				return;
			}

			uintptr_t baseAddr = reinterpret_cast<uintptr_t>(module);

			// ApplyMorphs hook (unchanged)
			if (!applyMorphsHook.Create(reinterpret_cast<LPVOID>(baseAddr + GetVersionOffset(0x10040, 0xFFCC)), &HookedApplyMorphs)) {
				logger::warn("Failed to create ApplyMorphsToShapes hook!");
			} else {
				logger::info("Create ApplyMorphsToShapes hook success!");
				originalApply = reinterpret_cast<ApplyMorphsToShapes*>(applyMorphsHook.GetTrampoline());
			}

			// Body morph interface pointer (unchanged)
			g_bodyMorphInterface = reinterpret_cast<BodyMorphInterface*>(baseAddr + GetVersionOffset(0xFB440, 0xF4DF0));

			// --- NAF Bridge F4EE Remove Overlay fix ---
			// Replace the hardcoded offset below with GetVersionOffset(...) if you know the alternate offset for other versions.
			
			constexpr uintptr_t removeOverlayInstanceOffset = 0xF5040;  // replace with GetVersionOffset(a, b) if appropriate
			constexpr size_t removeOverlayVTableIndex = 9;

			uintptr_t instanceAddr = baseAddr + removeOverlayInstanceOffset;
			if (instanceAddr == 0) {
				logger::warn("instance address is null, cannot create applyRemoveOverlayHook hook!");
				return;
			}

			// Read vtable pointer (first field of the object)
			uintptr_t* vtable = nullptr;
			// Defensive check: ensure the instance pointer is sane
			if (instanceAddr < 0x10000) {  // simple sanity check — adapt as needed
				logger::warn("instanceAddr appears invalid, aborting remove overlay hook creation");
				return;
			}

			// NOTE: dereferencing arbitrary addresses can crash if wrong; make sure instanceAddr is correct for this process.
			vtable = *reinterpret_cast<uintptr_t**>(instanceAddr);
			if (vtable == nullptr) {
				logger::warn("vtable is null, cannot create hook!");
				return;
			}

			uintptr_t funcAddr = vtable[removeOverlayVTableIndex];
			if (funcAddr == 0) {
				logger::warn("vtable[9] is null, cannot create hook!");
				return;
			}

			LPVOID target = reinterpret_cast<LPVOID>(funcAddr);

			if (!applyRemoveOverlayHook.Create(target, &HookedRemoveOverlay)) {
				logger::warn("Failed to create applyRemoveOverlayHook hook!");
			} else {
				logger::info("Create applyRemoveOverlayHook hook success!");
				originalRemoveOverlay = reinterpret_cast<RemoveOverlayHandler>(applyRemoveOverlayHook.GetTrampoline());
			}

			// REMOVE ALL HOOK
			/*size_t removeAllVTableIndex = 10;
			uintptr_t funcAddrRemoveAll = vtable[removeAllVTableIndex];
			if (funcAddrRemoveAll == 0) {
				logger::warn("vtable[10] is null, cannot create RemoveAll hook!");
			} else {
				LPVOID targetAll = reinterpret_cast<LPVOID>(funcAddrRemoveAll);

				if (!applyRemoveAllHook.Create(targetAll, &HookedRemoveAll)) {
					logger::warn("Failed to create applyRemoveAllHook hook!");
				} else {
					logger::info("Create applyRemoveAllHook hook success!");
					originalRemoveAll = reinterpret_cast<RemoveAllHandler>(applyRemoveAllHook.GetTrampoline());
				}
			}*/
		}
	}

	void Init()
	{
		if (F4SE::GetPluginInfo("F4EE").has_value()) {
			// Unfortunately expired has never changed F4EE's plugin version, so we gotta do this the old fashioned way.
			if (auto sum = checksum("Data/F4SE/Plugins/f4ee.dll"); sum > 0) {
				isInstalled = true;
				std::string verStr = "";

				switch (sum) {
				case LOOKSMENU_1_6_18_CHECKSUM:
					version = k1_6_18;
					verStr = "1.6.18";
					break;
				case LOOKSMENU_1_6_20_CHECKSUM:
					version = k1_6_20;
					verStr = "1.6.20";
					break;
				default:
					isInstalled = false;
				}

				if (isInstalled) {
					logger::info("LooksMenu version {} detected.", verStr);
					detail::RegisterHook();
				} else {
					logger::info("Unsupported LooksMenu version. Checksum: {:X}", sum);
				}

			} else {
				logger::warn("Failed to get LooksMenu version, support disabled.");
			}
		} else {
			logger::info("LooksMenu not installed, support disabled.");
		}
	}

	void SetMorph(RE::Actor* actor, const RE::BSFixedString& morph, RE::BGSKeyword* keyword, float value)
	{
		if (isInstalled && actor && detail::g_bodyMorphInterface) {
			bool isFemale = (actor->GetSex() == 1);
			detail::g_bodyMorphInterface->SetMorph(actor, isFemale, morph, keyword, value);
		}
	}

	void UpdateMorphs(RE::Actor* actor)
	{
		if (isInstalled && actor && detail::g_bodyMorphInterface) {
			detail::g_bodyMorphInterface->UpdateMorphs(actor);
		}
	}

	void RemoveMorphsByName(RE::Actor* actor, const RE::BSFixedString& morph)
	{
		if (isInstalled && actor && detail::g_bodyMorphInterface) {
			bool isFemale = (actor->GetSex() == RE::Actor::Sex::Female);
			detail::g_bodyMorphInterface->RemoveMorphsByName(actor, isFemale, morph);
		}
	}

	void RemoveMorphsByKeyword(RE::Actor* actor, RE::BGSKeyword* keyword)
	{
		if (isInstalled && actor && detail::g_bodyMorphInterface) {
			bool isFemale = (actor->GetSex() == RE::Actor::Sex::Female);
			detail::g_bodyMorphInterface->RemoveMorphsByKeyword(actor, isFemale, keyword);
		}
	}

	float GetMorph(RE::Actor* actor, const RE::BSFixedString& morph, RE::BGSKeyword* keyword)
	{
		if (isInstalled && actor && detail::g_bodyMorphInterface) {
			bool isFemale = (actor->GetSex() == RE::Actor::Sex::Female);
			return detail::g_bodyMorphInterface->GetMorph(actor, isFemale, morph, keyword);
		}
		return 0.0f;
	}
}

#pragma warning(pop)
