#pragma once
#include "Menu/BindableMenu.h"

namespace Menu::NAF
{
	class FaceAnimCreatorHandler : public BindableMenu<FaceAnimCreatorHandler, SUB_MENU_TYPE::kFaceAnimCreator>
	{
	public:

		#define MORPH_BIND(m) MENU_BINDING_WARG(FaceAnimCreatorHandler::SetMorph, static_cast<uint8_t>(m)), true, 0, 100, GetMorph(static_cast<uint8_t>(m)), MENU_BINDING_WARG(FaceAnimCreatorHandler::RemoveMorphKey, static_cast<uint8_t>(m)), MorphHasKeyframe(static_cast<uint8_t>(m)), &tickerMap[static_cast<uint8_t>(m)]
		enum Stage
		{
			kNewAnim,
			kMain,
			kBrows,
			kEyes,
			kNose,
			kCheeks,
			kJaws,
			kLips,
			kExtra
		};

		enum DeltaType
		{
			kErase,
			kInsert,
			kChangeValue
		};
		
		PersistentMenuState::CreatorData::ProjectFaceAnim* data = PersistentMenuState::CreatorData::GetSingleton()->QActiveFaceAnimProject();
		std::optional<RE::ActorHandle>* previewTarget = &PersistentMenuState::CreatorData::GetSingleton()->faceAnimTarget;

		//WIP change delta system
		struct Delta
		{
			DeltaType type;
			uint8_t morph;
			int32_t frame;
			int32_t valueDelta;

			void Undo() {
				switch (type) {
					case kChangeValue:
					{

						break;
					}
				}
			}

			static Delta EraseKey(uint8_t, int32_t) {
				Delta result{ kErase };
				return result;
			}

			void Redo() {}

			/*
			static Delta SetKey(uint8_t morph, int32_t frame, int32_t val) {
				Delta result{ kChangeValue, morph, frame, val };
				bool isEyes = morph > 253;
				
				auto tl = data->animData.GetTimeline(morph, isEyes, false);
				if (tl == nullptr) {
					result.type = kInsert;
					tl = data->animData.MakeTimeline(isEyes ? 0 : morph, isEyes);
				} else {

				}
				
				auto& targetFrame = tl->keys[frame];
				double internalVal = static_cast<double>(val) * 0.01;
				if (isEyes) {
					internalVal = FaceAnimation::ConvertEyeCoordRange(internalVal, morph == 255, true);
				}
				
				switch (morph) {
				case 255:
					targetFrame.eyesValue.u = internalVal;
					break;
				case 254:
					targetFrame.eyesValue.v = internalVal;
					break;
				default:
					targetFrame.value = static_cast<float>(internalVal);
				}

				return result;
			}
			*/
		};

		struct Action
		{
			std::string actionName = "";
			std::vector<Delta> deltas;

			void AddDelta(const Delta& d) {
				deltas.push_back(d);
			}
		};

		struct ActionContainer
		{
			std::deque<Action> undoQueue;
			std::deque<Action> redoQueue;

			void PushAction(Action& a) {
				redoQueue.clear();
				undoQueue.push_front(a);
				if (undoQueue.size() > 50) {
					undoQueue.pop_back();
				}
			}
			
			bool Undo(std::string& nameOut) {
				if (undoQueue.size() < 1) {
					return false;
				}

				auto a = undoQueue.front();
				nameOut = a.actionName;
				undoQueue.pop_front();
				redoQueue.push_front(a);

				for (auto& d : a.deltas) {
					d.Undo();
				}

				return true;
			}

			bool Redo(std::string& nameOut) {
				if (redoQueue.size() < 1) {
					return false;
				}

				auto a = redoQueue.front();
				nameOut = a.actionName;
				redoQueue.pop_front();
				undoQueue.push_front(a);

				for (auto& d : a.deltas) {
					d.Redo();
				}

				return true;
			}
		};

		Stage currentStage;
		int editingFrame = 0;
		int moveFrame = 0;
		bool playingPreview = false;
		bool doHavokSync = false;
		std::unordered_map<uint8_t, int> tickerMap;
		bool initialized = false;

		using BindableMenu::BindableMenu;
		virtual ~FaceAnimCreatorHandler()
		{
			UpdatePreviewTarget(nullptr);
		}

