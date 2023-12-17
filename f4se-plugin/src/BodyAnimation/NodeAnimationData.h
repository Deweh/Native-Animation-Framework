#pragma once
#include "Misc/MathUtil.h"

namespace BodyAnimation
{
	using namespace Serialization::General;
	using RegisterFor3DChangeFunctor = std::function<void(SerializableRefHandle a_ref)>;

	enum EulerAngleOrder : uint8_t
	{
		XYZ,
		ZXY
	};

	struct NodeTransform
	{
		RE::NiQuaternion rotate;
		RE::NiPoint3 translate;

		NodeTransform()
		{
		}

		/*
		NodeTransform(ik_node_t* node) {
			translate.x = static_cast<float>(node->position.x);
			translate.y = static_cast<float>(node->position.y);
			translate.z = static_cast<float>(node->position.z);
			rotate.w = static_cast<float>(node->rotation.w);
			rotate.x = static_cast<float>(node->rotation.x);
			rotate.y = static_cast<float>(node->rotation.y);
			rotate.z = static_cast<float>(node->rotation.z);
		}
		*/

		NodeTransform(const RE::NiQuaternion& r, const RE::NiPoint3& t) :
			rotate(r), translate(t) {}

		NodeTransform(const RE::NiTransform& mTransform) {
			rotate.FromRotation(mTransform.rotate);
			translate = mTransform.translate;
		}

		NodeTransform(RE::NiPoint3 rotation, const RE::NiPoint3& translation, bool isDegrees = false)
		{
			if (isDegrees)
				rotation = MathUtil::DegreesToRadians(rotation);

			rotate.FromEulerAnglesXYZ(rotation.x, rotation.y, rotation.z);
			translate = translation;
		}

		inline static NodeTransform Identity() {
			return NodeTransform{ { 0, 0, 0, 0 }, { 0, 0, 0 } };
		}

		void MakeIdentity() {
			rotate.x = 0;
			rotate.y = 0;
			rotate.z = 0;
			rotate.w = 0;
			translate.x = 0;
			translate.y = 0;
			translate.z = 0;
		}

		bool IsIdentity() const {
			return rotate.x == 0 && rotate.y == 0 &&
			       rotate.z == 0 && rotate.w == 0 &&
			       translate.x == 0 && translate.y == 0 &&
			       translate.z == 0;
		}

		void RotationFromEulerAngles(float x, float y, float z, bool degrees = false, EulerAngleOrder order = EulerAngleOrder::XYZ) {
			if (degrees) {
				x = MathUtil::DegreeToRadian(x);
				y = MathUtil::DegreeToRadian(y);
				z = MathUtil::DegreeToRadian(z);
			}
			RE::NiMatrix3 tempMatrix;
			if (order == EulerAngleOrder::ZXY) {
				tempMatrix.FromEulerAnglesZXY(z, x, y);
			} else {
				tempMatrix.FromEulerAnglesXYZ(x, y, z);
			}
			
			rotate.x = 0;
			rotate.y = 0;
			rotate.z = 0;
			rotate.w = 0;
			rotate.FromRotation(tempMatrix);
		}

		RE::NiPoint3 RotationToEulerAngles(bool degrees = false, EulerAngleOrder order = EulerAngleOrder::XYZ) const {
			RE::NiMatrix3 tempMatrix;
			rotate.ToRotation(tempMatrix);
			RE::NiPoint3 result;
			if (order == EulerAngleOrder::ZXY) {
				tempMatrix.ToEulerAnglesZXY(result.z, result.x, result.y);
			} else {
				tempMatrix.ToEulerAnglesXYZ(result.x, result.y, result.z);
			}

			if (degrees) {
				result = MathUtil::RadiansToDegrees(result);
			}

			return result;
		}

		static void ShortestPathSlerp(RE::NiQuaternion& output, const RE::NiQuaternion& begin, const RE::NiQuaternion& end, float time) {
			RE::NiQuaternion adjustedEnd = end;
			float dotProduct = begin.x * end.x + begin.y * end.y + begin.z * end.z + begin.w * end.w;
			if (dotProduct < 0.0f) {
				adjustedEnd.x *= -1;
				adjustedEnd.y *= -1;
				adjustedEnd.z *= -1;
				adjustedEnd.w *= -1;
			}
			output.Slerp(time, begin, adjustedEnd);
		}

		void Lerp(const NodeTransform& begin, const NodeTransform& end, float time) {
			ShortestPathSlerp(rotate, begin.rotate, end.rotate, time);
			translate.x = std::lerp(begin.translate.x, end.translate.x, time);
			translate.y = std::lerp(begin.translate.y, end.translate.y, time);
			translate.z = std::lerp(begin.translate.z, end.translate.z, time);
		}

		void ToComplex(RE::NiTransform& trans) const {
			rotate.ToRotation(trans.rotate);
			trans.translate = translate;
		}

		template <class Archive>
		void serialize(Archive& ar, const uint32_t) {
			ar(rotate.x, rotate.y, rotate.z, rotate.w, translate);
		}
	};

