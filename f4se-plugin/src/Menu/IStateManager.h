#pragma once
#include "HighlightedObjectsContainer.h"

namespace Menu
{
	class IMenuHandler;

	enum SUB_MENU_TYPE
	{
		kMain,
		kNewScene,
		kManageScenes,
		kCreator,
		kFaceAnimCreator,
		kSettings,
		kInventories,
		kBodyAnimCreator,
		kItemExplorer,
		kNone
	};

	class MenuItemData
	{
	public:
		std::string label;
		bool bold;
		bool ticker;
		std::string value = "";
	};

	enum ElementType : int
	{
		Button = 0,
		Ticker = 1,
		Info = 2
	};

	struct PanelElementData
	{
		std::string label;
		ElementType itemType;
	};

	enum TickerEventType : int
	{
		Left = 0,
		Right = 1,
		Main = 2
	};

	struct PersistentMenuState
	{
		inline static PersistentMenuState* GetSingleton()
		{
			static PersistentMenuState instance;
			return &instance;
		}

		struct CreatorData
		{
			inline static CreatorData* GetSingleton() {
				static CreatorData instance;
				return &instance;
			}

			struct ProjectFaceAnim
			{
				std::string id;
				FaceAnimation::FrameBasedAnimData animData;
			};

			struct ProjectBodyAnim
			{
				std::string id;
				size_t duration;
			};

			struct StudioData
			{
				RE::NiPointer<RE::Actor> actor;
				std::string animId;
				float sampleRate;
				float originalScale;
			};

			std::string projectName = "";
			std::vector<ProjectFaceAnim> faceAnims;
			BodyAnimation::NANIM bodyAnims;
			std::optional<size_t> activeFaceAnim = std::nullopt;
			std::optional<std::string> activeBodyAnim = std::nullopt;
			std::vector<StudioData> studioActors;
			std::optional<RE::ActorHandle> faceAnimTarget = std::nullopt;

			ProjectFaceAnim* QActiveFaceAnimProject() {
				if (!activeFaceAnim.has_value() || activeFaceAnim.value() >= faceAnims.size()) {
					return nullptr;
				}

				return &faceAnims[activeFaceAnim.value()];
			}

			BodyAnimation::NANIM::AnimationData* QActiveBodyAnimProject() {
				if (!activeBodyAnim.has_value() || !Utility::VectorContains(bodyAnims.GetAnimationList(), activeBodyAnim.value())) {
					return nullptr;
				}

				return &bodyAnims.animations.value[activeBodyAnim.value()];
			}

			BodyAnimation::NANIM::AnimationData* QBodyAnimProject(const std::string& id) {
				if (!Utility::VectorContains(bodyAnims.GetAnimationList(), id)) {
					return nullptr;
				}

				return &bodyAnims.animations.value[id];
			}

			void ClearActiveFaceAnimProject() {
				activeFaceAnim = std::nullopt;
			}

			void ClearActiveBodyAnimProject() {
				activeBodyAnim = std::nullopt;
			}

			bool SetActiveFaceAnimProject(const std::string& id) {
				for (size_t i = 0; i < faceAnims.size(); i++) {
					if (faceAnims[i].id == id) {
						activeFaceAnim = i;
						return true;
					}
				}

				return false;
			}

			bool SetActiveBodyAnimProject(const std::string& id) {
				if (Utility::VectorContains(bodyAnims.GetAnimationList(), id)) {
					activeBodyAnim = id;
					return true;
				}

				return false;
			}

			std::string GetSavePath() {
				return std::format("Data\\NAF\\{}_project", projectName);
			}

			std::optional<std::string> GetCannotBeSaved() {
				if (projectName.size() < 1) {
					return "A project name must be set before saving.";
				}

				bool hasBodyAnims = !bodyAnims.GetAnimationList().empty();
				if (faceAnims.size() < 1 && !hasBodyAnims) {
					return "No project data to save. Try creating a new animation first.";
				}

				return std::nullopt;
			}

			bool Save() {
				bool hasBodyAnims = !bodyAnims.GetAnimationList().empty();
				std::string basePath = GetSavePath();
				std::string xmlPath = std::format("{}.xml", basePath);
				std::string nanimPath = std::format("{}.nanim", basePath);

				pugi::xml_document outDoc;
				for (auto& p : faceAnims) {
					p.animData.ToXML(p.id, outDoc);
				}

				if (hasBodyAnims) {
					if (!bodyAnims.SaveToFile(nanimPath)) {
						return false;
					}
					outDoc.append_child("bodyAnimFile").append_attribute("path").set_value(nanimPath.c_str());
				}

				try {
					outDoc.save_file(xmlPath.c_str());
				} catch (std::exception ex) {
					return false;
				}

				return true;
			}
		};

		struct SceneData : public Singleton<SceneData>
		{
			uint64_t pendingSceneId = 0;
			bool isWalkInstance = true;
			std::optional<RE::ActorHandle> pendingActor = std::nullopt;
		};

		SUB_MENU_TYPE restoreSubmenu = kNone;
		CreatorData* creatorData = CreatorData::GetSingleton();
		SceneData* sceneData = SceneData::GetSingleton();
	};

	class IStateManager
	{
	public:
		// While menu handlers can access this vector, they should not write directly to it. Instead, they should call RefreshList or SetItem.
		std::vector<MenuItemData> menuItems = std::vector<MenuItemData>();

		inline static IStateManager* activeInstance = nullptr;
		RE::GameMenuBase* menuInstance;
		std::unique_ptr<IMenuHandler> currentMenu;
		HighlightedObjectsContainer highlightedObjects;

		IStateManager(RE::GameMenuBase* _menuInst) {
			menuInstance = _menuInst;
		}

		virtual ~IStateManager() {}

		// Switches the current menu handler to newMenu and immediately refreshes the item list, optionally resetting the scroll to top.
		virtual void GotoMenu(SUB_MENU_TYPE newMenu, bool resetScroll, bool resetHighlight = true) = 0;

		// Closes the entire menu, which will delete the current menu handler & state manager.
		virtual void CloseMenu() = 0;

		// Calls GetItemList on the current menu handler, then refreshes the item list.
		virtual void RefreshList(bool resetScroll) = 0;

		// Modifies a single item in the item list. More efficient than refreshing the entire list for small operations. (If the size of the list has changed, RefreshList must be used.)
		virtual bool SetItem(int index, std::string label, bool bold) = 0;

		virtual void SetPanelShown(bool shown, bool leftAlign = false) = 0;

		virtual void ShowNotification(std::string text, float time = 5.0f) = 0;

		virtual void SetMenuTitle(std::string text) = 0;

		virtual void TextEntryCompleted(bool, std::string) = 0;

		virtual void ShowTextEntry() = 0;

		virtual void TickerSelected(int, int) = 0;
		
		virtual void SetTickerText(std::string, int) = 0;

		virtual void SetPanelElements(std::vector<PanelElementData>) = 0;

		virtual void DynamicTickerSelected(int, int) = 0;

		virtual void SetElementText(std::string, int) = 0;
	};

	inline static std::unordered_map<SUB_MENU_TYPE, const std::function<std::unique_ptr<IMenuHandler>(IStateManager*)>> menuTypes;
}
