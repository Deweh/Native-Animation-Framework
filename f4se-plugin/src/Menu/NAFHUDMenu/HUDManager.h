#pragma once
#include <unordered_set>

//stupid windows
#undef DrawText

namespace RE
{
	class HUDMenu
	{
	public:
		bool CheckHUDMode(const BSFixedString& a_type)
		{
			using func_t = decltype(&HUDMenu::CheckHUDMode);
			REL::Relocation<func_t> func{ REL::ID(1317216) };
			return func(this, a_type);
		}

	};
}

namespace Menu
{
	typedef void(OnHUDModeChanged)(RE::HUDMenu*);

	struct Vector2
	{
		float x = 0.0f;
		float y = 0.0f;
	};

	struct HUDElement
	{
		struct ShapeInfo
		{
			float width;
			float height;
			float borderSize;
			uint32_t borderColor;
			float borderAlpha;

			template <class Archive>
			void serialize(Archive& ar, const uint32_t)
			{
				ar(width, height, borderSize, borderColor, borderAlpha);
			}
		};

		struct TextInfo
		{
			std::string text;
			float textSize;
			bool bold;
			bool italic;
			bool underline;

			template <class Archive>
			void serialize(Archive& ar, const uint32_t)
			{
				ar(text, textSize, bold, italic, underline);
			}
		};

		struct LineInfo
		{
			float thickness;
			float endX;
			float endY;

			template <class Archive>
			void serialize(Archive& ar, const uint32_t)
			{
				ar(thickness, endX, endY);
			}
		};

		float x;
		float y;
		uint32_t color;
		float alpha;
		std::variant<ShapeInfo, TextInfo, LineInfo> subInfo;
		uint32_t mask = 0;
		uint32_t attachedTo = 0;
		std::set<uint32_t> children;

		template <class Archive>
		void serialize(Archive& ar, const uint32_t)
		{
			ar(x, y, color, alpha, subInfo, mask, attachedTo, children);
		}
	};

	struct HUDTranslation
	{
		float startX;
		float startY;
		float endX;
		float endY;
		float totalTime;
		Easing::Function ease = Easing::Function::InOutCubic;
		bool gameTime = false;
		float elapsedTime = 0.0f;

		template <class Archive>
		void serialize(Archive& ar, const uint32_t)
		{
			ar(startX, startY, endX, endY, totalTime, ease, gameTime, elapsedTime);
		}
	};

	class HUDManager : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
	{
	public:
		typedef std::unordered_map<uint32_t, HUDElement> HUDElementMap;
		typedef std::unordered_map<uint32_t, HUDTranslation> HUDTranslationMap;

		struct PersistentState
		{
			inline static HUDElementMap elements;
			inline static std::vector<uint32_t> displayOrder;
			inline static HUDTranslationMap activeTranslations;

			template <class Archive>
			void serialize(Archive& ar, const uint32_t)
			{
				ar(elements, displayOrder, activeTranslations);
			}
		};

		static HUDManager* GetSingleton() {
			static HUDManager instance;
			return &instance;
		}

		virtual RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent& a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
		{
			static bool closeQueued = false;
			if (a_event.menuName == "HUDMenu") {
				auto UI = RE::UI::GetSingleton();
				auto UIMessageQueue = RE::UIMessageQueue::GetSingleton();

				if (UI && UIMessageQueue) {
					if (!a_event.opening)
						closeQueued = true;
					UIMessageQueue->AddMessage("NAFHUDMenu", a_event.opening ? RE::UI_MESSAGE_TYPE::kShow : RE::UI_MESSAGE_TYPE::kHide);
				}
			} else if (a_event.menuName == "NAFHUDMenu" && a_event.opening == false) {
				if (!closeQueued) {
					RE::UIMessageQueue::GetSingleton()->AddMessage("NAFHUDMenu", RE::UI_MESSAGE_TYPE::kShow);
				} else {
					closeQueued = false;
				}
			}

			return RE::BSEventNotifyControl::kContinue;
		}

		static uint32_t DrawRectangle(float a_X, float a_Y, float a_width, float a_height, uint32_t a_color = 0xFFFFFF, float a_alpha = 1.0f, float a_borderSize = 0.0f, uint32_t a_borderColor = 0, float a_borderAlpha = 0.0f, uint32_t hndlOverride = 0)
		{
			uint32_t hndl;
			if (hndlOverride > 0) {
				hndl = hndlOverride;
			} else {
				hndl = Data::Uid::GetUI();
			}

			if (hndlOverride == 0) {
				std::unique_lock l{ elementsLock };
				state->elements[hndl] = { a_X, a_Y, a_color, a_alpha, HUDElement::ShapeInfo{ a_width, a_height, a_borderSize, a_borderColor, a_borderAlpha } };
				state->displayOrder.push_back(hndl);
			}

			InvokeAS3(HUD_AddRectangle, hndl, a_color, a_alpha, a_borderSize, a_borderColor, a_borderAlpha, a_X, a_Y, a_width, a_height);
			return hndl;
		}

