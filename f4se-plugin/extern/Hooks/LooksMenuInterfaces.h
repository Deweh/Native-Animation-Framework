#pragma once
#pragma warning(push)
#pragma warning(disable: 4100)
#include <fstream>
#include <future>
#include <unordered_map>
#include <unordered_set>

#include "FixedString.h"

#include "F4SE/F4SE.h"
#include "RE/Fallout.h"
#include <F4SE/API.h>

#include <Windows.h>

namespace logger = F4SE::log;

#ifndef LOOKSMENU_1_6_20_CHECKSUM
#	define LOOKSMENU_1_6_20_CHECKSUM 0x59F7081
#endif  // !LOOKSMENU_1_6_20_CHECKSUM


enum LMVersion
{
	k1_6_20,
	k1_6_18
};

inline LMVersion version;

class BodyMorphInterface
{
public:
	virtual void Save(const F4SE::SerializationInterface*, uint32_t);
	virtual bool Load(const F4SE::SerializationInterface*, bool, uint32_t, const std::unordered_map<uint32_t, std::string>&);
	virtual void Revert();

	virtual void LoadBodyGenSliderMods();
	virtual void ClearBodyGenSliders();

	virtual bool LoadBodyGenSliders(const std::string& filePath);

	virtual void ForEachSlider(uint8_t gender, std::function<void(const std::shared_ptr<uint8_t>& slider)> func);

	virtual std::shared_ptr<uint8_t> GetTrishapeMap(const char* relativePath);
	virtual std::shared_ptr<uint8_t> GetMorphMap(RE::Actor* actor, bool isFemale);

	virtual void SetMorph(RE::Actor* actor, bool isFemale, const RE::BSFixedString& morph, RE::BGSKeyword* keyword, float value);
	virtual float GetMorph(RE::Actor* actor, bool isFemale, const RE::BSFixedString& morph, RE::BGSKeyword* keyword);

	virtual void GetKeywords(RE::Actor* actor, bool isFemale, const RE::BSFixedString& morph, std::vector<RE::BGSKeyword*>& keywords);
	virtual void GetMorphs(RE::Actor* actor, bool isFemale, std::vector<RE::BSFixedString>& morphs);
	virtual void RemoveMorphsByName(RE::Actor* actor, bool isFemale, const RE::BSFixedString& morph);
	virtual void RemoveMorphsByKeyword(RE::Actor* actor, bool isFemale, RE::BGSKeyword* keyword);
	virtual void ClearMorphs(RE::Actor* actor, bool isFemale);
	virtual void CloneMorphs(RE::Actor* source, RE::Actor* target);

	virtual void GetMorphableShapes(RE::NiAVObject* node, std::vector<std::shared_ptr<uint8_t>>& shapes);
	virtual bool ApplyMorphsToShapes(RE::Actor* actor, RE::NiAVObject* slotNode);
	virtual bool ApplyMorphsToShape(RE::Actor* actor, const std::shared_ptr<uint8_t>& morphableShape);
	virtual bool UpdateMorphs(RE::Actor* actor);
};

typedef uint32_t UniqueID;

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

		void Save(const F4SESerializationInterface* intfc, uint32_t kVersion);
		bool Load(const F4SESerializationInterface* intfc, uint32_t kVersion, const std::unordered_map<uint32_t, StringTableItem>& stringTable);
	};
	typedef std::shared_ptr<OverlayData> OverlayDataPtr;

	class PriorityMap : public std::multimap<int32_t, OverlayDataPtr>
	{
	public:
		void Save(const F4SESerializationInterface* intfc, uint32_t kVersion);
		bool Load(const F4SESerializationInterface* intfc, bool isFemale, uint32_t kVersion, const std::unordered_map<uint32_t, StringTableItem>& stringTable);
	};
	typedef std::shared_ptr<PriorityMap> PriorityMapPtr;

	class OverlayMap : public std::unordered_map<uint32_t, PriorityMapPtr>
	{
	public:
		void Save(const F4SESerializationInterface* intfc, uint32_t kVersion);
		bool Load(const F4SESerializationInterface* intfc, bool isFemale, uint32_t kVersion, const std::unordered_map<uint32_t, StringTableItem>& stringTable);
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

	virtual void Save(const F4SESerializationInterface* intfc, uint32_t kVersion);
	virtual bool Load(const F4SESerializationInterface* intfc, uint32_t kVersion, const std::unordered_map<uint32_t, StringTableItem>& stringTable);
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
};

uint64_t checksum(const std::string& filename);

