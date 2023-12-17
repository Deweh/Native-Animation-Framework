#pragma once
#include "NodeAnimationData.h"
#include "IK.h"
#include "NANIM.h"

namespace BodyAnimation
{
	struct NodeAnimationGenerator
	{
		bool paused = false;
		float localTime = 0.0f;
		std::vector<NodeTransform> output;
		std::unique_ptr<NodeAnimation> animData = nullptr;

		bool HasAnimation() const {
			return animData != nullptr;
		}

		void SetAnimation(std::unique_ptr<NodeAnimation> anim) {
			animData = std::move(anim);
			output.clear();
			if (animData != nullptr) {
				output.resize(animData->timelines.size());
				for (auto& t : animData->timelines) {
					t.Init();
				}
			}
		}

		void Update(float deltaTime) {
			if (!paused) {
				localTime += deltaTime;
				while (localTime >= animData->duration) {
					localTime -= animData->duration;
				}
			}

			for (size_t i = 0; i < animData->timelines.size(); i++) {
				animData->timelines[i].GetValueAtTime(localTime, output[i]);
			}
		}
	};

	struct NodeAnimationRecorder
	{
		float localTime = 0.0f;
		float sampleRate = 0.033f;
		float recordInterval = 0.0f;
		std::unique_ptr<NodeAnimation> animData = nullptr;

		void Reset() {
			localTime = 0.0f;
			recordInterval = sampleRate + 0.001f;
			animData.reset();
		}

		void Init(size_t size) {
			animData = std::make_unique<NodeAnimation>();
			animData->timelines.resize(size);
		}

		void Update(float deltaTime, const std::vector<RE::NiPointer<RE::NiAVObject>>& nodes) {
			localTime += deltaTime;
			recordInterval += deltaTime;

			if (recordInterval < sampleRate)
				return;

			recordInterval = 0.0f;

			animData->duration = localTime;
			for (size_t i = 0; i < nodes.size(); i++) {
				if (nodes[i] != nullptr) {
					animData->timelines[i].keys.insert({ localTime, NodeKeyframe{ NodeTransform(nodes[i]->local) } });
				}
			}
		}

		void SaveRecording(const std::string& filePath, const std::vector<std::string>& nodeMap)
		{
			std::thread([data = std::move(animData), filePath = filePath, nodeMap = nodeMap]() {
				NANIM file;
				file.SetAnimation("default", nodeMap, data.get());
				file.SaveToFile(filePath);
			}).detach();
			animData = nullptr;
		}
	};

	struct NodeAnimationCreator
	{
		struct ChangeDelta
		{
			size_t frame;
			size_t nodeIndex;
			std::optional<NodeTransform> forwardValue;
			std::optional<NodeTransform> backValue;
		};

		struct HistoryEntry
		{
			std::string displayName = "";
			std::vector<ChangeDelta> deltas;

			void SetNameFromDelta(const ChangeDelta& d) {
				displayName = std::format("Transform Node {} on Frame {}", d.nodeIndex, d.frame);
			}
		};

		struct BAKE_DATA
		{
			std::unique_ptr<NodeAnimation> animData;
			size_t updateCount;
			float curTime = 0.0f;
			float sampleRate = 0.1f;

			bool StepForward() {
				auto next = curTime + sampleRate;
				if (next < animData->duration) {
					curTime = next;
					return true;
				} else if (std::fabs(curTime - animData->duration) < 0.001f) {
					return false;
				} else {
					curTime = animData->duration;
					return true;
				}
			}
		};

		enum FrameAction
		{
			kNoChange,
			kCreateIfNotFound,
			kDelete
		};

		enum AdjustmentMode
		{
			kPosition,
			kRotation
		};

		enum LoadResult
		{
			kOK,
			kDataLoss,
			kFailed
		};

		enum InterpType
		{
			kLinear,
			kSquad,
			kCatmullRom,
			kNaturalCubic
		};

		std::optional<HistoryEntry> currentEntry = std::nullopt;
		std::optional<ChangeDelta> currentDelta = std::nullopt;
		std::deque<HistoryEntry> undoHistory;
		std::deque<HistoryEntry> redoHistory;

