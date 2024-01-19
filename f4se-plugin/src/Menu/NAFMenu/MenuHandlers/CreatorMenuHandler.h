#pragma once
#include "Menu/BindableMenu.h"

namespace Menu::NAF
{
	class CreatorMenuHandler : public BindableMenu<CreatorMenuHandler, SUB_MENU_TYPE::kCreator>
	{
	public:
		using BindableMenu::BindableMenu;

		virtual ~CreatorMenuHandler()
		{
			if (clearActors) {
				PersistentMenuState::CreatorData::GetSingleton()->studioActors.clear();
			}
		}

		enum Stage
		{
			kMain,
			kLoadProject,

			kNewFaceAnim,
			kFaceAnimMain,
			kChangeFaceAnimDur,
			
			kNewBodyAnim,
			kBodyAnimMain,
			kChangeBodyAnimDur,

			kMultiCharSetup,
			kMultiCharSelection,
			kMultiCharAnimPick
		};

		enum RetimeMode
		{
			ChangeEndpoint,
			RescaleAnim
		};

		const float defaultSampleRate = 0.0333333f;
		Stage currentStage = kMain;
		RetimeMode timingMode = RetimeMode::ChangeEndpoint;
		std::string newActorBehGraph = "";
		std::string newID = "";
		int32_t newDur = 0;
		PersistentMenuState::CreatorData::ProjectFaceAnim newFaceAnim;
		PersistentMenuState::CreatorData* data = PersistentMenuState::CreatorData::GetSingleton();
		bool clearActors = true;

		int32_t studioActorCount = 1;
		size_t selectedStudioActor = 0;

		bool doActorSelection = false;
		std::function<void(bool, RE::Actor*)> actorSelectionCallback = nullptr;

		virtual void InitSubmenu() override {
			BindableMenu::InitSubmenu();
			if (data->QActiveFaceAnimProject() != nullptr) {
				currentStage = kFaceAnimMain;
				return;
			}
			if (data->QActiveBodyAnimProject() != nullptr) {
				currentStage = kBodyAnimMain;
				return;
			}
		}

