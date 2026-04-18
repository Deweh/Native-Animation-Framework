#pragma once
#include "Scripts/Papyrus/FunctionArgs.h"
#include <Windows.h>
#include <ppl.h>
#include <set>
#include "Serialization/General.h"

class GameUtil
{
public:
	// Calculates the distance between two NiPoint3 points.
	static double GetDistance(RE::NiPoint3 pt1, RE::NiPoint3 pt2)
	{
		return sqrt(pow(pt2.x - pt1.x, 2) + pow(pt2.y - pt1.y, 2) + pow(pt2.z - pt1.z, 2) * 1.0);
	}

	struct GraphTime
	{
		float current = 0.0f;
		float total = 0.0f;
	};

	struct GraphLock
	{
		GraphLock(GraphLock&) = delete;

		GraphLock(RE::BSSpinLock& lock) {
			_lock = std::addressof(lock);
			_lock->lock();
		}

		GraphLock(GraphLock&& other) {
			_lock = other._lock;
			other._lock = nullptr;
			character = other.character;
			graph = other.graph;
			rootGen = other.rootGen;
		}

		GraphLock(std::nullptr_t) {}

		~GraphLock() {
			if (_lock != nullptr)
				_lock->unlock();
		}

		operator bool() {
			return _lock != nullptr;
		}

		RE::hkbCharacter* character = nullptr;
		RE::hkbBehaviorGraph* graph = nullptr;
		RE::hkbGenerator* rootGen = nullptr;

	protected:
		RE::BSSpinLock* _lock = nullptr;
	};

	static GraphLock AcquireGraphLock(RE::IAnimationGraphManagerHolder* graphHolder) {
		if (graphHolder == nullptr)
			return nullptr;

		RE::BSTSmartPointer<RE::BSAnimationGraphManager> manager;
		if (!graphHolder->GetAnimationGraphManagerImpl(manager) || manager == nullptr)
			return nullptr;

		GraphLock result(manager->updateLock);
		if (manager->graph.size() < 1 || manager->graph.size() <= manager->activeGraph)
			return nullptr;

		auto& character = manager->graph[manager->activeGraph]->character;
		result.character = &character;
		result.graph = character.behaviorGraph.get();

		if (result.graph == nullptr)
			return nullptr;

		result.rootGen = result.graph->rootGenerator.get();
		if (result.rootGen == nullptr)
			return nullptr;

		return result;
	}

	static void SetAnimationGraphTime(GraphLock& gl, float time) {
		RE::hkbContext context(gl.character);
		gl.graph->setActiveGeneratorLocalTime(&context, gl.rootGen, time);
	}

	static bool GetAnimationGraphTime(GraphLock& gl, GraphTime& timeOut) {
		auto rootGen = reinterpret_cast<RE::hkbGenerator*>(gl.graph->getNodeClone(gl.rootGen));
		if (rootGen == nullptr || rootGen->syncInfo->duration == 0.0f)
			return false;

		timeOut.current = rootGen->syncInfo->localTime;
		timeOut.total = rootGen->syncInfo->duration;
		return true;
	}

	static bool SetAnimationGraphTime(RE::IAnimationGraphManagerHolder* graphHolder, float time) {
		if (auto gl = AcquireGraphLock(graphHolder); gl) {
			SetAnimationGraphTime(gl, time);
			return true;
		}

		return false;
	}

	static bool GetAnimationGraphTime(RE::IAnimationGraphManagerHolder* graphHolder, GraphTime& timeOut) {
		if (auto gl = AcquireGraphLock(graphHolder); gl) {
			return GetAnimationGraphTime(gl, timeOut);
		}

		return false;
	}

	//Calculates the refScale value for an actor given a target scale.
	static float CalcActorScale(RE::Actor* a_target, float a_scale)
	{
		return a_scale / a_target->GetNPC()->GetHeight(a_target, a_target->GetVisualsRace());
	}

	//Sets actor scale via an absolute value.
	//i.e. if GetScale() returns 1.3 and 1.3 is passed to this function, the actor's scale will not change.
	static void SetActorScale(RE::Actor* a_target, float a_scale)
	{
		a_target->SetScale(CalcActorScale(a_target, a_scale));
	}

