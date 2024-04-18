#pragma once

namespace BodyAnimation
{
	struct NodeAnimationGraph
	{
		enum GraphState : uint8_t
		{
			kIdle,
			kTransition,
			kGenerator,
			kRecording
		};

		enum GraphFlags : uint16_t
		{
			kNoFlags = 0,
			kTemporary = 1u << 0,
			kUnloaded3D = 1u << 1,
			kNoActiveIKChains = 1u << 2
		};

		enum TransitionType : uint8_t
		{
			kNone,
			kHavokToGraph,
			kGraphToHavok,
			kGraphSnapshotToHavok,
			kGeneratorToGenerator
		};

		std::mutex updateLock;
		Serialization::General::SerializableRefHandle targetHandle;
		GraphState state = kIdle;
		F4SE::stl::enumeration<GraphFlags, uint16_t> flags = kNoFlags;

		RE::NiPointer<RE::NiAVObject> rootNode;
		std::vector<RE::NiPointer<RE::NiAVObject>> nodes;
		std::vector<std::string> nodeMap;

		RegisterFor3DChangeFunctor reg3dCallback = nullptr;
		std::function<void()> unreg3dCallback = nullptr;

		IKManager ikManager = IKManager(
			nodeMap,
			std::function<void(bool)>(std::bind(&NodeAnimationGraph::OnIKChainActiveChanged, this, std::placeholders::_1))
		);

		std::string animationFilePath = "";
		std::string animationId = "";
		NodeAnimationGenerator generator;
		NodeAnimationRecorder recorder;
		std::unique_ptr<NodeAnimationCreator> creator = nullptr;

		TransitionType activeTransition;
		std::vector<NodeTransform> transitionFromSnapshot;
		std::vector<NodeTransform> transitionOutput;
		float transitionLocalTime = 0.0f;
		float transitionDuration = 0.01f;

		~NodeAnimationGraph() {
			SetDisableOCBP(false);

			if (unreg3dCallback != nullptr)
				unreg3dCallback();
		}

		NodeAnimationGraph() {
			flags.set(kTemporary, kNoActiveIKChains);
		}

		void SetDisableOCBP(bool a_disable) {
			if (F4SE::GetPluginInfo("OCBP plugin").has_value()) {
				auto r = targetHandle.get();
				for (auto& n : nodeMap) {
					GameUtil::CallGlobalPapyrusFunction("OCBP_API", "SetBoneToggle", r.get(), a_disable, n);
				}
			}
		}

		void OnIKChainActiveChanged(bool chainActive) {
			if (chainActive) {
				flags.reset(kNoActiveIKChains);
			} else {
				flags.set(kNoActiveIKChains);
			}
		}

		bool IsAnimating() const {
			return (state == kGenerator || 
				(state == kTransition && 
				(activeTransition == kGeneratorToGenerator || 
				activeTransition == kHavokToGraph))) &&
				generator.HasAnimation();
		}

		void SetGraphData(const Data::GraphInfo& d) {
			nodeMap = d.nodeList;
			SetDisableOCBP(true);
			ikManager.ClearChains();
			for (const auto& pair : d.chains) {
				const auto& c = pair.second;
				std::unique_ptr<IKHolder> h;
				if (c.nodes.size() == 3 && c.poleNode.has_value()) {
					h = std::make_unique<IKTwoBoneHolder>(c.nodes, c.poleParent, c.poleStartPos);
				} else {
					h = std::make_unique<IKArbitraryHolder>(c.nodes);
				}
				h->holderId = pair.first;
				h->SetControlsTranslation(c.controlsTranslation);
				ikManager.AddChain(
					std::move(h),
					c.effectorNode,
					c.poleNode);
			}
			ikManager.UpdateMappings();
			transitionOutput.resize(nodeMap.size());
			transitionFromSnapshot.resize(nodeMap.size());
		}

		bool UpdateNodes(RE::TESObjectREFR* ref)
		{
			if (ref->GetHandle().native_handle_const() == targetHandle.hash()) {
				rootNode.reset();
				nodes.clear();
				ikManager.ClearNodes();
				if (ref == nullptr) {
					flags.set(kUnloaded3D);
					return false;
				}

				auto _3d = ref->Get3D();
				if (_3d == nullptr) {
					flags.set(kUnloaded3D);
					return false;
				}

				rootNode = RE::NiPointer<RE::NiAVObject>(_3d);

				for (const auto& n : nodeMap) {
					nodes.emplace_back(_3d->GetObjectByName(n));
				}

				flags.reset(kUnloaded3D);
				ikManager.GetNodes(nodes, _3d);
			} else {
				ikManager.OnOther3DChange(ref);
			}
			
			return true;
		}

