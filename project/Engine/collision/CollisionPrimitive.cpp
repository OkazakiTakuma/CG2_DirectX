#include "CollisionPrimitive.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace {
constexpr float kEpsilon = 0.00001f;

float AbsDot(const Vector3& a, const Vector3& b) {
	return std::fabs(Dot(a, b));
}

Vector3 GetPointOnLine(const Vector3& origin, const Vector3& diff, float t) {
	return origin + t * diff;
}

bool IsPointInsideTriangle(const Vector3& point, const TriangleColliderShape& triangle, const Vector3& normal) {
	for (int i = 0; i < 3; ++i) {
		const Vector3& a = triangle.vertices[i];
		const Vector3& b = triangle.vertices[(i + 1) % 3];
		const Vector3 edge = b - a;
		const Vector3 toPoint = point - a;
		if (Dot(Cross(edge, toPoint), normal) < -kEpsilon) {
			return false;
		}
	}
	return true;
}

/// <summary>
/// AABBWithParamRange の交差判定を行います。
/// </summary>
bool IntersectAABBWithParamRange(const AABBColliderShape& aabb, const Vector3& origin, const Vector3& diff, float minT, float maxT) {
	float tMin = minT;
	float tMax = maxT;
	const float originValues[3] = {origin.x, origin.y, origin.z};
	const float diffValues[3] = {diff.x, diff.y, diff.z};
	const float minValues[3] = {aabb.min.x, aabb.min.y, aabb.min.z};
	const float maxValues[3] = {aabb.max.x, aabb.max.y, aabb.max.z};

	for (int axis = 0; axis < 3; ++axis) {
		if (std::fabs(diffValues[axis]) < kEpsilon) {
			if (originValues[axis] < minValues[axis] || originValues[axis] > maxValues[axis]) {
				return false;
			}
			continue;
		}

		float t1 = (minValues[axis] - originValues[axis]) / diffValues[axis];
		float t2 = (maxValues[axis] - originValues[axis]) / diffValues[axis];
		if (t1 > t2) {
			std::swap(t1, t2);
		}
		tMin = std::max(tMin, t1);
		tMax = std::min(tMax, t2);
		if (tMin > tMax) {
			return false;
		}
	}

	return true;
}

Vector3 ToOBBLocalPoint(const OBBColliderShape& obb, const Vector3& point) {
	const Vector3 diff = point - obb.center;
	return {
	    Dot(diff, obb.orientation[0]),
	    Dot(diff, obb.orientation[1]),
	    Dot(diff, obb.orientation[2])
	};
}

bool IsSeparatedOnAxis(const OBBColliderShape& a, const OBBColliderShape& b, const Vector3& axis) {
	const float axisLength = Length(axis);
	if (axisLength <= kEpsilon) {
		return false;
	}

	const Vector3 normalizedAxis = Normalize(axis);
	const Vector3 centerDiff = b.center - a.center;
	const float distance = std::fabs(Dot(centerDiff, normalizedAxis));
	const float radiusA =
	    a.halfSize.x * AbsDot(a.orientation[0], normalizedAxis) +
	    a.halfSize.y * AbsDot(a.orientation[1], normalizedAxis) +
	    a.halfSize.z * AbsDot(a.orientation[2], normalizedAxis);
	const float radiusB =
	    b.halfSize.x * AbsDot(b.orientation[0], normalizedAxis) +
	    b.halfSize.y * AbsDot(b.orientation[1], normalizedAxis) +
	    b.halfSize.z * AbsDot(b.orientation[2], normalizedAxis);

	return distance > radiusA + radiusB;
}
}

/// <summary>
/// SphereToSphere の当たり判定を行います。
/// </summary>
bool IsCollisionSphereToSphere(const SphereColliderShape& a, const SphereColliderShape& b) {
	const float radius = a.radius + b.radius;
	const Vector3 diff = b.center - a.center;
	return Dot(diff, diff) <= radius * radius;
}

/// <summary>
/// SphereToPlane の当たり判定を行います。
/// </summary>
bool IsCollisionSphereToPlane(const SphereColliderShape& sphere, const PlaneColliderShape& plane) {
	const Vector3 normal = Normalize(plane.normal);
	const float distance = std::fabs(Dot(sphere.center, normal) - plane.distance);
	return distance <= sphere.radius;
}

/// <summary>
/// SegmentToPlane の当たり判定を行います。
/// </summary>
bool IsCollisionSegmentToPlane(const SegmentColliderShape& segment, const PlaneColliderShape& plane) {
	const Vector3 normal = Normalize(plane.normal);
	const float denominator = Dot(normal, segment.diff);
	if (std::fabs(denominator) < kEpsilon) {
		return false;
	}
	const float t = (plane.distance - Dot(segment.origin, normal)) / denominator;
	return t >= 0.0f && t <= 1.0f;
}

/// <summary>
/// LineToPlane の当たり判定を行います。
/// </summary>
bool IsCollisionLineToPlane(const LineColliderShape& line, const PlaneColliderShape& plane) {
	const Vector3 normal = Normalize(plane.normal);
	return std::fabs(Dot(normal, line.diff)) >= kEpsilon;
}

