#pragma once
#include "Bridge/Consts.h"
#include "Bridge/IniParser/Ini.h"
#include "LooksMenu/LooksMenu.h"
#include <boost/json.hpp>
#include <fstream>

namespace Data
{
	/* Структуры данных для конфигурации FannyAnimation.
	 * Позволяют задавать параметры анимации для разных рас и полов через JSON файлы.
	 */

	// ============================================================================
	// CONFIGURATION STRUCTURES
	// ============================================================================

	// Информация о морфе: имя и максимальное значение
	struct MorphInfo
	{
		std::string name;       // Имя морфа (например "VaginaPenetrate")
		float maxValue = 1.0f;  // Максимальное значение морфа при полном проникновении
	};

	// Информация о пенетраторе: имя кости и пороговые расстояния
	struct TipNodeInfo
	{
		std::string nodeName;         // Имя кости (например "Penis_05")
		float startDistance = 15.0f;  // Расстояние начала эффекта проникновения
		float fullDistance = 2.0f;    // Расстояние полного проникновения
	};

	// Конфигурация для конкретной расы/пола
	struct FannyAnimationActorConfig
	{
		// Идентификация
		bool isFemale = true;                    // true = женский пол, false = мужской
		std::unordered_set<RE::TESRace*> races;  // Набор рас, к которым применяется конфиг

		// ============================================================================
		// RECEIVER NODES - точки приёма проникновения (вагина, анус)
		// ============================================================================
		std::string vaginaDeepNode;  // Глубокая точка вагины (например "Pelvis_skin")
		std::string anusDeepNode;    // Глубокая точка ануса (например "Pelvis_Rear_skin")

		// ============================================================================
		// RECEIVER MORPHS - морфы, применяемые при проникновении
		// ============================================================================
		std::vector<MorphInfo> vaginaMorphs;  // Морфы вагины при проникновении
		std::vector<MorphInfo> anusMorphs;    // Морфы ануса при проникновении

		// ============================================================================
		// TIP NODES - все кости пенетраторов с параметрами
		// ============================================================================
		std::vector<TipNodeInfo> tipNodes;  // Все пенетраторы (пенис, пальцы, язык и т.д.)

		// ============================================================================
		// UTILITY METHODS
		// ============================================================================

		// Проверяет, подходит ли конфиг для данного актёра
		bool IsApplicable(RE::Actor* actor) const
		{
			if (!actor)
				return false;

			// Проверка пола
			bool actorIsFemale = (actor->GetSex() == RE::Actor::Sex::Female);
			if (actorIsFemale != isFemale)
				return false;

			// Проверка расы (если список пуст - это fallback конфиг)
			if (!races.empty()) {
				if (!actor->GetVisualsRace() || races.find(actor->GetVisualsRace()) == races.end())
					return false;
			}

			return true;
		}

		// Проверяет наличие узлов-приёмников у актёра
		bool HasReceiverNodes(RE::Actor* actor) const
		{
			if (!actor)
				return false;
			auto root = actor->Get3D();
			if (!root)
				return false;

			return (!vaginaDeepNode.empty() && root->GetObjectByName(vaginaDeepNode)) ||
			       (!anusDeepNode.empty() && root->GetObjectByName(anusDeepNode));
		}
	};

	// Глобальные настройки системы FannyAnimation
	struct FannyAnimationGlobalSettings
	{
		float cameraMaxDistance = 200.0f;   // Максимальное расстояние камеры для обновления
		float cameraCheckInterval = 1.0f;   // Интервал проверки расстояния камеры (секунды)
		float morphTransitionSpeed = 5.0f;  // Скорость перехода морфов

		// Применяет fallback значения по умолчанию
		static FannyAnimationGlobalSettings GetDefault()
		{
			return { 200.0f, 1.0f, 5.0f };
		}
	};

	// Контейнер всех конфигураций
	struct FannyAnimationConfig
	{
		FannyAnimationGlobalSettings globalSettings;
		std::vector<FannyAnimationActorConfig> actorConfigs;     // Конфиги с привязкой к расам
		std::vector<FannyAnimationActorConfig> fallbackConfigs;  // Fallback конфиги (без рас)
		bool isLoaded = false;                                   // Флаг загрузки конфига