class SkinTemplate
{
public:
	SkinTemplate() :
		skin(0), sort(0)
	{
		face[0] = 0;
		face[1] = 0;
		rear[0] = 0;
		rear[1] = 0;
		head[0] = 0;
		head[1] = 0;
		gender = 2;
	}

	F4EEFixedString name;
	uint32_t face[2];
	uint32_t head[2];
	uint32_t rear[2];
	uint32_t skin;
	int32_t sort;
	uint8_t gender;

	RE::BGSTextureSet* GetTextureSet(bool isFemale);
	RE::TESObjectARMO* GetSkinArmor();
	RE::BGSHeadPart* GetHead(bool isFemale);
	RE::BGSHeadPart* GetRearHead(bool isFemale);
};
typedef std::shared_ptr<SkinTemplate> SkinTemplatePtr;

template <typename T>
class SafeDataHolder
{
protected:
	RE::BSSpinLock m_lock;

public:
	T m_data;

	void Lock(void);
	void Release(void);
};

class SkinInterface
{
public:
	enum
	{
		kVersion1 = 1,
		kSerializationVersion = kVersion1,
	};

	virtual void Save(const F4SESerializationInterface* intfc, uint32_t kVersion);
	virtual bool Load(const F4SESerializationInterface* intfc, uint32_t kVersion, const std::unordered_map<uint32_t, StringTableItem>& stringTable);
	virtual void Revert();

	virtual void LoadSkinMods();
	virtual bool LoadSkinTemplates(const std::string& filePath);

	virtual bool AddSkinOverride(RE::Actor* actor, const F4EEFixedString& id, bool isFemale);
	virtual F4EEFixedString GetSkinOverride(RE::Actor* actor);
	virtual SkinTemplatePtr GetSkinTemplate(RE::Actor* actor);
	virtual bool RemoveSkinOverride(RE::Actor* actor);
	virtual void CloneSkinOverride(RE::Actor* source, RE::Actor* target);
	virtual bool UpdateSkinOverride(RE::Actor* actor, bool doFace);

	RE::TESObjectARMO* GetBackupSkin(RE::TESNPC* npc, bool& exists);
	RE::BGSTextureSet* GetBackupFace(RE::Actor* actor, RE::TESNPC* npc, bool isFemale, bool& exists);
	RE::BGSHeadPart* GetBackupHeadPart(RE::Actor* actor, RE::TESNPC* npc, bool isFemale, uint32_t partType);

	virtual void ForEachSkinTemplate(std::function<void(const F4EEFixedString&, const SkinTemplatePtr&)> functor);

	virtual void ClearMods();

protected:
	SafeDataHolder<std::unordered_map<uint32_t, uint32_t>> m_skinBackup;  // Stores a mapping of TESNPC formid to the previous TESObjectARMO
	//SafeDataHolder<std::unordered_map<UInt32, UInt32>>			m_faceBackup[2];	// Stores a mapping of TESNPC formid to the previous BGSTextureSet
	std::unordered_map<F4EEFixedString, SkinTemplatePtr> m_skinTemplates;        // Mapping of template id to template object
	SafeDataHolder<std::unordered_map<uint32_t, StringTableItem>> m_skinOverride;  // Maps Actor to specific Override id
};

class CharGenInterface
{
public:
	CharGenInterface() {}

	virtual DWORD SavePreset(const std::string& filePath);
	virtual DWORD LoadPreset(const std::string& filePath);

	virtual void LoadTintTemplateMods();

	virtual bool LoadTintCategories(const std::string& filePath);
	virtual bool LoadTintTemplates(const std::string& filePath);

	virtual bool SaveTintCategories(const RE::TESRace* race, const std::string& filePath);
	virtual bool SaveTintTemplates(const RE::TESRace* race, const std::string& filePath);

	virtual void LoadHairColorMods();
	virtual bool LoadHairColorData(const std::string& filePath, const ModInfo* modInfo);

	virtual void UnlockHeadParts();
	virtual void UnlockTints();

	virtual void ProcessHairColor(RE::NiAVObject* node, RE::BGSColorForm* colorForm, RE::BSShaderMaterial* /*RE::BSLightingShaderMaterialBase* shaderMaterial*/);
	virtual const char* ProcessEyebrowPath(RE::TESNPC* npc);

	virtual void ClearHairColorMods();

	/*virtual TESNPC::HeadData * ProcessHeadData(TESNPC * npc);

	virtual void SetBaseTintTextureOverride(BGSTextureSet * textureSet);
	void LockBaseTextureOverride();
	void ReleaseBaseTextureOverride();*/

	bool IsLUTUsed(const F4EEFixedString& str);
	bool GetLUTFromColor(RE::BGSColorForm* color, F4EEFixedString& str);

protected:
	std::unordered_set<F4EEFixedString, std::hash<F4EEFixedString>> m_LUTs;
	std::unordered_map<RE::BGSColorForm*, F4EEFixedString> m_LUTMap;