	// Gets the full name of the corresponding actor.
	// Returns an empty string if unable to get the full name.
	static std::string GetActorName(RE::Actor* targetActor) {
		std::string emptyName = "";

		if (!targetActor)
			return emptyName;

		auto* npc = targetActor->GetNPC();

		if (!npc)
			return emptyName;

		auto* fullName = npc->GetFullName();

		if (!fullName)
			return emptyName;

		return std::string(fullName);
	}

	static RE::bhkCharacterController* GetCharController(RE::Actor* targetActor) {
		if (!targetActor) {
			return nullptr;
		}

		auto aiProcess = targetActor->currentProcess;
		if (!aiProcess) {
			return nullptr;
		}

		auto middleHigh = aiProcess->middleHigh;
		if (!middleHigh) {
			return nullptr;
		}

		auto charController = middleHigh->charController;
		if (!charController) {
			return nullptr;
		}

		return charController.get();
	}

	// Will always return false if the actor is not valid.
	static bool ActorIsEnabled(RE::Actor* targetActor, bool require3d = true) {
		return !RefIsDisabled(targetActor) && (!require3d || targetActor->Get3D() != nullptr);
	}

	// Will always return false if the actor is not valid.
	/*static bool ActorIsAlive(RE::Actor* targetActor, bool require3d = true) {
		return !targetActor ? false : ActorIsEnabled(targetActor, require3d) && targetActor->lifeState == 0;
	}*/
	static bool ActorIsAlive(RE::Actor* targetActor, bool require3d = true) {
		return !targetActor ? false : ActorIsEnabled(targetActor, require3d) && targetActor->lifeState != RE::ACTOR_LIFE_STATE::kDead;
	}

	static bool RefIsDisabled(RE::TESObjectREFR* targetRef) {
		return targetRef == nullptr || targetRef->IsDeleted() || targetRef->IsDisabled();
	}

	// Gets all refs of type T in the corresponding cell, filtered by the 'filter' function.
	template<class T>
	static std::vector<T*> GetRefsInCell(RE::TESObjectCELL* cell, const std::function<bool(T*)>& filter)
	{
		std::vector<T*> result;

		if (!cell) {
			return result;
		}

		RE::BSAutoLock l{ cell->spinLock };
		auto& references = cell->references;

		for (uint32_t i = 0; i < references.size(); i++) {
			T* refr = references[i]->As<T>();

			if (refr && filter(refr)) {
				result.push_back(refr);
			}
		}

		return result;
	}

	// Gets all refs of type T in the currently loaded area, filtered by the 'filter' function.
	template<class T>
	static std::vector<T*> GetRefsInLoadedArea(const std::function<bool(T*)>& filter)
	{
		std::vector<T*> result;

		const auto& [map, lock] = RE::TESForm::GetAllForms();
		RE::BSAutoReadLock l{ lock };
		std::mutex results_lock;
		
		concurrency::parallel_for_each(map->begin(), map->end(), [&](RE::BSTTuple<const uint32_t, RE::TESForm*> ele) {
			RE::TESObjectCELL* cell = ele.second->As<RE::TESObjectCELL>();

			if (cell && cell->loadedData) {
				auto refs = GetRefsInCell(cell, filter);
				std::scoped_lock lock(results_lock);
				result.reserve(result.size() + refs.size());
				result.insert(result.end(), refs.begin(), refs.end());
			}
		});

		return result;
	}

	template <typename T>
	struct RefDistInfo
	{
		double distance;
		std::string name;
		T* ref;
	};

	template <typename T>
	static std::vector<RefDistInfo<T>> GenerateRefDistMap(std::function<bool(T*)> filter, const std::vector<T*> addToResultList = {})
	{
		std::vector<RefDistInfo<T>> result;
		auto resultList = GameUtil::GetRefsInLoadedArea<T>(filter);
		for (auto& e : addToResultList) {
			resultList.push_back(e);
		}

		for (auto r : resultList) {
			double dist = GameUtil::GetDistance(player->data.location, r->data.location);

			if (dist >= 0.1)
				dist = dist / 25.0;

			auto name = r->GetDisplayFullName();
			result.push_back({ dist, name ? std::string(name) : "", r });
		}

		std::sort(result.begin(), result.end(), [](RefDistInfo<T>& a, RefDistInfo<T>& b) { return a.distance < b.distance; });

		return result;
	}