	struct NodeKeyframe
	{
		NodeTransform value;
		
		template <class Archive>
		void serialize(Archive& ar, const uint32_t) {
			ar(value);
		}
	};

	struct NodeTimeline
	{
		std::map<float, NodeKeyframe> keys;
		std::map<float, NodeKeyframe>::iterator cachedNextIter;
		std::map<float, NodeKeyframe>::iterator cachedPrevIter;

		NodeTimeline()
		{
		}

		NodeTimeline(std::map<float, NodeKeyframe> _keys) : keys(_keys)
		{
		}

		void Init() {
			cachedNextIter = keys.begin();
			cachedPrevIter = keys.begin();
		}

		template <class Archive>
		void serialize(Archive& ar, const uint32_t)
		{
			ar(keys);
		}

		std::map<float, NodeKeyframe>::iterator GetNextIterClamped(const std::map<float, NodeKeyframe>::iterator& currentIter) {
			auto res = std::next(currentIter);
			if (res != keys.end()) {
				return res;
			} else {
				return currentIter;
			}
		}

		std::map<float, NodeKeyframe>::iterator NextOrLowerBound(float t) {
			auto prevKey = cachedNextIter;
			auto nextKey = GetNextIterClamped(cachedNextIter);

			if (nextKey->first < t || prevKey->first > t) {
				return keys.lower_bound(t);
			} else {
				return nextKey;
			}
		}

		void GetValueAtTime(float t, NodeTransform& transform)
		{
			if (keys.size() < 1) {
				transform.MakeIdentity();
				return;
			}

			if (cachedNextIter->first < t || cachedPrevIter->first > t) {
				auto nextKey = NextOrLowerBound(t);

				if (nextKey == keys.end()) {
					transform = std::prev(nextKey)->second.value;
					return;
				} else if (nextKey == keys.begin() || keys.size() < 2) {
					transform = nextKey->second.value;
					return;
				} else {
					cachedNextIter = nextKey;
					cachedPrevIter = std::prev(nextKey);
				}
			}

			if (t == cachedPrevIter->first) {
				transform = cachedPrevIter->second.value;
			} else if (t == cachedNextIter->first) {
				transform = cachedNextIter->second.value;
			} else {
				float normalizedTime = (t - cachedPrevIter->first) / (cachedNextIter->first - cachedPrevIter->first);
				transform.Lerp(cachedPrevIter->second.value, cachedNextIter->second.value, normalizedTime);
			}
		}
	};

	struct NodeAnimation
	{
		float duration = 0.001f;
		std::vector<NodeTimeline> timelines;

		template <class Archive>
		void serialize(Archive& ar, const uint32_t)
		{
			ar(duration, timelines);
		}
	};

	struct FrameBasedNodeTimeline
	{
		std::map<size_t, NodeKeyframe> keys;
	};

	struct FrameBasedNodeAnimation
	{
		struct ConversionResult
		{
			std::unique_ptr<FrameBasedNodeAnimation> data;
			bool dataLoss = false;
		};

		float sampleRate = 0.033333f;
		size_t duration = 2;
		std::vector<FrameBasedNodeTimeline> timelines;

		float GetRuntimeDuration() {
			return static_cast<float>(duration) * sampleRate;
		}

		std::unique_ptr<NodeAnimation> ToRuntime(bool properLoop = true) {
			std::unique_ptr<NodeAnimation> result = std::make_unique<NodeAnimation>();
			UpdateRuntime(result.get(), properLoop);
			return result;
		}

		void UpdateRuntime(NodeAnimation* target, bool properLoop = true) {
			if (target->timelines.size() < timelines.size())
				target->timelines.resize(timelines.size());
			target->duration = GetRuntimeDuration() - (properLoop ? sampleRate : 0.0f);

			for (size_t i = 0; i < timelines.size(); i++) {
				UpdateRuntimeSelective(i, target->timelines[i], true);
			}
		}

		void UpdateRuntimeSelective(size_t tlIndex, NodeTimeline& target, bool noCheck = false) {
			if (noCheck || tlIndex < timelines.size()) {
				target.keys.clear();
				auto& tl = timelines[tlIndex];
				for (const auto& k : tl.keys) {
					target.keys[static_cast<float>(k.first) * sampleRate] = k.second;
				}
				target.Init();
			}
		}

