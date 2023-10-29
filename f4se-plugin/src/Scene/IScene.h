#pragma once
#include "Serialization/General.h"
#include "Tasks/TimerThread.h"
#include "Data/Uid.h"
#include "Tasks/TaskFunctor.h"
#include "Scene/DynamicIdle.h"
#include "Scene/IControlSystem.h"

#define SCENE_FUNCTOR()                             \
	using SceneFunctor::SceneFunctor;               \
	template <class Archive>                        \
	void serialize(Archive& ar, const uint32_t)                     \
	{                                               \
		ar(cereal::base_class<SceneFunctor>(this)); \
	}

namespace Scene
{
	uint32_t GetInputsToDisableForScene()
	{
		using RE::UserEvents::USER_EVENT_FLAG;

		union
		{
			uint32_t u = 0;
			F4SE::stl::enumeration<USER_EVENT_FLAG, uint32_t> e;
		} disabledInputs;

		disabledInputs.e.set(USER_EVENT_FLAG::kActivate);
		disabledInputs.e.set(USER_EVENT_FLAG::kFighting);
		disabledInputs.e.set(USER_EVENT_FLAG::kJumping);
		disabledInputs.e.set(USER_EVENT_FLAG::kMainFour);
		disabledInputs.e.set(USER_EVENT_FLAG::kMovement);
		disabledInputs.e.set(USER_EVENT_FLAG::kPOVSwitch);
		disabledInputs.e.set(USER_EVENT_FLAG::kSneaking);
		disabledInputs.e.set(USER_EVENT_FLAG::kVATS);

		return disabledInputs.u;
	}

	class SceneFunctor : public Tasks::TaskFunctor
	{
	public:
		uint64_t sceneId = 0;

		SceneFunctor()
		{
		}

		SceneFunctor(uint64_t _sceneId) :
			sceneId(_sceneId)
		{
		}

		template <class Archive>
		void serialize(Archive& ar, const uint32_t)
		{
			ar(cereal::base_class<Tasks::TaskFunctor>(this), sceneId);
		}
	};

	class SceneDelegate : public F4SE::ITaskDelegate
	{
	public:
		uint64_t sceneId = 0;

		SceneDelegate() :
			F4SE::ITaskDelegate()
		{
		}

		SceneDelegate(uint64_t _sceneId) :
			F4SE::ITaskDelegate(), sceneId(_sceneId)
		{
		}
	};

	enum SceneState : uint16_t
	{
		Initializing = 0,
		Active = 1,
		Ending = 3,
		PendingDeletion = 2
	};

	enum ProxyResult
	{
		NoChange,
		RemoveNow
	};

	enum SyncState : uint8_t
	{
		Synced,
		SettingUp,
		WaitingForLoad,
		SyncingTimes
	};

	enum ErrorCode : int32_t
	{
		kNone = 0,
		kBadLocation = 1,
		kNoActors = 2,
		kInvalidActor = 3,
		kActorInScene = 4,
		kPlayerWrongWorldspace = 5,
		kNullPosition = 6,
		kAlreadyStarted = 7,
		kNoAvailablePositions = 8,
		kSpecifiedPositionNotAvailable = 9
	};

	struct StartResult
	{
		ErrorCode err = kNone;

		const std::string_view GetErrorMessage() const {
			switch (err) {
			case kBadLocation:
				return "No location ref, or location ref has no parent cell.";
			case kNoActors:
				return "Actor array must have at least one actor.";
			case kInvalidActor:
				return "One or more provided actors are None/Disabled/Dead.";
			case kActorInScene:
				return "One or more provided actors are already in a scene and/or marked as unusable.";
			case kPlayerWrongWorldspace:
				return "Player must be in the same worldspace as the scene location.";
			case kNullPosition:
				return "The provided position is no longer available.";
			case kAlreadyStarted:
				return "The scene has already been started.";
			case kNoAvailablePositions:
				return "No available positions for provided combination of actors.";
			case kSpecifiedPositionNotAvailable:
				return "Provided position ID is not installed or not available for the provided combination of actors.";
			default:
				return "";
			}
		}

		operator bool() {
			return err == kNone;
		}
	};

	using namespace Serialization::General;
	struct IdleInfo
	{
		BodyAnimation::SmartIdle idle;

		template <class Archive>
		void load(Archive& ar, const uint32_t ver)
		{
			if (ver < 2) {
				std::optional<std::string> dynIdle = std::nullopt;
				SerializableIdle regularIdle;
				ar(dynIdle, regularIdle);
				if (dynIdle.has_value()) {
					idle.SetHKXPath(dynIdle.value());
				} else {
					idle.SetIdleForm(regularIdle.get());
				}
			} else {
				ar(idle);
			}
		}

