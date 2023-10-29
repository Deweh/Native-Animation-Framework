#pragma once

class MathUtil
{
public:
	inline static constexpr float DEGREES_PER_RADIAN{ SHORT_PI / 180.0f };
	inline static constexpr float RADIANS_PER_DEGREE{ 180.0f / SHORT_PI };
	inline static constexpr float MAX_RADIAN{ 2.0f * SHORT_PI };
	inline static constexpr float MAX_DEGREE{ 360.0f };

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

	static RE::NiQuaternion NormalizeQuat(const RE::NiQuaternion& q) {
		float magnitude = std::sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
		float scalar = 1.0f / magnitude;
		return { q.w * scalar, q.x * scalar, q.y * scalar, q.z * scalar };
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
};
