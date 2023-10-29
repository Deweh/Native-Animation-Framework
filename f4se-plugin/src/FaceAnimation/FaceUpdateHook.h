#pragma once
#include <shared_mutex>
#include "Serialization/General.h"
#include "FaceAnimation/Animation.h"

namespace FaceAnimation
{
	namespace FaceUpdateHook
	{
		using namespace Serialization::General;

		struct FaceData
		{
			struct AnimInfo
			{
				std::string animationId = "";
				bool loop = false;
				bool havokSync = false;
			};

			std::unique_ptr<FaceAnimation> anim = nullptr;
			std::string animationId = "";
			std::optional<EyeVector> eyeOverride = std::nullopt;
			std::optional<AnimInfo> animBackup = std::nullopt;
			GameUtil::GraphTime syncInfoCache;

			template <class Archive>
			void save(Archive& ar, const uint32_t) const
			{
				if (anim == nullptr) {
					ar(animationId, eyeOverride, false);
				} else {
					ar(animationId, eyeOverride, true, anim->timeElapsed, anim->loop, anim->havokSync);
				}
			}

			template <class Archive>
			void load(Archive& ar, const uint32_t)
			{
				bool hasAnim;
				ar(animationId, eyeOverride, hasAnim);
				if (hasAnim) {
					auto loadedAnim = std::make_unique<FaceAnimation>();
					ar(loadedAnim->timeElapsed, loadedAnim->loop, loadedAnim->havokSync);

					if (loadedAnim->LoadData(animationId)) {
						anim = std::move(loadedAnim);
					}
				}
			}
		};

		struct PersistentState
		{
			std::unordered_map<RE::BSFaceGenAnimationData*, SerializableActorHandle> managedDatas;
			std::unordered_map<SerializableActorHandle, FaceData> managedAnims;

			template <class Archive>
			void save(Archive& ar, const uint32_t) const
			{
				ar(managedAnims);
			}

			template <class Archive>
			void load(Archive& ar, const uint32_t)
			{
				ar(managedAnims);
				for (auto& pair : managedAnims) {
					auto d = GameUtil::GetFaceAnimData(pair.first.get().get());
					if (d != nullptr) {
						managedDatas[d] = pair.first;
					}
				}
			}
		};

		typedef bool(Update)(RE::BSFaceGenAnimationData*, float, bool, float);
		typedef bool(UpdateLip)(RE::BSFaceGenAnimationData*, float);

		static REL::Relocation<Update> OriginalUpdate;
		static REL::Relocation<UpdateLip> OriginalUpdateLip;
		inline static std::unique_ptr<PersistentState> state = std::make_unique<PersistentState>();
		static std::shared_mutex stateLock;
		static std::unordered_map<SerializableActorHandle, std::string> loadingAnims;
		static std::mutex loadingAnimsLock;
		static std::unordered_map<SerializableActorHandle, RE::NiPointer<RE::BSGeometry>> eyeGeoCache;
		static std::shared_mutex geoCacheLock;

		RE::BSGeometry* GetCachedEyeGeometry(SerializableActorHandle hndl)
		{
			RE::BSGeometry* result = nullptr;
			std::shared_lock ls{ geoCacheLock };
			auto cacheIter = eyeGeoCache.find(hndl);
			if (cacheIter != eyeGeoCache.end()) {
				result = cacheIter->second.get();
			}
			if (!result) {
				result = GameUtil::GetEyeGeometry(hndl.get().get());
				ls.unlock();
				std::unique_lock le{ geoCacheLock };
				eyeGeoCache[hndl] = RE::NiPointer<RE::BSGeometry>(result);
			}
			return result;
		}

		SerializableActorHandle IsDataManaged(RE::BSFaceGenAnimationData* data) {
			SerializableActorHandle result;
			auto iter = state->managedDatas.find(data);
			if (iter != state->managedDatas.end()) {
				result = iter->second;
			}
			return result;
		}

