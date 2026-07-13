#pragma once
#include "../collision/CollisionPrimitive.h"
#include "../flame/Component.h"
#include "../flame/GameObject.h"
#include "LineDrawer.h"
#include <algorithm>
#include <cmath>

class SphereColliderComponent : public Component {
public:
	/// <summary>
	/// デバッグ表示が有効な場合、球コライダーを線で描画します。
	/// </summary>
	void Draw3D() override {
#ifndef USE_IMGUI
		return;
#else
		if (!isDrawDebug_) {
			return;
		}

		DrawDebugSphere(GetWorldSphere(), isColliding_ ? Vector4{1.0f, 0.2f, 0.2f, 1.0f} : Vector4{0.2f, 0.7f, 1.0f, 1.0f});
#endif
	}

	/// <summary>
	/// オブジェクトの Transform を反映したワールド空間の球を取得します。
	/// </summary>
	/// <returns>ワールド空間の球コライダー形状を返します。</returns>
	SphereColliderShape GetWorldSphere() const {
		SphereColliderShape result{};
		if (!GetOwner()) {
			return result;
		}

		const EulerTransform& transform = GetOwner()->GetTransform();
		float maxScale = std::fabs(transform.scale.x);
		if (std::fabs(transform.scale.y) > maxScale) {
			maxScale = std::fabs(transform.scale.y);
		}
		if (std::fabs(transform.scale.z) > maxScale) {
			maxScale = std::fabs(transform.scale.z);
		}
		result.center = transform.translate + centerOffset_;
		result.radius = radius_ * maxScale;
		return result;
	}

	void SetCenterOffset(const Vector3& centerOffset) { centerOffset_ = centerOffset; }
	const Vector3& GetCenterOffset() const { return centerOffset_; }
	void SetRadius(float radius) { radius_ = radius < 0.0f ? 0.0f : radius; }
	float GetRadius() const { return radius_; }
	void SetDrawDebug(bool isDrawDebug) { isDrawDebug_ = isDrawDebug; }
	bool GetDrawDebug() const { return isDrawDebug_; }
	void SetPushBackEnabled(bool isPushBackEnabled) { isPushBackEnabled_ = isPushBackEnabled; }
	bool GetPushBackEnabled() const { return isPushBackEnabled_; }
	void SetColliding(bool isColliding) { isColliding_ = isColliding; }
	bool IsColliding() const { return isColliding_; }

private:
	/// <summary>
	/// 球を3軸の円として線描画します。
	/// </summary>
	/// <param name="sphere">描画する球コライダー形状を指定します。</param>
	/// <param name="color">描画色を指定します。</param>
	static void DrawDebugSphere(const SphereColliderShape& sphere, const Vector4& color) {
		constexpr uint32_t kSegmentCount = 16;
		constexpr float kTwoPi = 6.28318530718f;

		for (uint32_t index = 0; index < kSegmentCount; ++index) {
			const float currentAngle = kTwoPi * static_cast<float>(index) / static_cast<float>(kSegmentCount);
			const float nextAngle = kTwoPi * static_cast<float>(index + 1) / static_cast<float>(kSegmentCount);

			const float currentCos = std::cos(currentAngle) * sphere.radius;
			const float currentSin = std::sin(currentAngle) * sphere.radius;
			const float nextCos = std::cos(nextAngle) * sphere.radius;
			const float nextSin = std::sin(nextAngle) * sphere.radius;

			LineDrawer::GetInstance()->DrawLine(
			    {sphere.center.x + currentCos, sphere.center.y + currentSin, sphere.center.z},
			    {sphere.center.x + nextCos, sphere.center.y + nextSin, sphere.center.z},
			    color
			);
			LineDrawer::GetInstance()->DrawLine(
			    {sphere.center.x, sphere.center.y + currentCos, sphere.center.z + currentSin},
			    {sphere.center.x, sphere.center.y + nextCos, sphere.center.z + nextSin},
			    color
			);
			LineDrawer::GetInstance()->DrawLine(
			    {sphere.center.x + currentCos, sphere.center.y, sphere.center.z + currentSin},
			    {sphere.center.x + nextCos, sphere.center.y, sphere.center.z + nextSin},
			    color
			);
		}
	}

	Vector3 centerOffset_{0.0f, 0.0f, 0.0f};
	float radius_ = 0.5f;
	bool isDrawDebug_ = true;
	bool isPushBackEnabled_ = false;
	bool isColliding_ = false;
};
