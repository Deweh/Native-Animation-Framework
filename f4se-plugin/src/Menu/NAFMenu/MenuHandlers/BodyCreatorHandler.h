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
			kManageNodes
		};

		Stage currentStage = kManageNodes;

		virtual BindingsVector GetBindings() override
		{
			auto data = PersistentMenuState::CreatorData::GetSingleton();
			manager->SetMenuTitle(data->activeBodyAnim.has_value() ? data->activeBodyAnim.value() : "<Error>");
			BindingsVector result;

			ConfigurePanel({
				{ "Save Changes", Button, Bind(&BodyCreatorHandler::SaveChanges) },
				{ "Bake Animation", Button, Bind(&BodyCreatorHandler::BakeAnim) }
			});

			switch (currentStage) {
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
			}
			
			return result;
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
			if (currentStage == kManageIK) {
				currentStage = kManageNodes;
				manager->RefreshList(true);
			} else {
				manager->CloseMenu();
			}
		}
	};
}
