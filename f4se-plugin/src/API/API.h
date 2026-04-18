#pragma once

//Below is the external portion of the API - it should be copied into other plugins to make use of the API.

namespace NAFAPI
{
	namespace detail
	{
		template <typename FT, typename... Args>
		void invoke(const std::string_view& funcName, Args... _args)
		{
			const auto hndl = GetModuleHandleA("NAF.dll");
			if (hndl != NULL) {
				const auto addr = GetProcAddress(hndl, funcName.data());
				if (addr != NULL) {
					(reinterpret_cast<FT>(addr))(_args...);
				}
			}
		}

		template <typename FT, typename R, typename... Args>
		R invokeWithReturn(const std::string_view& funcName, R defaultReturn, Args... _args)
		{
			const auto hndl = GetModuleHandleA("NAF.dll");
			if (hndl != NULL) {
				const auto addr = GetProcAddress(hndl, funcName.data());
				if (addr != NULL) {
					return (reinterpret_cast<FT>(addr))(_args...);
				}
			}
			return defaultReturn;
		}
	}

	//Checks if the NAF.dll module is loaded.
	bool IsAvailable()
	{
		const auto hndl = GetModuleHandleA("NAF.dll");
		return hndl != NULL;
	}

	/*
	* Loads the specified NANIM animation on the current thread.
	* 
	* a_filePath: The file path to the .nanim file, starting from the Fallout4 folder.
	* a_animationId: The ID of the animation within the .nanim file to load. For baked animations, this is "default".
	* a_raceEditorId: The editor ID of the race this animation will be played on. Used to determine the target skeleton.
	* 
	* Returns 0 if an error occured, otherwise returns a handle for the loaded animation.
	*/
	uint64_t LoadNANIM(
		const char* a_filePath,
		const char* a_animationId,
		const char* a_raceEditorId)
	{
		return detail::invokeWithReturn<decltype(LoadNANIM)*>("NAFAPI_LoadNANIM", 0ui64, a_filePath, a_animationId, a_raceEditorId);
	}

	/*
	* Plays a loaded NANIM animation on the specified actor.
	* 
	* a_actor: The actor the animation will be played on.
	* a_animationHndl: Handle for a loaded NANIM animation.
	* a_transitionTime: The amount of time to take to blend from the current animation to the new animation, in seconds.
	* a_moveAnim: If true, the provided animation will be "moved" into NAF's graph system and the handle will be invalidated. Otherwise, a copy will be performed.
	* 
	* Returns true if the animation was successfully started.
	*/
	bool PlayNANIM(
		RE::Actor* a_actor,
		uint64_t a_animationHndl,
		float a_transitionTime,
		bool a_moveAnim)
	{
		return detail::invokeWithReturn<decltype(PlayNANIM)*>("NAFAPI_PlayNANIM", false, a_actor, a_animationHndl, a_transitionTime, a_moveAnim);
	}

	/*
	* Loads an NANIM animation on a separate thread then immediately plays it on the specified actor.
	* This operation can be cancelled by calling any other NANIM function on the specified actor before the load is finished.
	* 
	* a_actor: The actor the animation will be played on.
	* a_filePath: The file path to the .nanim file, starting from the Fallout4 folder.
	* a_animationId: The ID of the animation within the .nanim file to load. For baked animations, this is "default".
	* a_transitionTime: The amount of time to take to blend from the current animation to the new animation, in seconds.
	* 
	* Returns true if no errors occured. Note: The animation might not play even if this function returns true,
	* as animation loading & parsing occurs on a different thread, and it is unknown if an error will occur on file read or whilst parsing.
	* To be absolutely sure the animation will play, use LoadNANIM + PlayNANIM instead.
	*/
	bool QueueNANIM(
		RE::Actor* a_actor,
		const char* a_filePath,
		const char* a_animationId,
		float a_transitionTime)
	{
		return detail::invokeWithReturn<decltype(QueueNANIM)*>("NAFAPI_QueueNANIM", false, a_actor, a_filePath, a_animationId, a_transitionTime);
	}