		static uint32_t DrawText(const std::string& a_text, float a_X, float a_Y, float a_txtSize = 12.0f, uint32_t a_color = 0xFFFFFF, float a_alpha = 1.0f, bool a_bold = false, bool a_italic = false, bool a_underline = false, uint32_t hndlOverride = 0)
		{
			uint32_t hndl;
			if (hndlOverride > 0) {
				hndl = hndlOverride;
			} else {
				hndl = Data::Uid::GetUI();
			}

			if (hndlOverride == 0) {
				std::unique_lock l{ elementsLock };
				state->elements[hndl] = { a_X, a_Y, a_color, a_alpha, HUDElement::TextInfo{ a_text, a_txtSize, a_bold, a_italic, a_underline } };
				state->displayOrder.push_back(hndl);
			}

			InvokeAS3(HUD_AddText, hndl, a_text.c_str(), a_txtSize, a_X, a_Y, a_color, a_alpha, a_bold, a_italic, a_underline);
			return hndl;
		}

		static uint32_t DrawLine(float a_startX, float a_startY, float a_endX, float a_endY, float a_thickness = 1.0f, uint32_t a_color = 0xFFFFFF, float a_alpha = 1.0f, uint32_t hndlOverride = 0)
		{
			uint32_t hndl;
			if (hndlOverride > 0) {
				hndl = hndlOverride;
			} else {
				hndl = Data::Uid::GetUI();
			}

			if (hndlOverride == 0) {
				std::unique_lock l{ elementsLock };
				state->elements[hndl] = { a_startX, a_startY, a_color, a_alpha, HUDElement::LineInfo{ a_thickness, a_endX, a_endY } };
				state->displayOrder.push_back(hndl);
			}

			InvokeAS3(HUD_AddLine, hndl, a_color, a_alpha, a_thickness, a_startX, a_startY, a_endX, a_endY);
			return hndl;
		}

		static void SetElementPosition(uint32_t handle, float a_X, float a_Y) {
			std::unique_lock l{ elementsLock };
			auto targetEle = state->elements.find(handle);
			if (targetEle != state->elements.end()) {
				float diffX = a_X - targetEle->second.x;
				float diffY = a_Y - targetEle->second.y;
				l.unlock();
				SetElementPosByDiff(handle, diffX, diffY);
			}
		}

		static void SetElementPosByDiff(uint32_t handle, float a_XDiff, float a_YDiff) {
			std::stack<uint32_t> stack;
			stack.push(handle);
			std::unique_lock l{ elementsLock };

			while (!stack.empty()) {
				auto current = stack.top();
				stack.pop();

				auto targetEle = state->elements.find(current);
				if (targetEle != state->elements.end()) {
					targetEle->second.x += a_XDiff;
					targetEle->second.y += a_YDiff;

					InvokeAS3(HUD_SetElementPosition, current, targetEle->second.x, targetEle->second.y);
					for (auto h : targetEle->second.children) {
						stack.push(h);
					}
				}
			}
		}

		static float GetElementWidth(uint32_t handle)
		{
			auto res = InvokeAS3(HUD_GetElementWidth, handle);

			if (res.IsNumber()) {
				return static_cast<float>(res.GetNumber());
			} else {
				return 0.0f;
			}
		}

		static float GetElementHeight(uint32_t handle)
		{
			auto res = InvokeAS3(HUD_GetElementHeight, handle);

			if (res.IsNumber()) {
				return static_cast<float>(res.GetNumber());
			} else {
				return 0.0f;
			}
		}

		static Vector2 GetElementPosition(uint32_t handle) {
			std::unique_lock l{ elementsLock };
			auto targetEle = state->elements.find(handle);
			if (targetEle != state->elements.end()) {
				return { targetEle->second.x, targetEle->second.y };
			} else {
				return { 0.0f, 0.0f };
			}
		}

		static void RemoveElement(uint32_t handle) {
			RemoveElements({ handle });
		}

