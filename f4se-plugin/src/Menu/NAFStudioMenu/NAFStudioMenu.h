#pragma once

namespace Menu
{
	class NAFStudioMenu : public RE::GameMenuBase
	{
	public:
		using Graph = BodyAnimation::NodeAnimationGraph;
		using Creator = BodyAnimation::NodeAnimationCreator;

		enum ModifierKeys : uint8_t
		{
			kNone = 0,
			kLControl = 1u << 0,
			kRControl = 1u << 1
		};

		//Util Functions

		static bool WorldPtToScreenPt3(const RE::NiPoint3& worldPt, RE::NiPoint3& screenPt)
		{
			static const RE::NiCamera* worldCam = RE::Main::WorldRootCamera();
			const auto& worldToCam = worldCam->worldToCam;

			float trace = (worldPt.x * worldToCam[3][0]) + (worldPt.y * worldToCam[3][1]) + ((worldPt.z * worldToCam[3][2]) + worldToCam[3][3]);
			if (trace <= 0.00001f) {
				return false;
			}

			float traceInv = 1.0f / trace;
			screenPt.x = (((worldPt.y * worldToCam[0][1]) + (worldPt.x * worldToCam[0][0])) + ((worldPt.z * worldToCam[0][2]) + worldToCam[0][3])) * traceInv;
			screenPt.y = (((worldPt.y * worldToCam[1][1]) + (worldPt.x * worldToCam[1][0])) + ((worldPt.z * worldToCam[1][2]) + worldToCam[1][3])) * traceInv;
			screenPt.z = (((worldPt.y * worldToCam[2][1]) + (worldPt.x * worldToCam[2][0])) + ((worldPt.z * worldToCam[2][2]) + worldToCam[2][3])) * traceInv;
			screenPt.x = (screenPt.x + 1.0f) * 0.5f;
			screenPt.y = (screenPt.y + 1.0f) * 0.5f;
			return true;
		}

		static bool IsActive() {
			return studioInstance.load() != nullptr;
		}

		static std::optional<std::string> BakeAnimation(
			bool doPackage = false,
			const std::string_view& pkgId = "",
			bool restrictGenders = true,
			bool useScales = false)
		{
			std::optional<std::string> result = std::nullopt;
			auto data = PersistentMenuState::CreatorData::GetSingleton();
			if (auto inst = GetInstance(); inst != nullptr) {

				std::vector<std::pair<SerializableActorHandle, Creator::BAKE_DATA>> bakeData;
				for (const auto& hndl : inst->managedActors) {
					BodyAnimation::GraphHook::VisitGraph(hndl.get().get(), [&](Graph* g) {
						bakeData.emplace_back(hndl, g->creator->SetupBake());
					});
				}

				bool stepsLeft = true;
				while (stepsLeft) {
					//This is a bit ugly, but it's needed for IK targets parented to other actors.
					for (auto& pair : bakeData) {
						BodyAnimation::GraphHook::VisitGraph(pair.first.get().get(), [&](Graph* g) {
							g->creator->GenerateAtTime(pair.second);
						});
					}
					for (auto& pair : bakeData) {
						BodyAnimation::GraphHook::VisitGraph(pair.first.get().get(), [&](Graph* g) {
							g->creator->CalculateInverseKinematics();
						});
					}
					for (auto& pair : bakeData) {
						BodyAnimation::GraphHook::VisitGraph(pair.first.get().get(), [&](Graph* g) {
							g->creator->SampleAtTime(pair.second);
						});
					}
					for (auto& pair : bakeData) {
						stepsLeft = stepsLeft && pair.second.StepForward();
					}
				}

				auto path = data->GetSavePath() + "_";
				BodyAnimation::NANIM animContainer;

				if (doPackage) {
					path += "packaged.nanim";
					animContainer.characters.animId = pkgId;
					for (size_t i = 0; i < bakeData.size(); i++) {
						auto& pair = bakeData[i];
						auto& a = data->studioActors[i];
						BodyAnimation::GraphHook::VisitGraph(pair.first.get().get(), [&](Graph* g) {
							animContainer.SetAnimation(a.animId, g->nodeMap, pair.second.animData.get());
						});
						auto& c = animContainer.characters.data.emplace_back();
						c.animId = a.animId;
						c.gender = restrictGenders ? static_cast<ActorGender>(a.actor->GetSex()) : ActorGender::Any;
						RE::BSFixedString behGraph;
						a.actor->GetAnimationGraphProjectName(behGraph);
						c.behaviorGraphProject = behGraph;
						if (useScales) {
							c.scale = a.actor->GetScale();
						}
					}
					animContainer.SaveToFile(path);
					result = path;
				} else {
					for (size_t i = 0; i < bakeData.size(); i++) {
						auto& pair = bakeData[i];
						auto& a = data->studioActors[i];
						BodyAnimation::GraphHook::VisitGraph(pair.first.get().get(), [&](Graph* g) {
							animContainer.SetAnimation("default", g->nodeMap, pair.second.animData.get());
						});
						animContainer.SaveToFile(std::format("{}{}.nanim", path, Utility::StringRestrictChars(a.animId, ALPHANUMERIC_UNDERSCORE_HYPHEN)));
					}
					result = (path + "*.nanim");
				}
			}
			return result;
		}

		static void SaveChanges() {
			if (auto inst = GetInstance(); inst != nullptr) {
				inst->SaveChangesImpl();
			}
		}

