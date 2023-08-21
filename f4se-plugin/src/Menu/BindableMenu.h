#pragma once
#include "IMenuHandler.h"

#define MENU_BINDING(func) std::function<void(int)>(std::bind(&func, this, std::placeholders::_1))
#define MENU_BINDING_WARG(func, arg) std::function<void(int)>(std::bind(&func, this, arg, std::placeholders::_1))
#define MENU_BIND_TICKER(v) MENU_BINDING_WARG(BindableMenu::SetVarFromTicker, v)

namespace Menu
{
	template<typename T>
	struct TickerBinding
	{
		T value;
		T minValue;
		T maxValue;
		std::optional<std::vector<T>> possibleValues = std::nullopt;
	};

	struct BindingInfo
	{
		std::string text;
		std::optional<std::function<void(int)>> selectionFunc = std::nullopt;
		bool isTicker = false;
		int minValue = 0;
		int maxValue = 100;
		int value = 0;
		std::optional<std::function<void(int)>> rightClickFunc = std::nullopt;
		bool bold = false;
		int* indexOut = nullptr;
		std::optional<std::function<void(int)>> hoverFunc = std::nullopt;
	};

	struct ElementBinding
	{
		std::string label;
		ElementType itemType;
		std::optional<std::function<void(int)>> selectionFunc = std::nullopt;
		int* indexOut = nullptr;
		int minValue = 0;
		int maxValue = 100;
		int value = 0;
	};

	static void QueueHotReload(const std::string& initMsg, const std::string& completeMsg)
	{
		static std::atomic<bool> reloading{ false };
		if (reloading || IStateManager::activeInstance == nullptr)
			return;

		reloading = true;
		IStateManager::activeInstance->ShowNotification(initMsg, 2000.0f);

		std::thread([completeMsg = completeMsg]() {
			Data::Global::HotReload();
			F4SE::GetTaskInterface()->AddUITask([completeMsg = completeMsg]() {
				if (IStateManager::activeInstance != nullptr) {
					IStateManager::activeInstance->ShowNotification(completeMsg);
				}
				reloading = false;
			});
		}).detach();
	}

	typedef std::vector<BindingInfo> BindingsVector;
	typedef std::vector<ElementBinding> ElementBindingsVector;

	class MenuStub
	{
	public:
		class Parent
		{
		public:
			virtual void ReturnFromStub(const std::any& = nullptr) = 0;
			virtual void RefreshList(bool resetScroll) = 0;
			virtual HighlightedObjectsContainer* QHighlightContainer() = 0;

			virtual ~Parent() {}
		};

		virtual ~MenuStub() {}

		Parent* parent;

		virtual BindingsVector GetBindings() = 0;
		virtual void Back() = 0;
	};

	class FileBrowserStub : public MenuStub
	{
	public:
		std::filesystem::path currentFolder;
		std::filesystem::path rootFolder;
		std::string extensionFilter = "";

		struct BrowserResult
		{
			bool successful;
			std::string selectedPath;
		};

		FileBrowserStub(const std::filesystem::path& initialPath, const std::string& filter = "") {
			currentFolder = initialPath;
			rootFolder = initialPath;
			extensionFilter = filter;
		}

		bool IsAtRoot() {
			return currentFolder == rootFolder;
		}

		BindingsVector GetBindings() override {
			BindingsVector result;
			if (!IsAtRoot()) {
				result.push_back({ "../", MENU_BINDING(FileBrowserStub::GotoParentDirectory) });
			}

			try {
				for (auto& f : std::filesystem::directory_iterator(currentFolder)) {
					auto p = f.path();
					if (f.exists()) {
						if (f.is_directory()) {
							result.push_back({ std::format("/{}/", p.filename().generic_string()), MENU_BINDING_WARG(FileBrowserStub::GotoSubDirectory, p.filename().generic_string()) });
						} else if (p.has_filename() && p.has_extension() && (extensionFilter.empty() || p.extension().generic_string() == extensionFilter)) {
							result.push_back({ p.filename().generic_string(), MENU_BINDING_WARG(FileBrowserStub::SelectFile, p.filename().generic_string()) });
						}
					}
				}
			} catch (...) {}

			return result;
		}