		virtual void InitSubmenu() override
		{
			BindableMenu::InitSubmenu();
			manager->SetMenuTitle("Face Animator");
			currentStage = kMain;
			UpdatePreviewTarget(RE::PlayerCharacter::GetSingleton()->As<RE::Actor>());
		}

		void RefreshPanel() {
			std::string targetName = "[None]";
			if(previewTarget->has_value()) {
				auto a = previewTarget->value().get().get();
				if (a != nullptr) {
					targetName = GameUtil::GetActorName(a);
				}
			}

			ConfigurePanel({
				{ "Editing Frame:", Info },
				{ std::to_string(editingFrame), Ticker, MENU_BINDING(FaceAnimCreatorHandler::SetEditingFrame), nullptr, 0, data->animData.duration, editingFrame },
				{ playingPreview ? "Stop" : "Play", Button, MENU_BINDING(FaceAnimCreatorHandler::TogglePreview) },
				{ doHavokSync ? "Sync Preview to Body Anim: ON" : "Sync Preview to Body Anim: OFF", Button, MENU_BINDING(FaceAnimCreatorHandler::ToggleHavokSync) },
				{ "Copy/Move Current Frame To:", Info },
				{ std::to_string(moveFrame), Ticker, MENU_BIND_TICKER(&moveFrame), nullptr, 0, data->animData.duration, moveFrame },
				{ "Copy Frame", Button, MENU_BINDING_WARG(FaceAnimCreatorHandler::CopyFrame, false) },
				{ "Move Frame", Button, MENU_BINDING_WARG(FaceAnimCreatorHandler::CopyFrame, true) },
				{ "Preview Target: " + targetName, Info },
				{ "Set Target to Player", Button, MENU_BINDING_WARG(FaceAnimCreatorHandler::SetPreviewTarget, true) },
				{ "Set Target to Selected Console Ref", Button, MENU_BINDING_WARG(FaceAnimCreatorHandler::SetPreviewTarget, false) },
				{ "Remove All Keys at Current Frame", Button, MENU_BINDING(FaceAnimCreatorHandler::RemoveCurrentKeys) },
			}, true);
		}