		void TransitionToAnimation(std::unique_ptr<NodeAnimation> anim, float duration = 1.3f) {
			transitionLocalTime = 0.0f;
			transitionDuration = duration;

			switch (state) {
			case kRecording:
			case kIdle:
				if (anim != nullptr) {
					activeTransition = kHavokToGraph;
				} else {
					return;
				}
				break;
			case kGenerator:
				if (anim != nullptr) {
					SnapshotTransformsForTransition();
					activeTransition = kGeneratorToGenerator;
				} else {
					activeTransition = kGraphToHavok;
				}
				break;
			case kTransition:
				if (anim != nullptr) {
					SnapshotTransformsForTransition();
					activeTransition = kGeneratorToGenerator;
				} else {
					SnapshotTransformsForTransition();
					activeTransition = kGraphSnapshotToHavok;
				}
				break;
			}

			state = kTransition;
			if (anim != nullptr) {
				generator.SetAnimation(std::move(anim));
				generator.localTime = 0.0f;
			}
		}

		void SetRecording(bool a_record) {
			if (a_record && state != kRecording) {
				recorder.Reset();
				recorder.Init(nodes.size());
				state = kRecording;
			} else if (!a_record && state == kRecording) {
				state = kIdle;
			}
		}

		bool GetRecording() const {
			return state == kRecording;
		}

		void Update(float deltaTime) {
			switch (state) {
			case kGenerator:
				UpdateGenerator(deltaTime, true);
				break;
			case kTransition:
				UpdateTransition(deltaTime, activeTransition);
				break;
			case kRecording:
				UpdateRecorder(deltaTime);
				break;
			case kIdle:
				ikManager.Update();
				break;
			}
		}

	private:
		void PushOutput(const std::vector<NodeTransform>& a_output) {
			size_t updateCount = nodes.size() > a_output.size() ? a_output.size() : nodes.size();

			for (size_t i = 0; i < updateCount; i++) {
				if (nodes[i] != nullptr && !a_output[i].IsIdentity())
					a_output[i].ToComplex(nodes[i]->local);
			}

			if (state == kGenerator) {
				ikManager.Update(a_output);
			}
		}

		bool UpdateGenerator(float deltaTime, bool output = false) {
			if (!generator.HasAnimation()) {
				state = kIdle;
				return false;
			}

			generator.Update(deltaTime);
			if (output) {
				PushOutput(generator.output);
			}

			return true;
		}

		void UpdateRecorder(float deltaTime) {
			recorder.Update(deltaTime, nodes);
		}

		NodeTransform NodeTransformOrDefault(size_t idx) {
			if (nodes[idx] != nullptr)
				return NodeTransform(nodes[idx]->local);

			return NodeTransform::Identity();
		}

		void UpdateTransition(float deltaTime, TransitionType a_type) {
			//Since the GraphHook executes directly after the Havok graph
			//finishes updating, the current node transforms on the character
			//will be those from the Havok graph. We can use this information
			//to interpolate between our graph's current state and the Havok
			//graph's current state.
			switch (a_type) {
			case kHavokToGraph:
				UpdateTransition(deltaTime, kGenerator, [&](size_t i) {
					return std::pair<NodeTransform, NodeTransform>(
						NodeTransformOrDefault(i),
						generator.output[i]
					);
				});
				break;
			case kGraphToHavok:
				UpdateTransition(deltaTime, kIdle, [&](size_t i) {
					return std::pair<NodeTransform, NodeTransform>(
						generator.output[i],
						NodeTransformOrDefault(i)
					);
				});
				break;
			case kGeneratorToGenerator:
				UpdateTransition(deltaTime, kGenerator, [&](size_t i) {
					return std::pair<NodeTransform, NodeTransform>(
						transitionFromSnapshot[i],
						generator.output[i]
					);
				});
				break;
			case kGraphSnapshotToHavok:
				UpdateTransition(deltaTime, kIdle, [&](size_t i) {
					return std::pair<NodeTransform, NodeTransform>(
						transitionFromSnapshot[i],
						NodeTransformOrDefault(i)
					);
				});
				break;
			}
		}

		void UpdateTransition(float deltaTime, GraphState endState, std::function<std::pair<NodeTransform, NodeTransform>(size_t)> lerpParams) {
			if (UpdateGenerator(deltaTime)) {
				transitionLocalTime += deltaTime;

				if (transitionLocalTime >= transitionDuration) {
					state = endState;
					activeTransition = kNone;

					if (transitionDuration < 0.01f) {
						transitionDuration = 0.01f;
					}

					transitionLocalTime = transitionDuration;
				}

				size_t count = nodes.size() > generator.output.size() ? generator.output.size() : nodes.size();
				for (size_t i = 0; i < count; i++) {
					auto trans = lerpParams(i);
					if (trans.first.IsIdentity()) {
						transitionOutput[i] = trans.second;
					} else if(trans.second.IsIdentity()) {
						transitionOutput[i] = trans.first;
					} else {
						transitionOutput[i].Lerp(trans.first, trans.second, static_cast<float>(Easing::easeInOutCubic(MathUtil::NormalizeTime(0.0f, transitionDuration, transitionLocalTime))));
					}
				}

				if (state == kIdle) {
					generator.SetAnimation(nullptr);
				}

				PushOutput(transitionOutput);
			}
		}

		void SnapshotTransformsForTransition() {
			for (size_t i = 0; i < nodes.size(); i++) {
				transitionFromSnapshot[i] = NodeTransformOrDefault(i);
			}
		}
	};
}
