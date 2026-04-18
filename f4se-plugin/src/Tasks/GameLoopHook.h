#pragma once

namespace Tasks
{
	namespace GameLoopHook
	{
		struct ActorAlignInfo
		{
			RE::NiPointer<RE::Actor> actor;
			RE::NiPoint3 location;
			RE::NiPoint3 angle;
		};

		typedef bool(ProcessQueues)(void*, float, uint32_t);

		REL::Relocation<ProcessQueues> OriginalProcessQueues;

		std::mutex alignActorsLock;
		std::vector<ActorAlignInfo> alignActors;

		std::vector<ActorAlignInfo>::iterator GetAlignActorsIter(RE::Actor* a_actor)
		{
			auto iter = alignActors.begin();
			for (;iter != alignActors.end(); iter++)
			{
				if (iter->actor.get() == a_actor) {
					break;
				}
			}
			return iter;
		}

		void ForceAlignActor(RE::Actor* a_actor, const RE::NiPoint3& a_location, const RE::NiPoint3& a_angle)
		{
			if (!a_actor)
				return;

			std::unique_lock l{ alignActorsLock };
			if (auto iter = GetAlignActorsIter(a_actor); iter != alignActors.end()) {
				iter->location = a_location;
				iter->angle = a_angle;
			} else {
				alignActors.emplace_back(RE::NiPointer<RE::Actor>(a_actor), a_location, a_angle);
			}
		}

		bool StopAligningActor(RE::Actor* a_actor)
		{
			std::unique_lock l{ alignActorsLock };
			if (auto iter = GetAlignActorsIter(a_actor); iter != alignActors.end()) {
				alignActors.erase(iter);
				return true;
			}
			return false;
		}

		bool HookedGameLoop(void* qintfc, float unk01, uint32_t unk02) {
			bool res = OriginalProcessQueues(qintfc, unk01, unk02);

			Scene::SceneManager::UpdateScenes();
			Scene::OrderedActionQueue::Update();

			std::unique_lock l{ alignActorsLock };
			for (const auto& info : alignActors) {
				if (!MathUtil::CoordsWithinError(info.actor->data.angle, info.angle)) {
					info.actor->SetAngleOnReference(info.angle);
				}

				if (!MathUtil::CoordsWithinError(info.actor->data.location, info.location)) {
					info.actor->SetPosition(info.location, true);
					info.actor->DisableCollision();
					info.actor->SetNoCollision(true);
				}
			}

			return res;
		}

		void RegisterHook(F4SE::Trampoline& trampoline)
		{
			//Main::UpdateNonRenderSafeAITasks + 0x1E
			//Call to TaskQueueInterface::ProcessQueues
			REL::Relocation<ProcessQueues> updateCallLoc{ REL::ID(338354), 0x1E };

			OriginalProcessQueues = trampoline.write_call<5>(updateCallLoc.address(), &HookedGameLoop);
		}
	}
}
