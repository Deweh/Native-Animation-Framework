#pragma once
#include "Misc/dh.h"
#include "Misc/ysp.h"

class MathUtil
{
public:
	inline static constexpr float DEGREES_PER_RADIAN{ SHORT_PI / 180.0f };
	inline static constexpr float RADIANS_PER_DEGREE{ 180.0f / SHORT_PI };
	inline static constexpr float MAX_RADIAN{ 2.0f * SHORT_PI };
	inline static constexpr float MAX_DEGREE{ 360.0f };

	struct DirectionVectors
	{
		RE::NiPoint3 x;
		RE::NiPoint3 y;
		RE::NiPoint3 z;

		void transform(const std::function<void(RE::NiPoint3&)> f) {
			f(x);
			f(y);
			f(z);
		}
	};

	static RE::NiTransform CalculateWorldAscending(RE::NiNode* rootNode, const RE::NiAVObject* target, const RE::NiTransform* rootTransformOverride = nullptr)
	{
		if (rootNode == nullptr) {
			return target->world;
		}

		RE::NiTransform result = target->local;
		const RE::NiNode* curNode = target->parent;

		while (curNode != rootNode && curNode != nullptr) {
			result = ApplyCoordinateSpace(curNode->local, result);
			curNode = curNode->parent;
		}

		if (curNode == rootNode) {
			result = ApplyCoordinateSpace(rootTransformOverride != nullptr ? *rootTransformOverride : rootNode->world, result);
		}

		return result;
	}

	static RE::NiTransform CalculateWorldDescending(RE::NiNode* rootNode, const RE::NiAVObject* target, const RE::NiTransform* rootTransformOverride = nullptr) {
		if (rootNode == nullptr) {
			return target->world;
		}

		//NiTransform is the current node's parent's calculated global transform.
		std::stack<std::pair<RE::NiNode*, RE::NiTransform>> pendingNodes;

		for (auto& c : rootNode->children) {
			if (auto n = c->IsNode(); n != nullptr) {
				pendingNodes.push({ n, rootTransformOverride != nullptr ? *rootTransformOverride : rootNode->world });
			}
		}

		while (!pendingNodes.empty()) {
			std::pair<RE::NiNode*, RE::NiTransform> current = pendingNodes.top();
			pendingNodes.pop();

			RE::NiTransform result;
			if (!current.first->children.empty() || current.first == target) {
				result = ApplyCoordinateSpace(current.second, current.first->local);
			}

			if (current.first == target) {
				return result;
			}

			for (auto c : current.first->children) {
				if (auto n = c->IsNode(); n != nullptr) {
					pendingNodes.push({ n, result });
				}
			}
		}

		return target->world;
	}

	static RE::NiTransform ApplyCoordinateSpace(const RE::NiTransform& parentSpace, const RE::NiTransform& childSpace) {
		RE::NiTransform result;
		parentSpace.Multiply(result, childSpace);
		return result;
	}

	static DirectionVectors QuatToDirVectors(const RE::NiQuaternion& q) {
		DirectionVectors result;
		RE::NiMatrix3 mat;
		q.ToRotation(mat);

		result.x = NormalizePt3({ mat.entry[0].pt[0], mat.entry[0].pt[1], mat.entry[0].pt[2] });
		result.y = NormalizePt3({ mat.entry[1].pt[0], mat.entry[1].pt[1], mat.entry[1].pt[2] });
		result.z = NormalizePt3({ mat.entry[2].pt[0], mat.entry[2].pt[1], mat.entry[2].pt[2] });

		return result;
	}

	//Faster than 1.0f/std::sqrt(x), but only accurate to around 11 bits.
	static float FastReverseSqrt(const float x) {
		__m128 temp = _mm_set_ss(x);
		temp = _mm_rsqrt_ss(temp);
		return _mm_cvtss_f32(temp);
	}

	static RE::NiQuaternion QuatSquad(const RE::NiQuaternion& q0, const RE::NiQuaternion& q1, const RE::NiQuaternion& q2, const RE::NiQuaternion& q3, float t)
	{
		RE::NiQuaternion s1;
		RE::NiQuaternion s2;
		RE::NiQuaternion result;

		s1.Slerp(t, q0, q3);
		s2.Slerp(t, q1, q2);
		result.Slerp(2.0f * t * (1.0f - t), s1, s2);
		return result;
	}

	static RE::NiQuaternion NormalizeQuat(const RE::NiQuaternion& q) {
		auto rsqrt = FastReverseSqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
		return { q.w * rsqrt, q.x * rsqrt, q.y * rsqrt, q.z * rsqrt };
	}

