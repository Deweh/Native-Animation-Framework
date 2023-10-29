#pragma once
#include "Menu/NAFHUDMenu/SceneHUD.h"

namespace Scene
{
	class BaseControlSystem : public IControlSystem
	{
	public:
		enum StateFlag : uint32_t
		{
			kStarted = 1u << 0,
			kEndQueued = 1u << 1,
			kEnding = 1u << 2
		};

		std::shared_ptr<const Data::Animation> currentAnim;
		RE::stl::enumeration<StateFlag> state;
		std::string id;
		std::string queuedNextId = "";
		std::string startMorphSet;
		std::string stopMorphSet;

		virtual std::string QSystemID() override {
			return id;
		}

		virtual std::string QAnimationID() override {
			if (currentAnim != nullptr) {
				return currentAnim->id;
			} else {
				return "";
			}
		}

		virtual void OnAnimationLoop(IControllable* scn) override
		{
			scn->ReportSystemCompletion();
		}

		virtual void SetInfo(Data::Position::ControlSystemInfo* info) override {
			id = info->id;
			currentAnim = info->anim;
			startMorphSet = info->startMorphSet;
			stopMorphSet = info->stopMorphSet;
		}

		virtual void OnBegin(IControllable* scn, std::string_view) override {
			scn->ApplyMorphSet(startMorphSet);
			scn->TransitionToAnimation(currentAnim);
			state.set(StateFlag::kStarted);
			
			if (state.any(StateFlag::kEndQueued)) {
				OnEnd(scn, queuedNextId);
			}
		}

		virtual void OnEnd(IControllable* scn, std::string_view nextId) override {
			state.set(StateFlag::kEndQueued);

			if (!state.any(StateFlag::kStarted)) {
				queuedNextId = nextId;
				return;
			}

			state.set(StateFlag::kEnding);

			scn->ApplyMorphSet(stopMorphSet);
			scn->PushQueuedControlSystem();
		}

		template <class Archive>
		void serialize(Archive& ar, const uint32_t)
		{
			ar(cereal::base_class<IControlSystem>(this), currentAnim, state._impl, id, queuedNextId, startMorphSet, stopMorphSet);
		}
	};

	class AnimationGroupControlSystem : public BaseControlSystem
	{
	public:
		std::shared_ptr<const Data::AnimationGroup> group;
		size_t currentStage = 0;
		uint32_t loopsRemaining = 1;
		std::vector<std::pair<uint32_t, size_t>> weights;

		virtual void SetInfo(Data::Position::ControlSystemInfo* info) override
		{
			BaseControlSystem::SetInfo(info);
			if (info->type == Data::kAnimationGroupSystem) {
				auto groupInfo = static_cast<Data::Position::AnimationGroupInfo*>(info);
				group = groupInfo->group;

				if (!group->sequential) {
					for (size_t i = 0; i < group->stages.size(); i++) {
						weights.push_back({ group->stages[i].weight, i });
					}
				}
			}
		}

		virtual void OnBegin(IControllable* scn, std::string_view lastId) override
		{
			SetLoopInfo(group->stages[currentStage]);
			BaseControlSystem::OnBegin(scn, lastId);
		}

		void SetLoopInfo(const Data::AnimationGroup::Stage& s) {
			loopsRemaining = Utility::RandomNumber(s.loopMin, s.loopMax);
			if (loopsRemaining < 1)
				loopsRemaining = 1;
		}

		void TransitionToStage(IControllable* scn, size_t index) {
			currentStage = index;
			const Data::AnimationGroup::Stage& targetStage = group->stages[currentStage];
			SetLoopInfo(targetStage);

			currentAnim = Data::GetAnimation(targetStage.animation);
			scn->TransitionToAnimation(currentAnim);
		}

		virtual void OnAnimationLoop(IControllable* scn) override
		{
			loopsRemaining--;
			if (loopsRemaining == 0) {
				if (group->sequential) {
					if ((currentStage + 1) < group->stages.size()) {
						TransitionToStage(scn, currentStage + 1);
					} else {
						TransitionToStage(scn, 0);
						scn->ReportSystemCompletion();
					}
				} else {
					TransitionToStage(scn, Utility::SelectWeightedRandom(weights));
					scn->ReportSystemCompletion();
				}
			}
		}

