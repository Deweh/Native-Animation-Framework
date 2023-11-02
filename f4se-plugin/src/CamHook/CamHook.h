#pragma once
#include "Misc/Easing.h"
#include "Misc/MathUtil.h"

namespace CamHook
{
	struct AtomicPoint3
	{
		std::atomic<float> x = 0.0f;
		std::atomic<float> y = 0.0f;
		std::atomic<float> z = 0.0f;

		RE::NiPoint3 get() {
			return { x, y, z };
		}

		void operator=(const RE::NiPoint3& rhs) {
			x = rhs.x;
			y = rhs.y;
			z = rhs.z;
		}

		bool IsIdentity() {
			return x == 0.0f && y == 0.0f && z == 0.0f;
		}

		void MakeIdentity() {
			x = 0.0f;
			y = 0.0f;
			z = 0.0f;
		}
	};

	inline static std::atomic<bool> useExternalInput = false;
	inline static std::atomic<bool> useExternalOrbit = false;
	inline static std::atomic<float> leftRightInput = 0.0f;
	inline static std::atomic<float> forwardBackInput = 0.0f;
	inline static std::atomic<float> upDownInput = 0.0f;
	inline static std::atomic<float> turnRightLeftInput = 0.0f;
	inline static std::atomic<float> turnUpDownInput = 0.0f;
	inline static AtomicPoint3 orbitTarget;

	void SetUseExternalInput(bool use)
	{
		useExternalInput = use;
		leftRightInput = 0.0f;
		forwardBackInput = 0.0f;
		upDownInput = 0.0f;
		turnRightLeftInput = 0.0f;
		turnUpDownInput = 0.0f;
	}

	namespace detail
	{
		typedef void(UpdateSig)(RE::FreeCameraState*);

		REL::Relocation<UpdateSig> OriginalUpdate;
		DetourXS updateHook;
		std::atomic<uint16_t> pendingInput = 0;

		bool active = false;
		RE::NiPointer<RE::NiAVObject> lookAtBase;
		RE::NiPointer<RE::NiAVObject> lookAtNode;
		safe_mutex lock;

		bool UpdateLookAtNode() {
			if (lookAtBase != nullptr) {
				auto n = lookAtBase->GetObjectByName(Data::Settings::Values.sLookAtCamTarget.get());
				if (n != nullptr) {
					lookAtNode = RE::NiPointer<RE::NiAVObject>(n);
					return true;
				}
			}
			return false;
		}

		class InputListener : public Data::EventListener<InputListener>
		{
		public:
			InputListener()
			{
				RegisterListener(Data::Events::HUD_E_KEY_DOWN, &InputListener::OnHudKey);
				RegisterListener(Data::Events::HUD_Q_KEY_DOWN, &InputListener::OnHudKey);
				RegisterListener(Data::Events::HUD_E_KEY_UP, &InputListener::OnHudKey);
				RegisterListener(Data::Events::HUD_Q_KEY_UP, &InputListener::OnHudKey);
			}

			void OnHudKey(Data::Events::event_type a_key, Data::Events::EventData&)
			{
				switch (a_key) {
				case Data::Events::HUD_E_KEY_DOWN:
					pendingInput = 2;
					break;
				case Data::Events::HUD_Q_KEY_DOWN:
					pendingInput = 1;
					break;
				case Data::Events::HUD_Q_KEY_UP:
					if (pendingInput == 1)
						pendingInput = 0;
					break;
				case Data::Events::HUD_E_KEY_UP:
					if (pendingInput == 2)
						pendingInput = 0;
					break;
				}
			}

			~InputListener() {
				pendingInput = 0;
			}
		};

		std::unique_ptr<InputListener> inputReceiver = nullptr;

		//RE::NiPoint3 location;
		//RE::NiMatrix3 rotation;