		virtual BindingsVector GetBindings() override
		{
			BindingsVector result;
			manager->SetPanelShown(false);

			if (doActorSelection) {
				auto distMap = GameUtil::GenerateRefDistMap<RE::Actor>([](RE::Actor* a) {
					return GameUtil::ActorIsAlive(a) && !a->HasKeyword(Data::Forms::ActorTypeChildKW);
				});

				for (auto& info : distMap) {
					result.push_back({ std::format("[{:.2f}] {}", info.distance, info.name), Bind(&CreatorMenuHandler::OnActorSelection, info.ref) });
				}
				return result;
			}

			switch (currentStage) {
				case kMultiCharSetup:
				{
					result.push_back({ "Number of Actors:", MENU_BINDING(CreatorMenuHandler::NumCharsChanged), true, 1, 10, studioActorCount });

					bool oneMissing = false;
					for (size_t i = 0; i < data->studioActors.size(); i++) {
						const auto& a = data->studioActors[i];
						if (a.actor == nullptr || a.animId.empty()) {
							result.push_back({ "[ Empty Slot ]", MENU_BINDING_WARG(CreatorMenuHandler::SelectMultiChar, i) });
							oneMissing = true;
						} else {
							result.push_back({ std::format("[ {} - {} ]", a.animId, GameUtil::GetActorName(a.actor.get())), MENU_BINDING_WARG(CreatorMenuHandler::SelectMultiChar, i) });
						}
					}

					if (!oneMissing) {
						result.push_back({ "Confirm", MENU_BINDING(CreatorMenuHandler::MultiCharFinish) });
					}

					return result;
				}
				case kMultiCharSelection:
				{
					auto& selection = data->studioActors[selectedStudioActor];
					return {
						{ "Animation: " + (selection.animId.empty() ? "[None]" : selection.animId), MENU_BINDING(CreatorMenuHandler::EditAnimMultiChar) },
						{ "Actor: " + (selection.actor == nullptr ? "[None]" : GameUtil::GetActorName(selection.actor.get())), MENU_BINDING(CreatorMenuHandler::EditActorMultiChar) },
						{ "Confirm", MENU_BINDING(CreatorMenuHandler::MultiCharConfirmSlot) }
					};
				}
				case kMultiCharAnimPick:
				{
					for (auto& a : data->bodyAnims.GetAnimationList()) {
						result.push_back({ a, MENU_BINDING_WARG(CreatorMenuHandler::MultiCharChooseAnim, a) });
					}
					return result;
				}
				case kMain:
				{
					ConfigurePanel({
						{ "New Project", Button,  MENU_BINDING_WARG(CreatorMenuHandler::NewProject, false) },
						{ "Load Project", Button, MENU_BINDING_WARG(CreatorMenuHandler::LoadProject, false) },
						{ "Save Project", Button, MENU_BINDING_WARG(CreatorMenuHandler::SaveProject, false) },
						{ "Save Pose Snapshot", Button, MENU_BINDING(CreatorMenuHandler::SavePoseSnapshot) }
					});

					manager->SetMenuTitle("Creator");

					result.push_back({ std::format("[Project Name]: {}", data->projectName.size() > 0 ? data->projectName : "[None]"), MENU_BINDING(CreatorMenuHandler::ChangeProjectName) });

					for (auto& f : data->faceAnims) {
						result.push_back({ std::format("[->] Face Anim '{}'", f.id), MENU_BINDING_WARG(CreatorMenuHandler::EditFaceAnim, f.id) });
					}

					for (auto& b : data->bodyAnims.GetAnimationList()) {
						result.push_back({ std::format("[->] Body Anim '{}'", b), MENU_BINDING_WARG(CreatorMenuHandler::EditBodyAnim, b) });
					}

					result.push_back({ "[+] New Face Animation", MENU_BINDING(CreatorMenuHandler::NewFaceAnim) });
					result.push_back({ "[+] New Body Animation", MENU_BINDING(CreatorMenuHandler::NewBodyAnim) });
					result.push_back({ "[*] Setup Multi-Actor Studio Instance", MENU_BINDING(CreatorMenuHandler::GotoMultiChar) });

					return result;
				}
				case kLoadProject:
				{
					manager->SetMenuTitle("Load Project");
					return GetLoadableProjects();
				}
				case kBodyAnimMain:
				{
					manager->SetMenuTitle(data->activeBodyAnim.value());
					result.push_back({ "[->] Open in Animation Studio", MENU_BINDING(CreatorMenuHandler::GotoBodyAnim) });
					result.push_back({ "[Animation ID]: " + data->activeBodyAnim.value(), MENU_BINDING(CreatorMenuHandler::SetBodyAnimID) });
					result.push_back({ "[Animation Duration]: " + std::to_string(QBodyAnimFrameDuration()) + " frames @30FPS", MENU_BINDING(CreatorMenuHandler::GotoBodyAnimDur) });
					result.push_back({ "[X] Delete", MENU_BINDING_WARG(CreatorMenuHandler::DeleteBodyAnim, false) });
					return result;
				}
				case kChangeBodyAnimDur:
				{
					manager->SetMenuTitle("Set Anim Duration");
					if (data->activeBodyAnim.has_value()) {
						newDur = static_cast<int32_t>(QBodyAnimFrameDuration());
						return {
							{ "Animation Frames (30FPS):", MENU_BIND_TICKER(&newDur), true, 3, 120, newDur },
							{ std::format("[Retime Mode]: {}", GetTimeRescaleString()), MENU_BINDING(CreatorMenuHandler::SwitchTimeRescale) },
							{ "Confirm", MENU_BINDING(CreatorMenuHandler::ChangeBodyAnimDur) },
						};
					} else {
						result.push_back({ "<Error>" });
					}
				}
				case kNewBodyAnim:
				{
					manager->SetMenuTitle("New Body Animation");
					return {
						{ "Animation Frames (30FPS):", MENU_BIND_TICKER(&newDur), true, 3, 120, newDur },
						{ std::format("[Animation ID]: {}", newID.size() > 0 ? newID : "[None]"), MENU_BINDING(CreatorMenuHandler::SetNewBodyAnimID) },
						{ "[+] Create", MENU_BINDING(CreatorMenuHandler::CreateBodyAnim) }
					};
				}
				case kFaceAnimMain:
				{
					auto proj = data->QActiveFaceAnimProject();
					if (proj != nullptr) {
						manager->SetMenuTitle(proj->id);
						result.push_back({ "[->] Open in Face Animator", MENU_BINDING(CreatorMenuHandler::GotoFaceAnim) });
						result.push_back({ "[Animation ID]: " + proj->id, MENU_BINDING(CreatorMenuHandler::SetFaceAnimID) });
						result.push_back({ "[Animation Duration]: " + std::to_string(proj->animData.duration) + " frames @30FPS", MENU_BINDING(CreatorMenuHandler::GotoFaceAnimDur) });
						result.push_back({ "[X] Delete", MENU_BINDING_WARG(CreatorMenuHandler::DeleteFaceAnim, false) });
					} else {
						result.push_back({ "<Error>" });
					}
					
					return result;
				}
				case kChangeFaceAnimDur:
				{
					manager->SetMenuTitle("Set Anim Duration");
					auto proj = data->QActiveFaceAnimProject();
					if (proj != nullptr) {
						newDur = proj->animData.duration;
						return {
							{ "Animation Frames (30FPS):", MENU_BIND_TICKER(&newDur), true, 3, 120000, newDur },
							{ std::format("[Retime Mode]: {}", GetTimeRescaleString()), MENU_BINDING(CreatorMenuHandler::SwitchTimeRescale) },
							{ "Confirm", MENU_BINDING(CreatorMenuHandler::ChangeFaceAnimDur) },
						};
					} else {
						result.push_back({ "<Error>" });
					}
				}
				case kNewFaceAnim:
				{
					manager->SetMenuTitle("New Face Animation");
					return {
						//120,000 frames = roughly 1 hour at 30fps
						{ "Animation Frames (30FPS):", MENU_BIND_TICKER(&newFaceAnim.animData.duration), true, 3, 120000, newFaceAnim.animData.duration },
						{ std::format("[Animation ID]: {}", newFaceAnim.id.size() > 0 ? newFaceAnim.id : "[None]"), MENU_BINDING(CreatorMenuHandler::SetNewFaceAnimID) },
						{ "[+] Create", MENU_BINDING(CreatorMenuHandler::CreateFaceAnim) }
					};
				}
				default:
				{
					return {
						{ "No options." }
					};
				}
			}
		}