		template <class Archive>
		void serialize(Archive& ar, const uint32_t)
		{
			ar(cereal::base_class<BaseControlSystem>(this), group, currentStage, loopsRemaining, weights);
		}
	};

	class EndingControlSystem : public IControlSystem
	{
	public:
		void OnBegin(IControllable* scn, std::string_view) override
		{
			F4SE::GetTaskInterface()->AddTask([uid = scn->QUID()] { SceneManager::StopScene(uid); });
		}

		template <class Archive>
		void serialize(Archive& ar, const uint32_t)
		{
			ar(cereal::base_class<IControlSystem>(this));
		}
	};

	class PositionTreeControlSystem : public IControlSystem, public IControllable, public Data::EventListener<PositionTreeControlSystem>
	{
	public:
		enum TreeNodeState : uint8_t
		{
			SubSystemCompleted = 1u << 0,
			TimerCompleted = 1u << 1
		};

		std::string id = "";
		IControllable* parent = nullptr;
		uint64_t parentId = 0;
		std::unique_ptr<IControlSystem> subSystem;
		std::unique_ptr<IControlSystem> queuedSubSystem;
		std::shared_ptr<const Data::PositionTree> tree;
		std::shared_ptr<const Data::PositionTree::Node> currentNode;
		bool hudActive = false;
		bool autoAdvance = false;
		size_t nextIndex = 0;
		F4SE::stl::enumeration<TreeNodeState, uint8_t> nodeState;

		virtual uint64_t QUID() override
		{
			return parentId;
		}

		virtual bool QAutoAdvance() override
		{
			return autoAdvance;
		}

		void CheckParentPointer()
		{
			if (parent == nullptr) {
				std::shared_ptr<IScene> scn;
				if (SceneManager::GetScene(parentId, scn)) {
					parent = scn.get();
				}
			}
		}

		virtual bool HasPlayer() override
		{
			CheckParentPointer();
			if (parent != nullptr)
				return parent->HasPlayer();
			return false;
		}

		virtual bool PushQueuedControlSystem() override
		{
			if (queuedSubSystem != nullptr) {
				std::string lastId = "";
				if (subSystem != nullptr) {
					lastId = subSystem->QAnimationID();
				}
				subSystem.reset(queuedSubSystem.release());
				queuedSubSystem = nullptr;
				subSystem->OnBegin(this, lastId);
				return true;
			} else {
				CheckParentPointer();
				if (parent != nullptr)
					return parent->PushQueuedControlSystem();
				return false;
			}
		}

		virtual void TransitionToAnimation(std::shared_ptr<const Data::Animation> anim) override
		{
			CheckParentPointer();
			if (parent != nullptr)
				parent->TransitionToAnimation(anim);
		}

		virtual void SoftEnd() override {
			CheckParentPointer();
			if (parent != nullptr)
				parent->SoftEnd();
		}

		virtual void ApplyMorphSet(const std::string& a_id) override
		{
			CheckParentPointer();
			if (parent != nullptr)
				parent->ApplyMorphSet(a_id);
		}

		virtual void StartTimer(uint16_t a_id, double durationMs) override
		{
			CheckParentPointer();
			if (parent != nullptr)
				parent->StartTimer(a_id, durationMs);
		}

		virtual void StopTimer(uint16_t a_id) override
		{
			CheckParentPointer();
			if (parent != nullptr)
				parent->StopTimer(a_id);
		}

		virtual void ReportSystemCompletion() override {
			nodeState.set(TreeNodeState::SubSystemCompleted);
			Advance(false, true);
		}

		void QueueSubSystem(std::unique_ptr<IControlSystem> sys)
		{
			queuedSubSystem = std::move(sys);
			std::string nextId = queuedSubSystem->QAnimationID();
			if (subSystem != nullptr) {
				subSystem->OnEnd(this, nextId);
			} else {
				PushQueuedControlSystem();
			}
		}