		void Back() override {
			if (IsAtRoot()) {
				BrowserResult res{ false };
				parent->ReturnFromStub(res);
			} else {
				GotoParentDirectory();
			}
		}

		void GotoParentDirectory(int = 0) {
			currentFolder = currentFolder.parent_path();
			parent->RefreshList(true);
		}

		void GotoSubDirectory(const std::string& dirName, int) {
			currentFolder /= dirName;
			parent->RefreshList(true);
		}
		
		void SelectFile(const std::string& fileName, int) {
			BrowserResult res{ true, currentFolder.append(fileName).generic_string() };
			parent->ReturnFromStub(res);
		}
	};

	class ObjectSelectorStub : public MenuStub
	{
	public:

	};

	template <typename T, SUB_MENU_TYPE ST>
	class BindableMenu : public IMenuHandler, public Data::EventListener<T>, public MenuStub::Parent
	{
	private:
		inline static bool Register() {
			menuTypes.insert({ ST, [](IStateManager* s) { return std::make_unique<T>(s); } });
			return true;
		}

		inline static bool registered = Register();

	protected:
		//Function must have void return type, and take int as its last parameter.
		template <typename F, class... Args>
		std::function<void(int)> Bind(F func, Args... _args)
		{
			return std::function<void(int)>(std::bind(func, static_cast<T*>(this), _args..., std::placeholders::_1));
		}

	public:

		using IMenuHandler::IMenuHandler;

		virtual ~BindableMenu() {}

		virtual void InitSubmenu() override {
			complexItemList = true;
		}

		virtual BindingsVector GetBindings()
		{
			return {};
		}

		void RegisterUIListener(Data::Events::event_type evnt)
		{
			std::unique_lock l{ Data::EventListener<T>::eventRegistrationlock };
			if (Data::EventListener<T>::eventRegistrations.contains(evnt))
				return;
			l.unlock();
			Data::EventListener<T>::RegisterListener(evnt, &T::OnEvent);
		}

		void RegisterUIListener(std::vector<Data::Events::event_type> evnts) {
			for (auto& e : evnts) {
				RegisterUIListener(e);
			}
		}

	private:
		void OnEvent(Data::Events::event_type evnt, Data::Events::EventData& data) {
			F4SE::GetTaskInterface()->AddUITask([evnt = evnt, data = data]() {
				auto m = GetActiveInstance();
				if (!m)
					return;

				m->OnUIEvent(evnt, data);
			});
		}

	public:

		virtual void OnUIEvent(Data::Events::event_type, const Data::Events::EventData&) {
		}

		static T* GetActiveInstance()
		{
			auto s = IStateManager::activeInstance;
			if (!s)
				return nullptr;

			auto m = s->currentMenu.get();
			if (!m)
				return nullptr;

			auto mCast = dynamic_cast<T*>(m);
			if (!mCast)
				return nullptr;

			return mCast;
		}

		virtual std::vector<MenuItemData> GetComplexItemList() override
		{
			if (activeStub != nullptr) {
				bindings = activeStub->GetBindings();
			} else {
				bindings = GetBindings();
			}

			if (searchWidgetEnabled && searchTerm.has_value()) {
				const std::string termLower = Utility::StringToLower(searchTerm.value());
				for (auto iter = bindings.begin(); iter != bindings.end();) {
					if (Utility::StringToLower(iter->text).find(termLower) == std::string::npos) {
						iter = bindings.erase(iter);
					} else {
						iter++;
					}
				}
			}
			
			std::vector<MenuItemData> result;
			result.reserve(bindings.size());
			for (size_t i = 0; i < bindings.size(); i++) {
				auto& b = bindings[i];
				if (b.indexOut != nullptr && i < INT32_MAX) {
					(*b.indexOut) = static_cast<int32_t>(i);
				}
				result.push_back({ b.text, b.bold, b.isTicker, std::to_string(b.value) });
			}

			return result;
		}