		static void RemoveElements(std::unordered_set<uint32_t> handles)
		{
			for (auto& h : handles) {
				StopElementTranslation(h);
				SetElementMask(h, 0);
			}
			std::vector<uint32_t> pendingMaskRemovals;
			std::unique_lock l{ elementsLock };
			for (auto& h : handles) {
				state->elements.erase(h);
			}
			for (auto& e : state->elements) {
				if (handles.contains(e.second.attachedTo)) {
					e.second.attachedTo = 0;
				}
				for (auto iter = e.second.children.begin(); iter != e.second.children.end();) {
					if (handles.contains(*iter)) {
						iter = e.second.children.erase(iter);
					} else {
						iter++;
					}
				}
				if (handles.contains(e.second.mask)) {
					pendingMaskRemovals.push_back(e.first);
				}
			}
			for (auto iter = state->displayOrder.begin(); iter != state->displayOrder.end();) {
				if (handles.contains(*iter)) {
					iter = state->displayOrder.erase(iter);
				} else {
					iter++;
				}
			}
			l.unlock();
			for (auto& h : pendingMaskRemovals) {
				SetElementMask(h, 0);
			}
			for (auto& h : handles) {
				InvokeAS3(HUD_RemoveElement, h);
			}
		}

		static void SetAlpha(float a_alpha) {
			InvokeAS3(HUD_SetAlpha, a_alpha);
		}

		static void MoveElementToBack(uint32_t handle) {
			std::unique_lock l{ elementsLock };
			if (state->elements.contains(handle)) {
				for (auto iter = state->displayOrder.begin(); iter != state->displayOrder.end();) {
					if (*iter == handle) {
						std::rotate(state->displayOrder.begin(), iter, state->displayOrder.end());
						break;
					} else {
						iter++;
					}
				}
			}

			InvokeAS3(HUD_MoveElementToBack, handle);
		}

		static void MoveElementToFront(uint32_t handle) {
			std::unique_lock l{ elementsLock };
			if (state->elements.contains(handle)) {
				for (auto iter = state->displayOrder.begin(); iter != state->displayOrder.end();) {
					if (*iter == handle) {
						state->displayOrder.erase(iter);
						break;
					} else {
						iter++;
					}
				}

				state->displayOrder.push_back(handle);
			}

			InvokeAS3(HUD_MoveElementToFront, handle);
		}

		static void SetElementMask(uint32_t handle, uint32_t maskHandle) {
			std::unique_lock l{ elementsLock };
			if (state->elements.contains(handle)) {
				if (state->elements.contains(maskHandle)) {
					state->elements[handle].mask = maskHandle;
				} else {
					state->elements[handle].mask = 0;
				}
			}

			InvokeAS3(HUD_SetElementMask, handle, maskHandle);
		}

		static void AttachElementTo(uint32_t handle, uint32_t targetHandle) {
			std::unique_lock l{ elementsLock };
			if (state->elements.contains(handle)) {
				if (state->elements.contains(targetHandle)) {
					state->elements[handle].attachedTo = targetHandle;
					state->elements[targetHandle].children.insert(handle);
				} else {
					auto& attachedTo = state->elements[handle].attachedTo;
					if (state->elements.contains(attachedTo)) {
						state->elements[attachedTo].children.erase(handle);
					}
					attachedTo = 0;
				}
			}
		}

		static void TranslateElementTo(uint32_t handle, float a_X, float a_Y, float time, Easing::Function ease = Easing::Function::InOutCubic, bool gameTime = false)
		{
			std::scoped_lock l{ translationsLock, elementsLock };
			if (state->elements.contains(handle)) {
				auto& e = state->elements[handle];
				state->activeTranslations[handle] = { e.x, e.y, a_X, a_Y, time, ease, gameTime };
			}
		}

		static void StopElementTranslation(uint32_t handle)
		{
			std::scoped_lock l{ translationsLock };
			if (state->activeTranslations.contains(handle)) {
				SetElementPosition(handle, state->activeTranslations[handle].endX, state->activeTranslations[handle].endY);
				state->activeTranslations.erase(handle);
			}
		}

		static void Reset() {
			std::scoped_lock l{ translationsLock, elementsLock };
			ClearMenuElements();
			state->activeTranslations.clear();
			state->elements.clear();
			state->displayOrder.clear();
		}

		static void SetMenuInstance(RE::GameMenuBase* inst)
		{
			menuInstance = inst;
			if (!menuInstance) {
				initialized = false;
				hidden = false;
			} else if(!initialized) {
				ResyncElements();
				initialized = true;
				Data::Events::Send(Data::Events::HUD_INIT);
			}
		}

		static void OnHUDModeChanged(RE::HUDMenu* hud) {
			if ((hud->CheckHUDMode("Pipboy")) && !hidden) {
				SetAlpha(0.0f);
				hidden = true;
			} else if (hidden) {
				SetAlpha(1.0f);
				hidden = false;
			}
			OriginalHUDModeChanged(hud);
		}