		bool SwitchToNode(std::shared_ptr<const Data::PositionTree::Node> n)
		{
			auto p = Data::GetPosition(n->position);
			if (!p)
				return false;

			auto sys = GetControlSystem(p, true);
			if (!sys)
				return false;

			nodeState._impl = 0;
			nextIndex = 0;
			currentNode = n;

			QueueSubSystem(std::move(sys));
			return true;
		}

		virtual void SetInfo(Data::Position::ControlSystemInfo* info) override
		{
			if (info->type == Data::kPositionTreeSystem) {
				id = info->id;
				auto treeInfo = static_cast<Data::Position::PositionTreeInfo*>(info);
				tree = treeInfo->tree;
				currentNode = tree->tree;
			}
		}

		virtual void OnBegin(IControllable* scn, std::string_view lastId) override
		{
			parentId = scn->QUID();
			CheckParentPointer();

			if (scn->HasPlayer()) {
				hudActive = true;
				autoAdvance = scn->QAutoAdvance();
			} else {
				autoAdvance = true;
			}

			subSystem = GetControlSystem(Data::GetPosition(currentNode->position), true);
			if (subSystem != nullptr)
				subSystem->OnBegin(this, lastId);

			if (autoAdvance) {
				Advance(true);
			} else if (hudActive) {
				RegisterListener(Data::Events::HUD_DOWN_KEY_DOWN, &PositionTreeControlSystem::OnHudKey);
				RegisterListener(Data::Events::HUD_UP_KEY_DOWN, &PositionTreeControlSystem::OnHudKey);
				RegisterListener(Data::Events::HUD_LEFT_KEY_DOWN, &PositionTreeControlSystem::OnHudKey);
				RegisterListener(Data::Events::HUD_RIGHT_KEY_DOWN, &PositionTreeControlSystem::OnHudKey);
				UpdateHUDState();
			}	
		}

		virtual void OnAnimationLoop(IControllable*) override
		{
			if (subSystem != nullptr)
				subSystem->OnAnimationLoop(this);
		}

		virtual void OnEnd(IControllable* scn, std::string_view nextId) override
		{
			if (hudActive) {
				UnregisterListener(Data::Events::HUD_DOWN_KEY_DOWN);
				UnregisterListener(Data::Events::HUD_UP_KEY_DOWN);
				UnregisterListener(Data::Events::HUD_LEFT_KEY_DOWN);
				UnregisterListener(Data::Events::HUD_RIGHT_KEY_DOWN);
				Menu::SceneHUD::Clear();
			}
			if (subSystem != nullptr) {
				subSystem->OnEnd(scn, nextId);
			} else {
				scn->PushQueuedControlSystem();
			}
		}

		virtual std::string QAnimationID() override
		{
			if (subSystem)
				return subSystem->QAnimationID();
			return "";
		}

		virtual std::string QSystemID() override
		{
			if (subSystem) {
				return subSystem->QSystemID();
			} else {
				return id;
			}
		}

		virtual void OnTimer(uint16_t a_id) override
		{
			if (a_id == 100) {
				nodeState.set(TreeNodeState::TimerCompleted);
				Advance(false, true);
			} else if (subSystem != nullptr) {
				subSystem->OnTimer(a_id);
			}
		}

		void Advance(bool noChange = false, bool checkConditions = false) {
			if (checkConditions && (!currentNode || !autoAdvance || !nodeState.any(TreeNodeState::TimerCompleted) ||
				(currentNode->forceComplete && !nodeState.any(TreeNodeState::SubSystemCompleted)))) {
				return;
			}

			if (!noChange) {
				if (currentNode->children.size() > 0) {
					std::shared_ptr<const Data::PositionTree::Node> nextNode;
					if (nextIndex < currentNode->children.size()) {
						nextNode = currentNode->children[nextIndex];
					} else {
						nextNode = currentNode->children[0];
					}
					SwitchToNode(nextNode);
				} else {
					SoftEnd();
					return;
				}
			}

			CheckParentPointer();
			double timerDur = Data::Settings::Values.iDefaultSceneDuration;

			if (currentNode->duration > 0.5f) {
				timerDur = currentNode->duration;
			} else if (parent != nullptr && parent->QDuration() > 0.5) {
				timerDur = parent->QDuration();
			}


			StartTimer(100, timerDur * 1000);

			if (currentNode->children.size() > 0) {
				nextIndex = Utility::RandomNumber(0ui64, currentNode->children.size() - 1);
			}
			UpdateHUDState(timerDur);
		}