		// Находит подходящий конфиг для актёра
		const FannyAnimationActorConfig* FindConfigForActor(RE::Actor* actor) const
		{
			if (!actor)
				return nullptr;

			// Сначала ищем в основных конфигах (с расами)
			for (const auto& config : actorConfigs) {
				if (config.IsApplicable(actor))
					return &config;
			}

			// Если не нашли - ищем в fallback конфигах
			for (const auto& config : fallbackConfigs) {
				if (config.IsApplicable(actor))
					return &config;
			}

			return nullptr;
		}

		// ============================================================================
		// JSON PARSING
		// ============================================================================

		// Парсит расу из строки формата "Plugin.esm:0x123456"
		static RE::TESRace* ParseRaceFromString(const std::string& raceStr)
		{
			auto colonPos = raceStr.find(':');
			if (colonPos == std::string::npos)
				return nullptr;

			std::string plugin = raceStr.substr(0, colonPos);
			std::string formIdStr = raceStr.substr(colonPos + 1);

			// Убираем "0x" если есть
			if (formIdStr.size() > 2 && formIdStr[0] == '0' && (formIdStr[1] == 'x' || formIdStr[1] == 'X')) {
				formIdStr = formIdStr.substr(2);
			}

			uint32_t formId = 0;
			try {
				formId = std::stoul(formIdStr, nullptr, 16);
			} catch (...) {
				return nullptr;
			}

			auto dataHandler = RE::TESDataHandler::GetSingleton();
			if (!dataHandler)
				return nullptr;

			return dataHandler->LookupForm<RE::TESRace>(formId, plugin);
		}

		// Парсит массив морфов из JSON
		static std::vector<MorphInfo> ParseMorphArray(const boost::json::array& arr)
		{
			std::vector<MorphInfo> result;
			for (const auto& item : arr) {
				if (!item.is_object())
					continue;
				const auto& obj = item.as_object();

				MorphInfo morph;
				if (auto it = obj.find("name"); it != obj.end() && it->value().is_string()) {
					morph.name = it->value().as_string().c_str();
				}
				if (auto it = obj.find("maxValue"); it != obj.end() && it->value().is_number()) {
					morph.maxValue = static_cast<float>(it->value().as_double());
				}

				if (!morph.name.empty()) {
					result.push_back(morph);
				}
			}
			return result;
		}

		// Парсит массив tip nodes из JSON
		static std::vector<TipNodeInfo> ParseTipNodeArray(const boost::json::array& arr)
		{
			std::vector<TipNodeInfo> result;
			for (const auto& item : arr) {
				if (!item.is_object())
					continue;
				const auto& obj = item.as_object();

				TipNodeInfo node;
				if (auto it = obj.find("nodeName"); it != obj.end() && it->value().is_string()) {
					node.nodeName = it->value().as_string().c_str();
				}
				if (auto it = obj.find("startDistance"); it != obj.end() && it->value().is_number()) {
					node.startDistance = static_cast<float>(it->value().as_double());
				}
				if (auto it = obj.find("fullDistance"); it != obj.end() && it->value().is_number()) {
					node.fullDistance = static_cast<float>(it->value().as_double());
				}

				if (!node.nodeName.empty()) {
					result.push_back(node);
				}
			}
			return result;
		}