		static std::vector<std::pair<std::string, bool>> GetTargetChains() {
			std::vector<std::pair<std::string, bool>> result;
			if (auto inst = GetInstance(); inst != nullptr) {
				inst->VisitTargetGraph([&](Graph* g) {
					for (auto& c : g->ikManager.GetChainList()) {
						result.emplace_back(c, g->ikManager.GetChainEnabled(c));
					}
				});
			}
			return result;
		}

		static bool GetTargetChainEnabled(const std::string& id)
		{
			bool result = false;
			if (auto inst = GetInstance(); inst != nullptr) {
				inst->VisitTargetGraph([&](Graph* g) {
					result = g->ikManager.GetChainEnabled(id);
				});
			}
			return result;
		}

		static void SetTargetChainEnabled(const std::string& id, bool a_enabled) {
			if (auto inst = GetInstance(); inst != nullptr) {
				inst->VisitTargetGraph([&](Graph* g) {
					g->ikManager.SetChainEnabled(id, a_enabled);
				});
			}
		}

		static void ResetTargetChain(const std::string& id) {
			if (auto inst = GetInstance(); inst != nullptr) {
				inst->VisitTargetGraph([&](Graph* g) {
					g->ikManager.ResetChainTarget(id);
				});
			}
		}

		static std::vector<std::string> GetManagedRefNodes(size_t idx) {
			std::vector<std::string> result;
			if (auto inst = GetInstance(); inst != nullptr && idx <= inst->managedActors.size()) {
				BodyAnimation::GraphHook::VisitGraph(inst->managedActors[idx].get().get(), [&](Graph* g) {
					for (size_t i = 0; i < g->nodeMap.size(); i++) {
						result.emplace_back(g->nodeMap[i]);
					}
				});
			}
			return result;
		}

		static std::vector<std::pair<std::string, bool>> GetTargetNodes() {
			std::vector<std::pair<std::string, bool>> result;
			if (auto inst = GetInstance(); inst != nullptr) {
				inst->VisitTargetGraph([&](Graph* g) {
					for (size_t i = 0; i < g->nodeMap.size(); i++) {
						result.emplace_back(g->nodeMap[i], inst->GetNodeMarkedVisible(i));
					}
				});
			}
			return result;
		}

		static void SetTargetNodeVisible(size_t idx, bool a_visible)
		{
			if (auto inst = GetInstance(); inst != nullptr) {
				inst->MarkNodeVisible(idx, a_visible);
			}
		}

		//Menu Functions

		void VisitManagedGraphs(const std::function<void(Graph*)>& visitFunc) {
			for (const auto& hndl : managedActors) {
				BodyAnimation::GraphHook::VisitGraph(hndl.get().get(), visitFunc);
			}
		}

		bool VisitTargetGraph(const std::function<void(Graph*)>& visitFunc) {
			if (targetHandle.has_value()) {
				return BodyAnimation::GraphHook::VisitGraph(targetHandle->get().get(), visitFunc);
			}
			return false;
		}

		RE::Scaleform::GFx::Value CreateNodeMarker(size_t idx, const std::string_view& clickType, int iconType) {
			assert(idx < UINT32_MAX);
			
			RE::Scaleform::GFx::Value result;
			RE::Scaleform::GFx::Value args[1];
			args[0] = static_cast<uint32_t>(idx);
			menuObj.Invoke("CreateNodeMarker", &result, args, 1);
			result.SetMember("sClickType", clickType.data());
			args[0] = iconType;
			result.Invoke("SetIconType", nullptr, args, 1);

			return result;
		}

		void InitActors() {
			auto data = PersistentMenuState::CreatorData::GetSingleton();
			std::optional<RE::NiPoint3> location = std::nullopt;
			std::optional<RE::NiPoint3> angle = std::nullopt;
			for (auto& a : data->studioActors) {
				if (!location || !angle) {
					location = a.actor->data.location;
					angle = RE::NiPoint3{ 0, 0, a.actor->data.angle.z };
				}

				Tasks::GameLoopHook::ForceAlignActor(a.actor.get(), location.value(), angle.value());
				AddManagedRef(a.actor.get(), data->bodyAnims, a.animId, a.sampleRate);
			}
		}

		bool SetTarget(size_t idx) {
			if (idx >= managedActors.size())
				return false;

			auto a_target = managedActors[idx].get();

			if (a_target == nullptr)
				return false;

			ClearTarget();
			targetHandle = a_target->GetActorHandle();

			VisitTargetGraph([&](Graph* g) {
				RE::Scaleform::GFx::Value result;
				RE::Scaleform::GFx::Value args[1];
				auto ikMappings = g->ikManager.GetLookupMap();

				for (size_t i = 0; i < g->nodes.size(); i++) {
					if (auto iter = ikMappings.find(i); iter != ikMappings.end()) {
						if (iter->second.target == BodyAnimation::IKManager::IKMapping::Type::kPole) {
							nodeMarkers.push_back(CreateNodeMarker(i, "NodeMarker", 1));
						} else {
							nodeMarkers.push_back(CreateNodeMarker(i, "NodeMarker", 2));
						}
					} else {
						nodeMarkers.push_back(CreateNodeMarker(i, "NodeMarker", 0));
					}
				}

				SetTimelineSize(g->creator->GetMaxFrame());
			});
			UpdateTimelineData();
			return true;
		}