	/*
	* Stops any NANIM animation currently playing on the specified actor.
	* 
	* a_actor: The actor to stop any NANIM animations on.
	* a_transitionTime: The amount of time to take to blend from the current animation to the new animation, in seconds.
	* 
	* Returns true if there was an NANIM animation playing on the specified actor, otherwise false.
	*/
	bool StopNANIM(
		RE::Actor* a_actor,
		float a_transitionTime)
	{
		return detail::invokeWithReturn<decltype(StopNANIM)*>("NAFAPI_StopNANIM", false, a_actor, a_transitionTime);
	}

	/*
	* Destroys an object associated with a handle.
	* 
	* a_handle: The handle for the object to destroy.
	* 
	* Returns true if the handle was valid, otherwise false. The handle will no longer be valid after this call.
	*/
	bool FreeHandle(
		uint64_t a_handle)
	{
		return detail::invokeWithReturn<decltype(FreeHandle)*>("NAFAPI_FreeHandle", false, a_handle);
	}

	//For AAF-NAF Bridge
	bool ApplyEquipmentSet(RE::Actor* a, const std::string& id)
	{
		return detail::invokeWithReturn<decltype(ApplyEquipmentSet)*>("NAFAPI_ApplyEquipmentSet", false, a, id);
	}

	bool GetPositionInstalled(const std::string& id)
	{
		return detail::invokeWithReturn<decltype(GetPositionInstalled)*>("NAFAPI_GetPositionInstalled", false, id);
	}

	bool ApplyMorphSet(RE::Actor* a, const std::string& id)
	{
		return detail::invokeWithReturn<decltype(ApplyMorphSet)*>("NAFAPI_ApplyMorphSet", false, a, id);
	}

	bool CompleteWalkForActor(const RE::Actor* a)
	{
		return detail::invokeWithReturn<decltype(CompleteWalkForActor)*>("NAFAPI_CompleteWalkForActor", false, a);
	}

	bool CompleteWalkForScene(uint64_t scene_id)
	{
		return detail::invokeWithReturn<decltype(CompleteWalkForScene)*>("NAFAPI_CompleteWalkForScene", false, scene_id);
	}

	const char* FindPositionBySceneSettings(const std::vector<RE::Actor*>& a_actors, const std::optional<const Papyrus::NAF::SceneSettings>& a_settings)
	{
		return detail::invokeWithReturn<decltype(FindPositionBySceneSettings)*>("NAFAPI_FindPositionBySceneSettings", "10", a_actors, a_settings);
	}

	const Scene::SceneManager* GetSceneManager()
	{
		const Scene::SceneManager* null_ptr = nullptr;
		return detail::invokeWithReturn<decltype(GetSceneManager)*>("NAFAPI_GetSceneManager", null_ptr);
	}

	void HotReload(bool RebuildFiles)
	{
		detail::invoke<decltype(HotReload)*>("NAFAPI_HotReload", RebuildFiles);
	}

	const char* ValidateSceneParamsIgnoreInScene
		(std::vector<RE::Actor*> a_actors, std::optional<Papyrus::NAF::SceneSettings> a_settings)
	{
		return detail::invokeWithReturn<decltype(ValidateSceneParamsIgnoreInScene)*>("NAFAPI_ValidateSceneParamsIgnoreInScene", "", a_actors, a_settings);
	}

