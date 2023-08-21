#pragma once
#include <fstream>
#pragma warning(push)
#pragma warning(disable: 4100)

namespace LooksMenu
{
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
		DetourXS applyMorphsHook;
		ApplyMorphsToShapes* originalApply;

		BodyMorphInterface* g_bodyMorphInterface = nullptr;

		uint64_t GetVersionOffset(uint64_t off1_6_18, uint64_t off1_6_20) {
			switch (version) {
			case k1_6_18:
				return off1_6_18;
			case k1_6_20:
				return off1_6_20;
			default:
				return 0;
			}
		}

		bool HookedApplyMorphs(BodyMorphInterface* a1, RE::Actor* a2, RE::NiAVObject* a3) {
			std::this_thread::sleep_for(std::chrono::microseconds(1));
			return originalApply(a1, a2, a3);
		}

		void RegisterHook() {
			uintptr_t baseAddr = reinterpret_cast<uintptr_t>(GetModuleHandleA("f4ee.dll"));

			if (!applyMorphsHook.Create(reinterpret_cast<LPVOID>(baseAddr + GetVersionOffset(0x10040, 0xFFCC)), &HookedApplyMorphs)) {
				logger::warn("Failed to create ApplyMorphsToShapes hook!");
			} else {
				originalApply = reinterpret_cast<ApplyMorphsToShapes*>(applyMorphsHook.GetTrampoline());
			}

			g_bodyMorphInterface = reinterpret_cast<BodyMorphInterface*>(baseAddr + GetVersionOffset(0xFB440, 0xF4DF0));
		}
	}

	void Init() {
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

	void RemoveMorphsByName(RE::Actor* actor, const RE::BSFixedString& morph) {
		if (isInstalled && actor && detail::g_bodyMorphInterface) {
			bool isFemale = (actor->GetSex() == 1);
			detail::g_bodyMorphInterface->RemoveMorphsByName(actor, isFemale, morph);
		}
	}
}

#pragma warning(pop)