		virtual BindingsVector GetBindings() override
		{
			if (!initialized && currentStage != kNewAnim) {
				RefreshPanel();
				initialized = true;
			}

			switch (currentStage) {
			case kMain:
				manager->SetMenuTitle("Face Animator");
				return {
					{ "Brows", MENU_BINDING_WARG(FaceAnimCreatorHandler::SetStage, kBrows) },
					{ "Eyes", MENU_BINDING_WARG(FaceAnimCreatorHandler::SetStage, kEyes) },
					{ "Nose", MENU_BINDING_WARG(FaceAnimCreatorHandler::SetStage, kNose) },
					{ "Cheeks", MENU_BINDING_WARG(FaceAnimCreatorHandler::SetStage, kCheeks) },
					{ "Mouth", MENU_BINDING_WARG(FaceAnimCreatorHandler::SetStage, kJaws) },
					{ "Lips", MENU_BINDING_WARG(FaceAnimCreatorHandler::SetStage, kLips) },
					{ "Extra", MENU_BINDING_WARG(FaceAnimCreatorHandler::SetStage, kExtra) },
				};
			case kBrows:
				manager->SetMenuTitle("Brows");
				return {
					{ "Squeeze:", MORPH_BIND(0) },
					{ "Left Outer Up:", MORPH_BIND(3) },
					{ "Right Outer Up:", MORPH_BIND(26) },
					{ "Left Outer Down:", MORPH_BIND(16) },
					{ "Right Outer Down:", MORPH_BIND(39) },
					{ "Left Middle Up:", MORPH_BIND(14) },
					{ "Right Middle Up:", MORPH_BIND(37) },
					{ "Left Middle Down:", MORPH_BIND(13) },
					{ "Right Middle Down:", MORPH_BIND(36) },
				};
			case kEyes:
				manager->SetMenuTitle("Eyes");
				return {
					{ "X:", MENU_BINDING_WARG(FaceAnimCreatorHandler::SetEyes, true), true, -100, 100, GetEyes(true), MENU_BINDING(FaceAnimCreatorHandler::RemoveEyesKey), EyesHasKeyframe(), &tickerMap[254] },
					{ "Y:", MENU_BINDING_WARG(FaceAnimCreatorHandler::SetEyes, false), true, -100, 100, GetEyes(false), MENU_BINDING(FaceAnimCreatorHandler::RemoveEyesKey), EyesHasKeyframe(), &tickerMap[255] },
					{ "Emissive Mult:", MENU_BINDING_WARG(FaceAnimCreatorHandler::SetMorph, 100ui8), true, 0, 1000, GetMorph(100ui8), MENU_BINDING_WARG(FaceAnimCreatorHandler::RemoveMorphKey, 100ui8), MorphHasKeyframe(100ui8), &tickerMap[100ui8] },
					{ "Left Upper Lid Down:", MORPH_BIND(18) },
					{ "Right Upper Lid Down:", MORPH_BIND(41) },
					{ "Left Upper Lid Up:", MORPH_BIND(19) },
					{ "Right Upper Lid Up:", MORPH_BIND(42) },
					{ "Left Lower Lid Down:", MORPH_BIND(9) },
					{ "Right Lower Lid Down:", MORPH_BIND(32) },
					{ "Left Lower Lid Up:", MORPH_BIND(10) },
					{ "Right Lower Lid Up:", MORPH_BIND(33) },
				};
			case kNose:
				manager->SetMenuTitle("Nose");
				return {
					{ "Left Up:", MORPH_BIND(15) },
					{ "Right Up:", MORPH_BIND(38) },
				};
			case kCheeks:
				manager->SetMenuTitle("Cheeks");
				return {
					{ "Left Up:", MORPH_BIND(4) },
					{ "Right Up:", MORPH_BIND(27) },
				};
			case kJaws:
				manager->SetMenuTitle("Mouth");
				return {
					{ "Forward:", MORPH_BIND(1) },
					{ "Open:", MORPH_BIND(2) },
					{ "Left:", MORPH_BIND(6) },
					{ "Right:", MORPH_BIND(29) },
					{ "Tongue:", MORPH_BIND(49) }
				};
			case kLips:
				manager->SetMenuTitle("Lips");
				return {
					{ "Sticky:", MORPH_BIND(45) },
					{ "Pucker:", MORPH_BIND(25) },
					{ "Left Smile:", MORPH_BIND(17) },
					{ "Right Smile:", MORPH_BIND(40) },
					{ "Left Frown:", MORPH_BIND(5) },
					{ "Right Frown:", MORPH_BIND(28) },
					{ "Left Corner In:", MORPH_BIND(7) },
					{ "Right Corner In:", MORPH_BIND(30) },
					{ "Left Corner Out:", MORPH_BIND(8) },
					{ "Right Corner Out:", MORPH_BIND(31) },
					{ "Left Lower Up:", MORPH_BIND(12) },
					{ "Right Lower Up:", MORPH_BIND(35) },
					{ "Left Lower Down:", MORPH_BIND(11) },
					{ "Right Lower Down:", MORPH_BIND(34) },
					{ "Left Upper Up:", MORPH_BIND(21) },
					{ "Right Upper Up:", MORPH_BIND(44) },
					{ "Left Upper Down:", MORPH_BIND(20) },
					{ "Right Upper Down:", MORPH_BIND(43) },
					{ "Upper Funnel:", MORPH_BIND(46) },
					{ "Lower Funnel:", MORPH_BIND(22) },
					{ "Upper Roll In:", MORPH_BIND(47) },
					{ "Lower Roll In:", MORPH_BIND(23) },
					{ "Upper Roll Out:", MORPH_BIND(48) },
					{ "Lower Roll Out:", MORPH_BIND(24) },
				};
			case kExtra:
				manager->SetMenuTitle("Extra");
				return {
					{ "Extra 1:", MORPH_BIND(50) },
					{ "Extra 2:", MORPH_BIND(51) },
					{ "Extra 3:", MORPH_BIND(52) },
					{ "Extra 4:", MORPH_BIND(53) }
				};
			default:
				return {
					{ "No options.", std::nullopt }
				};
			}
		}

		void CopyFrame(bool delOld, int) {
			if (moveFrame == editingFrame) {
				manager->ShowNotification("Error: Cannot move/copy frame to itself.");
				return;
			}

			uint32_t count = 0;

			for (auto& tl : data->animData.timelines) {
				if (tl.keys.contains(editingFrame)) {
					if (tl.isEyes) {
						tl.keys[moveFrame].eyesValue = tl.keys[editingFrame].eyesValue;
					} else {
						tl.keys[moveFrame].value = tl.keys[editingFrame].value;
					}
					if (delOld) {
						tl.keys.erase(editingFrame);
					}
					count++;
				}
			}

			data->animData.RemoveEmptyTimelines();
			PushChangesToPreview();

			manager->ShowNotification(std::format("{} {} morphs(s) from frame {} to frame {}.", delOld ? "Moved" : "Copied", count, editingFrame, moveFrame));
			manager->RefreshList(false);
		}

