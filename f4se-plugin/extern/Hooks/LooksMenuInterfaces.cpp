#include "LooksMenuInterfaces.h"

void BodyMorphInterface::Save(const F4SE::SerializationInterface*, uint32_t) {}
bool BodyMorphInterface::Load(const F4SE::SerializationInterface*, bool, uint32_t, const std::unordered_map<uint32_t, std::string>&) { return false; }
void BodyMorphInterface::Revert() {}

void BodyMorphInterface::LoadBodyGenSliderMods()
{}
void BodyMorphInterface::ClearBodyGenSliders() {}

bool BodyMorphInterface::LoadBodyGenSliders(const std::string& filePath) { return false; }

void BodyMorphInterface::ForEachSlider(uint8_t gender, std::function<void(const std::shared_ptr<uint8_t>& slider)> func) {}

std::shared_ptr<uint8_t> BodyMorphInterface::GetTrishapeMap(const char* relativePath) { return nullptr; }
std::shared_ptr<uint8_t> BodyMorphInterface::GetMorphMap(RE::Actor* actor, bool isFemale) { return nullptr; }

void BodyMorphInterface::SetMorph(RE::Actor* actor, bool isFemale, const RE::BSFixedString& morph, RE::BGSKeyword* keyword, float value) {}
float BodyMorphInterface::GetMorph(RE::Actor* actor, bool isFemale, const RE::BSFixedString& morph, RE::BGSKeyword* keyword) { return 0.0f; }

void BodyMorphInterface::GetKeywords(RE::Actor* actor, bool isFemale, const RE::BSFixedString& morph, std::vector<RE::BGSKeyword*>& keywords) {}
void BodyMorphInterface::GetMorphs(RE::Actor* actor, bool isFemale, std::vector<RE::BSFixedString>& morphs) {}
void BodyMorphInterface::RemoveMorphsByName(RE::Actor* actor, bool isFemale, const RE::BSFixedString& morph) {}
void BodyMorphInterface::RemoveMorphsByKeyword(RE::Actor* actor, bool isFemale, RE::BGSKeyword* keyword) {}
void BodyMorphInterface::ClearMorphs(RE::Actor* actor, bool isFemale) {}
void BodyMorphInterface::CloneMorphs(RE::Actor* source, RE::Actor* target) {}

void BodyMorphInterface::GetMorphableShapes(RE::NiAVObject* node, std::vector<std::shared_ptr<uint8_t>>& shapes) {}
bool BodyMorphInterface::ApplyMorphsToShapes(RE::Actor* actor, RE::NiAVObject* slotNode) { return false; }
bool BodyMorphInterface::ApplyMorphsToShape(RE::Actor* actor, const std::shared_ptr<uint8_t>& morphableShape) { return false; }
bool BodyMorphInterface::UpdateMorphs(RE::Actor* actor) { return false; }

void OverlayInterface::OverlayData::UpdateFlags() {}
void OverlayInterface::OverlayData::Save(const F4SESerializationInterface* intfc, uint32_t kVersion){};
bool OverlayInterface::OverlayData::Load(const F4SESerializationInterface* intfc, uint32_t kVersion, const std::unordered_map<uint32_t, StringTableItem>& stringTable){};

void OverlayInterface::PriorityMap::Save(const F4SESerializationInterface* intfc, uint32_t kVersion){};
bool OverlayInterface::PriorityMap::Load(const F4SESerializationInterface* intfc, bool isFemale, uint32_t kVersion, const std::unordered_map<uint32_t, StringTableItem>& stringTable){};

void OverlayInterface::OverlayMap::Save(const F4SESerializationInterface* intfc, uint32_t kVersion){};
bool OverlayInterface::OverlayMap::Load(const F4SESerializationInterface* intfc, bool isFemale, uint32_t kVersion, const std::unordered_map<uint32_t, StringTableItem>& stringTable) { return false; };

void OverlayInterface::Save(const F4SESerializationInterface* intfc, uint32_t kVersion){};
bool OverlayInterface::Load(const F4SESerializationInterface* intfc, uint32_t kVersion, const std::unordered_map<uint32_t, StringTableItem>& stringTable) { return false; };
void OverlayInterface::Revert(){};

void OverlayInterface::LoadOverlayMods(){};
void OverlayInterface::ClearMods() {}
bool OverlayInterface::LoadOverlayTemplates(const std::string& filePath) { return false; };