		//Multi-Char Setup

		void GotoMultiChar(int) {
			studioActorCount = 1;
			data->studioActors.clear();
			data->studioActors.resize(1);
			currentStage = kMultiCharSetup;
			manager->RefreshList(true);
		}

		void NumCharsChanged(int val) {
			studioActorCount = val;
			data->studioActors.resize(val);
			manager->RefreshList(false);
		}

		void SelectMultiChar(size_t idx, int) {
			selectedStudioActor = idx;
			currentStage = kMultiCharSelection;
			manager->RefreshList(true);
		}

		void MultiCharConfirmSlot(int) {
			data->studioActors[selectedStudioActor].sampleRate = QBodyAnimSampleRate();
			currentStage = kMultiCharSetup;
			manager->RefreshList(true);
		}

		void EditActorMultiChar(int) {
			GetActorInput([&](bool ok, RE::Actor* a) {
				if (ok) {
					bool inSlot = false;
					for (size_t i = 0; i < data->studioActors.size(); i++) {
						if (i != selectedStudioActor && data->studioActors[i].actor.get() == a) {
							manager->ShowNotification("Multiple slots cannot use the same actor.");
							inSlot = true;
							break;
						}
					}
					if (!inSlot) {
						auto& s = data->studioActors[selectedStudioActor];
						s.actor.reset(a);
						s.originalScale = a->GetScale();
					}
				}
				manager->RefreshList(true);
			});
		}