		NodeAnimationGenerator* generator;
		std::vector<RE::NiPointer<RE::NiAVObject>>* nodeList = nullptr;
		std::vector<std::string>* nodeMap = nullptr;
		std::unique_ptr<FrameBasedNodeAnimation> animData = std::make_unique<FrameBasedNodeAnimation>();
		IKManager* ikManager;

		AdjustmentMode adjustMode = kPosition;
		InterpType posInterpType = kNaturalCubic;
		InterpType rotInterpType = kSquad;

		NodeAnimationCreator(NodeAnimationGenerator* gen, std::vector<RE::NiPointer<RE::NiAVObject>>* nodes, std::vector<std::string>* nMap, IKManager* ikMan)
		{
			nodeList = nodes;
			nodeMap = nMap;
			ikManager = ikMan;
			generator = gen;
			generator->paused = true;
			generator->localTime = 0.0001f;
			PushDataToGenerator(false);
		}

		BAKE_DATA SetupBake() {
			PushDataToGenerator(true);
			BAKE_DATA result;
			result.animData = std::make_unique<NodeAnimation>();
			result.animData->duration = animData->GetRuntimeDuration() - animData->sampleRate;
			result.animData->timelines.resize(animData->timelines.size());
			result.updateCount = animData->timelines.size() < nodeList->size() ? animData->timelines.size() : nodeList->size();
			result.sampleRate = animData->sampleRate;
			return result;
		}

		void GenerateAtTime(const BAKE_DATA& data) {
			generator->localTime = data.curTime;
			generator->Update(0.0f);

			for (size_t i = 0; i < data.updateCount; i++) {
				if (nodeList->at(i) != nullptr && !generator->output[i].IsIdentity())
					generator->output[i].ToComplex(nodeList->at(i)->local);
			}
		}

		void CalculateInverseKinematics() {
			ikManager->Update(generator->output, true);
		}

		void SampleAtTime(BAKE_DATA& data) {
			for (size_t i = 0; i < data.updateCount; i++) {
				if (nodeList->at(i) != nullptr)
					data.animData->timelines[i].keys[data.curTime].value = nodeList->at(i)->local;
			}
		}

		void BakeToNANIM(const std::string& name, NANIM& container) {
			auto data = SetupBake();

			do {
				GenerateAtTime(data);
				CalculateInverseKinematics();
				SampleAtTime(data);
			} while (data.StepForward());
			
			container.SetAnimation(name, *nodeMap, data.animData.get());
		}

		void SaveToNANIM(const std::string& name, NANIM& container) {
			auto data = animData->ToRuntime(false);
			container.SetAnimation(name, *nodeMap, data.get());
		}

		LoadResult LoadFromNANIM(const std::string& name, const NANIM& container, float sampleRate = 0.033333f) {
			std::unique_ptr<NodeAnimation> data;
			if (!container.GetAnimation(name, *nodeMap, data))
				return LoadResult::kFailed;

			auto result = FrameBasedNodeAnimation::FromRuntime(data.get(), sampleRate);
			animData = std::move(result.data);
			if (animData->timelines.size() < nodeList->size()) {
				animData->timelines.resize(nodeList->size());
			}

			generator->paused = true;
			generator->localTime = 0.0001f;
			PushDataToGenerator(true);
			return result.dataLoss ? LoadResult::kDataLoss : LoadResult::kOK;
		}

		bool IsPlaying() {
			return !generator->paused;
		}

		void SetPlaying(bool playing) {
			generator->paused = !playing;
		}

		void SetTimeNormalized(float tNorm) {
			SetTime(animData->GetRuntimeDuration() * tNorm);
		}

		void SetTime(float t) {
			if (generator->HasAnimation()) {
				generator->localTime = std::clamp(t, 0.00001f, (generator->animData->duration + animData->sampleRate) - 0.001f);
			}
		}

		float GetTimeNormalized() {
			return generator->localTime / animData->GetRuntimeDuration();
		}

		float GetUINormalizedTime()
		{
			return GetTimeNormalized() * static_cast<float>(GetMaxFrame());
		}

		void SetUINormalizedTime(float t)
		{
			SetTimeNormalized(t / static_cast<float>(GetMaxFrame()));
		}