		void ToggleHavokSync(int index) {
			doHavokSync = !doHavokSync;
			manager->SetElementText(std::format("Sync Preview to Body Anim: {}", doHavokSync ? "ON" : "OFF"), index);
			PushChangesToPreview();
		}

		void SetPreviewTarget(bool isPlayer, int) {
			if (isPlayer) {
				UpdatePreviewTarget(RE::PlayerCharacter::GetSingleton()->As<RE::Actor>());
				RefreshPanel();
				manager->RefreshList(false);
			} else {
				auto ref = RE::Console::GetPickRef().get().get();
				if (ref != nullptr) {
					auto a = ref->As<RE::Actor>();
					if (a != nullptr) {
						UpdatePreviewTarget(a);
						RefreshPanel();
						manager->RefreshList(false);
					} else {
						manager->ShowNotification("Error: Selected console ref is not an actor.");
					}
				} else {
					manager->ShowNotification("Error: No selected console ref.");
				}
			}
		}

		void UpdatePreviewTarget(RE::Actor* newTarget) {
			if (previewTarget->has_value()) {
				FaceAnimation::FaceUpdateHook::StopAnimation(previewTarget->value(), true);
			}
			if (newTarget == nullptr) {
				(*previewTarget) = std::nullopt;
				return;
			}
			(*previewTarget) = newTarget->GetActorHandle();

			auto inst = std::make_unique<FaceAnimation::FaceAnimation>();
			inst->data.duration = 2.0;
			FaceAnimation::FaceUpdateHook::StartAnimation(previewTarget->value(), std::move(inst), "", true);
			PushChangesToPreview();
		}

		double GetTimeOfCurrentFrame()
		{
			return (static_cast<double>(editingFrame + (editingFrame < data->animData.duration ? 1 : 0)) / static_cast<double>(30));
		}

		void PushChangesToPreview() {
			if (!previewTarget->has_value()) {
				return;
			}

			FaceAnimation::FaceUpdateHook::VisitAnimation(previewTarget->value(), [&](FaceAnimation::FaceAnimation* anim) {
				anim->havokSync = doHavokSync;
				anim->paused = !playingPreview;
				anim->loop = true;
				data->animData.ToRuntimeData(&anim->data);
				if (!playingPreview) {
					anim->timeElapsed = GetTimeOfCurrentFrame();
				}
			});
		}

		void TogglePreview(int index) {
			playingPreview = !playingPreview;
			manager->SetElementText(playingPreview ? "Stop" : "Play", index);
			PushChangesToPreview();
		}

		void SetEditingFrame(int frame) {
			editingFrame = frame;
			manager->RefreshList(false);

			if (playingPreview || !previewTarget->has_value()) {
				return;
			}

			FaceAnimation::FaceUpdateHook::VisitAnimation(previewTarget->value(), [&](FaceAnimation::FaceAnimation* anim) {
				anim->timeElapsed = GetTimeOfCurrentFrame();
			});
		}

		void RemoveCurrentKeys(int) {
			for (auto& tl : data->animData.timelines) {
				if (tl.keys.contains(editingFrame)) {
					tl.keys.erase(editingFrame);
				}
			}
			data->animData.RemoveEmptyTimelines();
			PushChangesToPreview();
			manager->RefreshList(false);
		}

		bool EyesHasKeyframe() {
			auto tl = data->animData.GetTimeline(0, true, false);
			return (tl != nullptr && tl->keys.contains(editingFrame));
		}

		void SetEyes(bool isX, int val) {
			double finalVal = (static_cast<double>(val) / static_cast<double>(100.0));
			auto tl = data->animData.GetTimeline(0, true);
			if (isX) {
				finalVal *= -0.25;
				tl->keys[editingFrame].eyesValue = { finalVal, tl->keys[editingFrame].eyesValue.v };
			} else {
				finalVal *= 0.2;
				tl->keys[editingFrame].eyesValue = { tl->keys[editingFrame].eyesValue.u, finalVal };
			}
			PushChangesToPreview();
			if (tickerMap[254] < manager->menuItems.size() && tickerMap[255] < manager->menuItems.size()) {
				manager->SetItem(tickerMap[254], manager->menuItems[tickerMap[254]].label, true);
				manager->SetItem(tickerMap[255], manager->menuItems[tickerMap[255]].label, true);
			}
		}

