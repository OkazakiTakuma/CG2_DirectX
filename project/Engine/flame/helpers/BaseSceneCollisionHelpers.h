#pragma once

#include "../BaseScene.h"
#include "../../math/MathConstants.h"

#include <cmath>

/// <summary>
/// BaseSceneの衝突押し戻し計算を提供します。
/// </summary>
namespace BaseSceneCollisionHelpers {

inline float ClampFloat(float value, float minValue, float maxValue) {
	if (value < minValue) {
		return minValue;
	}
	if (value > maxValue) {
		return maxValue;
	}
	return value;
}

inline Vector3 NormalizeOrFallback(const Vector3& direction, const Vector3& fallback) {
	const float length = Length(direction);
	if (length <= MathConstants::kNormalizationEpsilon) {
		return Normalize(fallback);
	}
	return Normalize(direction);
}

inline float ProjectOBBRadius(const OBBColliderShape& obb, const Vector3& axis) {
	return
	    obb.halfSize.x * std::fabs(Dot(obb.orientation[0], axis)) +
	    obb.halfSize.y * std::fabs(Dot(obb.orientation[1], axis)) +
	    obb.halfSize.z * std::fabs(Dot(obb.orientation[2], axis));
}

inline Vector3 ClosestPointOnOBB(const OBBColliderShape& obb, const Vector3& point) {
	const Vector3 diff = point - obb.center;
	Vector3 result = obb.center;
	const float halfSizes[3] = {obb.halfSize.x, obb.halfSize.y, obb.halfSize.z};
	for (int axisIndex = 0; axisIndex < 3; ++axisIndex) {
		const float distance = ClampFloat(Dot(diff, obb.orientation[axisIndex]), -halfSizes[axisIndex], halfSizes[axisIndex]);
		result = result + distance * obb.orientation[axisIndex];
	}
	return result;
}

inline bool CalculateSphereSpherePushBack(const SphereColliderShape& a, const SphereColliderShape& b, Vector3& directionAToB, float& penetration) {
	const Vector3 diff = b.center - a.center;
	const float distance = Length(diff);
	const float radius = a.radius + b.radius;
	penetration = radius - distance;
	if (penetration <= 0.0f) {
		return false;
	}
	directionAToB = NormalizeOrFallback(diff, {1.0f, 0.0f, 0.0f});
	return true;
}

inline bool CalculateOBBSpherePushBack(const OBBColliderShape& obb, const SphereColliderShape& sphere, Vector3& directionOBBToSphere, float& penetration) {
	const Vector3 closestPoint = ClosestPointOnOBB(obb, sphere.center);
	const Vector3 diff = sphere.center - closestPoint;
	const float distance = Length(diff);
	if (distance > MathConstants::kNormalizationEpsilon) {
		penetration = sphere.radius - distance;
		if (penetration <= 0.0f) {
			return false;
		}
		directionOBBToSphere = Normalize(diff);
		return true;
	}

	const Vector3 localDiff = sphere.center - obb.center;
	float localPosition[3] = {
	    Dot(localDiff, obb.orientation[0]),
	    Dot(localDiff, obb.orientation[1]),
	    Dot(localDiff, obb.orientation[2])
	};
	const float halfSizes[3] = {obb.halfSize.x, obb.halfSize.y, obb.halfSize.z};
	int nearestAxis = 0;
	float nearestFaceDistance = halfSizes[0] - std::fabs(localPosition[0]);
	for (int axisIndex = 1; axisIndex < 3; ++axisIndex) {
		const float faceDistance = halfSizes[axisIndex] - std::fabs(localPosition[axisIndex]);
		if (faceDistance < nearestFaceDistance) {
			nearestFaceDistance = faceDistance;
			nearestAxis = axisIndex;
		}
	}

	const float sign = localPosition[nearestAxis] < 0.0f ? -1.0f : 1.0f;
	directionOBBToSphere = sign * obb.orientation[nearestAxis];
	penetration = sphere.radius + nearestFaceDistance;
	return penetration > 0.0f;
}

inline bool CalculateOBBOBBPushBack(const OBBColliderShape& a, const OBBColliderShape& b, Vector3& directionAToB, float& penetration) {
	Vector3 axes[15] = {
	    a.orientation[0], a.orientation[1], a.orientation[2],
	    b.orientation[0], b.orientation[1], b.orientation[2],
	    Cross(a.orientation[0], b.orientation[0]), Cross(a.orientation[0], b.orientation[1]), Cross(a.orientation[0], b.orientation[2]),
	    Cross(a.orientation[1], b.orientation[0]), Cross(a.orientation[1], b.orientation[1]), Cross(a.orientation[1], b.orientation[2]),
	    Cross(a.orientation[2], b.orientation[0]), Cross(a.orientation[2], b.orientation[1]), Cross(a.orientation[2], b.orientation[2])
	};

	const Vector3 centerDiff = b.center - a.center;
	bool foundAxis = false;
	float minimumOverlap = 0.0f;
	Vector3 minimumAxis{1.0f, 0.0f, 0.0f};
	for (const Vector3& rawAxis : axes) {
		if (Length(rawAxis) <= MathConstants::kNormalizationEpsilon) {
			continue;
		}
		Vector3 axis = Normalize(rawAxis);
		const float signedDistance = Dot(centerDiff, axis);
		const float overlap = ProjectOBBRadius(a, axis) + ProjectOBBRadius(b, axis) - std::fabs(signedDistance);
		if (overlap <= 0.0f) {
			return false;
		}
		if (!foundAxis || overlap < minimumOverlap) {
			foundAxis = true;
			minimumOverlap = overlap;
			minimumAxis = signedDistance < 0.0f ? -1.0f * axis : axis;
		}
	}

	if (!foundAxis) {
		return false;
	}
	directionAToB = NormalizeOrFallback(minimumAxis, centerDiff);
	penetration = minimumOverlap;
	return true;
}

inline void ApplyColliderPushBack(GameObject* objectA, bool isPushBackA, GameObject* objectB, bool isPushBackB, const Vector3& directionAToB, float penetration) {
	if (!objectA || !objectB || objectA == objectB || penetration <= 0.0f || (!isPushBackA && !isPushBackB)) {
		return;
	}

	if (isPushBackA && isPushBackB) {
		objectA->GetTransform().translate = objectA->GetTransform().translate + (-0.5f * penetration) * directionAToB;
		objectB->GetTransform().translate = objectB->GetTransform().translate + (0.5f * penetration) * directionAToB;
		objectA->ApplyCollisionResponse(-1.0f * directionAToB);
		objectB->ApplyCollisionResponse(directionAToB);
		return;
	}
	if (isPushBackA) {
		objectA->GetTransform().translate = objectA->GetTransform().translate + (-penetration) * directionAToB;
		objectA->ApplyCollisionResponse(-1.0f * directionAToB);
		return;
	}
	objectB->GetTransform().translate = objectB->GetTransform().translate + penetration * directionAToB;
	objectB->ApplyCollisionResponse(directionAToB);
}

} // namespace BaseSceneCollisionHelpers