		void SnapToNearestFrame() {
			SetTime(static_cast<float>(GetCurrentFrame()) * animData->sampleRate);
		}

		size_t GetMaxFrame() {
			return animData->duration;
		}

		size_t GetCurrentFrame() {
			float currentFrame = generator->localTime * (1.0f / animData->sampleRate);
			float result = std::round(currentFrame);

			//Check if we're within floating point error of an exact frame.
			//Otherwise, use std::floor to get the current frame.
			if (std::fabs(currentFrame - result) > 0.0001f) {
				result = std::floor(currentFrame);
			}

			return std::clamp(static_cast<size_t>(result), 0ui64, GetMaxFrame() - 1);
		}

		void PushDataToGenerator(bool splineSample = false, const std::optional<size_t> selectiveNodeIndex = std::nullopt) {
			if (!splineSample) {
				if (selectiveNodeIndex.has_value() &&
					generator->HasAnimation() &&
					selectiveNodeIndex.value() < generator->animData->timelines.size())
				{
					animData->UpdateRuntimeSelective(*selectiveNodeIndex, generator->animData->timelines[*selectiveNodeIndex]);
				} else {
					generator->SetAnimation(animData->ToRuntime());
				}
			} else {
				std::function<std::unique_ptr<MathUtil::InterpolationSystem<RE::NiQuaternion>>()> rotInterp;
				std::function<std::unique_ptr<MathUtil::InterpolationSystem<RE::NiPoint3>>()> posInterp;

				switch (rotInterpType) {
				case kSquad:
					rotInterp = []() { return std::make_unique<MathUtil::QuatSquadSpline>(); };
					break;
				case kCatmullRom:
					rotInterp = []() { return std::make_unique<MathUtil::QuatCatmullRomSpline>(); };
					break;
				case kNaturalCubic:
					rotInterp = []() { return std::make_unique<MathUtil::QuatNaturalCubicSpline>(); };
					break;
				default:
					rotInterp = []() { return std::make_unique<MathUtil::QuatLinear>(); };
					break;
				}

				switch (posInterpType) {
				case kNaturalCubic:
					posInterp = []() { return std::make_unique<MathUtil::Pt3NaturalCubicSpline>(); };
					break;
				default:
					posInterp = []() { return std::make_unique<MathUtil::Pt3Linear>(); };
					break;
				}

				generator->SetAnimation(animData->ToRuntimeSampled(rotInterp, posInterp));
			}
		}

		bool NodeValid(size_t nodeIndex) {
			return nodeIndex < animData->timelines.size();
		}

		bool FrameExists(size_t nodeIndex, size_t frame) {
			return NodeValid(nodeIndex) && animData->timelines[nodeIndex].keys.contains(frame);
		}

		void GetTimelineState(std::vector<bool>& result, size_t nodeIndex) {
			result.resize(GetMaxFrame());
			std::fill(result.begin(), result.end(), false);

			if (NodeValid(nodeIndex)) {
				for (auto& pair : animData->timelines[nodeIndex].keys) {
					result[pair.first] = true;
				}
			}
		}

		bool VisitFrame(size_t nodeIndex, size_t frame, FrameAction action, std::function<void(NodeKeyframe&)> visitFunc = nullptr) {
			if (NodeValid(nodeIndex)) {
				auto& keys = animData->timelines[nodeIndex].keys;

				if (!keys.contains(frame)) {
					if (action == kCreateIfNotFound) {
						keys.emplace(frame, NodeKeyframe{ NodeTransform::Identity() });
					} else {
						return false;
					}
				}

				if (visitFunc != nullptr) {
					visitFunc(keys[frame]);
				}

				if (action == kDelete) {
					keys.erase(frame);
				}
			}

			return true;
		}

		void MoveFrame(size_t nodeIndex, size_t src, size_t dest) {
			if (src != dest && NodeValid(nodeIndex)) {
				auto& keys = animData->timelines[nodeIndex].keys;

				if (dest >= GetMaxFrame() || src >= GetMaxFrame() || keys.contains(dest))
					return;

				auto iter = keys.find(src);
				if (iter == keys.end())
					return;

				BeginHistoryAction();
				BeginChangeDelta(nodeIndex, dest);
				keys[dest] = iter->second;
				EndChangeDelta();
				BeginChangeDelta(nodeIndex, src);
				keys.erase(iter);
				EndChangeDelta();
				EndHistoryAction();
				PushDataToGenerator(true);
			}
		}

