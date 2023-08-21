#pragma once

namespace Scene
{
	namespace DynamicIdle
	{
		inline static safe_mutex lock;
		void Play(RE::Actor* a,
			const std::string& filename,
			const std::optional<std::string>& animEventOpt = std::nullopt,
			const std::optional<std::string>& behaviorGraphOpt = std::nullopt)
		{
			std::string animEvent = "dyn_ActivationLoop";
			std::string behaviorGraph = "Actors\\Character\\Behaviors\\RaiderRootBehavior.hkx";
			if (animEventOpt.has_value())
				animEvent = animEventOpt.value();
			if (behaviorGraphOpt.has_value())
				behaviorGraph = behaviorGraphOpt.value();

			std::unique_lock l{ lock };

			Data::Forms::NAFDynIdle->animFileName = filename;
			Data::Forms::NAFDynIdle->animEventName = animEvent;

			if (behaviorGraph == "Actors\\Character\\Behaviors\\RaiderRootBehavior.hkx" &&
				a->race != nullptr && a->race->formEditorID != "HumanRace" && a->race->rootBehaviorGraphName[0].size() > 0) {
				Data::Forms::NAFDynIdle->behaviorGraphName = a->race->rootBehaviorGraphName[0];
			} else {
				Data::Forms::NAFDynIdle->behaviorGraphName = behaviorGraph;
			}

			if (a != nullptr && a->currentProcess != nullptr) {
				a->currentProcess->PlayIdle(a, 0x35, Data::Forms::NAFDynIdle);
			}
		}
	}
}