/// <summary>
/// RayToPlane の当たり判定を行います。
/// </summary>
bool IsCollisionRayToPlane(const RayColliderShape& ray, const PlaneColliderShape& plane) {
	const Vector3 normal = Normalize(plane.normal);
	const float denominator = Dot(normal, ray.diff);
	if (std::fabs(denominator) < kEpsilon) {
		return false;
	}
	const float t = (plane.distance - Dot(ray.origin, normal)) / denominator;
	return t >= 0.0f;
}

/// <summary>
/// TriangleToSegment の当たり判定を行います。
/// </summary>
bool IsCollisionTriangleToSegment(const TriangleColliderShape& triangle, const SegmentColliderShape& segment) {
	const Vector3 edge01 = triangle.vertices[1] - triangle.vertices[0];
	const Vector3 edge02 = triangle.vertices[2] - triangle.vertices[0];
	const Vector3 normal = Normalize(Cross(edge01, edge02));
	const float denominator = Dot(normal, segment.diff);
	if (std::fabs(denominator) < kEpsilon) {
		return false;
	}

	const float distance = Dot(normal, triangle.vertices[0]);
	const float t = (distance - Dot(normal, segment.origin)) / denominator;
	if (t < 0.0f || t > 1.0f) {
		return false;
	}

	const Vector3 point = GetPointOnLine(segment.origin, segment.diff, t);
	return IsPointInsideTriangle(point, triangle, normal);
}

/// <summary>
/// AABBToAABB の当たり判定を行います。
/// </summary>
bool IsCollisionAABBToAABB(const AABBColliderShape& a, const AABBColliderShape& b) {
	return a.min.x <= b.max.x && a.max.x >= b.min.x &&
	       a.min.y <= b.max.y && a.max.y >= b.min.y &&
	       a.min.z <= b.max.z && a.max.z >= b.min.z;
}

/// <summary>
/// SphereToAABB の当たり判定を行います。
/// </summary>
bool IsCollisionSphereToAABB(const SphereColliderShape& sphere, const AABBColliderShape& aabb) {
	const Vector3 closestPoint{
	    std::clamp(sphere.center.x, aabb.min.x, aabb.max.x),
	    std::clamp(sphere.center.y, aabb.min.y, aabb.max.y),
	    std::clamp(sphere.center.z, aabb.min.z, aabb.max.z)
	};
	const Vector3 diff = closestPoint - sphere.center;
	return Dot(diff, diff) <= sphere.radius * sphere.radius;
}

/// <summary>
/// AABBToSegment の当たり判定を行います。
/// </summary>
bool IsCollisionAABBToSegment(const AABBColliderShape& aabb, const SegmentColliderShape& segment) {
	return IntersectAABBWithParamRange(aabb, segment.origin, segment.diff, 0.0f, 1.0f);
}

/// <summary>
/// AABBToLine の当たり判定を行います。
/// </summary>
bool IsCollisionAABBToLine(const AABBColliderShape& aabb, const LineColliderShape& line) {
	return IntersectAABBWithParamRange(aabb, line.origin, line.diff, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity());
}

/// <summary>
/// OBBToSphere の当たり判定を行います。
/// </summary>
bool IsCollisionOBBToSphere(const OBBColliderShape& obb, const SphereColliderShape& sphere) {
	const Vector3 sphereCenterInOBB = ToOBBLocalPoint(obb, sphere.center);
	const AABBColliderShape localAABB{{-obb.halfSize.x, -obb.halfSize.y, -obb.halfSize.z}, obb.halfSize};
	const SphereColliderShape localSphere{sphereCenterInOBB, sphere.radius};
	return IsCollisionSphereToAABB(localSphere, localAABB);
}

/// <summary>
/// OBBToLine の当たり判定を行います。
/// </summary>
bool IsCollisionOBBToLine(const OBBColliderShape& obb, const LineColliderShape& line) {
	const Vector3 localOrigin = ToOBBLocalPoint(obb, line.origin);
	const Vector3 localEnd = ToOBBLocalPoint(obb, line.origin + line.diff);
	const LineColliderShape localLine{localOrigin, localEnd - localOrigin};
	const AABBColliderShape localAABB{{-obb.halfSize.x, -obb.halfSize.y, -obb.halfSize.z}, obb.halfSize};
	return IsCollisionAABBToLine(localAABB, localLine);
}

/// <summary>
/// OBBToOBB の当たり判定を行います。
/// </summary>
bool IsCollisionOBBToOBB(const OBBColliderShape& a, const OBBColliderShape& b) {
	for (int axisIndex = 0; axisIndex < 3; ++axisIndex) {
		if (IsSeparatedOnAxis(a, b, a.orientation[axisIndex])) {
			return false;
		}
		if (IsSeparatedOnAxis(a, b, b.orientation[axisIndex])) {
			return false;
		}
	}

	for (int aAxisIndex = 0; aAxisIndex < 3; ++aAxisIndex) {
		for (int bAxisIndex = 0; bAxisIndex < 3; ++bAxisIndex) {
			if (IsSeparatedOnAxis(a, b, Cross(a.orientation[aAxisIndex], b.orientation[bAxisIndex]))) {
				return false;
			}
		}
	}

	return true;
}