		int GetEyes(bool isX) {
			auto tl = data->animData.GetTimeline(0, true, false);
			//If there is no key on the current frame, calculate the interpolated value on the current frame using the runtime data.
			if (!tl || !tl->keys.contains(editingFrame)) {
				if (tl != nullptr) {
					int res = 0;
					if (previewTarget->has_value()) {
						FaceAnimation::FaceUpdateHook::VisitAnimation(previewTarget->value(), [&](FaceAnimation::FaceAnimation* anim) {
							for (auto& tl : anim->data.timelines) {
								if (tl.isEyes) {
									auto resVec = tl.GetEyesValueAtTime(GetTimeOfCurrentFrame() / anim->data.duration);
									resVec.ConvertRange(false);
									auto resD = ((isX ? resVec.u : resVec.v) * 100);
									res = std::clamp(static_cast<int>(resD + (resD > 0 ? 0.1 : -0.1)), -100, 100);
									break;
								}
							}
						});
					}
					return res;
				} else {
					return 0;
				}
			} else {
				auto val = tl->keys[editingFrame].eyesValue;
				val.ConvertRange(false);
				double resD = ((isX ? val.u : val.v) * 100);
				return static_cast<int>(resD + (resD > 0 ? 0.1 : -0.1));
			}
		}

		void RemoveEyesKey(int)
		{
			auto tl = data->animData.GetTimeline(0, true, false);
			if (tl != nullptr && tl->keys.contains(editingFrame)) {
				tl->keys.erase(editingFrame);
				data->animData.RemoveEmptyTimelines();
				PushChangesToPreview();
				manager->RefreshList(false);
			}
		}

		bool MorphHasKeyframe(uint8_t morph) {
			auto tl = data->animData.GetTimeline(morph, false, false);
			return (tl != nullptr && tl->keys.contains(editingFrame));
		}

		int GetMorph(uint8_t morph) {
			auto tl = data->animData.GetTimeline(morph, false, false);
			//If there is no key on the current frame, calculate the interpolated value on the current frame using the runtime data.
			if (!tl || !tl->keys.contains(editingFrame)) {
				if (tl != nullptr) {
					int res = 0;
					if (previewTarget->has_value()) {
						FaceAnimation::FaceUpdateHook::VisitAnimation(previewTarget->value(), [&](FaceAnimation::FaceAnimation* anim) {
							for (auto& tl : anim->data.timelines) {
								if (!tl.isEyes && tl.morph == morph) {
									auto resF = tl.GetValueAtTime(GetTimeOfCurrentFrame() / anim->data.duration);
									res = static_cast<int>((resF * 100) + 0.1f);
									break;
								}
							}
						});
					}
					return res;
				} else {
					return 0;
				}
			} else {
				return static_cast<int>((tl->keys[editingFrame].value * 100) + 0.1f);
			}
		}

		void SetMorph(uint8_t morph, int val) {
			auto tl = data->animData.GetTimeline(morph);
			tl->keys[editingFrame].value = static_cast<float>(val) * static_cast<float>(0.01);
			PushChangesToPreview();
			if (tickerMap[morph] < manager->menuItems.size()) {
				manager->SetItem(tickerMap[morph], manager->menuItems[tickerMap[morph]].label, true);
			}
		}

		void RemoveMorphKey(uint8_t morph, int)
		{
			auto tl = data->animData.GetTimeline(morph, false, false);
			if (tl != nullptr && tl->keys.contains(editingFrame)) {
				tl->keys.erase(editingFrame);
				data->animData.RemoveEmptyTimelines();
				PushChangesToPreview();
				manager->RefreshList(false);
			}
		}

		void SetStage(Stage s, int = 0) {
			currentStage = s;
			manager->RefreshList(true);
		}

		virtual void Back() override
		{
			switch (currentStage) {
			case kBrows:
			case kEyes:
			case kNose:
			case kCheeks:
			case kJaws:
			case kLips:
			case kExtra:
				SetStage(kMain);
				break;
			default:
				manager->GotoMenu(kCreator, true);
				break;
			}
			
		}
	};
}
