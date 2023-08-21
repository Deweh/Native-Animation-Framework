#pragma once
#include "Menu/BindableMenu.h"

#define ALPHANUMERIC_UNDERSCORE_HYPHEN "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-"

namespace Menu::NAF
{
	class CreatorMenuHandler : public BindableMenu<CreatorMenuHandler, SUB_MENU_TYPE::kCreator>
	{
	public:
		using BindableMenu::BindableMenu;

		virtual ~CreatorMenuHandler() {}

		enum Stage
		{
			kMain,
			kNewFaceAnim,
			kFaceAnimMain,
			kChangeFaceAnimDur,
			kLoadProject
		};

		enum RetimeMode
		{
			ChangeEndpoint,
			RescaleAnim
		};

		Stage currentStage = kMain;
		RetimeMode timingMode;
		int32_t newDur = 0;
		PersistentMenuState::CreatorData::ProjectFaceAnim newFaceAnim;
		PersistentMenuState::CreatorData* data = PersistentMenuState::CreatorData::GetSingleton();

		virtual void InitSubmenu() override {
			BindableMenu::InitSubmenu();
			if (data->QActiveFaceAnimProject() != nullptr) {
				currentStage = kFaceAnimMain;
			}
		}

		virtual BindingsVector GetBindings() override
		{
			BindingsVector result;
			manager->SetPanelShown(false);

			switch (currentStage) {
				case kMain:
				{
					ConfigurePanel({
						{ "New Project", Button,  MENU_BINDING_WARG(CreatorMenuHandler::NewProject, false) },
						{ "Load Project", Button, MENU_BINDING_WARG(CreatorMenuHandler::LoadProject, false) },
						{ "Save Project", Button, MENU_BINDING_WARG(CreatorMenuHandler::SaveProject, false) }
					});

					manager->SetMenuTitle("Creator");

					result.push_back({ std::format("[Project Name]: {}", data->projectName.size() > 0 ? data->projectName : "[None]"), MENU_BINDING(CreatorMenuHandler::ChangeProjectName) });

					for (auto& f : data->faceAnims) {
						result.push_back({ std::format("[->] Face Anim '{}'", f.id), MENU_BINDING_WARG(CreatorMenuHandler::EditFaceAnim, f.id) });
					}

					result.push_back({ "[+] New Face Animation", MENU_BINDING(CreatorMenuHandler::NewFaceAnim) });

					return result;
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
							{ "Confirm", MENU_BINDING(CreatorMenuHandler::ChangeAnimDur) },
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
				case kLoadProject:
				{
					manager->SetMenuTitle("Load Project");
					return GetLoadableProjects();
				}
				default:
				{
					return {
						{ "No options." }
					};
				}
			}
		}

		void DeleteFaceAnim(bool confirmed, int) {
			if (!confirmed) {
				ConfigurePanel({
					{ "Are you sure you wish to delete this face anim?", Info },
					{ "Confirm", Button, MENU_BINDING_WARG(CreatorMenuHandler::DeleteFaceAnim, true) },
					{ "Cancel", Button, [&](int) { manager->RefreshList(false); } }
				});
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

			auto anims = doc.children("faceAnim");
			if (anims.empty()) {
				manager->ShowNotification("Failed to load project. No face animations found.");
				currentStage = kMain;
				manager->RefreshList(true);
				return;
			}

			PersistentMenuState::CreatorData loadingProject;
			loadingProject.projectName = projName;
			auto m = Data::XMLUtil::Mapper({}, {}, path);

			for (auto& anim : anims) {
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
			
			(*data) = std::move(loadingProject);
			currentStage = kMain;
			manager->RefreshList(true);
		}

		void LoadProject(bool, int)
		{
			currentStage = kLoadProject;
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

		void ChangeAnimDur(int) {
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

		void NewProject(bool confirmed, int) {
			if (!confirmed) {
				ConfigurePanel({ 
					{ "Are you sure you wish to make a new project?", Info },
					{ "All unsaved project data will be lost.", Info },
					{ "Confirm", Button, MENU_BINDING_WARG(CreatorMenuHandler::NewProject, true) },
					{ "Cancel", Button, [&](int) { manager->RefreshList(false); } }
				});
			} else {
				(*data) = PersistentMenuState::CreatorData();
				manager->RefreshList(false);
			}
		}

		void SaveProject(bool confirmed, int) {
			if (data->projectName.size() < 1) {
				manager->ShowNotification("A project name must be set before saving.");
				return;
			}
			if (data->faceAnims.size() < 1) {
				manager->ShowNotification("No project data to save. Try creating a new animation first.");
				return;
			}

			std::string path = std::format("Data\\NAF\\{}_project.xml", data->projectName);

			if (!confirmed && std::filesystem::exists(path)) {
				ConfigurePanel({ 
					{ "Overwrite " + path + "?", Info },
					{ "Confirm", Button, MENU_BINDING_WARG(CreatorMenuHandler::SaveProject, true) },
					{ "Cancel", Button, [&](int) { manager->RefreshList(false); } }
				});
				return;
			} else {
				manager->RefreshList(false);
			}

			pugi::xml_document outDoc;
			for (auto& p : data->faceAnims) {
				p.animData.ToXML(p.id, outDoc);
			}
			
			try {
				outDoc.save_file(path.c_str());
			} catch (std::exception ex) {
				manager->ShowNotification(std::format("Failed to save project. Full message: {}", ex.what()));
				return;
			}

			QueueHotReload("Saving...", std::format("Project saved to {}", path));
		}

		void ChangeProjectName(int) {
			GetTextInput([&](bool ok, const std::string& text) {
				if (ok) {
					//Remove new lines.
					auto txtFixed = text;
					txtFixed.erase(std::remove(txtFixed.begin(), txtFixed.end(), '\r'), txtFixed.end());
					txtFixed.erase(std::remove(txtFixed.begin(), txtFixed.end(), '\n'), txtFixed.end());

					if (txtFixed.find_first_not_of(ALPHANUMERIC_UNDERSCORE_HYPHEN) == std::string::npos) {
						data->projectName = txtFixed;
						manager->RefreshList(false);
					} else {
						manager->ShowNotification("Project names can only contain a-z, A-Z, 0-9, underscores and hyphens");
					}
				}
			});
		}

		void Goto(SUB_MENU_TYPE menuType, int)
		{
			manager->GotoMenu(menuType, true);
		}

		virtual void Back() override
		{
			if (currentStage == kFaceAnimMain) {
				data->ClearActiveFaceAnimProject();
			}

			if (currentStage == kMain) {
				manager->GotoMenu(Menu::kMain, true);
			} else {
				currentStage = kMain;
				manager->RefreshList(true);
			}
		}
	};
}