		// Парсит один актор-конфиг из JSON объекта
		static FannyAnimationActorConfig ParseActorConfig(const boost::json::object& obj, bool parseRaces = true)
		{
			FannyAnimationActorConfig config;

			// isFemale
			if (auto it = obj.find("isFemale"); it != obj.end() && it->value().is_bool()) {
				config.isFemale = it->value().as_bool();
			}

			// races (только если parseRaces = true)
			if (parseRaces) {
				if (auto it = obj.find("races"); it != obj.end() && it->value().is_array()) {
					for (const auto& raceItem : it->value().as_array()) {
						if (raceItem.is_string()) {
							if (auto race = ParseRaceFromString(raceItem.as_string().c_str())) {
								config.races.insert(race);
							}
						}
					}
				}
			}

			// vaginaDeepNode
			if (auto it = obj.find("vaginaDeepNode"); it != obj.end() && it->value().is_string()) {
				config.vaginaDeepNode = it->value().as_string().c_str();
			}

			// anusDeepNode
			if (auto it = obj.find("anusDeepNode"); it != obj.end() && it->value().is_string()) {
				config.anusDeepNode = it->value().as_string().c_str();
			}

			// vaginaMorphs
			if (auto it = obj.find("vaginaMorphs"); it != obj.end() && it->value().is_array()) {
				config.vaginaMorphs = ParseMorphArray(it->value().as_array());
			}

			// anusMorphs
			if (auto it = obj.find("anusMorphs"); it != obj.end() && it->value().is_array()) {
				config.anusMorphs = ParseMorphArray(it->value().as_array());
			}

			// tipNodes
			if (auto it = obj.find("tipNodes"); it != obj.end() && it->value().is_array()) {
				config.tipNodes = ParseTipNodeArray(it->value().as_array());
			}

			return config;
		}

		// Парсит глобальные настройки из JSON
		static FannyAnimationGlobalSettings ParseGlobalSettings(const boost::json::object& obj)
		{
			FannyAnimationGlobalSettings settings = FannyAnimationGlobalSettings::GetDefault();

			if (auto it = obj.find("cameraMaxDistance"); it != obj.end() && it->value().is_number()) {
				settings.cameraMaxDistance = static_cast<float>(it->value().as_double());
			}
			if (auto it = obj.find("cameraCheckInterval"); it != obj.end() && it->value().is_number()) {
				settings.cameraCheckInterval = static_cast<float>(it->value().as_double());
			}
			if (auto it = obj.find("morphTransitionSpeed"); it != obj.end() && it->value().is_number()) {
				settings.morphTransitionSpeed = static_cast<float>(it->value().as_double());
			}

			return settings;
		}

		// Загружает основной конфиг из JSON файла
		bool LoadFromFile(const std::string& filePath)
		{
			try {
				std::ifstream file(filePath);
				if (!file.is_open()) {
					logger::warn("FannyAnimation: Cannot open config file: {}", filePath);
					return false;
				}

				std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
				file.close();

				auto json = boost::json::parse(content);
				if (!json.is_object()) {
					logger::warn("FannyAnimation: Invalid JSON format in: {}", filePath);
					return false;
				}

				const auto& root = json.as_object();

				// Парсим globalSettings
				if (auto it = root.find("globalSettings"); it != root.end() && it->value().is_object()) {
					globalSettings = ParseGlobalSettings(it->value().as_object());
				} else {
					globalSettings = FannyAnimationGlobalSettings::GetDefault();
				}

				// Парсим actorConfigs
				actorConfigs.clear();
				if (auto it = root.find("actorConfigs"); it != root.end() && it->value().is_array()) {
					for (const auto& configItem : it->value().as_array()) {
						if (configItem.is_object()) {
							auto config = ParseActorConfig(configItem.as_object(), true);
							actorConfigs.push_back(config);
						}
					}
				}

				logger::info("FannyAnimation: Loaded {} actor configs from: {}", actorConfigs.size(), filePath);
				return true;

			} catch (const std::exception& e) {
				logger::error("FannyAnimation: Error parsing config file {}: {}", filePath, e.what());
				return false;
			}
		}