		void DeleteFrame(size_t nodeIndex, size_t frame)
		{
			BeginHistoryAction();
			BeginChangeDelta(nodeIndex, frame);
			VisitFrame(nodeIndex, frame, kDelete);
			EndChangeDelta();
			EndHistoryAction();
			PushDataToGenerator(true);
		}

		void CopyFrame(size_t nodeIndex, size_t frame) {
			if (NodeValid(nodeIndex)) {
				auto& keys = animData->timelines[nodeIndex].keys;
				size_t target = frame + 1;

				if (keys.contains(target) || target >= GetMaxFrame())
					return;

				auto iter = keys.find(frame);
				if (iter == keys.end())
					return;

				BeginHistoryAction();
				BeginChangeDelta(nodeIndex, target);
				keys[target] = iter->second;
				EndChangeDelta();
				EndHistoryAction();
				PushDataToGenerator(true);
			}
		}

		bool SetTransform(size_t nodeIndex, size_t frame, const NodeTransform& trans, bool createKey = true) {
			bool result = VisitFrame(nodeIndex, frame, createKey ? kCreateIfNotFound : kNoChange, [&](NodeKeyframe& k) {
				k.value = trans;
			});
			return result;
		}

		std::optional<NodeTransform> GetTransform(size_t nodeIndex, size_t frame) {
			std::optional<NodeTransform> result = std::nullopt;
			VisitFrame(nodeIndex, frame, kNoChange, [&](NodeKeyframe& k) {
				result = k.value;
			});
			return result;
		}

		NodeTransform GetCurrentWorldTransform(size_t nodeIndex, bool parentRotation = false)
		{
			auto& ikMappings = ikManager->GetLookupMap();
			if (auto iter = ikMappings.find(nodeIndex); iter != ikMappings.end()) {
				return ikManager->GetWorldTransform(iter->second, parentRotation);
			}
			if (nodeIndex < nodeList->size() && nodeList->at(nodeIndex) != nullptr) {
				auto& n = nodeList->at(nodeIndex);
				NodeTransform res = n->world;
				if (parentRotation && n->parent != nullptr) {
					res.rotate.FromRotation(n->parent->world.rotate);
				}
				return res;
			}
			return NodeTransform::Identity();
		}

		NodeTransform GetCurrentTransform(size_t nodeIndex) {
			auto& ikMappings = ikManager->GetLookupMap();
			if (auto iter = ikMappings.find(nodeIndex); iter != ikMappings.end()) {
				return ikManager->GetLocalTransform(iter->second);
			}
			if (nodeIndex < nodeList->size() && nodeList->at(nodeIndex) != nullptr) {
				return nodeList->at(nodeIndex)->local;
			}
			return NodeTransform::Identity();
		}

		void ProcessChangeDelta(size_t nodeIndex, size_t frame, const std::optional<NodeTransform>& destValue) {
			if (!destValue.has_value()) {
				VisitFrame(nodeIndex, frame, kDelete);
			} else {
				SetTransform(nodeIndex, frame, destValue.value());
			}
		}

		std::optional<std::string> Undo() {
			if (!undoHistory.empty()) {
				auto& entry = undoHistory.front();
				std::string result = entry.displayName;
				for (auto& d : entry.deltas) {
					ProcessChangeDelta(d.nodeIndex, d.frame, d.backValue);
				}
				redoHistory.push_front(entry);
				undoHistory.pop_front();
				PushDataToGenerator(true);
				return result;
			}
			return std::nullopt;
		}

		std::optional<std::string> Redo() {
			if (!redoHistory.empty()) {
				auto& entry = redoHistory.front();
				std::string result = entry.displayName;
				for (auto& d : entry.deltas) {
					ProcessChangeDelta(d.nodeIndex, d.frame, d.forwardValue);
				}
				undoHistory.push_front(entry);
				redoHistory.pop_front();
				PushDataToGenerator(true);
				return result;
			}
			return std::nullopt;
		}