		void EraseIfEmpty_NonThreadSafe(RE::ActorHandle targetActor)
		{
			if (state->managedAnims[targetActor].anim == nullptr && !state->managedAnims[targetActor].eyeOverride.has_value()) {
				state->managedAnims.erase(targetActor);
				for (auto iter = state->managedDatas.begin(); iter != state->managedDatas.end();) {
					if (iter->second.hash() == targetActor.native_handle_const()) {
						iter = state->managedDatas.erase(iter);
						break;
					} else {
						iter++;
					}
				}
				std::unique_lock l{ geoCacheLock };
				eyeGeoCache.erase(targetActor);
			}
		}

		bool HookedUpdateLip(RE::BSFaceGenAnimationData* data, float ptimeDelta) {
			std::shared_lock l{ stateLock };

			if (auto h = IsDataManaged(data); h) {
				l.unlock();
				return true;
			}

			l.unlock();
			return OriginalUpdateLip(data, ptimeDelta);
		}

		bool HookedUpdate(RE::BSFaceGenAnimationData* data, float timeDelta, bool unk01, float pGameTime)
		{
			bool result = OriginalUpdate(data, timeDelta, unk01, pGameTime);
			std::shared_lock l{ stateLock };

			if (auto h = IsDataManaged(data); h) {
				auto eyeGeo = GetCachedEyeGeometry(h);
				auto a = state->managedAnims.find(h);
				bool pendingDelete = false;
				RE::BSAutoLock bl{ data->instanceData.lock };

				if (a->second.anim != nullptr) {
					if (!a->second.anim->havokSync) {
						if (!a->second.anim->Update(data, eyeGeo, timeDelta)) {
							pendingDelete = true;
						}
					} else {
						const auto actor = h.get().get();
						auto& syncInfo = a->second.syncInfoCache;
						std::scoped_lock al{ a->second.anim->lock };
						if (!a->second.anim->paused && BodyAnimation::SmartIdle::GetGraphTime(actor, syncInfo) && syncInfo.current > 0.0f) {
							a->second.anim->timeElapsed = a->second.anim->loop ? std::fmod(syncInfo.current, a->second.anim->data.duration) : syncInfo.current;
						}
						a->second.anim->UpdateNoDelta(data, eyeGeo);
					}
				}

				if (a->second.eyeOverride.has_value()) {
					auto& eyes = a->second.eyeOverride.value();
					GameUtil::SetEyeCoords(eyeGeo, static_cast<float>(eyes.u), static_cast<float>(eyes.v));
				}

				l.unlock();

				if (pendingDelete) {
					std::unique_lock ul{ stateLock };
					a->second.anim = nullptr;
					EraseIfEmpty_NonThreadSafe(a->first);
				}
			}
			return result;
		}

		//Once an animation is passed to this function, it is considered "managed" by FaceUpdateHook.
		//If the animation needs to be accessed and/or modified after this point, use VisitAnimation().
		void StartAnimation(RE::ActorHandle targetActor, std::unique_ptr<FaceAnimation> anim, std::string id = "", bool animOverride = false)
		{
			auto a = targetActor.get();
			if (a == nullptr) {
				return;
			}
			std::unique_lock l{ stateLock };
			auto& managedActor = state->managedAnims[targetActor];
			if (managedActor.animBackup.has_value() && !animOverride) {
				managedActor.animBackup = { id, anim->loop, anim->havokSync };
				return;
			}
			if (animOverride && managedActor.anim != nullptr) {
				managedActor.animBackup = { managedActor.animationId, managedActor.anim->loop, managedActor.anim->havokSync };
			}
			anim->SetStartNow();
			managedActor.anim = std::move(anim);
			managedActor.animationId = id;
			state->managedDatas[GameUtil::GetFaceAnimData(a.get())] = targetActor;
		}