	//TESNPC::HeadData m_faceTextureOverride;
	//std::mutex m_faceTextureLock;

	RE::Actor* GetCurrentActor();
};

class ActorUpdateManager :
	public RE::BSTEventSink<RE::TESInitScriptEvent>,
	public RE::BSTEventSink<RE::TESObjectLoadedEvent>,
	public RE::BSTEventSink<RE::TESLoadGameEvent>
{
public:
	ActorUpdateManager();
	virtual ~ActorUpdateManager();

	virtual RE::BSEventNotifyControl ReceiveEvent(RE::TESObjectLoadedEvent* evn, RE::BSTEventSource<RE::TESObjectLoadedEvent>* a_source);
	virtual RE::BSEventNotifyControl ReceiveEvent(RE::TESInitScriptEvent* evn, RE::BSTEventSource<RE::TESInitScriptEvent>* a_source);

	virtual void Flush();
	virtual void PushUpdate(RE::Actor* actor);
	virtual void Revert();

	void SetLoading(bool loading);
	void ResolvePendingBodyGen();

	RE::BSSpinLock m_pendingLock;

	bool m_loading;                                 // True when the game is loading, false when the cell has loaded
	std::unordered_set<uint64_t> m_pendingActors;   // Stores the pending actors while loading (Populated while loading, erased during load, remaining actors get new morphs, cleared after)
	std::unordered_set<uint64_t> m_pendingUpdates;  // Stores the actors for update
};

RE::BSEventNotifyControl HookedReceiveEventObjectLoaded(ActorUpdateManager* manager, RE::TESObjectLoadedEvent* evn, void* dispatcher);
RE::BSEventNotifyControl HookedReceiveEventInitScript(ActorUpdateManager* manager, RE::TESInitScriptEvent* evn, void* dispatcher);

template <typename T>
class LooksMenuInterfaces
{
public:
	inline static T* GetInterface()
	{
		static_assert(std::is_same<T, BodyMorphInterface>::value || std::is_same<T, OverlayInterface>::value || std::is_same<T, SkinInterface>::value || std::is_same<T, CharGenInterface>::value || std::is_same<T, ActorUpdateManager>::value,
			"Template type must be either BodyMorphInterface/OverlayInterface/SkinInterface/CharGenInterface.");
RETURN_PTR:
		if constexpr (std::is_same<T, BodyMorphInterface>::value) {
			if (g_bodyMorphInterface)
				return g_bodyMorphInterface;
		} else if constexpr (std::is_same<T, OverlayInterface>::value) {
			if (g_OverlayInterface)
				return g_OverlayInterface;
		} else if constexpr (std::is_same<T, SkinInterface>::value) {
			if (g_SkinInterface)
				return g_SkinInterface;
		} else if constexpr (std::is_same<T, CharGenInterface>::value) {
			if (g_CharGenInterface)
				return g_CharGenInterface;
		} else if constexpr (std::is_same<T, ActorUpdateManager>::value) {
			if (g_ActorUpdateManager)
				return g_ActorUpdateManager;
		}
		// Инициализация происходит только один раз
		if (!isInitialized_) {
			std::unique_lock<std::mutex> lock(mutex_);
			if (!isInitialized_)
				InitializeInterfaces();
		}
		goto RETURN_PTR;
	}

private:
	// Статическая функция для получения смещения
	inline static uint32_t get_offset()
	{
		static_assert(std::is_same<T, BodyMorphInterface>::value || std::is_same<T, OverlayInterface>::value || std::is_same<T, SkinInterface>::value || std::is_same<T, CharGenInterface>::value || std::is_same<T, ActorUpdateManager>::value,
			"Template type must be either BodyMorphInterface/OverlayInterface/SkinInterface/CharGenInterface/ActorUpdateManager.");

		if constexpr (std::is_same<T, BodyMorphInterface>::value) {
			return offset_g_bodyMorphInterface;
		} else if constexpr (std::is_same<T, OverlayInterface>::value) {
			return offset_g_OverlayInterface;
		} else if constexpr (std::is_same<T, SkinInterface>::value) {
			return offset_g_SkinInterface;
		} else if constexpr (std::is_same<T, CharGenInterface>::value) {
			return offset_g_CharGenInterface;
		} else if constexpr (std::is_same<T, ActorUpdateManager>::value) {
			return offset_g_ActorUpdateManager;
		}

		return 0;  // Не должно происходить, но добавлено для избежания предупреждений компилятора
	}