		void EditAnimMultiChar(int) {
			currentStage = kMultiCharAnimPick;
			manager->RefreshList(true);
		}

		void MultiCharChooseAnim(std::string animName, int) {
			bool inSlot = false;
			for (size_t i = 0; i < data->studioActors.size(); i++) {
				if (i != selectedStudioActor && data->studioActors[i].animId == animName) {
					manager->ShowNotification("Multiple slots cannot use the same animation.");
					inSlot = true;
					break;
				}
			}
			if (!inSlot) {
				data->studioActors[selectedStudioActor].animId = animName;
			}
			currentStage = kMultiCharSelection;
			manager->RefreshList(true);
		}

		void MultiCharFinish(int) {
			float sharedTime = 0.0f;
			for (auto& a : data->studioActors) {
				if (a.actor == nullptr || a.animId.empty()) {
					manager->ShowNotification("All slots must be filled.");
					return;
				} else if (auto anim = data->QBodyAnimProject(a.animId);
					anim == nullptr || (sharedTime != 0.0f && std::fabs(anim->duration - sharedTime) > 0.001f)) {
					manager->ShowNotification("All animations must have the same duration.");
					return;
				} else {
					sharedTime = anim->duration;
				}
			}

			auto UI = RE::UI::GetSingleton();
			auto UIMessageQueue = RE::UIMessageQueue::GetSingleton();
			if (UI->GetMenuOpen("NAFStudioMenu")) {
				UIMessageQueue->AddMessage("NAFStudioMenu", RE::UI_MESSAGE_TYPE::kHide);
			}
			UIMessageQueue->AddMessage("NAFStudioMenu", RE::UI_MESSAGE_TYPE::kShow);

			F4SE::GetTaskInterface()->AddUITask([] {
				if (auto inst = NAFStudioMenu::GetInstance(); inst != nullptr) {
					inst->InitActors();
					inst->SetTarget(0);
				}
			});

			clearActors = false;
			manager->CloseMenu();
		}

		//General Funcs

		void SavePoseSnapshot(int) {
			manager->SetMenuTitle("Select Actor");
			GetActorInput([&](bool ok, RE::Actor* a) {
				if (ok) {
					BodyAnimation::GraphHook::VisitGraph(a, [&](BodyAnimation::NodeAnimationGraph* g) {
						std::vector<BodyAnimation::NodeTransform> result;
						for (auto& n : g->nodes) {
							if (n != nullptr) {
								result.push_back(n->local);
							} else {
								result.push_back(BodyAnimation::NodeTransform::Identity());
							}
						}
						BodyAnimation::NANIM poseContainer;
						poseContainer.SetAnimationFromPose("pose", 0.1f, g->nodeMap, result);
						if (!poseContainer.SaveToFile("Data\\NAF\\pose_snapshot.nanim")) {
							manager->ShowNotification("Failed to save pose to file.");
						} else {
							manager->ShowNotification(std::format("Saved pose to {}", "Data\\NAF\\pose_snapshot.nanim"));
						}
					});
				}
			});
		}

		void OnActorSelection(RE::Actor* a, int) {
			if (actorSelectionCallback != nullptr) {
				doActorSelection = false;
				manager->RefreshList(true);
				actorSelectionCallback(true, a);
			}
		}

		void GetActorInput(const std::function<void(bool, RE::Actor*)>& callback) {
			actorSelectionCallback = callback;
			doActorSelection = true;
			manager->RefreshList(true);
		}

		BindingsVector GetLoadableProjects() {
			BindingsVector res;
			const std::string projectEnding = "_project.xml";
			std::string basePath = "Data\\NAF\\";
			for (const auto& entry : std::filesystem::directory_iterator(basePath)) {
				auto path = entry.path();
				if (!entry.is_regular_file() || !path.has_filename()) {
					continue;
				}
				auto file = path.filename().generic_string();
				if (Utility::StringEndsWith(file, projectEnding)) {
					auto projName = file.substr(0, file.size() - projectEnding.size());
					res.push_back({ projName, MENU_BINDING_WARG(CreatorMenuHandler::ParseProject, projName) });
				}
			}
			return res;
		}