		Creator::LoadResult AddManagedRef(RE::Actor* a_target, const BodyAnimation::NANIM& animContainer, const std::string& animId, float sampleRate) {
			Creator::LoadResult result = Creator::LoadResult::kFailed;
			
			if (!a_target)
				return result;

			auto data = PersistentMenuState::CreatorData::GetSingleton();
			PackageOverride::Set(a_target->GetActorHandle(), Data::Forms::NAFLockPackage, true);
			managedActors.push_back(a_target->GetActorHandle());
			BodyAnimation::GraphHook::VisitGraph(a_target, [&](Graph* g) {
				if (g->creator == nullptr) {
					g->creator = std::make_unique<Creator>(&g->generator, &g->nodes, &g->nodeMap, &g->ikManager);
				}
				result = g->creator->LoadFromNANIM(animId, animContainer, sampleRate);
				g->state = Graph::kGenerator;

				if (auto meta = animContainer.GetMetaData(animId); meta.has_value()) {
					if (auto iter = meta->data.find("actor_scale"); animContainer.version.value >= 3 && iter != meta->data.end() && iter->second.size() > 0) {
						std::optional<float> s = std::nullopt;
						try {
							s = std::stof(iter->second[0]);
						} catch (...) {
							s = std::nullopt;
						}

						if (s) {
							GameUtil::SetActorScale(a_target, s.value());
						}
					}
					if (auto iter = meta->data.find("enabled_ik_chains"); iter != meta->data.end()) {
						for (auto& c : iter->second) {
							g->ikManager.SetChainEnabled(c, true);
						}
					}
					for (auto& c : g->ikManager.GetChainList()) {
						if (auto iter = meta->data.find(std::format("ik_target_parent_{}", c)); iter != meta->data.end() && iter->second.size() >= 2) {
							const auto& targetAnim = iter->second[0];
							const auto& targetNode = iter->second[1];
							for (auto& sData : data->studioActors) {
								if (sData.animId == targetAnim) {
									g->ikManager.SetChainTargetParent(c, sData.actor->GetHandle(), targetNode);
									break;
								}
							}
						}
					}
				}
			});
			return result;
		}

		void ClearTarget() {
			for (auto& n : nodeMarkers) {
				RE::Scaleform::GFx::Value args[1];
				args[0] = n;
				menuObj.Invoke("DestroyNodeMarker", nullptr, args, 1);
			}
			nodeMarkers.clear();
			targetHandle = std::nullopt;
		}

		void ClearManagedRefs() {
			for (auto& a : managedActors) {
				Tasks::GameLoopHook::StopAligningActor(a.get().get());
			}

			auto data = PersistentMenuState::CreatorData::GetSingleton();
			for (auto& a : data->studioActors) {
				PackageOverride::Clear(a.actor->GetActorHandle(), true);
				GameUtil::SetActorScale(a.actor.get(), a.originalScale);
				BodyAnimation::GraphHook::VisitGraph(a.actor.get(), [&](Graph* g) {
					for (auto& c : g->ikManager.GetChainList()) {
						g->ikManager.SetChainEnabled(c, false);
					}
					g->generator.paused = false;
					g->TransitionToAnimation(nullptr, 1.0f);
					g->creator.reset();
				});
				a.actor->EnableCollision();
				a.actor->SetNoCollision(false);
			}
			managedActors.clear();
		}

		static NAFStudioMenu* GetInstance() {
			return studioInstance;
		}

		static RE::IMenu* Create(const RE::UIMessage&)
		{
			auto result = new NAFStudioMenu();
			return result;
		}

		NAFStudioMenu()
		{
			menuFlags.set(
				RE::UI_MENU_FLAGS::kAllowSaving,
				RE::UI_MENU_FLAGS::kRequiresUpdate,
				RE::UI_MENU_FLAGS::kCompanionAppAllowed,
				RE::UI_MENU_FLAGS::kUsesCursor);

			depthPriority.set(RE::UI_DEPTH_PRIORITY::kHUD);

			const auto ScaleformManager = RE::BSScaleformManager::GetSingleton();
			if (!ScaleformManager->LoadMovieEx(*this, "Interface/NAFStudioMenu.swf", "root.Menu_mc", RE::Scaleform::GFx::Movie::ScaleModeType::kShowAll)) {
				RE::SendHUDMessage::ShowHUDMessage("Failed to load NAF Studio menu.", "UIMenuCancel", false, true);
				return;
			}

			filterHolder = RE::msvc::make_unique<RE::BSGFxShaderFXTarget>(*uiMovie, "root.Menu_mc");
			if (filterHolder) {
				filterHolder->CreateAndSetFiltersToHUD(RE::HUDColorTypes::kGameplayHUDColor);
				shaderFXObjects.push_back(filterHolder.get());
			}
		}

		virtual ~NAFStudioMenu()
		{
		}

		float GetSWFXPos(float screenXPos)
		{
			return (screenXPos * stageWidth) + widthAdjust;
		}

		float GetSWFYPos(float screenYPos)
		{
			return (1.0f - screenYPos) * stageHeight;
		}

