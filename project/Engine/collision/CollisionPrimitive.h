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

/// <summary>
/// SphereToSphere の当たり判定を行います。
/// </summary>
/// <param name="a">a に使用する値を指定します。</param>
/// <param name="b">b に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
bool IsCollisionSphereToSphere(const SphereColliderShape& a, const SphereColliderShape& b);
/// <summary>
/// SphereToPlane の当たり判定を行います。
/// </summary>
/// <param name="sphere">sphere に使用する値を指定します。</param>
/// <param name="plane">plane に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
bool IsCollisionSphereToPlane(const SphereColliderShape& sphere, const PlaneColliderShape& plane);
/// <summary>
/// SegmentToPlane の当たり判定を行います。
/// </summary>
/// <param name="segment">segment に使用する値を指定します。</param>
/// <param name="plane">plane に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
bool IsCollisionSegmentToPlane(const SegmentColliderShape& segment, const PlaneColliderShape& plane);
/// <summary>
/// LineToPlane の当たり判定を行います。
/// </summary>
/// <param name="line">line に使用する値を指定します。</param>
/// <param name="plane">plane に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
bool IsCollisionLineToPlane(const LineColliderShape& line, const PlaneColliderShape& plane);
/// <summary>
/// RayToPlane の当たり判定を行います。
/// </summary>
/// <param name="ray">ray に使用する値を指定します。</param>
/// <param name="plane">plane に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
bool IsCollisionRayToPlane(const RayColliderShape& ray, const PlaneColliderShape& plane);
/// <summary>
/// TriangleToSegment の当たり判定を行います。
/// </summary>
/// <param name="triangle">triangle に使用する値を指定します。</param>
/// <param name="segment">segment に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
bool IsCollisionTriangleToSegment(const TriangleColliderShape& triangle, const SegmentColliderShape& segment);
/// <summary>
/// AABBToAABB の当たり判定を行います。
/// </summary>
/// <param name="a">a に使用する値を指定します。</param>
/// <param name="b">b に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
bool IsCollisionAABBToAABB(const AABBColliderShape& a, const AABBColliderShape& b);
/// <summary>
/// SphereToAABB の当たり判定を行います。
/// </summary>
/// <param name="sphere">sphere に使用する値を指定します。</param>
/// <param name="aabb">aabb に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
bool IsCollisionSphereToAABB(const SphereColliderShape& sphere, const AABBColliderShape& aabb);
/// <summary>
/// AABBToSegment の当たり判定を行います。
/// </summary>
/// <param name="aabb">aabb に使用する値を指定します。</param>
/// <param name="segment">segment に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
bool IsCollisionAABBToSegment(const AABBColliderShape& aabb, const SegmentColliderShape& segment);
/// <summary>
/// AABBToLine の当たり判定を行います。
/// </summary>
/// <param name="aabb">aabb に使用する値を指定します。</param>
/// <param name="line">line に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
bool IsCollisionAABBToLine(const AABBColliderShape& aabb, const LineColliderShape& line);
/// <summary>
/// OBBToSphere の当たり判定を行います。
/// </summary>
/// <param name="obb">obb に使用する値を指定します。</param>
/// <param name="sphere">sphere に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
bool IsCollisionOBBToSphere(const OBBColliderShape& obb, const SphereColliderShape& sphere);
/// <summary>
/// OBBToLine の当たり判定を行います。
/// </summary>
/// <param name="obb">obb に使用する値を指定します。</param>
/// <param name="line">line に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
bool IsCollisionOBBToLine(const OBBColliderShape& obb, const LineColliderShape& line);
/// <summary>
/// OBBToOBB の当たり判定を行います。
/// </summary>
/// <param name="a">a に使用する値を指定します。</param>
/// <param name="b">b に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
bool IsCollisionOBBToOBB(const OBBColliderShape& a, const OBBColliderShape& b);