		void ParseProject(std::string projName, int) {
			std::string path = std::format("Data\\NAF\\{}_project.xml", projName);

			pugi::xml_document doc;
			try {
				doc.load_file(path.c_str());
			}
			catch (std::exception ex) {
				manager->ShowNotification(std::format("Failed to load project file. Full message: {}", ex.what()));
				currentStage = kMain;
				manager->RefreshList(true);
				return;
			}

			auto faceAnims = doc.children("faceAnim");
			auto bodyAnimFile = doc.child("bodyAnimFile");

			if (faceAnims.empty() && bodyAnimFile.empty()) {
				manager->ShowNotification("Failed to load project. No body or face animations found.");
				currentStage = kMain;
				manager->RefreshList(true);
				return;
			}

			PersistentMenuState::CreatorData loadingProject;
			loadingProject.projectName = projName;
			auto m = Data::XMLUtil::Mapper({}, {}, path);

			if (!faceAnims.empty()) {
				for (auto& anim : faceAnims) {
					m.SetCurrentNode(&anim);
					Data::FaceAnim animInfo;
					FaceAnimation::FrameBasedAnimData animData;

					if (!Data::FaceAnim::Parse(m, animInfo, false, nullptr, &animData)) {
						manager->ShowNotification("Failed to parse face animation. Check log for full message.");
						currentStage = kMain;
						manager->RefreshList(true);
						return;
					}

					loadingProject.faceAnims.push_back({ animInfo.id, std::move(animData) });
				}
			}

			if (!bodyAnimFile.empty()) {
				std::string baPath = bodyAnimFile.attribute("path").as_string();
				if (baPath.empty() || !loadingProject.bodyAnims.LoadFromFile(baPath)) {
					manager->ShowNotification("Failed to parse body animation.");
					currentStage = kMain;
					manager->RefreshList(true);
					return;
				}
			}
			
			(*data) = std::move(loadingProject);
			currentStage = kMain;
			manager->RefreshList(true);
		}

		void LoadProject(bool, int)
		{
			currentStage = kLoadProject;
			manager->RefreshList(true);
		}

		void NewProject(bool confirmed, int)
		{
			if (!confirmed) {
				ConfigurePanel({ { "Are you sure you wish to make a new project?", Info },
					{ "All unsaved project data will be lost.", Info },
					{ "Confirm", Button, MENU_BINDING_WARG(CreatorMenuHandler::NewProject, true) },
					{ "Cancel", Button, [&](int) { manager->RefreshList(false); } } });
			} else {
				(*data) = PersistentMenuState::CreatorData();
				manager->RefreshList(false);
			}
		}

		void SaveProject(bool confirmed, int)
		{
			if (auto msg = data->GetCannotBeSaved(); msg.has_value()) {
				manager->ShowNotification(msg.value());
				return;
			}

			std::string xmlPath = std::format("{}.xml", data->GetSavePath());

			if (!confirmed && std::filesystem::exists(xmlPath)) {
				ConfigurePanel({ { "Overwrite " + xmlPath + "?", Info },
					{ "Confirm", Button, MENU_BINDING_WARG(CreatorMenuHandler::SaveProject, true) },
					{ "Cancel", Button, [&](int) { manager->RefreshList(false); } } });
				return;
			} else {
				manager->RefreshList(false);
			}

			if (!data->Save()) {
				manager->ShowNotification(std::format("Failed to save {}", xmlPath));
				return;
			}

			QueueHotReload("Saving...", std::format("Project saved to {}", xmlPath));
		}