		virtual void ItemSelected(int index) override
		{
			if (index < bindings.size() && !bindings[index].isTicker && bindings[index].selectionFunc.has_value()) {
				bindings[index].selectionFunc.value()(index);
			}
		}

		virtual void ItemHoverChanged(int index, bool hovered) override {
			if (index < bindings.size() && bindings[index].hoverFunc.has_value()) {
				bindings[index].hoverFunc.value()(hovered);
			}
		}

		virtual void ItemRightClicked(int index) override {
			if (index < bindings.size() && bindings[index].rightClickFunc.has_value()) {
				bindings[index].rightClickFunc.value()(index);
			}
		}

		virtual void ButtonClicked(int index) override {
			if (index < dynBindings.size() && dynBindings[index].itemType == Button && dynBindings[index].selectionFunc.has_value()) {
				dynBindings[index].selectionFunc.value()(index);
			}
		}

		virtual void TickerSelected(int index, TickerEventType e) override {
			if (index < bindings.size() && bindings[index].isTicker) {
				auto& b = bindings[index];
				switch (e) {
				case Left:
					ChangeTickerValue(index, b.value - 1);
					break;
				case Right:
					ChangeTickerValue(index, b.value + 1);
					break;
				case Main:
					pendingTickerEntry = index;
					manager->ShowTextEntry();
					break;
				}
			}
		}

		virtual void DynamicTickerSelected(int index, TickerEventType e) override {
			if (index < dynBindings.size() && dynBindings[index].itemType == Ticker) {
				auto& b = dynBindings[index];
				switch (e) {
				case Left:
					ChangeTickerValue(index, b.value - 1, false, true);
					break;
				case Right:
					ChangeTickerValue(index, b.value + 1, false, true);
					break;
				case Main:
					pendingDynTickerEntry = index;
					manager->ShowTextEntry();
					break;
				}
			}
		}

		virtual void TextEntryCompleted(bool successful, std::string text) override
		{
			if (pendingTickerEntry > -1 || pendingDynTickerEntry > -1) {
				if (successful) {
					int num = 0;
					bool noError = true;
					try {
						num = std::stol(text);
					}
					catch (...) {
						manager->ShowNotification("Invalid number.", 1.0f);
						noError = false;
					}
					
					if (noError) {
						if (pendingTickerEntry > -1) {
							ChangeTickerValue(pendingTickerEntry, num, true, false);
						}
						if (pendingDynTickerEntry > -1) {
							ChangeTickerValue(pendingDynTickerEntry, num, true, true);
						}
					}
				}
				pendingTickerEntry = -1;
				pendingDynTickerEntry = -1;
			}
			if (textEntryCallback.has_value()) {
				text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
				text.erase(std::remove(text.begin(), text.end(), '\n'), text.end());
				textEntryCallback.value()(successful, text);
				textEntryCallback = std::nullopt;
			}
		}

		void ChangeTickerValue(int index, int value, bool clamp = false, bool dyn = false) {
			if (!dyn) {
				if (index < bindings.size()) {
					auto& b = bindings[index];
					if (!clamp) {
						if (value < b.minValue) {
							b.value = b.maxValue;
						} else if (value > b.maxValue) {
							b.value = b.minValue;
						} else {
							b.value = value;
						}
					} else {
						b.value = std::clamp(value, b.minValue, b.maxValue);
					}

					manager->SetTickerText(std::to_string(b.value), index);

					if (b.selectionFunc.has_value()) {
						b.selectionFunc.value()(b.value);
					}
				}
			} else {
				if (index < dynBindings.size()) {
					auto& b = dynBindings[index];
					if (!clamp) {
						if (value < b.minValue) {
							b.value = b.maxValue;
						} else if (value > b.maxValue) {
							b.value = b.minValue;
						} else {
							b.value = value;
						}
					} else {
						b.value = std::clamp(value, b.minValue, b.maxValue);
					}

					manager->SetElementText(std::to_string(b.value), index);

					if (b.selectionFunc.has_value()) {
						b.selectionFunc.value()(b.value);
					}
				}
			}
		}