		// Загружает fallback конфиг из JSON файла
		bool LoadFallbackFromFile(const std::string& filePath)
		{
			try {
				std::ifstream file(filePath);
				if (!file.is_open()) {
					logger::warn("FannyAnimation: Cannot open fallback config file: {}", filePath);
					return false;
				}

				std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
				file.close();

				auto json = boost::json::parse(content);
				if (!json.is_object()) {
					logger::warn("FannyAnimation: Invalid JSON format in fallback: {}", filePath);
					return false;
				}

				const auto& root = json.as_object();

				// Парсим actorConfigs как fallback (без рас)
				fallbackConfigs.clear();
				if (auto it = root.find("actorConfigs"); it != root.end() && it->value().is_array()) {
					for (const auto& configItem : it->value().as_array()) {
						if (configItem.is_object()) {
							auto config = ParseActorConfig(configItem.as_object(), false);  // false = не парсить расы
							fallbackConfigs.push_back(config);
						}
					}
				}

				logger::info("FannyAnimation: Loaded {} fallback configs from: {}", fallbackConfigs.size(), filePath);
				return true;

			} catch (const std::exception& e) {
				logger::error("FannyAnimation: Error parsing fallback config file {}: {}", filePath, e.what());
				return false;
			}
		}

		// Создаёт конфигурацию по умолчанию (hardcoded fallback)
		static FannyAnimationConfig CreateDefault()
		{
			FannyAnimationConfig data;

			// Глобальные настройки
			data.globalSettings = FannyAnimationGlobalSettings::GetDefault();

			// Fallback конфиг для женщин
			FannyAnimationActorConfig femaleConfig;
			femaleConfig.isFemale = true;
			femaleConfig.vaginaDeepNode = "Pelvis_skin";
			femaleConfig.anusDeepNode = "Pelvis_Rear_skin";
			femaleConfig.vaginaMorphs.push_back({ "VaginaPenetrate", 1.5f });
			femaleConfig.anusMorphs.push_back({ "ButtcheeksSpread", 0.5f });
			femaleConfig.anusMorphs.push_back({ "AnusPenetrate", 1.5f });
			femaleConfig.anusMorphs.push_back({ "AnusBack", 1.5f });
			femaleConfig.tipNodes.push_back({ "LArm_Finger33", 15.0f, 5.0f });
			femaleConfig.tipNodes.push_back({ "RArm_Finger33", 15.0f, 5.0f });
			femaleConfig.tipNodes.push_back({ "Tongue04", 10.0f, 3.0f });
			data.fallbackConfigs.push_back(femaleConfig);

			// Fallback конфиг для мужчин
			FannyAnimationActorConfig maleConfig;
			maleConfig.isFemale = false;
			maleConfig.anusDeepNode = "Pelvis_Rear_skin";
			maleConfig.anusMorphs.push_back({ "ButtcheeksSpread", 0.5f });
			maleConfig.anusMorphs.push_back({ "AnusPenetrate", 1.5f });
			maleConfig.anusMorphs.push_back({ "AnusBack", 1.5f });
			maleConfig.tipNodes.push_back({ "Penis_05", 15.0f, 2.0f });
			maleConfig.tipNodes.push_back({ "LArm_Finger33", 15.0f, 5.0f });
			maleConfig.tipNodes.push_back({ "RArm_Finger33", 15.0f, 5.0f });
			maleConfig.tipNodes.push_back({ "Tongue04", 10.0f, 3.0f });
			data.fallbackConfigs.push_back(maleConfig);

			data.isLoaded = true;
			return data;
		}
	};

	// ============================================================================
	// STATIC CONFIG MANAGER
	// ============================================================================

	class FannyAnimationConfigManager
	{
	public:
		// Получить синглтон конфига
		static FannyAnimationConfig& GetConfig()
		{
			static FannyAnimationConfig config;
			return config;
		}