		void OnHudKey(Data::Events::event_type e, Data::Events::EventData&)
		{
			SceneManager::VisitScene(parentId, [&](IScene* scn) {
				scn->controlSystem->Notify(e);
			});
		}

		void UpdateHUDState(double nodeDuration = 0.0)
		{
			if (hudActive) {
				Menu::SceneHUD::StateData result;
				result.fillDur = nodeDuration;
				result.disabled = autoAdvance;
				result.selectedOption = nextIndex;
				result.moreLeft = (currentNode->parent.lock() != nullptr);
				result.currentItem = currentNode->id;
				if (currentNode->children.size() > 0) {
					result.moreRight = currentNode->children[nextIndex]->children.size() > 0;
					for (auto& c : currentNode->children) {
						result.nextOptions.push_back(c->id);
					}
				}
				Menu::SceneHUD::PushState(result);
			}
		}

		void PlayOKSound()
		{
			RE::UIUtils::PlayMenuSound("UIMenuOK");
		}

		void PlayCancelSound()
		{
			RE::UIUtils::PlayMenuSound("UIMenuCancel");
		}

		virtual void Notify(const std::any& info) override {
			if (autoAdvance) {
				PlayCancelSound();
				return;
			}
			using Data::Events;
			if (const Events::event_type* e = std::any_cast<Events::event_type>(&info); e) {
				switch (*e) {
					case Events::HUD_DOWN_KEY_DOWN:
					{
						if (currentNode->children.size() > 1) {
							if (!(++nextIndex < currentNode->children.size())) {
								nextIndex = 0;
							}
							UpdateHUDState();
							PlayOKSound();
						} else {
							PlayCancelSound();
						}
						break;
					}
					case Events::HUD_UP_KEY_DOWN:
					{
						if (currentNode->children.size() > 1) {
							if (!(nextIndex --> 0)) {
								nextIndex = (currentNode->children.size() - 1);
							}
							UpdateHUDState();
							PlayOKSound();
						} else {
							PlayCancelSound();
						}
						break;
					}
					case Events::HUD_LEFT_KEY_DOWN:
					{
						auto c = currentNode;
						auto p = currentNode->parent.lock();
						if (p != nullptr && SwitchToNode(p)) {
							for (size_t i = 0; i < currentNode->children.size(); i++) {
								if (currentNode->children[i] == c) {
									nextIndex = i;
									break;
								}
							}
							UpdateHUDState();
							PlayOKSound();
						} else {
							PlayCancelSound();
						}
						break;
					}
					case Events::HUD_RIGHT_KEY_DOWN:
					{
						if (currentNode->children.size() > 0 && SwitchToNode(currentNode->children[nextIndex])) {
							UpdateHUDState();
							PlayOKSound();
						} else {
							PlayCancelSound();
						}
						break;
					}
				}
			}
		}

		template <class Archive>
		void serialize(Archive& ar, const uint32_t ver)
		{
			ar(cereal::base_class<IControlSystem>(this), parentId, subSystem, queuedSubSystem, tree, currentNode,
				hudActive, autoAdvance, nextIndex, nodeState._impl);

			if (ver >= 1) {
				ar(id);
			}
		}
	};
}

CEREAL_REGISTER_TYPE(Scene::BaseControlSystem);
CEREAL_REGISTER_TYPE(Scene::AnimationGroupControlSystem);
CEREAL_REGISTER_TYPE(Scene::EndingControlSystem);
CEREAL_REGISTER_TYPE(Scene::PositionTreeControlSystem);

CEREAL_CLASS_VERSION(Scene::PositionTreeControlSystem, 1);
