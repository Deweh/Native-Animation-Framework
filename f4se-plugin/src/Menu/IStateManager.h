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

			struct ProjectPosition
			{
				std::unique_ptr<Data::Position> positionImpl = nullptr;
				std::unique_ptr<Data::Animation> animationImpl = nullptr;

				void CreateData() {
					positionImpl = std::make_unique<Data::Position>();
					animationImpl = std::make_unique<Data::Animation>();
				}

				bool SetID(const std::string_view& id) {
					if (!positionImpl || !animationImpl)
						return false;

					positionImpl->id = id;
					positionImpl->idForType = id;
					positionImpl->posType = Data::Position::kAnimation;
					animationImpl->id = id;
					return true;
				}

				std::string GetID() {
					if (!positionImpl)
						return "";

					return positionImpl->id;
				}

				bool AddActorAndHKX(RE::Actor* a, std::string hkxPath) {
					if (!a || !positionImpl || !animationImpl)
						return false;

					animationImpl->slots.push_back(Data::Animation::Slot::FromActorAndHKX(a, hkxPath));
					return true;
				}

				size_t size() {
					if (!animationImpl || !positionImpl)
						return 0;

					return animationImpl->slots.size();
				}

				void remove(size_t index) {
					animationImpl->slots.erase(std::next(animationImpl->slots.begin(), index));
				}

				Data::Animation::Slot& operator[](size_t index) {
					return animationImpl->slots.at(index);
				}
			};

			std::string projectName = "";
			std::vector<ProjectFaceAnim> faceAnims;
			std::vector<ProjectPosition> positions;
			std::optional<size_t> activeFaceAnim = std::nullopt;
			std::optional<size_t> activePosition = std::nullopt;
			std::optional<RE::ActorHandle> faceAnimTarget = std::nullopt;

			ProjectFaceAnim* QActiveFaceAnimProject() {
				if (!activeFaceAnim.has_value() || activeFaceAnim.value() >= faceAnims.size()) {
					return nullptr;
				}

				return &faceAnims[activeFaceAnim.value()];
			}

			ProjectPosition* QActivePositionProject() {
				if (!activePosition.has_value() || activePosition.value() >= positions.size()) {
					return nullptr;
				}

				return &positions[activePosition.value()];
			}

			void ClearActiveFaceAnimProject() {
				activeFaceAnim = std::nullopt;
			}

			void ClearActivePositionProject() {
				activePosition = std::nullopt;
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

			bool SetActivePositionProject(const std::string& id) {
				for (size_t i = 0; i < positions.size(); i++) {
					if (positions[i].GetID() == id) {
						activePosition = i;
						return true;
					}
				}

				return false;
			}
		};

		struct SceneData : public Singleton<SceneData>
		{
			uint64_t pendingSceneId = 0;
			bool isWalkInstance = true;
			// open directly in submenu when set to !kNone
			SUB_MENU_TYPE restoreSubmenu = kNone;
		};
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