	template <class T>
	bool AddToMap(T* obj)
	{
		const auto hndl = GetModuleHandleA("NAF.dll");
		if (hndl == NULL)
			return false;

		if (std::is_same_v<T, Data::Race>) {
			const auto addr = GetProcAddress(hndl, "NAFAPI_AddToMapRace");
			if (addr == NULL) {
				return false;
			}
			typedef bool(__cdecl * f)(void*);
			return ((f)addr)(obj);
		} else if (std::is_same_v<T, Data::Animation>) {
			const auto addr = GetProcAddress(hndl, "NAFAPI_AddToMapAnimation");
			if (addr == NULL) {
				return false;
			}
			typedef bool(__cdecl * f)(void*);
			return ((f)addr)(obj);
		} else if (std::is_same_v<T, Data::Position>) {
			const auto addr = GetProcAddress(hndl, "NAFAPI_AddToMapPosition");
			if (addr == NULL) {
				return false;
			}
			typedef bool(__cdecl * f)(void*);
			return ((f)addr)(obj);
		} else if (std::is_same_v<T, Data::FaceAnim>) {
			const auto addr = GetProcAddress(hndl, "NAFAPI_AddToMapFaceAnim");
			if (addr == NULL) {
				return false;
			}
			typedef bool(__cdecl * f)(void*);
			return ((f)addr)(obj);
		} else if (std::is_same_v<T, Data::MorphSet>) {
			const auto addr = GetProcAddress(hndl, "NAFAPI_AddToMapMorphSet");
			if (addr == NULL) {
				return false;
			}
			typedef bool(__cdecl * f)(void*);
			return ((f)addr)(obj);
		} else if (std::is_same_v<T, Data::EquipmentSet>) {
			const auto addr = GetProcAddress(hndl, "NAFAPI_AddToMapEquipmentSet");
			if (addr == NULL) {
				return false;
			}
			typedef bool(__cdecl * f)(void*);
			return ((f)addr)(obj);
		} else if (std::is_same_v<T, Data::Action>) {
			const auto addr = GetProcAddress(hndl, "NAFAPI_AddToMapAction");
			if (addr == NULL) {
				return false;
			}
			typedef bool(__cdecl * f)(void*);
			return ((f)addr)(obj);
		} else if (std::is_same_v<T, Data::AnimationGroup>) {
			const auto addr = GetProcAddress(hndl, "NAFAPI_AddToMapAnimationGroup");
			if (addr == NULL) {
				return false;
			}
			typedef bool(__cdecl * f)(void*);
			return ((f)addr)(obj);
		} else if (std::is_same_v<T, Data::Furniture>) {
			const auto addr = GetProcAddress(hndl, "NAFAPI_AddToMapFurniture");
			if (addr == NULL) {
				return false;
			}
			typedef bool(__cdecl * f)(void*);
			return ((f)addr)(obj);
		} else if (std::is_same_v<T, Data::PositionTree>) {
			const auto addr = GetProcAddress(hndl, "NAFAPI_AddToMapPositionTree");
			if (addr == NULL) {
				return false;
			}
			typedef bool(__cdecl * f)(void*);
			return ((f)addr)(obj);
		} else if (std::is_same_v<T, Data::GraphInfo>) {
			const auto addr = GetProcAddress(hndl, "NAFAPI_AddToMapGraphInfo");
			if (addr == NULL) {
				return false;
			}
			typedef bool(__cdecl * f)(void*);
			return ((f)addr)(obj);
		} else
			return false;
	}

		std::shared_ptr<const Data::Animation> GetAnimation(const std::string& id)
	{
		std::shared_ptr<const Data::Animation> null_ptr(nullptr);
		return std::shared_ptr<const Data::Animation>(detail::invokeWithReturn<decltype(GetAnimation)*>("NAFAPI_GetAnimation", null_ptr, id));
	}

	std::shared_ptr<const Data::Position> GetPosition(const std::string& id)
	{
		std::shared_ptr<const Data::Position> null_ptr(nullptr);
		return std::shared_ptr<const Data::Position>(detail::invokeWithReturn<decltype(GetPosition)*>("NAFAPI_GetPosition", null_ptr, id));
	}

	std::shared_ptr<const Data::FaceAnim> GetFaceAnim(const std::string& id)
	{
		std::shared_ptr<const Data::FaceAnim> null_ptr(nullptr);
		return std::shared_ptr<const Data::FaceAnim>(detail::invokeWithReturn<decltype(GetFaceAnim)*>("NAFAPI_FaceAnim", null_ptr, id));
	}