		void ChangeProjectName(int)
		{
			GetTextInput([&](bool ok, const std::string& text) {
				if (ok) {
					if (text.find_first_not_of(ALPHANUMERIC_UNDERSCORE_HYPHEN) == std::string::npos) {
						data->projectName = text;
						manager->RefreshList(false);
					} else {
						manager->ShowNotification("Project names can only contain a-z, A-Z, 0-9, underscores and hyphens");
					}
				}
			});
		}

		//Face Anims

		void DeleteFaceAnim(bool confirmed, int) {
			if (!confirmed) {
				ConfigurePanel({ { "Are you sure you wish to delete this face anim?", Info },
					{ "Confirm", Button, MENU_BINDING_WARG(CreatorMenuHandler::DeleteFaceAnim, true) },
					{ "Cancel", Button, [&](int) { manager->RefreshList(false); } } });
				return;
			}

			auto proj = data->QActiveFaceAnimProject();
			if (proj == nullptr) {
				manager->ShowNotification("An error occured while trying to query the project data.");
				return;
			}

			auto id = proj->id;
			data->ClearActiveFaceAnimProject();
			for (auto iter = data->faceAnims.begin(); iter != data->faceAnims.end(); iter++) {
				if (iter->id == id) {
					data->faceAnims.erase(iter);
					break;
				}
			}

			currentStage = kMain;
			manager->RefreshList(true);
		}

		void EditFaceAnim(const std::string& id, int) {
			if (data->SetActiveFaceAnimProject(id)) {
				currentStage = kFaceAnimMain;
				manager->RefreshList(true);
			} else {
				manager->ShowNotification("An error occured while trying to query the project data.");
			}
		}

		void ChangeFaceAnimDur(int) {
			auto proj = data->QActiveFaceAnimProject();
			if (proj == nullptr) {
				manager->ShowNotification("An error occured while trying to query the project data.");
				return;
			}

			if (newDur == proj->animData.duration) {
				return;
			}

			if (timingMode == RescaleAnim) {
				double scale = static_cast<double>(newDur) / static_cast<double>(proj->animData.duration);

				for (auto& tl : proj->animData.timelines) {
					auto keysCopy = tl.keys;
					tl.keys.clear();
					for (auto& k : keysCopy) {
						tl.keys.insert({ static_cast<int32_t>(static_cast<double>(k.first) * scale), k.second });
					}
				}
			}

			proj->animData.duration = newDur;
			currentStage = kFaceAnimMain;
			manager->RefreshList(true);
		}

		void SwitchTimeRescale(int) {
			if (timingMode == RescaleAnim) {
				timingMode = ChangeEndpoint;
			} else {
				timingMode = RescaleAnim;
			}
			manager->RefreshList(false);
		}

		std::string GetTimeRescaleString() {
			switch (timingMode) {
			case RescaleAnim:
				return "Rescale Animation";
			case ChangeEndpoint:
				return "Change Endpoint";
			default:
				return "Unknown";
			}
		}

		void SetFaceAnimID(int) {
			GetTextInput([&](bool ok, const std::string& text) {
				if (ok) {
					auto proj = data->QActiveFaceAnimProject();
					if (proj == nullptr) {
						manager->ShowNotification("An error occured while trying to query the project data.");
						return;
					}

					for (auto& p : data->faceAnims) {
						if (p.id == text) {
							manager->ShowNotification("An animation with that ID already exists.");
							return;
						}
					}
					proj->id = text;
					manager->RefreshList(false);
				}
			});
		}

		void SetNewFaceAnimID(int)
		{
			GetTextInput([&](bool ok, const std::string& text) {
				if (ok) {
					newFaceAnim.id = text;
					manager->RefreshList(false);
				}
			});
		}

		void CreateFaceAnim(int) {
			if (newFaceAnim.id.size() < 1) {
				manager->ShowNotification("An animation ID must be set before creation.");
				return;
			}

			for (auto& p : data->faceAnims) {
				if (p.id == newFaceAnim.id) {
					manager->ShowNotification("An animation with that ID already exists.");
					return;
				}
			}

			data->faceAnims.push_back(newFaceAnim);
			currentStage = kMain;
			manager->RefreshList(true);
		}