		// Загрузить конфигурацию (вызывать один раз при загрузке игры в main.cpp)
		static bool LoadConfig(const std::string& mainConfigPath, const std::string& fallbackConfigPath)
		{
			auto& config = GetConfig();

			// Сначала создаём дефолтные значения
			config = FannyAnimationConfig::CreateDefault();

			// Пытаемся загрузить основной конфиг
			bool mainLoaded = config.LoadFromFile(mainConfigPath);
			if (mainLoaded) {
				logger::info("FannyAnimation: Main configuration loaded successfully.");
			} else {
				logger::warn("FannyAnimation: Main configuration not loaded, using defaults/fallbacks.");
			}

			// Пытаемся загрузить fallback конфиг
			bool fallbackLoaded = config.LoadFallbackFromFile(fallbackConfigPath);
			if (fallbackLoaded) {
				logger::info("FannyAnimation: Fallback configuration loaded successfully.");
			} else {
				logger::warn("FannyAnimation: Fallback configuration not loaded, using hardcoded defaults.");
			}

			// Если ничего не загрузилось - использем hardcoded defaults
			if (!mainLoaded && !fallbackLoaded) {
				logger::warn("FannyAnimation: Using hardcoded default configuration");
			}

			config.isLoaded = true;
			return mainLoaded || fallbackLoaded;
		}

		// Перезагрузить конфигурацию (для вызова из консоли/MCM)
		static bool ReloadConfig(const std::string& mainConfigPath, const std::string& fallbackConfigPath)
		{
			logger::info("FannyAnimation: Reloading configuration...");
			return LoadConfig(mainConfigPath, fallbackConfigPath);
		}

		// Проверить, загружен ли конфиг
		static bool IsConfigLoaded()
		{
			return GetConfig().isLoaded;
		}
	};

	// ============================================================================
	// MAIN CLASS
	// ============================================================================

	enum class TargetType
	{
		None,
		Vagina,
		Anus
	};

	/* Класс FannyAnimation для анимации вагины и ануса при проникновении.
	 * Отслеживает положение пенетраторов (пенис, пальцы, язык) относительно
	 * точек приёма (вагина, анус) и применяет соответствующие морфы.
	 * Использует статический конфиг из FannyAnimationConfigManager.
	 */
	class FannyAnimation
	{
	public:
		// ============================================================================
		// INTERNAL STRUCTURES
		// ============================================================================

		// Назначение пенетратора к цели
		struct PenetratorAssignment
		{
			RE::ActorHandle penetratorActor;
			std::string nodeName;
			TargetType targetType = TargetType::None;
			float currentFactor = 0.0f;  // Текущий фактор проникновения (0-1)
		};

		// Состояние получателя (актёр с вагиной/анусом)
		struct ReceiverState
		{
			RE::ActorHandle receiverHandle;
			const FannyAnimationActorConfig* config = nullptr;          // Ссылка на конфиг для этого актёра
			std::vector<PenetratorAssignment> assignments;              // Активные назначения пенетраторов
			std::unordered_map<std::string, float> currentMorphValues;  // Текущие значения морфов
			bool isActive = false;
			bool isCameraClose = true;
			float cameraCheckTimer = 0.0f;
		};

		// Информация о пенетраторе для расчётов
		struct PenetratorInfo
		{
			RE::ActorHandle actorHandle;
			std::string nodeName;
			RE::NiPoint3 position;
			float startDistance;
			float fullDistance;
			bool isValid;
		};

	private:
		std::vector<ReceiverState> receiverStates;
		std::vector<RE::ActorHandle> allActors;
		bool isEnabled = false;

		// Получить конфиг (удобный метод)
		static const FannyAnimationConfig& GetConfig()
		{
			return FannyAnimationConfigManager::GetConfig();
		}

	public:
		FannyAnimation() = default;

		~FannyAnimation()
		{
			StopTracking();
		}

		// ============================================================================
		// STATIC CONFIGURATION ACCESS
		// ============================================================================

		static RE::BGSKeyword* GetMorphKWD()
		{
			static auto kwd = RE::TESDataHandler::GetSingleton()->LookupForm<RE::BGSKeyword>(0x800, "AAF.esm");
			return kwd;
		}

		// ============================================================================
		// TRACKING CONTROL
		// ============================================================================

