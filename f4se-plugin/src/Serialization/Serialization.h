#pragma once
#include "General.h"

#define LOAD_PERSISTENT_STATE(_recordId, _recordName, _typePath)                               \
	case _recordId:                                                                            \
		{                                                                                      \
			auto state = std::make_unique<_typePath::PersistentState>();                       \
			if (Serialization::General::LoadRecord(_recordName, state)) {                      \
				_typePath::state.reset(state.release());                                       \
			}                                                                                  \
			break;                                                                             \
		}

// LOAD_PERSISTENT_STATE_SCENEMANAGER('SCNE', "scene", Scene::SceneManager);
// NAF Bridge fix for reversing actors in scenes NAF bug.
#define LOAD_PERSISTENT_STATE_SCENEMANAGER(_recordId, _recordName, _typePath)                   \
	case _recordId:                                                                             \
		{                                                                                       \
			auto state = std::make_unique<_typePath::PersistentState>();                        \
			if (Serialization::General::LoadRecord(_recordName, state)) {                       \
				auto& scenes = state->scenes;                                                   \
				for (auto& [id, scn] : scenes) {                                                \
					if (scn) {																	\
						std::reverse(scn->settings.actors.begin(), scn->settings.actors.end()); \
					}                                                                           \
				}                                                                               \
				_typePath::state.reset(state.release());                                        \
			}                                                                                   \
			break;                                                                              \
		}																						

#define LOAD_PERSISTENT_STATE_CL(_recordId, _recordName, _typePath, _statePath) \
	case _recordId:                                                             \
		{                                                                       \
			auto state = std::make_unique<_typePath::PersistentState>();        \
			if (Serialization::General::LoadRecord(_recordName, state)) {       \
				_statePath.reset(state.release());                              \
			}                                                                   \
			break;                                                              \
		}

#define LOAD_PERSISTENT_STATE_NP(_recordId, _recordName, _typePath, _statePath) \
	case _recordId:                                                             \
		{                                                                       \
			_typePath state;                                                    \
			if (Serialization::General::LoadRecord(_recordName, state)) {       \
				_statePath = std::move(state);                                  \
			}                                                                   \
			break;                                                              \
		}

#define SAVE_PERSISTENT_STATE(_recordId, _ver, _recordName, _statePath)                               \
	if (!a_intfc->OpenRecord(_recordId, _ver)) {                                                      \
		logger::error("Failed to open co-save {} record. Some data will not be saved!", _recordName); \
	} else {                                                                                          \
		Serialization::General::SaveRecord(_recordName, _statePath);                                  \
	}

namespace Serialization
{
	constexpr auto DISABLE_SERIALIZATION{ false };

	void SaveCallback(const F4SE::SerializationInterface* a_intfc)
	{
		if (DISABLE_SERIALIZATION) {
			return;
		}

		Utility::StartPerformanceCounter();

		auto tThread = Tasks::TimerThread::GetSingleton();
		tThread->Stop();

		{
			std::scoped_lock l{
				Scene::SceneManager::scenesMapLock,
				Scene::SceneManager::actorsWalkingLock,
				tThread->timerLock,
				FaceAnimation::FaceUpdateHook::loadingAnimsLock,
				FaceAnimation::FaceUpdateHook::stateLock,
				BodyAnimation::GraphHook::loadingAnimsLock,
				BodyAnimation::GraphHook::stateLock,
				Data::Uid::lock,
				PackageOverride::lock,
				Menu::HUDManager::elementsLock,
				Menu::HUDManager::translationsLock,
				Scene::OrderedActionQueue::lock,
				Menu::SceneHUD::lock
			};

			Serialization::General::s_intfc = a_intfc;

			SAVE_PERSISTENT_STATE('TASK', 5, "task", tThread->state);
			SAVE_PERSISTENT_STATE('SCNE', 5, "scene", Scene::SceneManager::state);
			SAVE_PERSISTENT_STATE('EQPT', 5, "equipment", Scene::OrderedActionQueue::state);
			SAVE_PERSISTENT_STATE('UID', 5, "UID", Data::Uid::state);
			SAVE_PERSISTENT_STATE('FACE', 5, "face animation", FaceAnimation::FaceUpdateHook::state);
			SAVE_PERSISTENT_STATE('HUD', 5, "HUD", Menu::HUDManager::state);
			SAVE_PERSISTENT_STATE('PACK', 5, "package override", PackageOverride::state);
			SAVE_PERSISTENT_STATE('SHUD', 5, "scene HUD", Menu::SceneHUD::state);
			SAVE_PERSISTENT_STATE('BODY', 5, "body animation", BodyAnimation::GraphHook::state);

			Serialization::General::s_intfc = nullptr;
		}

		logger::info("Finished serialization in {:.3f}ms", Utility::GetPerformanceCounterMS());

		tThread->Start();
	}