		void HookedUpdate(RE::FreeCameraState* s)
		{
			OriginalUpdate(s);
			bool doUpdate = false;

			std::unique_lock l{ lock };
			if (s->camera && s->camera->cameraRoot) {
				auto camNode = s->camera->cameraRoot;

				if (useExternalInput) {
					if (leftRightInput != 0.0f || forwardBackInput != 0.0f || upDownInput != 0.0f) {
						doUpdate = true;
						auto& mat = camNode->local.rotate;

						RE::NiPoint3 up = { mat.entry[2].pt[0], mat.entry[2].pt[1], mat.entry[2].pt[2] };
						RE::NiPoint3 forward = { mat.entry[1].pt[0], mat.entry[1].pt[1], mat.entry[1].pt[2] };
						RE::NiPoint3 left = { mat.entry[0].pt[0], mat.entry[0].pt[1], mat.entry[0].pt[2] };

						camNode->local.translate += (up * upDownInput);
						camNode->local.translate += (forward * forwardBackInput);
						camNode->local.translate += (left * leftRightInput);

						s->x = camNode->local.translate.x;
						s->y = camNode->local.translate.y;
						s->z = camNode->local.translate.z;
						
						if (useExternalOrbit.load() == true && !orbitTarget.IsIdentity()) {
							auto r = MathUtil::GetLookAtRotation(camNode->local.translate, orbitTarget.get());
							s->yaw = r.x;
							camNode->local.rotate.FromEulerAnglesZXY(s->yaw, s->pitch, 0.0f);
						}
						
					}

					if (turnRightLeftInput != 0.0f || turnUpDownInput != 0.0f) {
						doUpdate = true;
						s->pitch += MathUtil::DegreeToRadian(turnUpDownInput * 0.5f);
						s->yaw += MathUtil::DegreeToRadian(turnRightLeftInput * 0.5f);
						camNode->local.rotate.FromEulerAnglesZXY(s->yaw, s->pitch, 0.0f);
					}
				}

				if (active && Data::Settings::Values.bUseLookAtCam) {
					doUpdate = true;

					//We have to update both the free camera's numbers and the cam node's numbers to avoid input lag.
					switch (pendingInput) {
					case 1:
						s->z += 1.0f;
						camNode->local.translate.z += 1.0f;
						break;
					case 2:
						s->z -= 1.0f;
						camNode->local.translate.z -= 1.0f;
						break;
					}

					auto r = MathUtil::GetLookAtRotation(camNode->local.translate, lookAtNode->world.translate);
					s->pitch = r.y;
					s->yaw = r.x;

					camNode->local.rotate.FromEulerAnglesZXY(r.x, r.y, 0.0f);
				}

				if (doUpdate) {
					RE::NiUpdateData d;
					camNode->Update(d);
				}
			}
		}

		void RegisterHook()
		{
			REL::Relocation<UpdateSig> UpdateLoc{ REL::ID(541110) };

			if (!updateHook.Create(reinterpret_cast<LPVOID>(UpdateLoc.address()), &HookedUpdate)) {
				logger::warn("Failed to create FreeCameraState::Update hook!");
			} else {
				OriginalUpdate = reinterpret_cast<uintptr_t>(updateHook.GetTrampoline());
			}
		}
	}

	void SetActive(bool a_active)
	{
		std::unique_lock l{ detail::lock };
		detail::active = a_active;

		if (detail::active) {
			detail::inputReceiver = std::make_unique<detail::InputListener>();
		} else {
			detail::lookAtNode = nullptr;
			detail::lookAtBase = nullptr;
			detail::inputReceiver.reset();
		}
	}

	bool LookAt(RE::NiAVObject* a_lookAtBase)
	{
		if (!a_lookAtBase) {
			return false;
		}

		std::unique_lock l{ detail::lock };
		detail::lookAtBase = RE::NiPointer<RE::NiAVObject>(a_lookAtBase);
		if (detail::UpdateLookAtNode()) {
			SetActive(true);
			return true;
		} else {
			SetActive(false);
			return false;
		}
	}

	void OnSettingsChanged(Data::Events::event_type, Data::Events::EventData&)
	{
		std::unique_lock l{ detail::lock };
		if (detail::active && !detail::UpdateLookAtNode()) {
			SetActive(false);
		}
	}

	inline static bool settingsListenerRegistered = ([]() {
		Data::Events::Subscribe(Data::Events::SETTINGS_CHANGED, &OnSettingsChanged);
		return true;
	})();

	void RegisterHook() {
		detail::RegisterHook();
	}

	/*
	void SetPosition(const RE::NiPoint3& a_pos) {
		std::unique_lock l{ detail::lock };
		detail::location = a_pos;
	}

	
	void SetRotation(const RE::NiPoint3& a_rot)
	{
		std::unique_lock l{ detail::lock };
		detail::rotation.MakeIdentity();
		detail::rotation.FromEulerAnglesZXY(a_rot.z, a_rot.x, a_rot.y);
	}
	
	void SetLookAt(const RE::NiPoint3& a_target, float a_distance, float a_direction, float a_height) {
		auto p = MathUtil::MovePointAlongXY(a_target, a_direction, a_distance);
		p.z += a_height;
		SetPosition(p);
		auto r = MathUtil::GetLookAtRotation(p, a_target);
		SetRotation(r);
	}
	*/
}
