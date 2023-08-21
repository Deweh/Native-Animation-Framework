#pragma once

namespace Tasks
{
	namespace GameLoopHook
	{
		typedef bool(ProcessQueues)(void*, float, uint32_t);

		static REL::Relocation<ProcessQueues> OriginalProcessQueues;

		bool HookedGameLoop(void* qintfc, float unk01, uint32_t unk02) {
			bool res = OriginalProcessQueues(qintfc, unk01, unk02);
			Scene::SceneManager::UpdateScenes();
			Scene::OrderedActionQueue::Update();
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