	std::shared_ptr<const Data::MorphSet> GetMorphSet(const std::string& id)
	{
		std::shared_ptr<const Data::MorphSet> null_ptr(nullptr);
		return std::shared_ptr<const Data::MorphSet>(detail::invokeWithReturn<decltype(GetMorphSet)*>("NAFAPI_GetMorphSet", null_ptr, id));
	}

	std::shared_ptr<const Data::EquipmentSet> GetEquipmentSet(const std::string& id)
	{
		std::shared_ptr<const Data::EquipmentSet> null_ptr(nullptr);
		return std::shared_ptr<const Data::EquipmentSet>(detail::invokeWithReturn<decltype(GetEquipmentSet)*>("NAFAPI_GetEquipmentSet", null_ptr, id));
	}

	std::shared_ptr<const Data::Action> GetAction(const std::string& id)
	{
		std::shared_ptr<const Data::Action> null_ptr(nullptr);
		return std::shared_ptr<const Data::Action>(detail::invokeWithReturn<decltype(GetAction)*>("NAFAPI_GetAction", null_ptr, id));
	}

	std::shared_ptr<const Data::AnimationGroup> GetAnimationGroup(const std::string& id)
	{
		std::shared_ptr<const Data::AnimationGroup> null_ptr(nullptr);
		return std::shared_ptr<const Data::AnimationGroup>(detail::invokeWithReturn<decltype(GetAnimationGroup)*>("NAFAPI_GetAnimationGroup", null_ptr, id));
	}

	std::shared_ptr<const Data::PositionTree> GetPositionTree(const std::string& id)
	{
		std::shared_ptr<const Data::PositionTree> null_ptr(nullptr);
		return std::shared_ptr<const Data::PositionTree>(detail::invokeWithReturn<decltype(GetPositionTree)*>("NAFAPI_GetPositionTree", null_ptr, id));
	}

	std::shared_ptr<const Data::Race> GetRace(const std::string& id)
	{
		std::shared_ptr<const Data::Race> null_ptr(nullptr);
		return std::shared_ptr<const Data::Race>(detail::invokeWithReturn<decltype(GetRace)*>("NAFAPI_GetRace", null_ptr, id));
	}
}

//Below is the INTERNAL portion of the API - it should NOT be copied into other plugins.

namespace API_Internal
{
	namespace detail
	{
		struct APIObject
		{
			virtual ~APIObject() {}
		};

		std::mutex lock;
		std::unordered_map<uint64_t, std::unique_ptr<APIObject>> objects;
		uint64_t nextObjectHandle = 1;

		uint64_t GetNewHandle()
		{
			if (nextObjectHandle == UINT64_MAX) {
				nextObjectHandle = 1;
			}
			return nextObjectHandle++;
		}
	}

	struct APIAnimation : public detail::APIObject
	{
		std::unique_ptr<BodyAnimation::NodeAnimation> data = nullptr;
		std::string filePath = "";
		std::string id = "";
		virtual ~APIAnimation() {}
	};

	uint64_t AddObject(std::unique_ptr<detail::APIObject> obj)
	{
		std::unique_lock l{ detail::lock };

		uint64_t hndl = detail::GetNewHandle();
		detail::objects[hndl] = std::move(obj);

		return hndl;
	}

	//If the access function returns false, the object will be deleted.
	void AccessObject(uint64_t hndl, std::function<bool(detail::APIObject*)> func) {
		std::unique_lock l{ detail::lock };

		if (auto iter = detail::objects.find(hndl); iter != detail::objects.end()) {
			if (!func(iter->second.get())) {
				detail::objects.erase(iter);
			}
		}
	}

	bool FreeObject(uint64_t hndl) {
		std::unique_lock l{ detail::lock };

		if (auto iter = detail::objects.find(hndl); iter != detail::objects.end()) {
			detail::objects.erase(iter);
			return true;
		}

		return false;
	}
}