		void SetObjectPositionInWorld(RE::Scaleform::GFx::Value& obj, const BodyAnimation::NodeTransform& transform, bool isGizmo) {
			RE::NiPoint3 screenPos{ -100.0f, -100.0f, 0.99999f };
			WorldPtToScreenPt3(transform.translate, screenPos);
			screenPos.y = GetSWFYPos(screenPos.y);
			screenPos.x = GetSWFXPos(screenPos.x);
			obj.SetMember("x", screenPos.x);
			obj.SetMember("y", screenPos.y);
			float s = 0.000001f;
			if (screenPos.z >= 0.0f && !(transform.translate.x == 0.0f && transform.translate.y == 0.0f && transform.translate.z == 0.0f)) {
				s = (3.5f * (1.0f - screenPos.z)) * nodeSizeModifier;
			}
			obj.SetMember("scaleX", s);
			obj.SetMember("scaleY", s);

			if (isGizmo) {

				gizmoMovementScale = screenPos.z;
				float distScale = (10.5f * nodeSizeModifier) * (1.0f / s);
				auto v = MathUtil::QuatToDirVectors(transform.rotate);
				v.transform([&](RE::NiPoint3& p){
					p *= distScale;
					p += transform.translate;
				});

				RE::Scaleform::GFx::Value args[6];
				RE::NiPoint2 gizmoBasePos = { screenPos.x, screenPos.y };

				const auto CalculateGizmoPoint = [&](const RE::NiPoint3& worldPt) {
					WorldPtToScreenPt3(worldPt, screenPos);
					return RE::NiPoint2{
						GetSWFXPos(screenPos.x) - gizmoBasePos.x,
						GetSWFYPos(screenPos.y) - gizmoBasePos.y
					};
				};

				gizmoXPos = CalculateGizmoPoint(v.x);
				args[0] = gizmoXPos.x;
				args[1] = gizmoXPos.y;

				gizmoYPos = CalculateGizmoPoint(v.y);
				args[2] = gizmoYPos.x;
				args[3] = gizmoYPos.y;

				gizmoZPos = CalculateGizmoPoint(v.z);
				args[4] = gizmoZPos.x;
				args[5] = gizmoZPos.y;

				obj.Invoke("SetGizmoPoints", nullptr, args, 6);
			}
		}

		void SetScrubberPosition(double p) {
			RE::Scaleform::GFx::Value args[1];
			args[0] = p;
			menuObj.Invoke("SetScrubberPosition", nullptr, args, 1);
		}

		virtual void AdvanceMovie(float a_timeDelta, [[maybe_unused]] std::uint64_t a_time) override
		{
			GameMenuBase::AdvanceMovie(a_timeDelta, a_time);
			RE::NiPoint3 orbitTarget = { 0.0f, 0.0f, 0.0f };
			float localTime = 0.0f;
			VisitTargetGraph([&](Graph* g) {
				for (size_t i = 0; i < nodeMarkers.size(); i++) {
					bool isGizmo = i == gizmoIndex && gizmoActive;
					auto currentTransform = g->creator->GetCurrentWorldTransform(i, isGizmo && adjustMode == Creator::kPosition);
					if (isGizmo)
						orbitTarget = currentTransform.translate;

					SetObjectPositionInWorld(nodeMarkers[i], currentTransform, isGizmo);
				}

				if (g->creator->IsPlaying()) {
					SetScrubberPosition(g->creator->GetUINormalizedTime());
				}
				localTime = g->generator.localTime;
			});
			for (auto& hndl : managedActors) {
				if (auto a = hndl.get(); a != nullptr) {
					if (targetHandle.has_value() && targetHandle.value() != hndl) {
						BodyAnimation::GraphHook::VisitGraph(a.get(), [&](Graph* g) {
							g->generator.localTime = localTime;
						});
					}
					
				}
			}
			CamHook::orbitTarget = orbitTarget;
		}

		void MarkNodeVisible(size_t idx, bool visible) {
			if (idx >= nodeMarkers.size())
				return;

			nodeMarkers[idx].SetMember("bMarkedVisible", visible);
			if (showTargetNodes) {
				nodeMarkers[idx].SetMember("visible", visible);
			}
		}

		bool GetNodeMarkedVisible(size_t idx) {
			if (idx >= nodeMarkers.size())
				return false;

			RE::Scaleform::GFx::Value val;
			nodeMarkers[idx].GetMember("bMarkedVisible", &val);
			if (val.IsBoolean())
				return val.GetBoolean();

			return false;
		}

		void SetNodesVisible(bool visible) {
			for (size_t i = 0; i < nodeMarkers.size(); i++) {
				if (visible == false && i == gizmoIndex && gizmoActive)
					continue;

				if (visible == true) {
					RE::Scaleform::GFx::Value markedVis;
					nodeMarkers[i].GetMember("bMarkedVisible", &markedVis);
					if (markedVis.IsBoolean() && markedVis.GetBoolean() == false) {
						nodeMarkers[i].SetMember("visible", false);
						continue;
					}
				}
				nodeMarkers[i].SetMember("visible", visible);
			}
			showTargetNodes = visible;
		}

		virtual void MapCodeObjectFunctions() override
		{
			MapCodeMethodToASFunction("CloseMenu", 0);
			MapCodeMethodToASFunction("NotifyLoaded", 1);
			MapCodeMethodToASFunction("NotifyClicked", 2);
			MapCodeMethodToASFunction("NotifyGizmoMovement", 3);
			MapCodeMethodToASFunction("Log", 4);
			MapCodeMethodToASFunction("StartGizmoCursor", 5);
			MapCodeMethodToASFunction("StopGizmoCursor", 6);
			MapCodeMethodToASFunction("NotifyScrubberPosition", 7);
			MapCodeMethodToASFunction("NotifyKeyMoved", 8);
			MapCodeMethodToASFunction("NotifyKeySelected", 9);
		}

