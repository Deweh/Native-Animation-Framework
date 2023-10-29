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
			if (!paused)
				localTime += deltaTime;

			while (localTime >= animData->duration) {
				localTime -= animData->duration;
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

		void BakeToNANIM(const std::string& name, NANIM& container) {
			PushDataToGenerator(true);
			auto data = std::make_unique<NodeAnimation>();
			data->duration = animData->GetRuntimeDuration();
			data->timelines.resize(animData->timelines.size());
			size_t updateCount = animData->timelines.size() < nodeList->size() ? animData->timelines.size() : nodeList->size();

			for (float t = 0; t < data->duration; t += animData->sampleRate) {
				generator->localTime = t;
				generator->Update(0.0f);

				for (size_t i = 0; i < updateCount; i++) {
					if (nodeList->at(i) != nullptr && !generator->output[i].IsIdentity())
						generator->output[i].ToComplex(nodeList->at(i)->local);
				}

				ikManager->Update(generator->output, true);

				for (size_t i = 0; i < updateCount; i++) {
					if (nodeList->at(i) != nullptr)
						data->timelines[i].keys[t].value = nodeList->at(i)->local;
				}
			}
			
			container.SetAnimation(name, *nodeMap, data.get());
		}

		void SaveToNANIM(const std::string& name, NANIM& container) {
			auto data = animData->ToRuntime();
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
				generator->localTime = std::clamp(t, 0.00001f, generator->animData->duration - 0.001f);
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
				generator->SetAnimation(animData->ToRuntimeSplineSampled());
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

		NodeTransform GetCurrentWorldTransform(size_t nodeIndex)
		{
			auto& ikMappings = ikManager->GetLookupMap();
			if (auto iter = ikMappings.find(nodeIndex); iter != ikMappings.end()) {
				return ikManager->GetWorldTransform(iter->second);
			}
			if (nodeIndex < nodeList->size() && nodeList->at(nodeIndex) != nullptr) {
				return nodeList->at(nodeIndex)->world;
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

		void IncrementalAdjust(float x, float y, float z) {
			if (!currentDelta.has_value() || !FrameExists(currentDelta->nodeIndex, currentDelta->frame))
				return;

			NodeTransform targetTransform = GetTransform(currentDelta->nodeIndex, currentDelta->frame).value();

			if (adjustMode == kRotation) {
				EulerAngleOrder eulerOrder = y != 0 ? EulerAngleOrder::ZXY : EulerAngleOrder::XYZ;
				RE::NiPoint3 euler = targetTransform.RotationToEulerAngles(true, eulerOrder);
				euler.x += x * 0.5f;
				euler.y += y * 0.5f;
				euler.z += z * 0.5f;

				targetTransform.RotationFromEulerAngles(euler.x, euler.y, euler.z, true, eulerOrder);
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