		std::unique_ptr<NodeAnimation> ToRuntimeSampled(
			const std::function<std::unique_ptr<MathUtil::InterpolationSystem<RE::NiQuaternion>>()>& rotInterpCreator,
			const std::function<std::unique_ptr<MathUtil::InterpolationSystem<RE::NiPoint3>>()>& posInterpCreator)
		{
			std::unique_ptr<NodeAnimation> result = ToRuntime();

			concurrency::parallel_for_each(result->timelines.begin(), result->timelines.end(), [&](NodeTimeline& tl) {
				size_t s = tl.keys.size();
				if (s < 2)
					return;

				float minT = static_cast<float>(tl.keys.begin()->first);
				float maxT = static_cast<float>(std::prev(tl.keys.end())->first);
				float t = 0;
				bool doSample = true;

				if (s > 2) {
					auto rotInterp = rotInterpCreator();
					auto posInterp = posInterpCreator();

					//If the timeline has at least 3 keys, and the first & last keys are
					//on the first and last frames, do loop smoothing.
					//The loop is smoothed by copying the first two keys after the end,
					//and the last two keys before the beginning, effectively shaping the
					//spline curve for a seamless loop.
					bool doLoopSmoothing =
						tl.keys.begin()->first < 0.001f &&
						std::fabs(std::prev(tl.keys.end())->first - result->duration) < 0.001f;

					std::vector<float> X;
					std::vector<RE::NiPoint3> Yp;
					std::vector<RE::NiQuaternion> Yr;
					X.reserve(s);
					Yp.reserve(s);
					Yr.reserve(s);

					size_t lastIdx = 0;
					auto addKeyData = [&](const NodeTransform& val, float t){
						X.push_back(t);
						Yp.push_back(val.translate);
						RE::NiQuaternion q = val.rotate;
						if (lastIdx > 0) {
							const auto& q1 = Yr[lastIdx - 1];
							const auto& q2 = q;
							float dot = q1.w * q2.w + q1.x * q2.x + q1.y * q2.y + q1.z * q2.z;
							if (dot < 0.0f) {
								q = -q;
							}
						}
						Yr.push_back(q);
						lastIdx++;
					};

					if (doLoopSmoothing) {
						auto secondToLast = std::prev(std::prev(tl.keys.end()));
						auto thirdToLast = std::prev(secondToLast);

						float timeDiff = 0 - (result->duration - thirdToLast->first);
						addKeyData(thirdToLast->second.value, timeDiff);
						timeDiff = 0 - (result->duration - secondToLast->first);
						addKeyData(secondToLast->second.value, timeDiff);
					}

					for (auto& k : tl.keys) {
						addKeyData(k.second.value, k.first);
					}

					if (doLoopSmoothing) {
						auto secondToFirst = std::next(tl.keys.begin());
						auto thirdToFirst = std::next(secondToFirst);

						float timeDiff = result->duration + secondToFirst->first;
						addKeyData(secondToFirst->second.value, timeDiff);
						timeDiff = result->duration + thirdToFirst->first;
						addKeyData(thirdToFirst->second.value, timeDiff);
					}

					posInterp->SetData(X, Yp);
					rotInterp->SetData(X, Yr);

					tl.keys.clear();
					while(doSample) {
						if (t >= result->duration) {
							t = result->duration;
							doSample = false;
						}
						auto& val = tl.keys[t].value;
						float clampedT = std::clamp(t, minT, maxT);
						val.translate = (*posInterp)(clampedT);
						val.rotate = (*rotInterp)(clampedT);

						t += sampleRate;
					}
				} else {
					//If the timeline only has 2 keys, fall back to normal cubic easing.
					auto first = *tl.keys.begin();
					auto second = *std::next(tl.keys.begin());
					tl.keys.clear();
					
					while (doSample) {
						if (t >= result->duration) {
							t = result->duration;
							doSample = false;
						}
						auto& val = tl.keys[t].value;

						if (t <= minT) {
							val = first.second.value;
						} else if (t >= maxT) {
							val = second.second.value;
						} else {
							float cubicT = static_cast<float>(Easing::easeInOutCubic(MathUtil::NormalizeTime(minT, maxT, t)));
							val.translate.x = std::lerp(first.second.value.translate.x, second.second.value.translate.x, cubicT);
							val.translate.y = std::lerp(first.second.value.translate.y, second.second.value.translate.y, cubicT);
							val.translate.z = std::lerp(first.second.value.translate.z, second.second.value.translate.z, cubicT);
							NodeTransform::ShortestPathSlerp(val.rotate, first.second.value.rotate, second.second.value.rotate, cubicT);
						}

						t += sampleRate;
					}
				}
			});

			return result;
		}

		inline static ConversionResult FromRuntime(const NodeAnimation* a_data, float a_sampleRate = 0.033333f) {
			ConversionResult result;
			result.data = std::make_unique<FrameBasedNodeAnimation>();
			result.data->timelines.resize(a_data->timelines.size());
			float frameRate = 1.0f / a_sampleRate;
			result.data->sampleRate = a_sampleRate;
			result.data->duration = static_cast<size_t>(std::round(a_data->duration * frameRate));
			size_t maxFrame = result.data->duration - 1;

			for (size_t i = 0; i < a_data->timelines.size(); i++) {
				auto& runtimeTimeline = a_data->timelines[i];
				auto& resultTimeline = result.data->timelines[i];
				for (const auto& k : runtimeTimeline.keys) {
					size_t targetFrame = static_cast<size_t>(std::round(k.first * frameRate));
					if (targetFrame > maxFrame)
						targetFrame = maxFrame;
					if (resultTimeline.keys.contains(targetFrame)) {
						result.dataLoss = true;
					} else {
						resultTimeline.keys[targetFrame] = k.second;
					}
				}
			}

			return result;
		}
	};
}