		template <class Archive>
		void save(Archive& ar, const uint32_t) const
		{
			ar(idle);
		}
	};

	struct SceneSettings
	{
		std::vector<SerializableActorHandle> actors;
		std::shared_ptr<const Data::Position> startPosition = nullptr;
		SerializableRefHandle locationRefr;
		float duration = 0.0f;
		bool autoAdvance = false;
		bool ignoreCombat = false;

		std::vector<RE::NiPointer<RE::Actor>> QActors() const {
			std::vector<RE::NiPointer<RE::Actor>> result;
			result.reserve(actors.size());
			for (auto& hndl : actors) {
				result.push_back(hndl.get());
			}
			return result;
		}

		void ClearPreStartInfo() {
			startPosition = nullptr;
			actors.clear();
		}

		template <class Archive>
		void serialize(Archive& ar, const uint32_t)
		{
			ar(actors, startPosition, locationRefr, duration, autoAdvance, ignoreCombat);
		}
	};

	typedef std::unordered_map<SerializableActorHandle, IdleInfo> IdleCache;

	class IScene : public IControllable
	{
	public:
		uint64_t uid;
		safe_mutex lock;
		SceneActorsMap actors;
		SceneState status = SceneState::Initializing;
		SyncState syncStatus = SyncState::Synced;
		RE::NiPoint3 location;
		RE::NiPoint3 angle;
		float animMult = 100.0;
		IdleCache cachedIdlesMap;
		Tasks::TaskContainer tasks;
		std::unique_ptr<IControlSystem> controlSystem = nullptr;
		std::unique_ptr<IControlSystem> queuedSystem = nullptr;
		float lastAnimTime = 0.0f;
		float animTime = 0.0f;
		bool trackAnimTime = false;
		bool noUpdate = false;
		std::string startEquipSet;
		std::string stopEquipSet;
		SceneSettings settings;

		IScene()
		{
		}

		virtual bool PushQueuedControlSystem() { return false; }

		virtual void OnActorDeath(RE::Actor*) {}

		virtual void OnActorLocationChange(RE::Actor*, RE::BGSLocation*) {}

		virtual void OnActorHit(RE::Actor*, const RE::TESHitEvent&) {}

		virtual std::vector<std::string> QCachedHKXStrings() { return {}; }

		virtual bool Init(std::shared_ptr<const Data::Position>)
		{
			uid = Data::Uid::Get();
			return true;
		}

		virtual bool Begin()
		{
			logger::warn("Begin() was called on the IScene base class. This shouldn't happen!");
			return false;
		};

		virtual bool End()
		{
			logger::warn("End() was called on the IScene base class. This shouldn't happen!");
			return false;
		}

		virtual void SoftEnd() {}

		// Once a scene is attached to the SceneManager, Update will be called between every frame
		virtual void Update(){}

		virtual void SetDuration(float){}

		virtual void ApplyMorphSet(const std::string&){}

		virtual void ApplyEquipmentSet(const std::string&){}

		virtual void TransitionToAnimation(std::shared_ptr<const Data::Animation>){}

		virtual void PlayAnimations(){}

		virtual bool SetPosition(const std::string&) { return false; }

		virtual void SetOffset(const RE::NiPoint3&, const RE::NiPoint3&) {}

		virtual void StopSmoothSync() {}

		virtual void SetAnimMult(float) {}

		virtual void SetSyncState(SyncState) {}

		void ForEachActor(std::function<void(RE::Actor*, ActorPropertyMap&)> func, bool require3d = true)
		{
			RE::NiPointer<RE::Actor> currentActor = nullptr;
			for (auto& a : actors) {
				currentActor = a.first.get();
				if (GameUtil::ActorIsEnabled(currentActor.get(), require3d)) {
					func(currentActor.get(), a.second);
				}
			}
		}

		template <class Archive>
		void serialize(Archive& ar, const uint32_t)
		{
			ar(cereal::base_class<IControllable>(this), uid, actors, status, syncStatus, location,
				angle, animMult, cachedIdlesMap, tasks, controlSystem, queuedSystem, lastAnimTime,
				animTime, trackAnimTime, startEquipSet, stopEquipSet, settings);
		}
	};
}

CEREAL_REGISTER_TYPE(Scene::SceneFunctor);
CEREAL_CLASS_VERSION(Scene::IdleInfo, 2);