		void StartTracking(const std::vector<RE::NiPointer<RE::Actor>>& actors)
		{
			if (!LooksMenu::isInstalled)
				return;

			if (!GetMorphKWD()) {
				logger::error("FannyAnimation: Morph keyword not found, cannot start tracking.");
				return;
			}

			// Проверка настроек из MCM INI
			bool enable = true;
			std::string file;
			if (std::filesystem::exists(MCM_INI_PATH)) {
				file = MCM_INI_PATH;
			} else if (std::filesystem::exists(MCM_INI_PATH_ALT)) {
				file = MCM_INI_PATH_ALT;
			}

			if (!file.empty()) {
				ini::map map(file);
				enable = map.at("Settings/bAnimateFannies", false);
			}

			if (!enable)
				return;

			// Проверяем, загружен ли конфиг
			if (!FannyAnimationConfigManager::IsConfigLoaded()) {
				logger::warn("FannyAnimation: Config not loaded, cannot start tracking.");
				return;
			}

			const auto& config = GetConfig();

			receiverStates.clear();
			allActors.clear();
			isEnabled = true;

			// Сохраняем всех актёров
			for (const auto& actor : actors) {
				if (actor) {
					allActors.push_back(actor->GetActorHandle());
				}
			}

			// Создаём состояния для актёров-получателей
			for (const auto& actor : actors) {
				if (!actor)
					continue;

				const auto* actorConfig = config.FindConfigForActor(actor.get());
				if (actorConfig && actorConfig->HasReceiverNodes(actor.get())) {
					ReceiverState state;
					state.receiverHandle = actor->GetActorHandle();
					state.config = actorConfig;
					state.isActive = true;
					state.isCameraClose = true;
					state.cameraCheckTimer = 0.0f;
					receiverStates.push_back(state);
				}
			}
		}

		void StopTracking()
		{
			if (!LooksMenu::isInstalled)
				return;

			for (auto& state : receiverStates) {
				if (auto receiver = state.receiverHandle.get(); receiver != nullptr) {
					ResetMorphs(receiver.get());
				}
			}

			receiverStates.clear();
			allActors.clear();
			isEnabled = false;
		}

		void Update(float deltaTime)
		{
			if (!isEnabled || !LooksMenu::isInstalled)
				return;

			const auto& config = GetConfig();

			for (auto& state : receiverStates) {
				if (!state.isActive)
					continue;

				auto receiver = state.receiverHandle.get();
				if (!receiver) {
					state.isActive = false;
					continue;
				}

				// Проверка расстояния камеры
				state.cameraCheckTimer -= deltaTime;
				if (state.cameraCheckTimer <= 0.0f) {
					state.cameraCheckTimer = config.globalSettings.cameraCheckInterval;
					state.isCameraClose = IsCameraCloseToReceiver(receiver.get(), state.config);
				}

				if (state.isCameraClose) {
					UpdateReceiverState(state, deltaTime);
				}
			}
		}

		// ============================================================================
		// ACCESSORS
		// ============================================================================

		bool IsEnabled() const { return isEnabled; }

	private:
		// ============================================================================
		// UTILITY METHODS
		// ============================================================================

		static RE::NiPoint3 GetCameraPosition()
		{
			auto playerCamera = RE::PlayerCamera::GetSingleton();
			if (playerCamera && playerCamera->cameraRoot) {
				return playerCamera->cameraRoot->world.translate;
			}
			return RE::NiPoint3();
		}

		static bool IsCameraCloseToReceiver(RE::Actor* receiver, const FannyAnimationActorConfig* actorConfig)
		{
			if (!receiver || !actorConfig)
				return false;

			const auto& config = GetConfig();

			RE::NiPoint3 cameraPos = GetCameraPosition();
			RE::NiPoint3 receiverPos = GetNodeWorldPosition(receiver, actorConfig->vaginaDeepNode.c_str());

			if (IsZeroPosition(receiverPos)) {
				receiverPos = GetNodeWorldPosition(receiver, actorConfig->anusDeepNode.c_str());
			}

			float distance = CalculateDistance(cameraPos, receiverPos);
			return distance <= config.globalSettings.cameraMaxDistance;
		}

		static bool IsZeroPosition(const RE::NiPoint3& pos)
		{
			return pos.x == 0.0f && pos.y == 0.0f && pos.z == 0.0f;
		}