		virtual void Call(const Params& a_params) override
		{
			switch ((*((std::uint32_t*)&(a_params.userData)))) {
				case 0:
				{
					CloseMenu();
					break;
				}
				case 1:
				{
					OnMenuObjLoaded();
					break;
				}
				case 2:
				{
					if (a_params.argCount == 2 && a_params.args[0].IsString() && a_params.args[1].IsDisplayObject()) {
						OnClick(a_params.args[0].GetString(), a_params.args[1]);
					}
					break;
				}
				case 3:
				{
					if (a_params.argCount == 3 && a_params.args[0].IsUInt() && a_params.args[1].IsNumber() && a_params.args[2].IsNumber()) {
						OnGizmoMovement(a_params.args[0].GetUInt(), static_cast<float>(a_params.args[1].GetNumber()), static_cast<float>(a_params.args[2].GetNumber()));
					}
					break;
				}
				case 4:
				{
					if (a_params.argCount == 1 && a_params.args[0].IsString()) {
						OutputDebugStringA(std::format("{}\n", a_params.args[0].GetString()).c_str());
					}
					break;
				}
				case 5:
				{
					OnGizmoCursor(true);
					break;
				}
				case 6:
				{
					OnGizmoCursor(false);
					break;
				}
				case 7:
				{
					if (a_params.argCount == 1 && a_params.args[0].IsNumber()) {
						OnScrubberPositionChanged(a_params.args[0].GetNumber());
					}
					break;
				}
				case 8:
				{
					if (a_params.argCount == 2 && a_params.args[0].IsUInt() && a_params.args[1].IsUInt()) {
						OnKeyMoved(a_params.args[0].GetUInt(), a_params.args[1].GetUInt());
					}
				}
				case 9:
				{
					if (a_params.argCount == 1 && a_params.args[0].IsUInt()) {
						OnKeySelected(a_params.args[0].GetUInt());
					}
				}
			}
		}

		void CloseMenu()
		{
			if (auto UIMessageQueue = RE::UIMessageQueue::GetSingleton(); UIMessageQueue) {
				RE::UIUtils::PlayMenuSound("UIMenuCancel");
				UIMessageQueue->AddMessage("NAFStudioMenu", RE::UI_MESSAGE_TYPE::kHide);
			}
		}

		void OnMenuObjLoaded()
		{
			GetStageSize();
		}

		void SetPlayState(bool playing, bool forceUiUpdate = false) {
			VisitManagedGraphs([&](Graph* g) {
				forceUiUpdate = forceUiUpdate || (g->creator->IsPlaying() != playing);
				g->creator->SetPlaying(playing);
			});
			if (forceUiUpdate) {
				RE::Scaleform::GFx::Value args[1];
				args[0] = !playing;
				menuObj.Invoke("SetPlayState", nullptr, args, 1);
			}
		}

		void SetTimelineSize(size_t size) {
			RE::Scaleform::GFx::Value args[1];
			args[0] = static_cast<uint32_t>(size);
			menuObj.Invoke("SetTimelineSize", nullptr, args, 1);
		}

		void SetTimelineData(const std::vector<bool>& tlData) {
			RE::Scaleform::GFx::Value args[1];
			uiMovie->CreateArray(&args[0]);
			for (const bool b : tlData) {
				args[0].PushBack(b);
			}
			menuObj.Invoke("SetTimelineKeys", nullptr, args, 1);
		}

		void UpdateTimelineData() {
			size_t idx = UINT64_MAX;
			if (gizmoActive) {
				idx = gizmoIndex;
			}
			std::vector<bool> tlData;
			VisitTargetGraph([&](Graph* g) {
				g->creator->GetTimelineState(tlData, idx);
			});
			SetTimelineData(tlData);
			if (idx == UINT64_MAX) {
				selectedFrame = std::nullopt;
			}
			if (selectedFrame.has_value()) {
				RE::Scaleform::GFx::Value args[2];
				args[0] = selectedFrame.value();
				args[1] = true;
				menuObj.Invoke("SetKeySelected", nullptr, args, 2);
			}
		}

		void OnClick(const std::string_view& a_type, RE::Scaleform::GFx::Value& obj) {
			if (a_type == "NodeMarker") {
				RE::Scaleform::GFx::Value gIdx;
				if (!obj.GetMember("iIndex", &gIdx) || !gIdx.IsUInt()) {
					return;
				}

				uint32_t idx = gIdx.GetUInt();

				if (idx >= nodeMarkers.size()) {
					return;
				}

				RE::Scaleform::GFx::Value args[1];
				args[0] = true;
				obj.Invoke("SetGizmoVisible", nullptr, args, 1);

				gizmoIndex = idx;
				gizmoActive = true;

				UpdateAdjustModeIcon();
				SetNodesVisible(false);
				UpdateTimelineData();
			} else if (a_type == "Play") {
				VisitManagedGraphs([](Graph* g) {
					g->creator->SetPlaying(false);
				});
			} else if (a_type == "Pause") {
				VisitManagedGraphs([](Graph* g) {
					g->creator->SetPlaying(true);
				});
			}
		}