		void NewFaceAnim(int)
		{
			newFaceAnim = PersistentMenuState::CreatorData::ProjectFaceAnim();
			currentStage = kNewFaceAnim;
			manager->RefreshList(true);
		}

		void GotoFaceAnim(int) {
			manager->GotoMenu(kFaceAnimCreator, true);
		}

		void GotoFaceAnimDur(int) {
			currentStage = kChangeFaceAnimDur;
			manager->RefreshList(true);
		}

		//Body Anims

		float QBodyAnimSampleRate() {
			return defaultSampleRate;
		}

		size_t QBodyAnimFrameDuration() {
			auto proj = data->QActiveBodyAnimProject();
			if (!proj)
				return 0;

			return static_cast<size_t>(std::round(proj->duration / QBodyAnimSampleRate()));
		}

		void DeleteBodyAnim(bool confirmed, int)
		{
			if (!confirmed) {
				ConfigurePanel({ { "Are you sure you wish to delete this body anim?", Info },
					{ "Confirm", Button, MENU_BINDING_WARG(CreatorMenuHandler::DeleteBodyAnim, true) },
					{ "Cancel", Button, [&](int) { manager->RefreshList(false); } } });
				return;
			}

			if (!data->activeBodyAnim.has_value()) {
				manager->ShowNotification("An error occured while trying to query the project data.");
				return;
			}

			auto id = data->activeBodyAnim.value();
			data->ClearActiveBodyAnimProject();
			data->bodyAnims.RemoveAnimation(id);

			currentStage = kMain;
			manager->RefreshList(true);
		}

		void EditBodyAnim(const std::string& id, int)
		{
			if (data->SetActiveBodyAnimProject(id)) {
				currentStage = kBodyAnimMain;
				manager->RefreshList(true);
			} else {
				manager->ShowNotification("An error occured while trying to query the project data.");
			}
		}

		void ChangeBodyAnimDur(int)
		{
			auto proj = data->QActiveBodyAnimProject();
			if (proj == nullptr) {
				manager->ShowNotification("An error occured while trying to query the project data.");
				return;
			}

			auto frameRateDur = QBodyAnimFrameDuration();

			if (newDur == frameRateDur) {
				return;
			}

			if (timingMode == RescaleAnim) {
				float scale = static_cast<float>(newDur) / static_cast<float>(frameRateDur);

				for (auto& tl : proj->timelines) {
					for (auto& k : tl.second) {
						k.time *= scale;
					}
				}
			}

			proj->duration = static_cast<float>(newDur) * QBodyAnimSampleRate();
			currentStage = kBodyAnimMain;
			manager->RefreshList(true);
		}

		void SetBodyAnimID(int)
		{
			GetTextInput([&](bool ok, const std::string& text) {
				if (ok) {
					if (!data->activeBodyAnim.has_value()) {
						manager->ShowNotification("An error occured while trying to query the project data.");
						return;
					}

					if (!data->bodyAnims.ChangeAnimationName(data->activeBodyAnim.value(), text)) {
						manager->ShowNotification("A body anim with that ID already exists.");
						return;
					}

					data->activeBodyAnim = text;
					manager->RefreshList(false);
				}
			});
		}

		void SetNewBodyAnimID(int)
		{
			GetTextInput([&](bool ok, const std::string& text) {
				if (ok) {
					newID = text;
					manager->RefreshList(false);
				}
			});
		}

		static std::pair<std::vector<BodyAnimation::NodeTransform>, std::vector<std::string>> GetBasePose(RE::TESObjectREFR* a_target)
		{
			std::pair<std::vector<BodyAnimation::NodeTransform>, std::vector<std::string>> result;

			if (!a_target)
				return result;

			RE::BSFixedString behGraphName;
			a_target->GetAnimationGraphProjectName(behGraphName);
			if (auto info = Data::GetGraphInfo(behGraphName.c_str(), a_target->As<RE::Actor>()); info != nullptr) {
				result.second = info->nodeList;
				if (info->basePoseFile.has_value()) {
					BodyAnimation::NANIM poseContainer;
					if (poseContainer.LoadFromFile(std::format("Data\\NAF\\{}", info->basePoseFile.value()))) {
						poseContainer.GetAnimationAsPose("pose", info->nodeList, result.first);
					}
				} else if (!info->skeletonPose.empty()) {
					for (auto& t : info->skeletonPose) {
						result.first.push_back(t);
					}
				}
			}

			return result;
		}

