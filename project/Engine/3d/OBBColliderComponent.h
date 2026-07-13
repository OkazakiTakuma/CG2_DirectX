#pragma once
#include "../collision/CollisionPrimitive.h"
#include "../flame/Component.h"
#include "../flame/GameObject.h"
#include "LineDrawer.h"
#include "Matrix.h"
#include <algorithm>

class OBBColliderComponent : public Component {
public:
	/// <summary>
	/// 3D 要素の描画処理を行います。
	/// </summary>
	void Draw3D() override {
#ifndef USE_IMGUI
		return;
#else
		if (!isDrawDebug_) {
			return;
		}

		DrawDebugOBB(GetWorldOBB(), isColliding_ ? Vector4{1.0f, 0.2f, 0.2f, 1.0f} : Vector4{0.2f, 1.0f, 0.2f, 1.0f});
#endif
	}

	/// <summary>
	/// WorldOBB を取得します。
	/// </summary>
	/// <returns>処理結果を返します。</returns>
	OBBColliderShape GetWorldOBB() const {
		OBBColliderShape result{};
		if (!GetOwner()) {
			return result;
		}

		const EulerTransform& transform = GetOwner()->GetTransform();
		const Matrix4x4 rotateMatrix = MakeRotateXYZMatrix(transform.rotate);
		const Vector3 axisX{rotateMatrix.m[0][0], rotateMatrix.m[0][1], rotateMatrix.m[0][2]};
		const Vector3 axisY{rotateMatrix.m[1][0], rotateMatrix.m[1][1], rotateMatrix.m[1][2]};
		const Vector3 axisZ{rotateMatrix.m[2][0], rotateMatrix.m[2][1], rotateMatrix.m[2][2]};
		result.orientation[0] = Normalize(axisX);
		result.orientation[1] = Normalize(axisY);
		result.orientation[2] = Normalize(axisZ);

		const Vector3 scaledOffset{
		    centerOffset_.x * transform.scale.x,
		    centerOffset_.y * transform.scale.y,
		    centerOffset_.z * transform.scale.z
		};
		result.center =
		    transform.translate +
		    scaledOffset.x * result.orientation[0] +
		    scaledOffset.y * result.orientation[1] +
		    scaledOffset.z * result.orientation[2];
		result.halfSize = {
		    halfSize_.x < 0.0f ? 0.0f : halfSize_.x * std::fabs(transform.scale.x),
		    halfSize_.y < 0.0f ? 0.0f : halfSize_.y * std::fabs(transform.scale.y),
		    halfSize_.z < 0.0f ? 0.0f : halfSize_.z * std::fabs(transform.scale.z)
		};
		return result;
	}

	void SetCenterOffset(const Vector3& centerOffset) { centerOffset_ = centerOffset; }
	const Vector3& GetCenterOffset() const { return centerOffset_; }
	void SetHalfSize(const Vector3& halfSize) { halfSize_ = halfSize; }
	const Vector3& GetHalfSize() const { return halfSize_; }
	void SetDrawDebug(bool isDrawDebug) { isDrawDebug_ = isDrawDebug; }
	bool GetDrawDebug() const { return isDrawDebug_; }
	void SetPushBackEnabled(bool isPushBackEnabled) { isPushBackEnabled_ = isPushBackEnabled; }
	bool GetPushBackEnabled() const { return isPushBackEnabled_; }
	void SetColliding(bool isColliding) { isColliding_ = isColliding; }
	bool IsColliding() const { return isColliding_; }

private:
	/// <summary>
	/// DrawDebugOBB の処理を行います。
	/// </summary>
	/// <param name="obb">obb に使用する値を指定します。</param>
	/// <param name="color">色を指定します。</param>
	static void DrawDebugOBB(const OBBColliderShape& obb, const Vector4& color) {
		Vector3 vertices[8]{};
		for (int i = 0; i < 8; ++i) {
			const float xSign = (i & 1) ? 1.0f : -1.0f;
			const float ySign = (i & 2) ? 1.0f : -1.0f;
			const float zSign = (i & 4) ? 1.0f : -1.0f;
			vertices[i] =
			    obb.center +
			    (xSign * obb.halfSize.x) * obb.orientation[0] +
			    (ySign * obb.halfSize.y) * obb.orientation[1] +
			    (zSign * obb.halfSize.z) * obb.orientation[2];
		}

		static constexpr int kEdges[12][2] = {
		    {0, 1}, {1, 3}, {3, 2}, {2, 0},
		    {4, 5}, {5, 7}, {7, 6}, {6, 4},
		    {0, 4}, {1, 5}, {2, 6}, {3, 7}
		};

		for (const auto& edge : kEdges) {
			LineDrawer::GetInstance()->DrawLine(vertices[edge[0]], vertices[edge[1]], color);
		}
	}

	Vector3 centerOffset_{0.0f, 0.0f, 0.0f};
	Vector3 halfSize_{0.5f, 0.5f, 0.5f};
	bool isDrawDebug_ = true;
	bool isPushBackEnabled_ = false;
	bool isColliding_ = false;
};