		void OnGizmoMovement(uint32_t a_type, float mouseX, float mouseY) {
			RE::NiPoint3 changePt3{ 0, 0, 0 };

			const auto calcMovementAmount = [&](const RE::NiPoint2& gizmoPoint) {
				float dirX = gizmoPoint.x;
				float dirY = gizmoPoint.y;

				//If gizmo is in rotation mode, use the 2D direction perpendicular
				//to the gizmo line, as that is the direction the rotation is actually in.
				if (adjustMode == Creator::kRotation) {
					float tempX = dirX;
					dirX = -dirY;
					dirY = tempX;
				}

				float dirMagnitude = sqrt(dirX * dirX + dirY * dirY);
				float normalizedDirX = dirX / dirMagnitude;
				float normalizedDirY = dirY / dirMagnitude;

				float mouseMagnitude = sqrt(mouseX * mouseX + mouseY * mouseY);
				float normalizedMouseX = mouseX / mouseMagnitude;
				float normalizedMouseY = mouseY / mouseMagnitude;

				float dotProduct = normalizedDirX * normalizedMouseX + normalizedDirY * normalizedMouseY;

				return (dotProduct * mouseMagnitude) * gizmoMovementScale;
			};

			switch (a_type) {
			case 0:
				changePt3.x = calcMovementAmount(gizmoXPos);
				break;
			case 1:
				changePt3.y = calcMovementAmount(gizmoYPos);
				break;
			case 2:
				changePt3.z = calcMovementAmount(gizmoZPos);
				break;
			}

			VisitTargetGraph([&](Graph* g) {
				g->creator->IncrementalAdjust(changePt3.x, changePt3.y, changePt3.z);
			});
		}

		void OnGizmoCursor(bool active) {
			if (active) {
				SetPlayState(false);
			}

			VisitTargetGraph([&](Graph* g) {
				if (active) {
					g->creator->SnapToNearestFrame();
					SetScrubberPosition(g->creator->GetUINormalizedTime());
					g->creator->BeginIncrementalAdjust(gizmoIndex, g->creator->GetCurrentFrame(), adjustMode);
				} else {
					g->creator->EndIncrementalAdjust();
				}
			});
			movingGizmo = active;
			UpdateTimelineData();
		}

		void OnScrubberPositionChanged(double p) {
			SetPlayState(false);
			VisitManagedGraphs([&](Graph* g) {
				g->creator->SetUINormalizedTime(static_cast<float>(p));
			});
		}

		void OnKeyMoved(uint32_t oldIdx, uint32_t newIdx) {
			VisitTargetGraph([&](Graph* g) {
				g->creator->MoveFrame(gizmoIndex, oldIdx, newIdx);
			});
			if (selectedFrame.has_value() && selectedFrame.value() == oldIdx) {
				selectedFrame = newIdx;
			}
		}

		void OnKeySelected(uint32_t idx) {
			if (selectedFrame.has_value()) {
				RE::Scaleform::GFx::Value args[2];
				args[0] = selectedFrame.value();
				args[1] = false;
				menuObj.Invoke("SetKeySelected", nullptr, args, 2);
			}
			selectedFrame = idx;
			RE::Scaleform::GFx::Value args[2];
			args[0] = idx;
			args[1] = true;
			menuObj.Invoke("SetKeySelected", nullptr, args, 2);
		}

		void UpdateAdjustModeIcon() {
			if (gizmoActive && gizmoIndex < nodeMarkers.size()) {
				RE::Scaleform::GFx::Value args[1];
				args[0] = adjustMode == Creator::kRotation ? 1 : 2;
				nodeMarkers[gizmoIndex].Invoke("SetAdjustIcon", nullptr, args, 1);
			}
		}

		bool IsCtrlModifierActive() const {
			return activeModifiers.any(ModifierKeys::kLControl, ModifierKeys::kRControl);
		}

		void SetModifierKey(ModifierKeys k, bool active) {
			if (active) {
				activeModifiers.set(k);
			} else {
				activeModifiers.reset(k);
			}
		}

		void SetInputValue(bool active, bool negative, std::atomic<float>& value) {
			if (active && negative) {
				value = value > 0.0f ? 0.0f : -1.0f;
			} else if (!active && negative) {
				value = value < 0.0f ? 0.0f : value.load();
			} else if (active && !negative) {
				value = value < 0.0f ? 0.0f : 1.0f;
			} else if (!active && !negative) {
				value = value > 0.0f ? 0.0f : value.load();
			}
		}