		static void RegisterHook(F4SE::Trampoline& trampoline)
		{
			//HUDMenu::OnHUDMessage_SetMode + 0xA8
			//Call to HUDMenu::OnHUDModeChanged
			REL::Relocation<Menu::OnHUDModeChanged> modeChangedLoc{ REL::ID(1385889), 0xA8 };

			OriginalHUDModeChanged = trampoline.write_call<5>(modeChangedLoc.address(), &OnHUDModeChanged);
		}

		static void Update(float delta) {
			std::unique_lock l{ translationsLock };
			for (auto iter = state->activeTranslations.begin(); iter != state->activeTranslations.end();) {
				auto& p = *iter;
				if (p.second.gameTime && RE::Main::GetSingleton()->inMenuMode) {
					iter++;
					continue;
				}
				p.second.elapsedTime += delta;
				if (p.second.elapsedTime >= p.second.totalTime) {
					SetElementPosition(p.first, p.second.endX, p.second.endY);
					iter = state->activeTranslations.erase(iter);
				} else {
					float normalizedTime = static_cast<float>(Easing::Ease(p.second.elapsedTime / p.second.totalTime, p.second.ease));
					SetElementPosition(p.first, std::lerp(p.second.startX, p.second.endX, normalizedTime), std::lerp(p.second.startY, p.second.endY, normalizedTime));
					iter++;
				}
			}
		}

		static void ResyncElements()
		{
			if (!menuInstance) {
				return;
			}

			std::unordered_map<uint32_t, uint32_t> pendingMasks;

			{
				std::scoped_lock l{ translationsLock, elementsLock };
				ClearMenuElements();
				for (auto& h : state->displayOrder) {
					if (!state->elements.contains(h)) {
						continue;
					}
					auto& data = state->elements[h];
					if (data.mask != 0) {
						pendingMasks[h] = data.mask;
					}
					if (const auto subInfo = std::get_if<HUDElement::ShapeInfo>(&data.subInfo); subInfo) {
						DrawRectangle(
							data.x,
							data.y,
							subInfo->width,
							subInfo->height,
							data.color,
							data.alpha,
							subInfo->borderSize,
							subInfo->borderColor,
							subInfo->borderAlpha,
							h);
					} else if (const auto subInfoT = std::get_if<HUDElement::TextInfo>(&data.subInfo); subInfoT) {
						DrawText(
							subInfoT->text,
							data.x,
							data.y,
							subInfoT->textSize,
							data.color,
							data.alpha,
							subInfoT->bold,
							subInfoT->italic,
							subInfoT->underline,
							h);
					} else if (const auto subInfoL = std::get_if<HUDElement::LineInfo>(&data.subInfo); subInfoL) {
						DrawLine(
							data.x,
							data.y,
							subInfoL->endX,
							subInfoL->endY,
							subInfoL->thickness,
							data.color,
							data.alpha,
							h
						);
					}
				}
			}

			for (auto& p : pendingMasks) {
				if (state->elements.contains(p.second)) {
					SetElementMask(p.first, p.second);
				}
			}
		}

		inline static safe_mutex elementsLock;
		inline static safe_mutex translationsLock;
		inline static std::unique_ptr<PersistentState> state = std::make_unique<PersistentState>();

	private:
		static void ClearMenuElements() {
			InvokeAS3(HUD_ClearElements);
		}

		template <class... Ts>
		static RE::Scaleform::GFx::Value InvokeAS3(const std::string& funcName, Ts&&... inputs)
		{
			RE::Scaleform::GFx::Value result = "";
			if (!menuInstance) {
				return result;
			}

			std::array<RE::Scaleform::GFx::Value, sizeof...(inputs)> args;
			int i = 0;

			([&] {
				args[i++] = inputs;
			}(),...);

			menuInstance.load()->menuObj.Invoke(funcName.c_str(), &result, args.data(), sizeof...(inputs));
			
			return result;
		}

		static RE::Scaleform::GFx::Value InvokeAS3(const std::string& funcName)
		{
			RE::Scaleform::GFx::Value result;
			if (!menuInstance) {
				return result;
			}

			menuInstance.load()->menuObj.Invoke(funcName.c_str(), &result, nullptr, 0);
			return result;
		}

		inline static bool initialized = false;
		inline static bool hidden = false;
		inline static std::atomic<RE::GameMenuBase*> menuInstance = nullptr;

		inline static REL::Relocation<Menu::OnHUDModeChanged> OriginalHUDModeChanged;
	};
}
