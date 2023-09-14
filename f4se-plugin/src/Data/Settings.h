#pragma once

#define VAR_NAME(v) #v

namespace Data
{
	class Settings
	{
	public:
		class ThreadSafeString
		{
		public:
			ThreadSafeString() {}

			ThreadSafeString(const char* s) {
				_data = s;
			}

			ThreadSafeString(const std::string& s) {
				_data = s;
			}

			void operator=(const std::string& other) {
				std::unique_lock l{ lock };
				_data = other;
			}

			std::string get() {
				std::unique_lock l{ lock };
				return _data;
			}
		private:
			std::string _data;
			safe_mutex lock;
		};

		struct SettingValues
		{
			std::atomic<bool> bUseLookAtCam = false;
			ThreadSafeString sLookAtCamTarget = "HEAD";

			std::atomic<uint32_t> iDefaultSceneDuration = 30;

			std::atomic<bool> bHeadPartMorphPatch = false;
			ThreadSafeString sHeadPartPatchType = "";
			ThreadSafeString sHeadPartPatchTriPath = "";

			std::atomic<bool> bDisableRescaler = false;
		};

		struct UnsafeSettingValues
		{
			bool bUseLookAtCam;
			std::string sLookAtCamTarget;

			uint32_t iDefaultSceneDuration;

			bool bDisableRescaler;

			UnsafeSettingValues() {}

			void ToDefault() {
				SettingValues defVals;
				(*this) = defVals;
			}

			UnsafeSettingValues(SettingValues& other) {
				bUseLookAtCam = other.bUseLookAtCam;
				sLookAtCamTarget = other.sLookAtCamTarget.get();
				iDefaultSceneDuration = other.iDefaultSceneDuration;
				bDisableRescaler = other.bDisableRescaler;
			}
		};

		inline static SettingValues Values;

		static void PushValues(const UnsafeSettingValues& a_values)
		{
			Values.bUseLookAtCam = a_values.bUseLookAtCam;
			Values.sLookAtCamTarget = a_values.sLookAtCamTarget;
			Values.iDefaultSceneDuration = a_values.iDefaultSceneDuration;
			Values.bDisableRescaler = a_values.bDisableRescaler;

			Data::Events::Send(Data::Events::SETTINGS_CHANGED);
		}

		static void Load() {
			std::ifstream file(SETTINGS_INI_PATH);
			if (!file.is_open())
				return;

			auto vals = ParseINI(file);

			for (auto& v : vals) {
				auto iter = LoadMap.find(v.first);
				if (iter != LoadMap.end())
					iter->second(v.second);
			}

			Data::Events::Send(Data::Events::SETTINGS_CHANGED);
		}

		static bool Save() {
			std::ofstream file(SETTINGS_INI_PATH);
			if (!file.is_open())
				return false;

			std::unordered_map<std::string, std::string> SaveMap{
				{ VAR_NAME(Values.bUseLookAtCam), Values.bUseLookAtCam ? "true" : "false" },
				{ VAR_NAME(Values.sLookAtCamTarget), Values.sLookAtCamTarget.get() },
				{ VAR_NAME(Values.bHeadPartMorphPatch), Values.bHeadPartMorphPatch ? "true" : "false" },
				{ VAR_NAME(Values.sHeadPartPatchType), Values.sHeadPartPatchType.get() },
				{ VAR_NAME(Values.sHeadPartPatchTriPath), Values.sHeadPartPatchTriPath.get() },
				{ VAR_NAME(Values.iDefaultSceneDuration), std::format("{}", Values.iDefaultSceneDuration.load()) },
				{ VAR_NAME(Values.bDisableRescaler), Values.bDisableRescaler ? "true" : "false" },
			};

			WriteINI(file, SaveMap);

			return true;
		}

	private:
		static bool ParseBool(const std::string& val)
		{
			auto lVal = Utility::StringToLower(val);
			if (lVal == "true" || lVal == "1")
				return true;
			else
				return false;
		}

		static uint32_t ParseU32(const std::string& val, uint32_t defaultVal) {
			uint32_t result;
			try {
				result = std::stoul(val);
			}
			catch (...) {
				return defaultVal;
			}
			return result;
		}

		inline static const std::unordered_map<std::string, std::function<void(const std::string&)>> LoadMap{
			{ VAR_NAME(Values.bUseLookAtCam), [](auto& s) { Values.bUseLookAtCam = ParseBool(s); } },
			{ VAR_NAME(Values.sLookAtCamTarget), [](auto& s) { Values.sLookAtCamTarget = s; } },
			{ VAR_NAME(Values.bHeadPartMorphPatch), [](auto& s) { Values.bHeadPartMorphPatch = ParseBool(s); } },
			{ VAR_NAME(Values.sHeadPartPatchType), [](auto& s) { Values.sHeadPartPatchType = s; } },
			{ VAR_NAME(Values.sHeadPartPatchTriPath), [](auto& s) { Values.sHeadPartPatchTriPath = s; } },
			{ VAR_NAME(Values.iDefaultSceneDuration), [](auto& s) { Values.iDefaultSceneDuration = ParseU32(s, 30); } },
			{ VAR_NAME(Values.bDisableRescaler), [](auto& s) { Values.bDisableRescaler = ParseBool(s); } },
		};

		static std::unordered_map<std::string, std::string> ParseINI(std::istream& a_stream) {
			std::unordered_map<std::string, std::string> result;
			std::string line;
			while (std::getline(a_stream, line)) {
				auto commentPos = line.find(";", 0);
				if (commentPos != std::string::npos)
					line = line.substr(0, commentPos);

				auto delimiterPos = line.find("=", 0);
				if (delimiterPos != std::string::npos) {
					result.insert({
						line.substr(0, delimiterPos),
						line.substr(delimiterPos + 1, line.size() - (delimiterPos + 1))
					});
				}
			}

			return result;
		}

		static void WriteINI(std::ostream& a_stream, const std::unordered_map<std::string, std::string>& a_values)
		{
			for (auto& pair : a_values) {
				a_stream << std::format("{}={}\n", pair.first, pair.second);
			}
		}
	};
}
