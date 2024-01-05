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