	void LoadCallback(const F4SE::SerializationInterface* a_intfc)
	{
		Scene::SceneManager::m_justLoaded = true;  // NAF Bridge play animations after game was load fix
		
		if (DISABLE_SERIALIZATION) {
			return;
		}

		Utility::StartPerformanceCounter();

		auto tThread = Tasks::TimerThread::GetSingleton();
		tThread->Stop();

		{
			std::scoped_lock l{
				Scene::SceneManager::scenesMapLock,
				Scene::SceneManager::actorsWalkingLock,
				tThread->timerLock,
				FaceAnimation::FaceUpdateHook::loadingAnimsLock,
				FaceAnimation::FaceUpdateHook::stateLock,
				BodyAnimation::GraphHook::loadingAnimsLock,
				BodyAnimation::GraphHook::stateLock,
				Data::Uid::lock,
				PackageOverride::lock,
				Menu::HUDManager::elementsLock,
				Menu::HUDManager::translationsLock,
				Scene::OrderedActionQueue::lock,
				Menu::SceneHUD::lock
			};

			Serialization::General::s_intfc = a_intfc;

			uint32_t type;
			uint32_t length;
			uint32_t version;

			while (a_intfc->GetNextRecordInfo(type, version, length)) {
				if (version >= 5) {
					switch (type) {
						LOAD_PERSISTENT_STATE_CL('TASK', "task", Tasks::TimerThread, tThread->state);
						//LOAD_PERSISTENT_STATE('SCNE', "scene", Scene::SceneManager);
						LOAD_PERSISTENT_STATE_SCENEMANAGER('SCNE', "scene", Scene::SceneManager);  // NAF Bridge fix for reversing actors in scenes
						LOAD_PERSISTENT_STATE('EQPT', "equipment", Scene::OrderedActionQueue);
						LOAD_PERSISTENT_STATE('UID', "UID", Data::Uid);
						LOAD_PERSISTENT_STATE('FACE', "face animation", FaceAnimation::FaceUpdateHook);
						LOAD_PERSISTENT_STATE('HUD', "HUD", Menu::HUDManager);
						LOAD_PERSISTENT_STATE('PACK', "package override", PackageOverride);
						LOAD_PERSISTENT_STATE('SHUD', "scene HUD", Menu::SceneHUD);
						LOAD_PERSISTENT_STATE('BODY', "body animation", BodyAnimation::GraphHook);
					}
				}
			}

			Serialization::General::s_intfc = nullptr;
		}

		logger::info("Finished deserialization in {:.3f}ms", Utility::GetPerformanceCounterMS());

		tThread->Start();
	}

	void RevertCallback(const F4SE::SerializationInterface*)
	{
		Tasks::TimerThread::GetSingleton()->Reset();
		PackageOverride::Reset();
		FaceAnimation::FaceUpdateHook::Reset();
		BodyAnimation::GraphHook::Reset();
		Scene::SceneManager::Reset();
		Scene::OrderedActionQueue::Reset();
		Data::Uid::Reset();
		Menu::HUDManager::Reset();
		Menu::SceneHUD::Reset();
	}
}