		void CreateBodyAnim(int)
		{
			if (newID.size() < 1) {
				manager->ShowNotification("An animation ID must be set before creation.");
				return;
			}

			if (Utility::VectorContains(data->bodyAnims.GetAnimationList(), newID)) {
				manager->ShowNotification("A body animation with that ID already exists.");
				return;
			}

			manager->SetMenuTitle("Select Actor Type");
			GetActorInput([&](bool ok, RE::Actor* a) {
				if (ok) {
					auto basePose = GetBasePose(a);
					if (basePose.first.size() > 0) {
						data->bodyAnims.SetAnimationFromPose(newID, static_cast<float>(newDur) * QBodyAnimSampleRate(), basePose.second, basePose.first);
					} else {
						data->bodyAnims.SetEmptyAnimation(newID, static_cast<float>(newDur) * QBodyAnimSampleRate());
					}
					currentStage = kMain;
					manager->RefreshList(true);
				}
			});
		}

		void NewBodyAnim(int)
		{
			newID = "";
			newDur = 60;
			currentStage = kNewBodyAnim;
			manager->RefreshList(true);
		}

		void GotoBodyAnim(int)
		{
			manager->SetMenuTitle("Select Target Actor");
			GetActorInput([&](bool ok, RE::Actor* a) {
				if (ok) {
					data->studioActors.clear();
					data->studioActors.emplace_back(RE::NiPointer<RE::Actor>(a), data->activeBodyAnim.value(), QBodyAnimSampleRate(), a->GetScale());
					data->ClearActiveBodyAnimProject();

					auto UI = RE::UI::GetSingleton();
					auto UIMessageQueue = RE::UIMessageQueue::GetSingleton();
					if (UI->GetMenuOpen("NAFStudioMenu")) {
						UIMessageQueue->AddMessage("NAFStudioMenu", RE::UI_MESSAGE_TYPE::kHide);
					}
					UIMessageQueue->AddMessage("NAFStudioMenu", RE::UI_MESSAGE_TYPE::kShow);

					F4SE::GetTaskInterface()->AddUITask([] {
						if (auto inst = NAFStudioMenu::GetInstance(); inst != nullptr) {
							inst->InitActors();
							inst->SetTarget(0);
						}
					});

					clearActors = false;
					manager->CloseMenu();
				}
			});
		}

		void GotoBodyAnimDur(int)
		{
			currentStage = kChangeBodyAnimDur;
			manager->RefreshList(true);
		}

		//Misc

		void Goto(SUB_MENU_TYPE menuType, int)
		{
			manager->GotoMenu(menuType, true);
		}

		virtual void Back() override
		{
			if (doActorSelection) {
				doActorSelection = false;
				manager->RefreshList(true);
				if (actorSelectionCallback != nullptr) {
					actorSelectionCallback(false, nullptr);
				}
				return;
			}

			if (currentStage == kFaceAnimMain) {
				data->ClearActiveFaceAnimProject();
			} else if (currentStage == kBodyAnimMain) {
				data->ClearActiveBodyAnimProject();
			}

			switch (currentStage) {
			case kMain:
				manager->GotoMenu(Menu::kMain, true);
				break;
			case kMultiCharSelection:
				currentStage = kMultiCharSetup;
				manager->RefreshList(true);
				break;
			case kMultiCharAnimPick:
				currentStage = kMultiCharSelection;
				manager->RefreshList(true);
				break;
			case kMultiCharSetup:
				data->studioActors.clear();
			default:
				currentStage = kMain;
				manager->RefreshList(true);
				break;
			}
		}
	};
}