		static RE::NiPoint3 GetNodeWorldPosition(RE::Actor* actor, const char* nodeName)
		{
			if (!actor || !nodeName || nodeName[0] == '\0')
				return RE::NiPoint3();
			auto root = actor->Get3D();
			if (!root)
				return RE::NiPoint3();

			auto node = root->GetObjectByName(nodeName);
			if (!node)
				return RE::NiPoint3();

			return node->world.translate;
		}

		static float CalculateDistance(const RE::NiPoint3& a, const RE::NiPoint3& b)
		{
			float dx = a.x - b.x;
			float dy = a.y - b.y;
			float dz = a.z - b.z;
			return std::sqrt(dx * dx + dy * dy + dz * dz);
		}

		static float CalculatePenetrationFactor(float distance, float startDist, float fullDist)
		{
			if (distance >= startDist) {
				return 0.0f;
			}
			if (distance <= fullDist) {
				return 1.0f;
			}

			float range = startDist - fullDist;
			float normalizedDistance = distance - fullDist;
			return 1.0f - (normalizedDistance / range);
		}

		static float LerpMorph(float current, float target, float speed, float deltaTime)
		{
			float diff = target - current;
			float maxChange = speed * deltaTime;

			if (std::abs(diff) <= maxChange) {
				return target;
			}

			return current + (diff > 0 ? maxChange : -maxChange);
		}

		// ============================================================================
		// PENETRATOR GATHERING
		// ============================================================================

		std::vector<PenetratorInfo> GatherPenetrators(RE::Actor* receiver)
		{
			std::vector<PenetratorInfo> penetrators;
			RE::ActorHandle receiverHandle = receiver->GetActorHandle();
			const auto& config = GetConfig();

			for (const auto& actorHandle : allActors) {
				auto actor = actorHandle.get();
				if (!actor)
					continue;

				auto root = actor->Get3D();
				if (!root)
					continue;

				// Находим конфиг для этого актёра (как пенетратора)
				const auto* penConfig = config.FindConfigForActor(actor.get());
				if (!penConfig)
					continue;

				bool isSelf = (actorHandle == receiverHandle);

				// Проходим по всем tip nodes в конфиге
				for (const auto& nodeInfo : penConfig->tipNodes) {
					if (root->GetObjectByName(nodeInfo.nodeName)) {
						PenetratorInfo info;
						info.actorHandle = actorHandle;
						info.nodeName = nodeInfo.nodeName;
						info.position = GetNodeWorldPosition(actor.get(), nodeInfo.nodeName.c_str());
						info.startDistance = nodeInfo.startDistance;
						info.fullDistance = nodeInfo.fullDistance;
						info.isValid = !IsZeroPosition(info.position);

						// Для самопроникновения пропускаем пенис (Penis в названии)
						if (isSelf && nodeInfo.nodeName.find("Penis") != std::string::npos) {
							continue;
						}

						if (info.isValid) {
							penetrators.push_back(info);
						}
					}
				}
			}

			return penetrators;
		}

		// ============================================================================
		// STATE UPDATE
		// ============================================================================

