#pragma once
#include "Misc/MathUtil.h"
#include "QuatSpline.h"

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

		void ShortestPathSlerp(RE::NiQuaternion& output, const RE::NiQuaternion& begin, const RE::NiQuaternion& end, float time) {
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

		struct QuatSplineSegment
		{
			float startTime;
			float endTime;
			QuatSpline::quat q1;
			QuatSpline::quat q2;
			QuatSpline::vec3 v1;
			QuatSpline::vec3 v2;
		};

		float sampleRate = 0.033333f;
		size_t duration = 2;
		std::vector<FrameBasedNodeTimeline> timelines;

		float GetRuntimeDuration() {
			return static_cast<float>(duration) * sampleRate;
		}

		std::unique_ptr<NodeAnimation> ToRuntime() {
			std::unique_ptr<NodeAnimation> result = std::make_unique<NodeAnimation>();
			UpdateRuntime(result.get());
			return result;
		}

		void UpdateRuntime(NodeAnimation* target) {
			if (target->timelines.size() < timelines.size())
				target->timelines.resize(timelines.size());
			target->duration = GetRuntimeDuration();

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

		//Timeline must have at least 2 keys.
		inline static std::vector<QuatSplineSegment> CalculateAngularVelocities(const std::vector<std::pair<float, RE::NiQuaternion>>& XY)
		{
			std::vector<QuatSplineSegment> result;
			QuatSpline::quat lastRot = reinterpret_cast<const QuatSpline::quat&>(XY.begin()->second);
			QuatSpline::quat nextRot;
			QuatSpline::vec3 v1;
			QuatSpline::vec3 v2;

			bool first = true;
			bool last = false;

			for (auto iter = ++XY.begin(); iter != XY.end(); iter++) {
				auto prevKey = std::prev(iter);

				const QuatSpline::quat& end = reinterpret_cast<const QuatSpline::quat&>(iter->second);
				const QuatSpline::quat& begin = reinterpret_cast<const QuatSpline::quat&>(prevKey->second);
				nextRot = end;

				if (auto nxt = std::next(iter); nxt != XY.end()) {
					nextRot = reinterpret_cast<const QuatSpline::quat&>(nxt->second);
				} else {
					last = true;
				}

				QuatSpline::quat_catmull_rom_velocity(v1, v2, lastRot, begin, end, nextRot);

				result.emplace_back(
					prevKey->first,
					iter->first,
					begin,
					end,
					first ? QuatSpline::vec3{ 0, 0, 0} : v1,
					last ? QuatSpline::vec3{ 0, 0, 0} : v2);

				lastRot = begin;
				first = false;
			}
			return result;
		}

		std::unique_ptr<NodeAnimation> ToRuntimeSplineSampled() {
			std::unique_ptr<NodeAnimation> result = ToRuntime();

			concurrency::parallel_for_each(result->timelines.begin(), result->timelines.end(), [&](NodeTimeline& tl) {
				size_t s = tl.keys.size();
				
				std::vector<double> X, Yx, Yy, Yz;
				std::vector<std::pair<float, RE::NiQuaternion>> Yr;
				X.reserve(s);
				Yx.reserve(s);
				Yy.reserve(s);
				Yz.reserve(s);
				Yr.reserve(s);

				if (tl.keys.size() > 1) {
					for (auto& k : tl.keys) {
						auto& val = k.second.value;
						X.push_back(k.first);
						Yx.push_back(val.translate.x);
						Yy.push_back(val.translate.y);
						Yz.push_back(val.translate.z);
						Yr.emplace_back(k.first, val.rotate);
					}

					//Set velocity to 0 at start and end of the spline.
					tk::spline translateX(X, Yx, tk::spline::cspline_hermite,
						false,
						tk::spline::first_deriv,
						0.0,
						tk::spline::first_deriv,
						0.0);
					tk::spline translateY(X, Yy, tk::spline::cspline_hermite,
						false,
						tk::spline::first_deriv,
						0.0,
						tk::spline::first_deriv,
						0.0);
					tk::spline translateZ(X, Yz, tk::spline::cspline_hermite,
						false,
						tk::spline::first_deriv,
						0.0,
						tk::spline::first_deriv,
						0.0);
					std::vector<QuatSplineSegment> vels = CalculateAngularVelocities(Yr);

					tl.keys.clear();
					size_t curIndex = 0;
					float minT = static_cast<float>(X.front());
					float maxT = static_cast<float>(X.back());
					for (float t = 0; t < result->duration; t += sampleRate) {
						auto& val = tl.keys[t].value;
						float clampedT = std::clamp(t, minT, maxT);
						val.translate.x = static_cast<float>(translateX(clampedT));
						val.translate.y = static_cast<float>(translateY(clampedT));
						val.translate.z = static_cast<float>(translateZ(clampedT));
						
						if (std::fabs(clampedT - minT) < 0.001f) {
							val.rotate = Yr.front().second;
						} else if (std::fabs(clampedT - maxT) < 0.001f) {
							val.rotate = Yr.back().second;
						} else {
							if (clampedT > vels[curIndex].endTime && (curIndex + 1) < vels.size())
								curIndex += 1;

							auto& v = vels[curIndex];

							float normalizedTime = (clampedT - v.startTime) / (v.endTime - v.startTime);
							QuatSpline::quat_hermite(reinterpret_cast<QuatSpline::quat&>(val.rotate), normalizedTime, v.q1, v.q2, v.v1, v.v2);
						}
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

			for (size_t i = 0; i < a_data->timelines.size(); i++) {
				auto& runtimeTimeline = a_data->timelines[i];
				auto& resultTimeline = result.data->timelines[i];
				for (const auto& k : runtimeTimeline.keys) {
					size_t targetFrame = static_cast<size_t>(std::round(k.first * frameRate));
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