	inline static void InitializeInterfaces()
	{
		initPromise_ = std::promise<void>();
		initFuture_ = initPromise_.get_future();

		// Поток для инициализации
		std::thread([&]() {
			initPromise_.set_value();
		}).detach();

		initFuture_.wait();

		// Проверяем, установлен ли LooksMenu, только один раз
		if (F4SE::GetPluginInfo("F4EE").has_value() && !LooksMenu_isInstalled) {
			// Получаем контрольную сумму
			if (auto sum = checksum("Data/F4SE/Plugins/f4ee.dll"); sum > 0) {
				LooksMenu_isInstalled = true;
				std::string verStr = "";

				switch (sum) {
				case LOOKSMENU_1_6_20_CHECKSUM:
					version = k1_6_20;
					verStr = "1.6.20";
					break;
				default:
					LooksMenu_isInstalled = false;
				}

				if (LooksMenu_isInstalled) {
					logger::info("LooksMenu version {} detected.", verStr);
					HMODULE module = GetModuleHandleA("f4ee.dll");
					if (module == nullptr) {
						logger::error("Error: модуль f4ee.dll не найден");
						return;
					}

					uintptr_t baseAddr = reinterpret_cast<uintptr_t>(module);

					if (!g_bodyMorphInterface) {
						g_bodyMorphInterface = reinterpret_cast<BodyMorphInterface*>(baseAddr + offset_g_bodyMorphInterface);
						if (g_bodyMorphInterface) {
							logger::info("{} : initialized", typeid(*g_bodyMorphInterface).name());
						} else {
							logger::error("g_bodyMorphInterface is nullptr");
						}
					}
					if (!g_OverlayInterface) {
						g_OverlayInterface = reinterpret_cast<OverlayInterface*>(baseAddr + offset_g_OverlayInterface);
						if (g_OverlayInterface) {
							logger::info("{} : initialized", typeid(*g_OverlayInterface).name());
						} else {
							logger::error("g_OverlayInterface is nullptr");
						}
					}
					if (!g_SkinInterface) {
						g_SkinInterface = reinterpret_cast<SkinInterface*>(baseAddr + offset_g_SkinInterface);
						if (g_SkinInterface) {
							logger::info("{} : initialized", typeid(*g_SkinInterface).name());
						} else {
							logger::error("g_SkinInterface is nullptr");
						}
					}
					if (!g_CharGenInterface) {
						g_CharGenInterface = reinterpret_cast<CharGenInterface*>(baseAddr + offset_g_CharGenInterface);
						if (g_CharGenInterface) {
							logger::info("{} : initialized", typeid(*g_CharGenInterface).name());
						} else {
							logger::error("g_CharGenInterface is nullptr");
						}
					}
					if (!g_ActorUpdateManager) {
						g_ActorUpdateManager = reinterpret_cast<ActorUpdateManager*>(baseAddr + offset_g_ActorUpdateManager);
						if (g_ActorUpdateManager) {
							logger::info("{} : initialized", typeid(*g_ActorUpdateManager).name());
						} else {
							logger::error("g_ActorUpdateManager is nullptr");
						}
					}
					isInitialized_ = true;
				} else {
					logger::error("Unsupported LooksMenu version. Checksum: {:X}", sum);
				}
			} else {
				logger::error("Failed to get LooksMenu version, support disabled.");
			}
		} else {
			logger::error("LooksMenu not installed, support disabled.");
		}
	}

	inline static std::promise<void> initPromise_;
	inline static std::future<void> initFuture_;
	inline static std::mutex mutex_;
	inline static bool isInitialized_ = false;
	inline static BodyMorphInterface* g_bodyMorphInterface = nullptr;
	inline static OverlayInterface* g_OverlayInterface = nullptr;
	inline static SkinInterface* g_SkinInterface = nullptr;
	inline static CharGenInterface* g_CharGenInterface = nullptr;
	inline static ActorUpdateManager* g_ActorUpdateManager = nullptr;
	inline static bool LooksMenu_isInstalled = false;
	inline static std::string version;
	inline static const std::string k1_6_20 = "1.6.20";

	// Смещения для интерфейсов
	inline constinit static uint32_t offset_g_CharGenInterface = 0xF4BC0;
	inline constinit static uint32_t offset_g_BodygenInterface = 0xF4D20;
	inline constinit static uint32_t offset_g_bodyMorphInterface = 0xF4DF0;
	inline constinit static uint32_t offset_g_OverlayInterface = 0xF5040;
	inline constinit static uint32_t offset_g_SkinInterface = 0xF4F60;
	inline constinit static uint32_t offset_g_ActorUpdateManager = 0xF4C70; 
};
#pragma warning(pop)