		void HandleCameraInput(const RE::ButtonEvent* a_event) {
			switch (a_event->GetBSButtonCode()) {
				case RE::BS_BUTTON_CODE::kW:
				{
					SetInputValue(a_event->QJustPressed(), false, CamHook::forwardBackInput);
					break;
				}
				case RE::BS_BUTTON_CODE::kA:
				{
					SetInputValue(a_event->QJustPressed(), true, CamHook::leftRightInput);
					break;
				}
				case RE::BS_BUTTON_CODE::kS:
				{
					SetInputValue(a_event->QJustPressed(), true, CamHook::forwardBackInput);
					break;
				}
				case RE::BS_BUTTON_CODE::kD:
				{
					SetInputValue(a_event->QJustPressed(), false, CamHook::leftRightInput);
					break;
				}
				case RE::BS_BUTTON_CODE::kQ:
				{
					SetInputValue(a_event->QJustPressed(), false, CamHook::upDownInput);
					break;
				}
				case RE::BS_BUTTON_CODE::kE:
				{
					SetInputValue(a_event->QJustPressed(), true, CamHook::upDownInput);
					break;
				}
				case RE::BS_BUTTON_CODE::kUp:
				{
					SetInputValue(a_event->QJustPressed(), true, CamHook::turnUpDownInput);
					break;
				}
				case RE::BS_BUTTON_CODE::kLeft:
				{
					SetInputValue(a_event->QJustPressed(), true, CamHook::turnRightLeftInput);
					break;
				}
				case RE::BS_BUTTON_CODE::kRight:
				{
					SetInputValue(a_event->QJustPressed(), false, CamHook::turnRightLeftInput);
					break;
				}
				case RE::BS_BUTTON_CODE::kDown:
				{
					SetInputValue(a_event->QJustPressed(), false, CamHook::turnUpDownInput);
					break;
				}
				case RE::BS_BUTTON_CODE::kLAlt:
				{
					CamHook::useExternalOrbit = a_event->QJustPressed();
					break;
				}
			}
		}

		virtual void HandleEvent(const RE::ButtonEvent* a_event) override
		{
			if (inputEventHandlingEnabled && a_event->strUserEvent != "NextFocus") {
				switch (a_event->GetBSButtonCode()) {
					case RE::BS_BUTTON_CODE::kTab:
					{
						if (a_event->QJustPressed() && !movingGizmo)
							BackImpl();
						break;
					}
					case RE::BS_BUTTON_CODE::kLControl:
					{
						SetModifierKey(ModifierKeys::kLControl, a_event->QJustPressed());
						break;
					}
					case RE::BS_BUTTON_CODE::kRControl:
					{
						SetModifierKey(ModifierKeys::kRControl, a_event->QJustPressed());
						break;
					}
					case RE::BS_BUTTON_CODE::kZ:
					{
						if (a_event->QJustPressed() && IsCtrlModifierActive() && !movingGizmo)
							UndoRedo(true);
						break;
					}
					case RE::BS_BUTTON_CODE::kY:
					{
						if (a_event->QJustPressed() && IsCtrlModifierActive() && !movingGizmo)
							UndoRedo(false);
						break;
					}
					case RE::BS_BUTTON_CODE::kDelete:
					{
						if (a_event->QJustPressed() && gizmoActive && selectedFrame.has_value() && !movingGizmo) {
							VisitTargetGraph([&](Graph* g) {
								g->creator->DeleteFrame(gizmoIndex, selectedFrame.value());
							});
							selectedFrame = std::nullopt;
							UpdateTimelineData();
						}
						break;
					}
					case RE::BS_BUTTON_CODE::kC:
					{
						if (a_event->QJustPressed() && IsCtrlModifierActive() && gizmoActive && selectedFrame.has_value() && !movingGizmo) {
							VisitTargetGraph([&](Graph* g) {
								g->creator->CopyFrame(gizmoIndex, selectedFrame.value());
							});
							UpdateTimelineData();
						}
						break;
					}
					case RE::BS_BUTTON_CODE::kR:
					{
						if (a_event->QJustPressed() && adjustMode != Creator::kRotation && !movingGizmo) {
							adjustMode = Creator::kRotation;
							UpdateAdjustModeIcon();
						}
						break;
					}
					case RE::BS_BUTTON_CODE::kT:
					{
						if (a_event->QJustPressed() && adjustMode != Creator::kPosition && !movingGizmo) {
							adjustMode = Creator::kPosition;
							UpdateAdjustModeIcon();
						}
						break;
					}
					case RE::BS_BUTTON_CODE::kLBracket:
					{
						if (a_event->QJustPressed() && IsCtrlModifierActive()) {
							nodeSizeModifier -= 0.05f;
						}
						break;
					}
					case RE::BS_BUTTON_CODE::kRBracket:
					{
						if (a_event->QJustPressed() && IsCtrlModifierActive()) {
							nodeSizeModifier += 0.05f;
						}
						break;
					}
					default:
					{
						HandleCameraInput(a_event);
						break;
					}
				}
			}
		}

		void UndoRedo(bool isUndo) {
			VisitTargetGraph([&](Graph* g) {
				if (isUndo) {
					g->creator->Undo();
				} else {
					g->creator->Redo();
				}
			});
			selectedFrame = std::nullopt;
			UpdateTimelineData();
		}

		void BackImpl() {
			if (RE::UI::GetSingleton()->GetMenuOpen("NAFMenu"))
				return;

			if (gizmoActive) {
				if (gizmoIndex <= nodeMarkers.size()) {
					RE::Scaleform::GFx::Value args[1];
					args[0] = false;
					nodeMarkers[gizmoIndex].Invoke("SetGizmoVisible", nullptr, args, 1);
				}
				gizmoActive = false;
				SetNodesVisible(true);
				UpdateTimelineData();
			} else {
				CloseMenu();
			}
		}

		void CloseImpl() {
			SaveChangesImpl(true);
			ClearTarget();
			ClearManagedRefs();
		}