		void GetTextInput(std::function<void(bool, const std::string&)> callback)
		{
			textEntryCallback = callback;
			manager->ShowTextEntry();
		}

		void GetNumberInput(std::function<void(bool, int32_t)> callback) {
			GetTextInput([&](bool ok, const std::string& text) {
				if (ok) {
					int32_t num = 0;

					try {
						num = std::stol(text);
					} catch (...) {
						callback(false, -1);
						return;
					}

					callback(true, num);
				} else {
					callback(false, -1);
				}
			});
		}

		void ConfigurePanel(const ElementBindingsVector& binds = {}, bool leftAlign = false)
		{
			manager->SetPanelShown(false);
			dynBindings = binds;

			if (searchWidgetEnabled) {
				dynBindings.push_back({ "Search", Button, MENU_BINDING(BindableMenu::SetSearchTerm) });
				if (searchTerm.has_value()) {
					dynBindings.push_back({ "Clear Search", Button, MENU_BINDING(BindableMenu::ClearSearchTerm) });
					dynBindings.push_back({ std::format("Results For: {}", searchTerm.value()), Info });
				}
			}

			manager->SetPanelShown(true, leftAlign);

			std::vector<PanelElementData> menuData;
			menuData.reserve(dynBindings.size());
			for (size_t i = 0; i < dynBindings.size(); i++) {
				auto& b = dynBindings[i];
				menuData.push_back({ b.label, b.itemType });
				if (b.indexOut != nullptr && i <= INT32_MAX) {
					(*b.indexOut) = static_cast<int32_t>(i);
				}
			}

			manager->SetPanelElements(menuData);
		}

		void SetSearchWidgetEnabled(bool enabled = true) {
			searchWidgetEnabled = enabled;
			if (!enabled)
				searchTerm = std::nullopt;
		}

		void ClearSearchTerm(int) {
			searchTerm = std::nullopt;
			manager->RefreshList(true);
		}

		void SetSearchTerm(int) {
			GetTextInput([&](bool ok, const std::string& txt) {
				if (ok) {
					if (txt.size() > 0) {
						searchTerm = txt;
					} else {
						searchTerm = std::nullopt;
					}
					manager->RefreshList(true);
				}
			});
		}

		void ShowFileBrowser(const std::string& initialPath, const std::string& extensionFilter, std::function<void(bool, const std::string&)> callback)
		{
			activeStub = std::make_unique<FileBrowserStub>(initialPath, extensionFilter);
			activeStub->parent = this;
			fileBrowserCallback = callback;
			manager->RefreshList(true);
		}

		void SetVarFromTicker(int* v, int tickerVal) {
			(*v) = tickerVal;
		}

		void RefreshList(bool resetScroll) override {
			manager->RefreshList(resetScroll);
		}

		HighlightedObjectsContainer* QHighlightContainer() override {
			return &manager->highlightedObjects;
		}

		void ReturnFromStub(const std::any& data) override {
			activeStub.reset();
			if (auto browserRes = std::any_cast<FileBrowserStub::BrowserResult>(&data); browserRes && fileBrowserCallback.has_value()) {
				fileBrowserCallback.value()(browserRes->successful, browserRes->selectedPath);
				fileBrowserCallback = std::nullopt;
			}
		}

		virtual void Back() override {
			if (activeStub != nullptr) {
				activeStub->Back();
				return;
			}

			BackImpl();
		}

		virtual void BackImpl() {}

	private:
		BindingsVector bindings;
		ElementBindingsVector dynBindings;
		std::optional<std::function<void(bool, const std::string&)>> textEntryCallback = std::nullopt;
		int pendingTickerEntry = -1;
		int pendingDynTickerEntry = -1;

		bool searchWidgetEnabled = false;
		std::optional<std::string> searchTerm = std::nullopt;
		std::unique_ptr<MenuStub> activeStub = nullptr;
		std::optional<std::function<void(bool, const std::string&)>> fileBrowserCallback = std::nullopt;
	};
}