	static RE::NiPoint3 NormalizePt3(const RE::NiPoint3& p) {
		auto rsqrt = FastReverseSqrt(p.x * p.x + p.y * p.y + p.z * p.z);
		return { p.x * rsqrt, p.y * rsqrt, p.z * rsqrt };
	}

	static float NormalizeTime(float begin, float end, float time) {
		return (time - begin) / (end - begin);
	}

	static bool CoordsWithinError(const RE::NiPoint3A& a, const RE::NiPoint3& b) {
		return (
			fabs(a.x - b.x) < 0.00001f &&
			fabs(a.y - b.y) < 0.00001f &&
			fabs(a.z - b.z) < 0.00001f
		);
	}

	static float RadianToDegree(float a_radian)
	{
		return a_radian * RADIANS_PER_DEGREE;
	}

	static float DegreeToRadian(float a_degree)
	{
		return a_degree * DEGREES_PER_RADIAN;
	}

	static float Deg2RadCon(float a_degree) {
		float result = DegreeToRadian(a_degree);
		ConstrainRadian(result);
		return result;
	}

	static RE::NiPoint3 DegreesToRadians(const RE::NiPoint3& a_degrees)
	{
		return { DegreeToRadian(a_degrees.x), DegreeToRadian(a_degrees.y), DegreeToRadian(a_degrees.z) };
	}

	static RE::NiPoint3 RadiansToDegrees(const RE::NiPoint3& a_radians)
	{
		return { RadianToDegree(a_radians.x), RadianToDegree(a_radians.y), RadianToDegree(a_radians.z) };
	}

	static void ConstrainRadians(RE::NiPoint3& a_radians) {
		ConstrainRadian(a_radians.x);
		ConstrainRadian(a_radians.y);
		ConstrainRadian(a_radians.z);
	}

	static void ConstrainRadian(float& a_radian) {
		a_radian = fmod(a_radian, MAX_RADIAN);
		if (a_radian < 0)
			a_radian += MAX_RADIAN;
	}

	static float FlipRadian(float a_radian) {
		return a_radian > SHORT_PI ? a_radian - SHORT_PI : a_radian + SHORT_PI;
	}

	//Heading must be in radians.
	static void ApplyOffsetToLocalSpace(RE::NiPoint3& location, const RE::NiPoint3& offset, float heading)
	{
		float cosHeading = cos(heading);
		float sinHeading = -sin(heading);
		float flippedX = -offset.x;
		location.x += flippedX * cosHeading - offset.y * sinHeading;
		location.y += flippedX * sinHeading + offset.y * cosHeading;
		location.z += offset.z;
	}

	static RE::NiPoint2 GetLookAtRotation(const RE::NiPoint3& a_cameraPos, const RE::NiPoint3& a_objectPos)
	{
		RE::NiPoint3 dirVector;
		dirVector.x = a_objectPos.x - a_cameraPos.x;
		dirVector.y = a_objectPos.y - a_cameraPos.y;
		dirVector.z = a_objectPos.z - a_cameraPos.z;
		double distance = sqrt((dirVector.x * dirVector.x) + (dirVector.y * dirVector.y));

		float yaw = atan2(dirVector.x, dirVector.y);
		float pitch = (static_cast<float>(atan2(dirVector.z, distance))) * -1;
		ConstrainRadian(yaw);
		ConstrainRadian(pitch);

		return { yaw, pitch };
	}

	template <typename T>
	class InterpolationSystem
	{
	public:
		virtual void SetData(const std::vector<float>&, const std::vector<T>&)
		{
		}

		virtual T operator()(float)
		{
			return {};
		}

		virtual ~InterpolationSystem() {}
	};

	class Pt3Linear : public InterpolationSystem<RE::NiPoint3>
	{
	public:
		virtual ~Pt3Linear() {}

		const std::vector<float>* _X = nullptr;
		const std::vector<RE::NiPoint3>* _Y = nullptr;

		virtual void SetData(const std::vector<float>& X, const std::vector<RE::NiPoint3>& Y)
		{
			if (Y.size() < 2 || Y.size() != X.size())
				return;

			_X = &X;
			_Y = &Y;
		}

		virtual RE::NiPoint3 operator()(float t)
		{
			if (!_X || !_Y)
				return {};

			auto& X = *_X;
			auto& Y = *_Y;

			size_t idx = UINT64_MAX;

			for (size_t i = 0; i < X.size(); i++) {
				if (X[i] >= t) {
					idx = i;
					break;
				}
			}

			if (idx == UINT64_MAX) {
				return Y.back();
			} else if (idx == 0) {
				return Y.front();
			} else {
				float normalizedT = NormalizeTime(X[idx - 1], X[idx], t);
				const auto& first = Y[idx - 1];
				const auto& second = Y[idx];
				return {
					std::lerp(first.x, second.x, normalizedT),
					std::lerp(first.y, second.y, normalizedT),
					std::lerp(first.z, second.z, normalizedT),
				};
			}
		}
	};