	// Sends an event to all Papyrus scripts registered with RegisterForExternalEvent(eventName).
	template <class... Args>
	static void SendPapyrusEvent(const std::string& eventName, Args... _args) {
		SendPapyrusEventWithCallback(eventName, nullptr, _args...);
	}

	template <class... Args>
	static void SendPapyrusEventWithCallback(const std::string& eventName, RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback, Args... _args)
	{
		struct PapyrusEventData
		{
			RE::BSScript::IVirtualMachine* vm;
			RE::BSTThreadScrapFunction<bool(RE::BSScrapArray<RE::BSScript::Variable>&)> scrapFunc;
			RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
		};

		PapyrusEventData evntData;
		auto const papyrus = F4SE::GetPapyrusInterface();
		auto* vm = RE::GameVM::GetSingleton()->GetVM().get();

		evntData.scrapFunc = (Papyrus::FunctionArgs{ vm, _args... }).get();
		evntData.vm = vm;
		evntData.callback = callback;

		papyrus->GetExternalEventRegistrations(eventName, &evntData, [](uint64_t handle, const char* scriptName, const char* callbackName, void* dataPtr) {
			PapyrusEventData* d = static_cast<PapyrusEventData*>(dataPtr);
			d->vm->DispatchMethodCall(handle, scriptName, callbackName, d->scrapFunc, d->callback);
		});
	}

	template <class... Args>
	static void CallGlobalPapyrusFunction(std::string scriptName, std::string funcName, Args... _args)
	{
		auto* vm = RE::GameVM::GetSingleton()->GetVM().get();
		auto scrapFunc = (Papyrus::FunctionArgs{ vm, _args... }).get();

		vm->DispatchStaticCall(scriptName, funcName, scrapFunc, nullptr);
	}

	// Calls a Papyrus function on the corresponding form's script.
	// If handle is 0, this function will acquire a handle for the form. Otherwise, it will use the provided handle.
	// If persistHandle is true, the handle used will be persisted and returned. Otherwise, the handle will be released & 0 will be returned.
	template <typename T, class... Args>
	static void CallPapyrusFunctionOnForm(T* form, std::string scriptName, std::string funcName, uint64_t& handle, bool persistHandle, Args... _args)
	{
		auto* vm = RE::GameVM::GetSingleton()->GetVM().get();
		auto& handlePolicy = vm->GetObjectHandlePolicy();
		auto scrapFunc = (Papyrus::FunctionArgs{ vm, _args... }).get();

		if (handle == 0) {
			handle = handlePolicy.GetHandleForObject(RE::BSScript::GetVMTypeID<T>(), form);
		}
		
		vm->DispatchMethodCall(handle, scriptName, funcName, scrapFunc, nullptr);

		if (persistHandle) {
			handlePolicy.PersistHandle(handle);
			return;
		} else {
			handlePolicy.ReleaseHandle(handle);
			handle = 0;
			return;
		}
	}

	template <typename T, class... Args>
	static void CallPapyrusFunctionOnForm(T* form, std::string scriptName, std::string funcName, Args... _args)
	{
		uint64_t handle = 0;
		CallPapyrusFunctionOnForm(form, scriptName, funcName, handle, false, _args...);
	}

	static RE::BSFaceGenAnimationData* GetFaceAnimData(RE::Actor* targetActor) {
		if (!targetActor) {
			return nullptr;
		}

		auto aiProcess = targetActor->currentProcess;
		if (!aiProcess) {
			return nullptr;
		}

		auto middleHighProcess = aiProcess->middleHigh;
		if (!middleHighProcess) {
			return nullptr;
		}

		return middleHighProcess->faceAnimationData;
	}