UniqueID OverlayInterface::AddOverlay(RE::Actor* actor, bool isFemale, int32_t priority, const F4EEFixedString& templateName, const RE::NiColorA& tintColor, const RE::NiPoint2& offsetUV, const RE::NiPoint2& scaleUV) { return UniqueID{}; };
bool OverlayInterface::RemoveOverlay(RE::Actor* actor, bool isFemale, UniqueID uid) { return bool{}; };
bool OverlayInterface::RemoveAll(RE::Actor* actor, bool isFemale) { return bool{}; };
bool OverlayInterface::ReorderOverlay(RE::Actor* actor, bool isFemale, UniqueID uid, int32_t newPriority) { return bool{}; };

bool OverlayInterface::ForEachOverlay(RE::Actor* actor, bool isFemale, std::function<void(int32_t, const OverlayDataPtr&)> functor) { return bool{}; };
bool OverlayInterface::ForEachOverlayBySlot(RE::Actor* actor, bool isFemale, uint32_t slotIndex, std::function<void(int32_t, const OverlayDataPtr&, const F4EEFixedString&, bool)> functor) { return bool{}; };

void OverlayInterface::ForEachOverlayTemplate(bool isFemale, std::function<void(const F4EEFixedString&, const OverlayTemplatePtr&)> functor){};

bool OverlayInterface::UpdateOverlays(RE::Actor* actor) { return bool{}; };
bool OverlayInterface::UpdateOverlay(RE::Actor* actor, uint32_t slotIndex) { return bool{}; };

void OverlayInterface::CloneOverlays(RE::Actor* source, RE::Actor* target){};

UniqueID OverlayInterface::GetNextUID() { return UniqueID{}; };

RE::NiNode* OverlayInterface::GetOverlayRoot(RE::Actor* actor, RE::NiNode* rootNode, bool createIfNecessary) { return nullptr; };

const OverlayInterface::OverlayTemplatePtr OverlayInterface::GetTemplateByName(bool isFemale, const F4EEFixedString& name) { return OverlayTemplatePtr{}; };
const OverlayInterface::OverlayDataPtr OverlayInterface::GetOverlayByUID(UniqueID uid) { return OverlayDataPtr{}; };

std::pair<int32_t, OverlayInterface::OverlayDataPtr> OverlayInterface::GetActorOverlayByUID(RE::Actor* actor, bool isFemale, UniqueID uid) { return std::pair<int32_t, OverlayDataPtr>{}; };

bool OverlayInterface::HasSkinChildren(RE::NiAVObject* slot) { return bool{}; };
void OverlayInterface::LoadMaterialData(RE::TESNPC* npc, RE::BSTriShape* shape, const F4EEFixedString& material, bool effect, const OverlayDataPtr& overlayData){};

void OverlayInterface::DestroyOverlaySlot(RE::Actor* actor, RE::NiNode* overlayHolder, uint32_t slotIndex){};
bool OverlayInterface::UpdateOverlays(RE::Actor* actor, RE::NiNode* rootNode, RE::NiAVObject* object, uint32_t slotIndex) { return bool{}; };

RE::BGSTextureSet* SkinTemplate::GetTextureSet(bool isFemale) { return nullptr; };
RE::TESObjectARMO* SkinTemplate::GetSkinArmor() { return nullptr; };
RE::BGSHeadPart* SkinTemplate::GetHead(bool isFemale) { return nullptr; };
RE::BGSHeadPart* SkinTemplate::GetRearHead(bool isFemale) { return nullptr; };

template <typename T>
void SafeDataHolder<T>::Lock(void) { m_lock.Lock(); }

template <typename T>
void SafeDataHolder<T>::Release(void) { m_lock.Release(); }

void SkinInterface::Save(const F4SESerializationInterface* intfc, uint32_t kVersion){};
bool SkinInterface::Load(const F4SESerializationInterface* intfc, uint32_t kVersion, const std::unordered_map<uint32_t, StringTableItem>& stringTable) { return bool{}; };
void SkinInterface::Revert(){};

void SkinInterface::LoadSkinMods(){};
bool SkinInterface::LoadSkinTemplates(const std::string& filePath) { return bool{}; };