	class Pt3NaturalCubicSpline : public InterpolationSystem<RE::NiPoint3>
	{
	public:
		virtual ~Pt3NaturalCubicSpline() {}

		std::unique_ptr<tk::spline> implX = nullptr;
		std::unique_ptr<tk::spline> implY = nullptr;
		std::unique_ptr<tk::spline> implZ = nullptr;

		virtual void SetData(const std::vector<float>& X, const std::vector<RE::NiPoint3>& Y)
		{
			if (Y.size() < 2 || Y.size() != X.size())
				return;

			std::vector<double> Yx, Yy, Yz, Xd;
			Yx.reserve(Y.size());
			Yy.reserve(Y.size());
			Yz.reserve(Y.size());
			Xd.reserve(Y.size());

			for (auto& p : Y) {
				Yx.push_back(p.x);
				Yy.push_back(p.y);
				Yz.push_back(p.z);
			}

			for (auto& i : X) {
				Xd.push_back(i);
			}

			implX = std::make_unique<tk::spline>(Xd, Yx, tk::spline::cspline_hermite,
				false,
				tk::spline::first_deriv,
				0.0,
				tk::spline::first_deriv,
				0.0);
			implY = std::make_unique<tk::spline>(Xd, Yy, tk::spline::cspline_hermite,
				false,
				tk::spline::first_deriv,
				0.0,
				tk::spline::first_deriv,
				0.0);
			implZ = std::make_unique<tk::spline>(Xd, Yz, tk::spline::cspline_hermite,
				false,
				tk::spline::first_deriv,
				0.0,
				tk::spline::first_deriv,
				0.0);
		}

		virtual RE::NiPoint3 operator()(float t)
		{
			if (!implX || !implY || !implZ)
				return {};

			auto& X = *implX;
			auto& Y = *implY;
			auto& Z = *implZ;

			return {
				static_cast<float>(X(t)),
				static_cast<float>(Y(t)),
				static_cast<float>(Z(t))
			};
		}
	};

	class QuatLinear : public InterpolationSystem<RE::NiQuaternion>
	{
	public:
		virtual ~QuatLinear() {}

		const std::vector<float>* _X = nullptr;
		const std::vector<RE::NiQuaternion>* _Y = nullptr;

		virtual void SetData(const std::vector<float>& X, const std::vector<RE::NiQuaternion>& Y)
		{
			if (Y.size() < 2 || Y.size() != X.size())
				return;

			_X = &X;
			_Y = &Y;
		}

		virtual RE::NiQuaternion operator()(float t)
		{
			if (!_X || !_Y)
				return {};

			auto& X = *_X;
			auto& Y = *_Y;

			size_t idx = UINT64_MAX;
			
			for (size_t i = 0; i < X.size(); i++) {
				if (X[i] >= t) {
					idx = i;
					break;
				}
			}
			
			if (idx == UINT64_MAX) {
				return Y.back();
			} else if (idx == 0) {
				return Y.front();
			} else {
				float normalizedT = NormalizeTime(X[idx - 1], X[idx], t);
				RE::NiQuaternion result;
				result.Slerp(normalizedT, Y[idx - 1], Y[idx]);
				return result;
			}
		}
	};

	class QuatSquadSpline : public InterpolationSystem<RE::NiQuaternion>
	{
	private:
		RE::NiQuaternion CalculateTangent(
			const RE::NiQuaternion& q0,
			const RE::NiQuaternion& q1,
			const RE::NiQuaternion& q2)
		{
			RE::NiQuaternion result;
			result.Intermediate(q0, q1, q2);
			return result;
		}

	public:
		virtual ~QuatSquadSpline() {}

		struct Segment
		{
			float startTime;
			float endTime;
			RE::NiQuaternion q0;
			RE::NiQuaternion q1;
			RE::NiQuaternion t0;
			RE::NiQuaternion t1;
		};

		std::vector<Segment> segs;

