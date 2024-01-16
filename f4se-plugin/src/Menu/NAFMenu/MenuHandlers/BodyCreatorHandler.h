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
			kManageIKChain,
			kSelectAttachNode,
			kPackage,
			kSettings
		};

		Stage currentStage = kManageNodes;
		bool attachingNode = false;
		size_t attachTarget = 0;
		std::string activeChain = "";

		std::string pkgId = "";
		bool restrictGenders = true;
		bool useScales = true;

		virtual BindingsVector GetBindings() override
		{
			auto data = PersistentMenuState::CreatorData::GetSingleton();
			BindingsVector result;

			ConfigurePanel({
				{ "Save Changes", Button, Bind(&BodyCreatorHandler::SaveChanges) },
				{ "Bake Animation(s)", Button, Bind(&BodyCreatorHandler::BakeAnim) },
				{ "Package Animation(s)", Button, Bind(&BodyCreatorHandler::GotoPackageAnim) },
				{ "Manage IK Chains", Button, Bind(&BodyCreatorHandler::ManageIKChains) },
				{ "Select Target", Button, Bind(&BodyCreatorHandler::GotoSelectTarget) },
				{ "Settings", Button, Bind(&BodyCreatorHandler::GotoSettings) }
			});

			switch (currentStage) {
				case kSelectTarget:
				{
					manager->SetMenuTitle("Select Target");
					for (size_t i = 0; i < data->studioActors.size(); i++) {
						result.push_back({ GameUtil::GetActorName(data->studioActors[i].actor.get()), Bind(&BodyCreatorHandler::SelectTarget, i) });
					}
					break;
				}
				case kManageNodes:
				{
					manager->SetMenuTitle("Node Visibility");
					auto nodes = NAFStudioMenu::GetTargetNodes();
					for (size_t i = 0; i < nodes.size(); i++) {
						const auto& n = nodes[i];
						result.push_back({ std::format("[{}] {}", (n.second ? "X" : " "), n.first), Bind(&BodyCreatorHandler::SetNodeVisible, i, !n.second) });
					}
					break;
				}
				case kManageIK:
				{
					manager->SetMenuTitle("IK Chains");
					auto chains = NAFStudioMenu::GetTargetChains();
					for (const auto& c : chains) {
						result.push_back({ std::format("[{}] {}", (c.second ? "X" : " "), c.first), Bind(&BodyCreatorHandler::GotoManageChain, c.first) });
					}
					break;
				}
				case kManageIKChain:
				{
					manager->SetMenuTitle(activeChain);
					std::pair<SerializableRefHandle, std::string> curTargetParent;
					if (auto inst = NAFStudioMenu::GetInstance(); inst != nullptr) {
						inst->VisitTargetGraph([&](BodyAnimation::NodeAnimationGraph* g) {
							curTargetParent = g->ikManager.GetChainTargetParent(activeChain);
						});
					}
					std::string prntStr = "Target Parent: [None]";
					if (auto a = curTargetParent.first.get(); a != nullptr && !curTargetParent.second.empty()) {
						prntStr = std::format("Target Parent: {} - {}", a->GetBaseFullName(), curTargetParent.second);
					}
					result.push_back({ NAFStudioMenu::GetTargetChainEnabled(activeChain) ? "Disable" : "Enable", Bind(&BodyCreatorHandler::ToggleChainEnabled) });
					result.push_back({ prntStr, Bind(&BodyCreatorHandler::GotoAttachTarget) });
					result.push_back({ "Clear Target Parent", Bind(&BodyCreatorHandler::ClearAttachNode) });
					break;
				}
				case kSelectAttachNode:
				{
					manager->SetMenuTitle("Select Node");
					auto nodes = NAFStudioMenu::GetManagedRefNodes(attachTarget);
					for (auto& n : nodes) {
						result.push_back({ n, Bind(&BodyCreatorHandler::AttachNodeSelected, n) });
					}
					break;
				}
				case kPackage:
				{
					result.push_back({ std::format("Position ID: {}", pkgId.empty() ? "[None]" : pkgId), Bind(&BodyCreatorHandler::SetPackageID) });
					result.push_back({ std::format("Restrict Genders: {}", restrictGenders ? "ON" : "OFF"), Bind(&BodyCreatorHandler::ToggleBool, &restrictGenders) });
					result.push_back({ std::format("Use Current Scale(s): {}", useScales ? "ON" : "OFF"), Bind(&BodyCreatorHandler::ToggleBool, &useScales) });
					result.push_back({ "Package", Bind(&BodyCreatorHandler::PackageAnim) });
					break;
				}
				case kSettings:
				{
					const auto getInterpTypeString = [](BodyAnimation::NodeAnimationCreator::InterpType t) -> std::string {
						switch (t) {
							using enum BodyAnimation::NodeAnimationCreator::InterpType;
						default:
							return "None";
						case kCatmullRom:
							return "Catmull-Rom";
						case kNaturalCubic:
							return "Natural Cubic";
						case kSquad:
							return "Squad";
						}
					};

					BodyAnimation::NodeAnimationCreator::InterpType posInterpType = BodyAnimation::NodeAnimationCreator::kLinear;
					BodyAnimation::NodeAnimationCreator::InterpType rotInterpType = BodyAnimation::NodeAnimationCreator::kLinear;

					if (auto inst = NAFStudioMenu::GetInstance(); inst != nullptr) {
						inst->VisitTargetGraph([&](BodyAnimation::NodeAnimationGraph* g) {
							posInterpType = g->creator->posInterpType;
							rotInterpType = g->creator->rotInterpType;
						});
					}

					result.push_back({ "--- Motion Smoothing ---" });
					result.push_back({ "Rotation: " + getInterpTypeString(rotInterpType), Bind(&BodyCreatorHandler::ChangeRotInterp) });
					result.push_back({ "Position: " + getInterpTypeString(posInterpType), Bind(&BodyCreatorHandler::ChangePosInterp) });
				}
			}
			
			return result;
		}

		void SetPackageID(int) {
			GetTextInput([&](bool ok, const std::string& text) {
				if (ok) {
					pkgId = text;
					manager->RefreshList(false);
				}
			});
		}

		void ToggleBool(bool* b, int) {
			(*b) = !(*b);
			manager->RefreshList(false);
		}

		void GotoPackageAnim(int) {
			currentStage = kPackage;
			manager->RefreshList(true);
		}

		void GotoSelectTarget(int) {
			attachingNode = false;
			currentStage = kSelectTarget;
			manager->RefreshList(true);
		}

		void GotoAttachTarget(int) {
			attachingNode = true;
			currentStage = kSelectTarget;
			manager->RefreshList(true);
		}

		void GotoManageChain(const std::string& id, int) {
			activeChain = id;
			currentStage = kManageIKChain;
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
					g->ikManager.SetChainTargetParent(activeChain, data->studioActors[attachTarget].actor->GetHandle(), nodeName);
				});
			}
			NAFStudioMenu::ResetTargetChain(activeChain);
			currentStage = kManageIKChain;
			manager->RefreshList(true);
		}

		void ClearAttachNode(int) {
			if (auto inst = NAFStudioMenu::GetInstance(); inst != nullptr) {
				inst->VisitTargetGraph([&](BodyAnimation::NodeAnimationGraph* g) {
					g->ikManager.ClearChainTargetParent(activeChain);
				});
			}
			manager->RefreshList(false);
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
				manager->ShowNotification(std::format("Saved baked animation(s) to {}", res.value()));
			} else {
				manager->ShowNotification("Failed to save baked animation(s).");
			}
		}

		void PackageAnim(int) {
			if (auto res = NAFStudioMenu::BakeAnimation(true, pkgId, restrictGenders, useScales); res.has_value()) {
				manager->ShowNotification(std::format("Saved packaged animation(s) to {}", res.value()));
			} else {
				manager->ShowNotification("Failed to save packaged animation(s).");
			}
			currentStage = kManageNodes;
			manager->RefreshList(true);
		}

		void ManageIKChains(int) {
			currentStage = kManageIK;
			manager->RefreshList(true);
		}

		void SetNodeVisible(size_t idx, bool vis, int) {
			NAFStudioMenu::SetTargetNodeVisible(idx, vis);
			manager->RefreshList(false);
		}

		void ToggleChainEnabled(int) {
			bool enable = !NAFStudioMenu::GetTargetChainEnabled(activeChain);

			if (enable) {
				NAFStudioMenu::ResetTargetChain(activeChain);
			}
			NAFStudioMenu::SetTargetChainEnabled(activeChain, enable);
			manager->RefreshList(false);
		}

		void GotoSettings(int) {
			currentStage = kSettings;
			manager->RefreshList(true);
		}

		void ChangeRotInterp(int) {
			if (auto inst = NAFStudioMenu::GetInstance(); inst != nullptr) {
				inst->VisitTargetGraph([&](BodyAnimation::NodeAnimationGraph* g) {
					switch (g->creator->rotInterpType) {
					using enum BodyAnimation::NodeAnimationCreator::InterpType;
					default:
						g->creator->rotInterpType = kSquad;
						break;
					case kSquad:
						g->creator->rotInterpType = kCatmullRom;
						break;
					case kCatmullRom:
						g->creator->rotInterpType = kNaturalCubic;
						break;
					case kNaturalCubic:
						g->creator->rotInterpType = kLinear;
						break;
					}
					g->creator->PushDataToGenerator(true);
				});
			}
			manager->RefreshList(false);
		}

		void ChangePosInterp(int) {
			if (auto inst = NAFStudioMenu::GetInstance(); inst != nullptr) {
				inst->VisitTargetGraph([&](BodyAnimation::NodeAnimationGraph* g) {
					switch (g->creator->posInterpType) {
					using enum BodyAnimation::NodeAnimationCreator::InterpType;
					default:
						g->creator->posInterpType = kNaturalCubic;
						break;
					case kNaturalCubic:
						g->creator->posInterpType = kLinear;
						break;
					}
					g->creator->PushDataToGenerator(true);
				});
			}
			manager->RefreshList(false);
		}

		virtual void BackImpl() override
		{
			bool exit = false;
			switch (currentStage) {
			case kManageIKChain:
				currentStage = kManageIK;
				break;
			case kManageIK:
			case kPackage:
			case kSettings:
				currentStage = kManageNodes;
				break;
			case kSelectAttachNode:
				currentStage = kSelectTarget;
				break;
			case kSelectTarget:
				if (attachingNode) {
					currentStage = kManageIKChain;
				} else {
					currentStage = kManageNodes;
				}
				break;
			default:
				exit = true;
			}

			if (exit) {
				manager->CloseMenu();
			} else {
				manager->RefreshList(true);
			}
		}
	};
}