		void BeginHistoryAction(const std::string& displayName = "") {
			if (currentEntry.has_value())
				EndHistoryAction();

			currentEntry = HistoryEntry{ displayName };
		}

		void EndHistoryAction() {
			if (currentEntry.has_value()) {
				auto& e = currentEntry.value();
				if (!e.deltas.empty()) {
					if (e.displayName.empty()) {
						e.SetNameFromDelta(e.deltas[0]);
					}
					undoHistory.push_front(e);
				}
				currentEntry = std::nullopt;
				redoHistory.clear();
			}
		}

		void BeginChangeDelta(size_t nodeIndex, size_t frame) {
			if (currentDelta.has_value())
				EndChangeDelta();

			currentDelta = ChangeDelta{ frame, nodeIndex };
			currentDelta.value().backValue = GetTransform(nodeIndex, frame);
		}

		void EndChangeDelta() {
			if (currentDelta.has_value()) {
				if (currentEntry.has_value()) {
					auto& d = currentDelta.value();
					d.forwardValue = GetTransform(d.nodeIndex, d.frame);
					currentEntry->deltas.push_back(d);
				}
				currentDelta = std::nullopt;
			}
		}

		void BeginIncrementalAdjust(size_t nodeIndex, size_t frame, AdjustmentMode mode) {
			BeginHistoryAction();
			BeginChangeDelta(nodeIndex, frame);
			adjustMode = mode;
			if (!FrameExists(nodeIndex, frame)) {
				SetTransform(nodeIndex, frame, GetCurrentTransform(nodeIndex));
				PushDataToGenerator(false, nodeIndex);
			}
		}

		void IncrementalAdjust(float x, float y, float z, bool local = true) {
			if (!currentDelta.has_value() || !FrameExists(currentDelta->nodeIndex, currentDelta->frame))
				return;

			NodeTransform targetTransform = GetTransform(currentDelta->nodeIndex, currentDelta->frame).value();

			if (adjustMode == kRotation) {
				RE::NiPoint3 xAxis;
				RE::NiPoint3 yAxis;
				RE::NiPoint3 zAxis;

				if (local) {
					RE::NiMatrix3 mat;
					targetTransform.rotate.ToRotation(mat);
					xAxis = { mat.entry[0].pt[0], mat.entry[0].pt[1], mat.entry[0].pt[2] };
					yAxis = { mat.entry[1].pt[0], mat.entry[1].pt[1], mat.entry[1].pt[2] };
					zAxis = { mat.entry[2].pt[0], mat.entry[2].pt[1], mat.entry[2].pt[2] };
				} else {
					xAxis = { 1, 0, 0 };
					yAxis = { 0, 1, 0 };
					zAxis = { 0, 0, 1 };
				}

				RE::NiQuaternion adjustRot1;
				RE::NiQuaternion adjustRot2;
				RE::NiQuaternion adjustRot3;
				adjustRot1.FromAngleAxis(MathUtil::DegreeToRadian(x * 0.5f), xAxis);
				adjustRot2.FromAngleAxis(MathUtil::DegreeToRadian(y * 0.5f), yAxis);
				adjustRot3.FromAngleAxis(MathUtil::DegreeToRadian(z * 0.5f), zAxis);

				targetTransform.rotate = adjustRot1 * targetTransform.rotate;
				targetTransform.rotate = adjustRot2 * targetTransform.rotate;
				targetTransform.rotate = adjustRot3 * targetTransform.rotate;
				targetTransform.rotate = MathUtil::NormalizeQuat(targetTransform.rotate);
			} else {
				targetTransform.translate.x += x * 0.2f;
				targetTransform.translate.y += y * 0.2f;
				targetTransform.translate.z += z * 0.2f;
			}

			SetTransform(currentDelta->nodeIndex, currentDelta->frame, targetTransform);
			PushDataToGenerator(false, currentDelta->nodeIndex);
		}

		void EndIncrementalAdjust() {
			EndChangeDelta();
			EndHistoryAction();
			PushDataToGenerator(true);
		}
	};
}
