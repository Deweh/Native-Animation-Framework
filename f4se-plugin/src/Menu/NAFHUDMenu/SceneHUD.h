#pragma once
#include "HUDManager.h"

namespace Menu
{
	class SceneHUD
	{
	public:
		struct ItemHUDData
		{
			uint32_t boxHandle = 0;
			uint32_t txtHandle = 0;
			uint32_t lineHandle = 0;

			template <class Archive>
			void serialize(Archive& ar, const uint32_t)
			{
				ar(boxHandle, txtHandle, lineHandle);
			}
		};

		struct StateData
		{
			double fillDur = 0.0;
			bool disabled = false;
			bool moreRight = false;
			bool moreLeft = false;
			std::string currentItem;
			std::vector<std::string> nextOptions;
			size_t selectedOption = 0;
		};

		struct PersistentState
		{
			std::vector<ItemHUDData> items;

			template <class Archive>
			void serialize(Archive& ar, const uint32_t)
			{
				ar(items);
			}
		};

		inline static std::unique_ptr<PersistentState> state = std::make_unique<PersistentState>();
		inline static std::mutex lock;

		inline static constexpr float fontSize = 15.0f;
		inline static constexpr float boxPadding = 10.0f;
		inline static constexpr float verticalPadding = 8.0f;
		inline static constexpr float boxSpacing = 30.0f;
		inline static constexpr float yBase = 900;

		enum DrawDirection
		{
			kRight,
			kUp,
			kDown
		};

		struct DrawData
		{
			float currentY = yBase;
			float currentX = 0;
			uint32_t lastRightItem = 0;
			uint32_t lastUpItem = 0;
			uint32_t lastDownItem = 0;
		};

		static void PushState(const StateData& inst) {
			Clear();

			F4SE::GetTaskInterface()->AddUITask([inst = inst]() {
				std::vector<ItemHUDData> items;

				float alphaModifier = inst.disabled ? 0.7f : 1.0f;
				DrawData d;

				if (inst.moreLeft)
					items.push_back(DrawNextItem(d, "<", kRight, alphaModifier));

				auto current = DrawNextItem(d, inst.currentItem, kRight, alphaModifier);
				items.push_back(current);

				if (inst.nextOptions.size() > 0) {
					items.push_back(DrawNextItem(d, inst.nextOptions[inst.selectedOption], kRight, alphaModifier));
					size_t drawLimit = 4;
					if (inst.selectedOption > 0) {
						for (size_t i = inst.selectedOption; i --> 0 && drawLimit > 0;) {
							items.push_back(DrawNextItem(d, inst.nextOptions[i], kUp, alphaModifier));
							drawLimit--;
						}
					}
					drawLimit = 4;
					for (size_t i = (inst.selectedOption + 1); i < inst.nextOptions.size() && drawLimit > 0; i++) {
						items.push_back(DrawNextItem(d, inst.nextOptions[i], kDown, alphaModifier));
						drawLimit--;
					}
				}

				items.push_back({ DrawOutlineAroundBox(current.boxHandle, alphaModifier) });

				if (inst.fillDur > 0) {
					Vector2 bPos = Menu::HUDManager::GetElementPosition(current.boxHandle);
					bPos.x += 2.5f;
					bPos.y += 2.5f;
					Vector2 bSize = {
						Menu::HUDManager::GetElementWidth(current.boxHandle) - 5.0f,
						Menu::HUDManager::GetElementHeight(current.boxHandle) - 5.0f
					};

					auto msk = Menu::HUDManager::DrawRectangle(bPos.x, bPos.y, bSize.x, bSize.y, 0xFFFFFF, 1.0f);
					auto fill = Menu::HUDManager::DrawRectangle(bPos.x - bSize.x, bPos.y, bSize.x, bSize.y, 0xFFFFFF, (1.0f * alphaModifier));
					Menu::HUDManager::MoveElementToFront(current.txtHandle);
					Menu::HUDManager::SetElementMask(fill, msk);
					items.push_back({ fill, msk });
					Menu::HUDManager::TranslateElementTo(fill, bPos.x, bPos.y, static_cast<float>(inst.fillDur), Easing::Function::None, true);
				}

				if (inst.moreRight)
					items.push_back(DrawNextItem(d, ">", kRight, alphaModifier));

				std::unique_lock l{ lock };
				state->items = items;
			});
		}

		static void Clear() {
			std::unique_lock l{ lock };
			std::unordered_set<uint32_t> pendingDeletes;
			for (auto& i : state->items) {
				pendingDeletes.insert(i.boxHandle);
				pendingDeletes.insert(i.txtHandle);
				pendingDeletes.insert(i.lineHandle);
			}

			state->items.clear();

			F4SE::GetTaskInterface()->AddUITask([pendingDeletes = pendingDeletes]() {
				Menu::HUDManager::RemoveElements(pendingDeletes);
			});
		}