bool SkinInterface::AddSkinOverride(RE::Actor* actor, const F4EEFixedString& id, bool isFemale) { return bool{}; };
F4EEFixedString SkinInterface::GetSkinOverride(RE::Actor* actor) { return F4EEFixedString{}; };
SkinTemplatePtr SkinInterface::GetSkinTemplate(RE::Actor* actor) { return SkinTemplatePtr{}; };
bool SkinInterface::RemoveSkinOverride(RE::Actor* actor) { return bool{}; };
void SkinInterface::CloneSkinOverride(RE::Actor* source, RE::Actor* target){};
bool SkinInterface::UpdateSkinOverride(RE::Actor* actor, bool doFace) { return bool{}; };

RE::TESObjectARMO* SkinInterface::GetBackupSkin(RE::TESNPC* npc, bool& exists) { return nullptr; };
RE::BGSTextureSet* SkinInterface::GetBackupFace(RE::Actor* actor, RE::TESNPC* npc, bool isFemale, bool& exists) { return nullptr; };
RE::BGSHeadPart* SkinInterface::GetBackupHeadPart(RE::Actor* actor, RE::TESNPC* npc, bool isFemale, uint32_t partType) { return nullptr; };

void SkinInterface::ForEachSkinTemplate(std::function<void(const F4EEFixedString&, const SkinTemplatePtr&)> functor){};

void SkinInterface::ClearMods(){};

DWORD CharGenInterface::SavePreset(const std::string& filePath) { return DWORD{}; };
DWORD CharGenInterface::LoadPreset(const std::string& filePath) { return DWORD{}; };

void CharGenInterface::LoadTintTemplateMods(){};

bool CharGenInterface::LoadTintCategories(const std::string& filePath) { return bool{}; };
bool CharGenInterface::LoadTintTemplates(const std::string& filePath) { return bool{}; };

bool CharGenInterface::SaveTintCategories(const RE::TESRace* race, const std::string& filePath) { return bool{}; };
bool CharGenInterface::SaveTintTemplates(const RE::TESRace* race, const std::string& filePath) { return bool{}; };

void CharGenInterface::LoadHairColorMods(){};
bool CharGenInterface::LoadHairColorData(const std::string& filePath, const ModInfo* modInfo) { return bool{}; };

void CharGenInterface::UnlockHeadParts(){};
void CharGenInterface::UnlockTints(){};

void CharGenInterface::ProcessHairColor(RE::NiAVObject* node, RE::BGSColorForm* colorForm, RE::BSShaderMaterial* /*RE::BSLightingShaderMaterialBase* shaderMaterial*/){};
const char* CharGenInterface::ProcessEyebrowPath(RE::TESNPC* npc) { return nullptr; };

void CharGenInterface::ClearHairColorMods()
{
	m_LUTs.clear();
	m_LUTMap.clear();
}
/*virtual TESNPC::HeadData * ProcessHeadData(TESNPC * npc);

	virtual void SetBaseTintTextureOverride(BGSTextureSet * textureSet);
	void LockBaseTextureOverride();
	void ReleaseBaseTextureOverride();*/

bool CharGenInterface::IsLUTUsed(const F4EEFixedString& str) { return bool{}; };
bool CharGenInterface::GetLUTFromColor(RE::BGSColorForm* color, F4EEFixedString& str) { return bool{}; };

RE::Actor* CharGenInterface::GetCurrentActor() { return nullptr; };

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

ActorUpdateManager::ActorUpdateManager() :
	m_loading(false){}
ActorUpdateManager::~ActorUpdateManager() {}

RE::BSEventNotifyControl ActorUpdateManager::ReceiveEvent(RE::TESObjectLoadedEvent* evn, RE::BSTEventSource<RE::TESObjectLoadedEvent>* a_source) { return RE::BSEventNotifyControl{}; }
RE::BSEventNotifyControl ActorUpdateManager::ReceiveEvent(RE::TESInitScriptEvent* evn, RE::BSTEventSource<RE::TESInitScriptEvent>* a_source) { return RE::BSEventNotifyControl{}; }

void ActorUpdateManager::Flush() {}
void ActorUpdateManager::PushUpdate(RE::Actor* actor) {}
void ActorUpdateManager::Revert() {}

void ActorUpdateManager::SetLoading(bool loading) { m_loading = loading; }
void ActorUpdateManager::ResolvePendingBodyGen() {}