		void UpdateReceiverState(ReceiverState& state, float deltaTime)
		{
			auto receiver = state.receiverHandle.get();
			if (!receiver || !state.config)
				return;

			auto receiverRoot = receiver->Get3D();
			if (!receiverRoot)
				return;

			const auto* actorConfig = state.config;
			const auto& config = GetConfig();

			// Получаем позиции целей
			RE::NiPoint3 vaginaPos = GetNodeWorldPosition(receiver.get(), actorConfig->vaginaDeepNode.c_str());
			RE::NiPoint3 anusPos = GetNodeWorldPosition(receiver.get(), actorConfig->anusDeepNode.c_str());
			bool hasVagina = !IsZeroPosition(vaginaPos) && !actorConfig->vaginaDeepNode.empty();
			bool hasAnus = !IsZeroPosition(anusPos) && !actorConfig->anusDeepNode.empty();

			// Собираем все пенетраторы
			auto penetrators = GatherPenetrators(receiver.get());

			// Очищаем старые назначения
			state.assignments.clear();

			// Для каждого пенетратора находим ближайшую цель
			for (const auto& pen : penetrators) {
				float distToVagina = hasVagina ? CalculateDistance(pen.position, vaginaPos) : 9999.0f;
				float distToAnus = hasAnus ? CalculateDistance(pen.position, anusPos) : 9999.0f;

				TargetType target = TargetType::None;
				float distance = 9999.0f;

				if (distToVagina < distToAnus && distToVagina < pen.startDistance) {
					target = TargetType::Vagina;
					distance = distToVagina;
				} else if (distToAnus < distToVagina && distToAnus < pen.startDistance) {
					target = TargetType::Anus;
					distance = distToAnus;
				}

				if (target != TargetType::None) {
					PenetratorAssignment assignment;
					assignment.penetratorActor = pen.actorHandle;
					assignment.nodeName = pen.nodeName;
					assignment.targetType = target;
					assignment.currentFactor = CalculatePenetrationFactor(distance, pen.startDistance, pen.fullDistance);
					state.assignments.push_back(assignment);
				}
			}

			// Вычисляем максимальные факторы для каждой цели
			float maxVaginaFactor = 0.0f;
			float maxAnusFactor = 0.0f;

			for (const auto& assignment : state.assignments) {
				if (assignment.targetType == TargetType::Vagina) {
					maxVaginaFactor = std::max(maxVaginaFactor, assignment.currentFactor);
				} else if (assignment.targetType == TargetType::Anus) {
					maxAnusFactor = std::max(maxAnusFactor, assignment.currentFactor);
				}
			}

			// Обновляем морфы с плавным переходом
			float speed = config.globalSettings.morphTransitionSpeed;

			// Вагина морфы
			for (const auto& morphInfo : actorConfig->vaginaMorphs) {
				float targetValue = maxVaginaFactor * morphInfo.maxValue;
				float& currentValue = state.currentMorphValues[morphInfo.name];
				currentValue = LerpMorph(currentValue, targetValue, speed, deltaTime);
			}

			// Анус морфы
			for (const auto& morphInfo : actorConfig->anusMorphs) {
				float targetValue = maxAnusFactor * morphInfo.maxValue;
				float& currentValue = state.currentMorphValues[morphInfo.name];
				currentValue = LerpMorph(currentValue, targetValue, speed, deltaTime);
			}

			// Применяем морфы
			ApplyMorphs(receiver.get(), state);
		}

		// ============================================================================
		// MORPH APPLICATION
		// ============================================================================

		static void ApplyMorphs(RE::Actor* actor, ReceiverState& state)
		{
			if (!actor || !LooksMenu::isInstalled)
				return;

			auto kwd = GetMorphKWD();
			if (!kwd)
				return;

			static constexpr float MORPH_CHANGE_THRESHOLD = 0.1f;

			bool needUpdate = false;

			for (const auto& [morphName, currentValue] : state.currentMorphValues) {
				// Получаем текущее значение морфа через LooksMenu API
				RE::BSFixedString morphFixedStr(morphName.c_str());
				float lastAppliedValue = LooksMenu::GetMorph(actor, morphFixedStr, kwd);

				// Вычисляем разницу
				float valueDiff = std::abs(currentValue - lastAppliedValue);

				// Применяем морф только если разница превышает порог
				if (valueDiff > MORPH_CHANGE_THRESHOLD) {
					if (currentValue > 0.001f) {
						LooksMenu::SetMorph(actor, morphFixedStr, kwd, currentValue);
					} else {
						LooksMenu::RemoveMorphsByName(actor, morphFixedStr);
					}
					needUpdate = true;
				}
			}

			if (needUpdate) LooksMenu::UpdateMorphs(actor);
		}

		static void ResetMorphs(RE::Actor* actor)
		{
			if (!actor || !LooksMenu::isInstalled)
				return;

			LooksMenu::RemoveMorphsByKeyword(actor, GetMorphKWD());
			LooksMenu::UpdateMorphs(actor);
		}
	};
}