		static void Reset() {
			std::unique_lock l{ lock };
			state->items.clear();
		}

	protected:
		static ItemHUDData DrawNextItem(DrawData& d, std::string name, DrawDirection dir, float alphaModifier = 1.0f) {
			float eleHeight = 0;
			if (d.lastRightItem > 0) {
				eleHeight = HUDManager::GetElementHeight(d.lastRightItem);
			}

			switch (dir) {
				case kRight:
				{
					if (d.lastRightItem > 0) {
						d.currentX += HUDManager::GetElementWidth(d.lastRightItem);
					}
					d.currentY = yBase;
					d.currentX += boxSpacing;
					break;
				}
				case kUp:
				{
					if (d.lastUpItem > 0) {
						d.currentY = HUDManager::GetElementPosition(d.lastUpItem).y - eleHeight - verticalPadding;
					} else {
						d.currentY = HUDManager::GetElementPosition(d.lastRightItem).y - eleHeight - verticalPadding;
					}
					break;
				}
				case kDown:
				{
					if (d.lastDownItem > 0) {
						d.currentY = HUDManager::GetElementPosition(d.lastDownItem).y + eleHeight + verticalPadding;
					} else {
						d.currentY = HUDManager::GetElementPosition(d.lastRightItem).y + eleHeight + verticalPadding;
					}
					break;
				}
			}

			ItemHUDData currentItm = DrawItem(name, d.currentX, d.currentY, alphaModifier);

			if (dir == kRight && d.lastRightItem > 0) {
				currentItm.lineHandle = DrawLineBetweenElements(d.lastRightItem, currentItm.boxHandle, 2.0f, 0, alphaModifier);
			}

			switch (dir) {
			case kRight:
				d.lastRightItem = currentItm.boxHandle;
				break;
			case kUp:
				d.lastUpItem = currentItm.boxHandle;
				break;
			case kDown:
				d.lastDownItem = currentItm.boxHandle;
				break;
			}

			return currentItm;
		}

		static ItemHUDData DrawItem(std::string name, float x, float y, float alphaModifier = 1.0f)
		{
			ItemHUDData result;
			auto txt = HUDManager::DrawText(name, x + boxPadding, y + boxPadding, fontSize, 0, (1.0f * alphaModifier), true);
			result.txtHandle = txt;
			auto box = DrawBoxAroundText(txt, boxPadding, 0xFFFFFF, alphaModifier);
			result.boxHandle = box;

			return result;
		}

		static uint32_t DrawLineBetweenElements(uint32_t first, uint32_t second, float thickness = 2.0f, uint32_t color = 0, float alphaModifer = 1.0f)
		{
			auto firstPos = HUDManager::GetElementPosition(first);
			auto secondPos = HUDManager::GetElementPosition(second);

			auto firstHeight = HUDManager::GetElementHeight(first);
			auto firstWidth = HUDManager::GetElementWidth(first);
			auto secondHeight = HUDManager::GetElementHeight(second);

			return HUDManager::DrawLine(firstPos.x + firstWidth, firstPos.y + (firstHeight / 2.0f), secondPos.x, secondPos.y + (secondHeight / 2), thickness, color, (1.0f * alphaModifer));
		}

		static uint32_t DrawOutlineAroundBox(uint32_t boxHandle, float alphaModifier = 1.0f) {
			Vector2 boxPos = HUDManager::GetElementPosition(boxHandle);
			float boxHeight = HUDManager::GetElementHeight(boxHandle);
			float boxWidth = HUDManager::GetElementWidth(boxHandle);

			return HUDManager::DrawRectangle(boxPos.x - 2, boxPos.y - 2, boxWidth + 2, boxHeight + 2, 0xFFFFFF, 0.0f, 2.0f, 0xFFFFFF, (1.0f * alphaModifier));
		}

		static uint32_t DrawBoxAroundText(uint32_t txtHandle, float padding, uint32_t color = 0xFFFFFF, float alphaModifier = 1.0f)
		{
			Vector2 txtPos = HUDManager::GetElementPosition(txtHandle);
			float txtHeight = HUDManager::GetElementHeight(txtHandle);
			float txtWidth = HUDManager::GetElementWidth(txtHandle);
			uint32_t result = HUDManager::DrawRectangle(txtPos.x - padding, txtPos.y - padding, txtWidth + (padding * 2), txtHeight + (padding * 2), color, (0.70f * alphaModifier), 2.5f, 0x4a4a4a, (1.0f * alphaModifier));
			HUDManager::MoveElementToFront(txtHandle);
			return result;
		}
	};
}