	static void SetFlyCam(bool flyCam) {
		auto& settings = RE::INISettingCollection::GetSingleton()->settings;

		for (auto s : settings) {
			auto key = s->GetKey();
			if (key == "fFreeCameraRotationSpeed:Camera"sv || key == "fFreeCameraTranslationSpeed:Camera"sv) {
				s->SetFloat(2.0);
			}
		}

		const auto camera = RE::PlayerCamera::GetSingleton();

		if (camera && camera->currentState && (flyCam != (camera->currentState->id.get() == RE::CameraState::kFree))) {
			camera->ToggleFreeCameraMode(false);
		}
	}

	static void DisablePlayerCollision(bool disable) {
		auto& settings = RE::INISettingCollection::GetSingleton()->settings;

		for (auto s : settings) {
			if (s->GetKey() == "bDisablePlayerCollision:HAVOK"sv) {
				s->SetBinary(disable);
				break;
			}
		}
	}

	static RE::BSGeometry* GetEyeGeometry(RE::Actor* targetActor) {
		if (!targetActor) {
			return nullptr;
		}

		auto npc = targetActor->GetNPC();
		if (!npc) {
			return nullptr;
		}

		RE::BSFixedString partName = "";
		auto headParts = npc->GetHeadParts();
		for (size_t i = 0; i < headParts.size(); i++) {
			if (*(headParts[i]->type) == RE::BGSHeadPart::HeadPartType::kEyes) {
				partName = headParts[i]->formEditorID;
				break;
			}
		}

		if (partName == "") {
			return nullptr;
		}

		RE::NiAVObject* rootNode = targetActor->Get3D();
		if (!rootNode) {
			return nullptr;
		}

		RE::NiAVObject* eyeNode = rootNode->GetObjectByName(partName);

		if (!eyeNode) {
			return nullptr;
		}

		return eyeNode->IsGeometry();
	}

	static bool SetEyeCoords(RE::Actor* targetActor, float u, float v) {
		RE::BSGeometry* eyeGeo = GetEyeGeometry(targetActor);
		return SetEyeCoords(eyeGeo, u, v);
	}

	static bool SetEyeCoords(RE::BSGeometry* eyeGeo, float u, float v)
	{
		if (!eyeGeo) {
			return false;
		}

		RE::BSShaderProperty* shader = eyeGeo->QShaderProperty();
		if (!shader) {
			return false;
		}

		if (shader->material) {
			shader->material->SetOffsetUV(u, v);
			return true;
		} else {
			return false;
		}
	}

	static bool SetEmissiveMult(RE::BSGeometry* geo, float mult) {
		if (!geo) {
			return false;
		}

		RE::BSShaderProperty* shader = geo->QShaderProperty();
		if (!shader) {
			return false;
		}

		if (shader->material && shader->material->GetFeature() == 2) {
			RE::BSLightingShaderProperty* lightingShader = static_cast<RE::BSLightingShaderProperty*>(shader);
			lightingShader->emitColorScale = mult;
			return true;
		} else {
			return false;
		}
	}

	static float GetEmissiveMult(RE::BSGeometry* geo) {
		if (!geo) {
			return 0.0f;
		}

		RE::BSShaderProperty* shader = geo->QShaderProperty();
		if (!shader) {
			return 0.0f;
		}

		if (shader->material && shader->material->GetFeature() == 2) {
			RE::BSLightingShaderProperty* lightingShader = static_cast<RE::BSLightingShaderProperty*>(shader);
			return lightingShader->emitColorScale;
		} else {
			return 0.0f;
		}
	}

	static float GetAnimMult(RE::Actor* targetActor)
	{
		if (targetActor != nullptr && Data::Forms::AnimMultAV != nullptr) {
			return targetActor->GetActorValue(*Data::Forms::AnimMultAV);
		} else {
			return 0.0f;
		}
	}

	//Mult of 100.0 is normal animation speed.
	static void SetAnimMult(RE::Actor* targetActor, float mult)
	{
		if (targetActor != nullptr && Data::Forms::AnimMultAV != nullptr) {
			targetActor->SetActorValue(*Data::Forms::AnimMultAV, mult);
		}
	}
};
