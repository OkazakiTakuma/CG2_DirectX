#pragma once
#include "Vector.h"

struct SphereColliderShape {
	Vector3 center{};
	float radius = 1.0f;
};

struct LineColliderShape {
	Vector3 origin{};
	Vector3 diff{};
};

struct RayColliderShape {
	Vector3 origin{};
	Vector3 diff{};
};

struct SegmentColliderShape {
	Vector3 origin{};
	Vector3 diff{};
};

struct PlaneColliderShape {
	Vector3 normal{0.0f, 1.0f, 0.0f};
	float distance = 0.0f;
};

struct TriangleColliderShape {
	Vector3 vertices[3]{};
};

struct AABBColliderShape {
	Vector3 min{};
	Vector3 max{};
};

struct OBBColliderShape {
	Vector3 center{};
	Vector3 orientation[3] = {
	    {1.0f, 0.0f, 0.0f},
	    {0.0f, 1.0f, 0.0f},
	    {0.0f, 0.0f, 1.0f}
	};
	Vector3 halfSize{0.5f, 0.5f, 0.5f};
};

bool IsCollisionSphereToSphere(const SphereColliderShape& a, const SphereColliderShape& b);
bool IsCollisionSphereToPlane(const SphereColliderShape& sphere, const PlaneColliderShape& plane);
bool IsCollisionSegmentToPlane(const SegmentColliderShape& segment, const PlaneColliderShape& plane);
bool IsCollisionLineToPlane(const LineColliderShape& line, const PlaneColliderShape& plane);
bool IsCollisionRayToPlane(const RayColliderShape& ray, const PlaneColliderShape& plane);
bool IsCollisionTriangleToSegment(const TriangleColliderShape& triangle, const SegmentColliderShape& segment);
bool IsCollisionAABBToAABB(const AABBColliderShape& a, const AABBColliderShape& b);
bool IsCollisionSphereToAABB(const SphereColliderShape& sphere, const AABBColliderShape& aabb);
bool IsCollisionAABBToSegment(const AABBColliderShape& aabb, const SegmentColliderShape& segment);
bool IsCollisionAABBToLine(const AABBColliderShape& aabb, const LineColliderShape& line);
bool IsCollisionOBBToSphere(const OBBColliderShape& obb, const SphereColliderShape& sphere);
bool IsCollisionOBBToLine(const OBBColliderShape& obb, const LineColliderShape& line);
bool IsCollisionOBBToOBB(const OBBColliderShape& a, const OBBColliderShape& b);