		virtual void SetData(const std::vector<float>& X, const std::vector<RE::NiQuaternion>& Y)
		{
			segs.clear();

			for (size_t i = 1; i < Y.size(); i++) {
				//auto prevX = X[i - (i > 1 ? 2 : 1)];
				auto& prevY = Y[i - (i > 1 ? 2 : 1)];
				auto startX = X[i - 1];
				auto& startY = Y[i - 1];
				auto endX = X[i];
				auto& endY = Y[i];
				//auto nextX = X[i + ((i + 1) < Y.size() ? 1 : 0)];
				auto& nextY = Y[i + ((i + 1) < Y.size() ? 1 : 0)];

				segs.push_back({
					startX,
					endX,
					startY,
					endY,
					CalculateTangent(prevY, startY, endY),
					CalculateTangent(startY, endY, nextY)});
			}
		}

		virtual RE::NiQuaternion operator()(float t)
		{
			if (segs.empty())
				return {};

			Segment curSeg;

			if (t < segs.front().startTime) {
				curSeg = segs.front();
				t = curSeg.startTime;
			} else if (t >= segs.back().endTime) {
				curSeg = segs.back();
				t = curSeg.endTime;
			} else {
				for (auto& s : segs) {
					if (s.startTime <= t && s.endTime > t) {
						curSeg = s;
						break;
					}
				}
			}

			float normalizedT = NormalizeTime(curSeg.startTime, curSeg.endTime, t);
			return QuatSquad(curSeg.q0, curSeg.t0, curSeg.t1, curSeg.q1, normalizedT);
		}
	};

	class QuatCatmullRomSpline : public InterpolationSystem<RE::NiQuaternion>
	{
	public:
		virtual ~QuatCatmullRomSpline() {}

		struct Segment
		{
			float startTime;
			float endTime;
			dh::quat q1;
			dh::quat q2;
			dh::vec3 v1;
			dh::vec3 v2;
		};

		std::vector<Segment> segs;

		virtual void SetData(const std::vector<float>& X, const std::vector<RE::NiQuaternion>& Y)
		{
			segs.clear();

			if (Y.size() < 2 || Y.size() != X.size())
				return;

			dh::vec3 v1;
			dh::vec3 v2;

			for (size_t i = 1; i < X.size(); i++) {
				auto& prevY = reinterpret_cast<const dh::quat&>(Y[i - (i > 1 ? 2 : 1)]);
				auto startX = X[i - 1];
				auto& startY = reinterpret_cast<const dh::quat&>(Y[i - 1]);
				auto endX = X[i];
				auto& endY = reinterpret_cast<const dh::quat&>(Y[i]);
				auto& nextY = reinterpret_cast<const dh::quat&>(Y[i + ((i + 1) < Y.size() ? 1 : 0)]);

				dh::quat_catmull_rom_velocity(v1, v2, prevY, startY, endY, nextY);

				segs.emplace_back(
					startX,
					endX,
					startY,
					endY,
					v1,
					v2);
			}
		}

		virtual RE::NiQuaternion operator()(float t)
		{
			if (segs.empty())
				return {};

			Segment curSeg;

			if (t < segs.front().startTime) {
				curSeg = segs.front();
				t = curSeg.startTime;
			} else if (t >= segs.back().endTime) {
				curSeg = segs.back();
				t = curSeg.endTime;
			} else {
				for (auto& s : segs) {
					if (s.startTime <= t && s.endTime > t) {
						curSeg = s;
						break;
					}
				}
			}

			float normalizedT = NormalizeTime(curSeg.startTime, curSeg.endTime, t);
			RE::NiQuaternion result;
			dh::quat_hermite(reinterpret_cast<dh::quat&>(result), normalizedT, curSeg.q1, curSeg.q2, curSeg.v1, curSeg.v2);
			return result;
		}
	};

	class QuatNaturalCubicSpline : public InterpolationSystem<RE::NiQuaternion>
	{
	public:
		virtual ~QuatNaturalCubicSpline() {}

		std::unique_ptr<ysp::quaternion_spline_curve<float>> impl = nullptr;

		virtual void SetData(const std::vector<float>& X, const std::vector<RE::NiQuaternion>& Y)
		{
			if (Y.size() < 2 || Y.size() != X.size())
				return;

			std::vector<std::pair<float, ysp::quaternion<float>>> combined;
			combined.reserve(X.size());

			for (size_t i = 0; i < X.size(); i++) {
				combined.emplace_back(X[i], ysp::quaternion<float>(Y[i].w, Y[i].x, Y[i].y, Y[i].z));
			}

			impl = std::make_unique<ysp::quaternion_spline_curve<float>>(combined.begin(), combined.end(), 1e-8f);
		}

		virtual RE::NiQuaternion operator()(float t)
		{
			if (!impl)
				return {};

			auto res = (*impl)(t);
			RE::NiQuaternion resQ{
				res.R_component_1(),
				res.R_component_2(),
				res.R_component_3(),
				res.R_component_4()
			};

			return NormalizeQuat(resQ);
		}
	};
};