extern "C" __declspec(dllexport) uint64_t NAFAPI_LoadNANIM(
	const char* a_filePath,
	const char* a_animationId,
	const char* a_raceEditorId)
{
	auto g = Data::GetGraphInfo(a_raceEditorId);

	if (!g)
		return 0;

	auto anim = std::make_unique<API_Internal::APIAnimation>();
	anim->data = BodyAnimation::GraphHook::LoadAnimation(g, a_filePath, a_animationId);

	if (!anim->data)
		return 0;

	anim->filePath = a_filePath;
	anim->id = a_animationId;

	return API_Internal::AddObject(std::move(anim));
}

extern "C" __declspec(dllexport) bool NAFAPI_PlayNANIM(
	RE::Actor* a_actor,
	uint64_t a_animationHndl,
	float a_transitionTime,
	bool a_moveAnim)
{
	if (!a_actor || a_animationHndl < 1)
		return false;

	std::unique_ptr<BodyAnimation::NodeAnimation> anim = nullptr;
	std::string filePath;
	std::string id;

	API_Internal::AccessObject(a_animationHndl, [&](API_Internal::detail::APIObject* obj) {
		auto apiAnim = dynamic_cast<API_Internal::APIAnimation*>(obj);
		if (!apiAnim) {
			return true;
		}

		filePath = apiAnim->filePath;
		id = apiAnim->id;

		if (a_moveAnim) {
			anim = std::move(apiAnim->data);
			return false;
		} else {
			anim = std::make_unique<BodyAnimation::NodeAnimation>(*apiAnim->data);
			return true;
		}
	});

	if (!anim)
		return false;

	return BodyAnimation::GraphHook::StartAnimation(a_actor, std::move(anim), a_transitionTime, filePath, id);
}

extern "C" __declspec(dllexport) bool NAFAPI_QueueNANIM(
	RE::Actor* a_actor,
	const char* a_filePath,
	const char* a_animationId,
	float a_transitionTime)
{
	if (!a_actor)
		return false;
	 
	return BodyAnimation::GraphHook::LoadAndPlayAnimation(a_actor, a_filePath, a_transitionTime, a_animationId);
}

extern "C" __declspec(dllexport) bool NAFAPI_StopNANIM(
	RE::Actor* a_actor,
	float a_transitionTime)
{
	if (!a_actor)
		return false;

	return BodyAnimation::GraphHook::StopAnimation(a_actor, a_transitionTime);
}

extern "C" __declspec(dllexport) bool NAFAPI_FreeHandle(
	uint64_t a_handle)
{
	return API_Internal::FreeObject(a_handle);
}

//For AAF-NAF Bridge
extern "C" __declspec(dllexport) bool NAFAPI_ApplyEquipmentSet(RE::Actor* a, const std::string& id)
{
	return Data::ApplyEquipmentSet(a, id);
}

extern "C" __declspec(dllexport) bool NAFAPI_GetPositionInstalled(const std::string& id)
{
	return Data::GetPosition(id) == nullptr ? false : true;
}

extern "C" __declspec(dllexport) bool NAFAPI_ApplyMorphSet(RE::Actor* a, const std::string& id)
{
	return Data::ApplyMorphSet(a, id);
}

extern "C" __declspec(dllexport) bool NAFAPI_CompleteWalkForActor(const RE::Actor* akActor)
{
	if (!akActor) {
		return false;
	}
	bool result = false;

	Scene::SceneManager::GetSingleton()->VisitAllScenes([&](Scene::IScene* scn) 
		{
		scn->ForEachActor([&](RE::Actor* currentActor, Scene::ActorPropertyMap&) 
		{
			if (currentActor == akActor) {
				Scene::SceneManager::CompleteWalk(scn->uid);
				result = true;
			}
		});
	},
		true);

	return result;
}

extern "C" __declspec(dllexport) bool NAFAPI_CompleteWalkForScene(uint64_t scene_id)
{
	return Scene::SceneManager::CompleteWalk(scene_id);
}