		bool LoadAndPlayAnimation(RE::ActorHandle targetActor, std::string id, bool loop = false, bool havokSync = false)
		{
			if (auto targetAnim = Data::GetFaceAnim(id); targetAnim == nullptr) {
				return false;
			}

			auto newAnim = std::make_unique<FaceAnimation>();
			newAnim->loop = loop;
			newAnim->havokSync = havokSync;

			std::unique_lock l{ loadingAnimsLock };
			loadingAnims[targetActor] = id;
			l.unlock();

			std::thread([targetActor = std::move(targetActor), inst = std::move(newAnim), id = std::move(id)]() mutable {
				bool successful = inst->LoadData(id);
				std::unique_lock l{ loadingAnimsLock };
				if (loadingAnims.contains(targetActor) && loadingAnims[targetActor] == id) {
					loadingAnims.erase(targetActor);
					if (successful) {
						StartAnimation(targetActor, std::move(inst), id);
					}
				}
			}).detach();

			return true;
		}

		void StopAnimation(RE::ActorHandle targetActor, bool animOverride = false) {
			std::unique_lock l1{ loadingAnimsLock };
			if (loadingAnims.contains(targetActor)) {
				loadingAnims.erase(targetActor);
			}
			std::unique_lock l2{ stateLock };
			auto managedActor = state->managedAnims.find(targetActor);

			if (managedActor != state->managedAnims.end()) {
				if (!managedActor->second.animBackup.has_value()) {
					managedActor->second.anim = nullptr;
					state->managedAnims[targetActor].animationId = "";
					EraseIfEmpty_NonThreadSafe(targetActor);
				} else {
					if (animOverride) {
						FaceData::AnimInfo animInfo = managedActor->second.animBackup.value();
						managedActor->second.animBackup = std::nullopt;
						l1.unlock();
						l2.unlock();
						LoadAndPlayAnimation(targetActor, animInfo.animationId, animInfo.loop, animInfo.havokSync);
					} else {
						managedActor->second.animBackup = std::nullopt;
					}
				}
			}
		}

		//Thread-safe method for modifying an animation object already playing on an actor.
		bool VisitAnimation(RE::ActorHandle targetActor, std::function<void(FaceAnimation*)> visitFunc) {
			std::shared_lock l1{ stateLock };
			if (state->managedAnims.contains(targetActor) && state->managedAnims[targetActor].anim != nullptr) {
				auto& anim = state->managedAnims[targetActor].anim;
				std::scoped_lock l2{ anim->lock };
				visitFunc(anim.get());
				return true;
			} else {
				return false;
			}
		}

		void SetEyeOverride(RE::ActorHandle targetActor, double u, double v)
		{
			std::unique_lock l{ stateLock };
			state->managedAnims[targetActor].eyeOverride = EyeVector{ u * -0.25, v * 0.2 };
			state->managedDatas[GameUtil::GetFaceAnimData(targetActor.get().get())] = targetActor;
		}

		void ClearEyeOverride(RE::ActorHandle targetActor)
		{
			std::unique_lock l{ stateLock };
			if (state->managedAnims.contains(targetActor)) {
				state->managedAnims[targetActor].eyeOverride = std::nullopt;
				EraseIfEmpty_NonThreadSafe(targetActor);
			}
		}

		void Reset() {
			std::scoped_lock l{ loadingAnimsLock, stateLock, geoCacheLock };
			state = std::make_unique<PersistentState>();
			loadingAnims.clear();
			eyeGeoCache.clear();
		}

		void RegisterHook(F4SE::Trampoline& trampoline)
		{
			//BSFaceGenNiNode::UpdateMorphing + 0xBA
			//Call to BSFaceGenAnimationData::Update
			REL::Relocation<Update> faceGenUpdateCallLoc{ REL::ID(317245), 0xBA };

			//BSFaceGenAnimationData::Update + 0x158
			//Call to BSFaceGenAnimationData::UpdateMorphsFromLip
			REL::Relocation<UpdateLip> updateLipMorphsCallLoc{ REL::ID(3139), 0x158 };

			OriginalUpdate = trampoline.write_call<5>(faceGenUpdateCallLoc.address(), &HookedUpdate);
			OriginalUpdateLip = trampoline.write_call<5>(updateLipMorphsCallLoc.address(), &HookedUpdateLip);
		}
	}
}
