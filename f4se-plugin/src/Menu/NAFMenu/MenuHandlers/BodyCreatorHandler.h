#pragma once

namespace Menu::NAF
{
	class BodyCreatorHandler : public BindableMenu<BodyCreatorHandler, SUB_MENU_TYPE::kBodyAnimCreator>
	{
	public:
		using BindableMenu::BindableMenu;

		virtual ~BodyCreatorHandler() {}

		enum Stage
		{
			kManageIK,
			kManageNodes,
			kSelectTarget,
			kSelectAttachChain,
			kSelectAttachNode
		};

		Stage currentStage = kManageNodes;
		bool attachingNode = false;
		std::string attachChain = "";
		size_t attachTarget = 0;

		virtual BindingsVector GetBindings() override
		{
			auto data = PersistentMenuState::CreatorData::GetSingleton();
			manager->SetMenuTitle(data->activeBodyAnim.has_value() ? data->activeBodyAnim.value() : "<Error>");
			BindingsVector result;

			ConfigurePanel({
				{ "Save Changes", Button, Bind(&BodyCreatorHandler::SaveChanges) },
				{ "Bake Animation", Button, Bind(&BodyCreatorHandler::BakeAnim) },
				{ "Select Target", Button, Bind(&BodyCreatorHandler::GotoSelectTarget) },
				{ "Attach IK Target", Button, Bind(&BodyCreatorHandler::GotoAttachChain) }
			});

			switch (currentStage) {
				case kSelectTarget:
				{
					for (size_t i = 0; i < data->studioActors.size(); i++) {
						result.push_back({ GameUtil::GetActorName(data->studioActors[i].actor.get()), Bind(&BodyCreatorHandler::SelectTarget, i) });
					}
					break;
				}
				case kManageNodes:
				{
					result.push_back({ "[Manage IK Chains]", Bind(&BodyCreatorHandler::ManageIKChains) });

					auto nodes = NAFStudioMenu::GetTargetNodes();
					for (size_t i = 0; i < nodes.size(); i++) {
						const auto& n = nodes[i];
						result.push_back({ std::format("[{}] {}", (n.second ? "X" : " "), n.first), Bind(&BodyCreatorHandler::SetNodeVisible, i, !n.second) });
					}
					break;
				}
				case kManageIK:
				{
					auto chains = NAFStudioMenu::GetTargetChains();
					for (const auto& c : chains) {
						result.push_back({ std::format("[{}] {}", (c.second ? "X" : " "), c.first), Bind(&BodyCreatorHandler::SetChainEnabled, c.first, !c.second) });
					}
					break;
				}
				case kSelectAttachChain:
				{
					auto chains = NAFStudioMenu::GetTargetChains();
					for (const auto& c : chains) {
						result.push_back({ c.first, Bind(&BodyCreatorHandler::AttachChainSelected, c.first) });
					}
					break;
				}
				case kSelectAttachNode:
				{
					auto nodes = NAFStudioMenu::GetManagedRefNodes(attachTarget);
					for (auto& n : nodes) {
						result.push_back({ n, Bind(&BodyCreatorHandler::AttachNodeSelected, n) });
					}
					break;
				}
			}
			
			return result;
		}

		void GotoSelectTarget(int) {
			attachingNode = false;
			currentStage = kSelectTarget;
			manager->RefreshList(true);
		}

		void GotoAttachChain(int) {
			currentStage = kSelectAttachChain;
			manager->RefreshList(true);
		}

		void AttachChainSelected(const std::string& id, int) {
			attachChain = id;
			attachingNode = true;
			currentStage = kSelectTarget;
			manager->RefreshList(true);
		}

		void SelectTarget(size_t idx, int) {
			if (!attachingNode) {
				if (auto inst = NAFStudioMenu::GetInstance(); inst != nullptr) {
					inst->SetTarget(idx);
				}
				currentStage = kManageNodes;
			} else {
				attachTarget = idx;
				currentStage = kSelectAttachNode;
			}
			
			manager->RefreshList(true);
		}

		void AttachNodeSelected(std::string nodeName, int) {
			auto data = PersistentMenuState::CreatorData::GetSingleton();
			if (auto inst = NAFStudioMenu::GetInstance(); inst != nullptr) {
				inst->VisitTargetGraph([&](BodyAnimation::NodeAnimationGraph* g) {
					g->ikManager.SetChainTargetParent(attachChain, data->studioActors[attachTarget].actor->GetHandle(), nodeName);
				});
			}
			NAFStudioMenu::ResetTargetChain(attachChain);
			currentStage = kManageNodes;
			manager->RefreshList(true);
		}

		void SaveChanges(int) {
			auto data = PersistentMenuState::CreatorData::GetSingleton();
			if (auto msg = data->GetCannotBeSaved(); msg.has_value()) {
				manager->ShowNotification(msg.value());
				return;
			}

			NAFStudioMenu::SaveChanges();
			if (data->Save()) {
				manager->ShowNotification(std::format("Project saved to {}.xml", data->GetSavePath()));
			} else {
				manager->ShowNotification("Failed to save project.");
			}
		}

		void BakeAnim(int) {
			if (auto res = NAFStudioMenu::BakeAnimation(); res.has_value()) {
				manager->ShowNotification(std::format("Saved baked animation to {}", res.value()));
			} else {
				manager->ShowNotification("Failed to save baked animation.");
			}
		}

		void ManageIKChains(int) {
			currentStage = kManageIK;
			manager->RefreshList(true);
		}

		void SetNodeVisible(size_t idx, bool vis, int) {
			NAFStudioMenu::SetTargetNodeVisible(idx, vis);
			manager->RefreshList(false);
		}

		void SetChainEnabled(const std::string& c, bool enable, int) {
			if (enable) {
				NAFStudioMenu::ResetTargetChain(c);
			}
			NAFStudioMenu::SetTargetChainEnabled(c, enable);
			manager->RefreshList(false);
		}

		virtual void BackImpl() override
		{
			if (currentStage == kManageIK || currentStage == kSelectTarget ||
				currentStage == kSelectAttachChain || currentStage == kSelectAttachNode) {
				currentStage = kManageNodes;
				manager->RefreshList(true);
			} else {
				manager->CloseMenu();
			}
		}
	};
}