extern "C" __declspec(dllexport) const char* NAFAPI_FindPositionBySceneSettings(const std::vector<RE::Actor*>& a_actors, const std::optional<const Papyrus::NAF::SceneSettings>& a_settings)
{
	std::string result;
	if (a_actors.size() < 1) {
		result = { Scene::kNoActors };
		return result.c_str();
	}

	std::string a_position = "";
	std::string includeTags = "";
	std::string excludeTags = "";
	std::string requireTags = "";
	RE::TESObjectREFR* posRefr = nullptr;

	if (a_settings.has_value()) 
	{
		auto& s = a_settings.value();
		if (auto v = s.find<std::string>("position", true); v) 
		{
			a_position = v.value();
			if (!a_position.empty()) 
			{
				return a_position.c_str();
			}
		}
		if (auto v = s.find<RE::TESObjectREFR*>("positionRef", true); v) 
		{
			posRefr = v.value();
		}
		if (auto v = s.find<std::string>("includeTags", true); v) 
		{
			includeTags = v.value();
		}
		if (auto v = s.find<std::string>("excludeTags", true); v) 
		{
			excludeTags = v.value();
		}
		if (auto v = s.find<std::string>("requireTags", true); v) 
		{
			requireTags = v.value();
		}
	}

	std::optional<Data::Global::TagFilter> tFilter;
	if (includeTags.size() > 0 || excludeTags.size() > 0 || requireTags.size() > 0) 
	{
		Utility::TransformStringToLower(includeTags);
		Utility::TransformStringToLower(excludeTags);
		Utility::TransformStringToLower(requireTags);
		tFilter.emplace(
			Utility::SplitString(includeTags, ","),
			Utility::SplitString(excludeTags, ","),
			Utility::SplitString(requireTags, ","));
	}

	Data::AnimationFilter filter(a_actors);
	if (filter.numTotalActors != a_actors.size()) 
	{
		result = { Scene::kInvalidActor };
		return result.c_str();
	}
	auto positions = Data::Global::GetFilteredPositions(filter, a_position.size() < 1, false, posRefr, false, tFilter);

	if (a_position.size() < 1) 
	{
		if (positions.size() > 0) 
		{
			return Utility::SelectRandom(positions).c_str();
		} 
		else 
		{
			result = { Scene::kNoAvailablePositions };
			return result.c_str();
		}
	} 
	else if (!Utility::VectorContains(positions, a_position)) 
	{
		result = { Scene::kSpecifiedPositionNotAvailable };
		return result.c_str();
	}

	return "10";
}

extern "C" __declspec(dllexport) const Data::Animation* NAFAPI_GetAnimation(const std::string& id)
{
	return Data::GetAnimation(id).get();
}

extern "C" __declspec(dllexport) const Data::Position* NAFAPI_GetPosition(const std::string& id)
{
	return Data::GetPosition(id).get();
}

extern "C" __declspec(dllexport) const Data::FaceAnim* NAFAPI_GetFaceAnim(const std::string& id)
{
	return Data::GetFaceAnim(id).get();
}

extern "C" __declspec(dllexport) const Data::MorphSet* NAFAPI_GetMorphSet(const std::string& id)
{
	return Data::GetMorphSet(id).get();
}

extern "C" __declspec(dllexport) const Data::EquipmentSet* NAFAPI_GetEquipmentSet(const std::string& id)
{
	return Data::GetEquipmentSet(id).get();
}

extern "C" __declspec(dllexport) const Data::Action* NAFAPI_GetAction(const std::string& id)
{
	return Data::GetAction(id).get();
}

extern "C" __declspec(dllexport) const Data::AnimationGroup* NAFAPI_GetAnimationGroup(const std::string& id)
{
	return Data::GetAnimationGroup(id).get();
}

extern "C" __declspec(dllexport) const Data::PositionTree* NAFAPI_GetPositionTree(const std::string& id)
{
	return Data::GetPositionTree(id).get();
}

extern "C" __declspec(dllexport) const Data::Race* NAFAPI_GetRace(const std::string& id)
{
	return Data::GetRace(id).get();
}