		void SaveChangesImpl(bool clearActiveProject = false) {
			auto data = PersistentMenuState::CreatorData::GetSingleton();
			for (auto& a : data->studioActors) {
				BodyAnimation::GraphHook::VisitGraph(a.actor.get(), [&](Graph* g) {
					g->creator->SaveToNANIM(a.animId, data->bodyAnims);

					BodyAnimation::NANIM::AnimationData::MetaData meta;
					meta.data["actor_scale"].push_back(std::format("{}", a.actor->GetScale()));
					auto& enabledChains = meta.data["enabled_ik_chains"];
					for (auto& c : g->ikManager.GetChainList()) {
						if (g->ikManager.GetChainEnabled(c)) {
							enabledChains.push_back(c);
						}

						auto p = g->ikManager.GetChainTargetParent(c);
						if (auto actor = p.first.get(); actor != nullptr) {
							std::string targetAnim = "";
							for (auto& sData : data->studioActors) {
								if (sData.actor.get() == actor.get()) {
									targetAnim = sData.animId;
									break;
								}
							}
							meta.data[std::format("ik_target_parent_{}", c)] = { targetAnim, p.second };
						}
					}
					data->bodyAnims.SetMetaData(a.animId, meta);
				});
			}
			if (data->activeBodyAnim.has_value()) {
				if (clearActiveProject) {
					data->ClearActiveBodyAnimProject();
				}
			}
		}

		virtual RE::UI_MESSAGE_RESULTS ProcessMessage(RE::UIMessage& a_message) override
		{
			switch (*a_message.type) {
				case RE::UI_MESSAGE_TYPE::kShow:
				{
					studioInstance = this;
					auto ControlMap = RE::ControlMap::GetSingleton();
					ControlMap->PushInputContext(RE::UserEvents::INPUT_CONTEXT_ID::kBasicMenuNav);
					ControlMap->PushInputContext(RE::UserEvents::INPUT_CONTEXT_ID::kThumbNav);
					ControlMap->PushInputContext(RE::UserEvents::INPUT_CONTEXT_ID::kVirtualController);
					RE::SendHUDMessage::PushHUDMode("SpecialMode");
					RE::PlayerControls::GetSingleton()->blockPlayerInput = true;
					GameUtil::SetFlyCam(true);
					CamHook::SetUseExternalInput(true);

					return RE::UI_MESSAGE_RESULTS::kPassOn;
				}
				case RE::UI_MESSAGE_TYPE::kHide:
				{
					CloseImpl();
					studioInstance = nullptr;
					auto ControlMap = RE::ControlMap::GetSingleton();
					ControlMap->PopInputContext(RE::UserEvents::INPUT_CONTEXT_ID::kBasicMenuNav);
					ControlMap->PopInputContext(RE::UserEvents::INPUT_CONTEXT_ID::kThumbNav);
					ControlMap->PopInputContext(RE::UserEvents::INPUT_CONTEXT_ID::kVirtualController);
					
					if (!RE::UI::GetSingleton()->GetMenuOpen("NAFMenu")) {
						RE::SendHUDMessage::PopHUDMode("SpecialMode");
						RE::PlayerControls::GetSingleton()->blockPlayerInput = false;
					}

					GameUtil::SetFlyCam(false);
					CamHook::SetUseExternalInput(false);
					CamHook::orbitTarget.MakeIdentity();
					CamHook::useExternalOrbit = false;
						
					return RE::UI_MESSAGE_RESULTS::kPassOn;
				}
			default:
				return GameMenuBase::ProcessMessage(a_message);
			}
		}

		static RE::Setting& GetScreenHeight()
		{
			REL::Relocation<RE::Setting*> singleton{ REL::ID(1302430) };
			return *singleton;
		}

		static RE::Setting& GetScreenWidth()
		{
			REL::Relocation<RE::Setting*> singleton{ REL::ID(644641) };
			return *singleton;
		}

		void GetStageSize()
		{
			RE::Scaleform::GFx::Value result;

			menuObj.Invoke("GetStageWidth", &result);
			float swfWidth = static_cast<float>(result.GetNumber());

			menuObj.Invoke("GetStageHeight", &result);
			float swfHeight = static_cast<float>(result.GetNumber());

			float width = static_cast<float>(GetScreenWidth().GetInt());
			float height = static_cast<float>(GetScreenHeight().GetInt());

			float scaleYInverse = 1.0f / (height / swfHeight);
			float scaledWidth = width * scaleYInverse;
			widthAdjust = ((scaledWidth - swfWidth) / 2) * -1.0f;
			stageWidth = scaledWidth;
			stageHeight = swfHeight;
		}

		float widthAdjust = 0.0f;
		float stageWidth = 0.0f;
		float stageHeight = 0.0f;
		float nodeSizeModifier = 1.0f;

	private:
		enum SelectionType
		{
			kNode,
			kPole,
			kEffector
		};

		Creator::AdjustmentMode adjustMode = Creator::kRotation;
		F4SE::stl::enumeration<ModifierKeys, uint8_t> activeModifiers = ModifierKeys::kNone;
		std::optional<uint32_t> selectedFrame = std::nullopt;
		size_t gizmoIndex = 0;
		bool gizmoActive = false;
		bool movingGizmo = false;
		bool showTargetNodes = true;
		std::vector<RE::Scaleform::GFx::Value> nodeMarkers;
		std::optional<SerializableActorHandle> targetHandle = std::nullopt;
		float gizmoMovementScale = 1.0f;
		RE::NiPoint2 gizmoXPos;
		RE::NiPoint2 gizmoYPos;
		RE::NiPoint2 gizmoZPos;

		std::vector<SerializableActorHandle> managedActors;

		inline static std::atomic<NAFStudioMenu*> studioInstance = nullptr;
	};
}