//extern "C" __declspec(dllexport) const Data::FurnitureList* NAFAPI_GetFurnitureList(const std::string& id)
//{
//	return Data::GetFurniture(id).get();
//}
extern "C" __declspec(dllexport) const Scene::SceneManager* NAFAPI_GetSceneManager()
{
	return Scene::SceneManager::GetSingleton();
}

extern "C" __declspec(dllexport) bool NAFAPI_AddToMapRace(void* obj)
{
	Data::Global::Races.priority_insert(std::shared_ptr<Data::Race>(static_cast<Data::Race*>(obj)));
	return true;
}

extern "C" __declspec(dllexport) bool NAFAPI_AddToMapAnimation(void* obj)
{
	Data::Global::Animations.priority_insert(std::shared_ptr<Data::Animation>(static_cast<Data::Animation*>(obj)));
	return true;
}

extern "C" __declspec(dllexport) bool NAFAPI_AddToMapPosition(void* obj)
{
	Data::Global::Positions.priority_insert(std::shared_ptr<Data::Position>(static_cast<Data::Position*>(obj)));
	return true;
}

extern "C" __declspec(dllexport) bool NAFAPI_AddToMapFaceAnim(void* obj)
{
	Data::Global::FaceAnims.priority_insert(std::shared_ptr<Data::FaceAnim>(static_cast<Data::FaceAnim*>(obj)));
	return true;
}

extern "C" __declspec(dllexport) bool NAFAPI_AddToMapMorphSet(void* obj)
{
	Data::Global::MorphSets.priority_insert(std::shared_ptr<Data::MorphSet>(static_cast<Data::MorphSet*>(obj)));
	return true;
}

extern "C" __declspec(dllexport) bool NAFAPI_AddToMapEquipmentSet(void* obj)
{
	Data::Global::EquipmentSets.priority_insert(std::shared_ptr<Data::EquipmentSet>(static_cast<Data::EquipmentSet*>(obj)));
	return true;
}

extern "C" __declspec(dllexport) bool NAFAPI_AddToMapAction(void* obj)
{
	Data::Global::Actions.priority_insert(std::shared_ptr<Data::Action>(static_cast<Data::Action*>(obj)));
	return true;
}

extern "C" __declspec(dllexport) bool NAFAPI_AddToMapAnimationGroup(void* obj)
{
	Data::Global::AnimationGroups.priority_insert(std::shared_ptr<Data::AnimationGroup>(static_cast<Data::AnimationGroup*>(obj)));
	return true;
}

extern "C" __declspec(dllexport) bool NAFAPI_AddToMapFurniture(void* obj)
{
	Data::Global::Furnitures.priority_insert(std::shared_ptr<Data::Furniture>(static_cast<Data::Furniture*>(obj)));
	return true;
}

extern "C" __declspec(dllexport) bool NAFAPI_AddToMapPositionTree(void* obj)
{
	Data::Global::PositionTrees.priority_insert(std::shared_ptr<Data::PositionTree>(static_cast<Data::PositionTree*>(obj)));
	return true;
}

extern "C" __declspec(dllexport) bool NAFAPI_AddToMapGraphInfo(void* obj)
{
	Data::Global::GraphInfos.priority_insert(std::shared_ptr<Data::GraphInfo>(static_cast<Data::GraphInfo*>(obj)));
	return true;
}

extern "C" __declspec(dllexport) void NAFAPI_HotReload(bool RebuildFiles)
{
	Data::Global::HotReload(RebuildFiles);
}

extern "C" __declspec(dllexport) const char* NAFAPI_ValidateSceneParamsIgnoreInScene
	(std::vector<RE::Actor*> a_actors, std::optional<Papyrus::NAF::SceneSettings> a_settings)
{
	auto data = Papyrus::NAF::GetSceneData(a_actors, a_settings);
	if (!data.result) {
		return std::string(data.result.GetErrorMessage()).c_str();
	}

	if (auto res = Scene::SceneManager::ValidateStartSceneArgs(data.settings, true); !res) {
		return std::string(res.GetErrorMessage()).c_str();
	}

	return "";
}

