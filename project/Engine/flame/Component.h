#pragma once
#include "../base/struct.h"
#include "MathConstants.h"

class GameObject;

/// <summary>
/// GameObjectへ追加できる振る舞いの共通インターフェースです。
/// 有効状態、所有者、共通の重力応答を管理し、更新・描画処理を派生クラスへ委譲します。
/// </summary>
class Component {
public:
	virtual ~Component() = default;

	virtual void Initialize() {}
	virtual void Update() {}
	virtual void Draw() {}
	virtual void Draw2D() {}
	virtual void Draw3D() {}
	virtual void Finalize() {}

	void SetOwner(GameObject* owner) { owner_ = owner; }
	GameObject* GetOwner() const { return owner_; }

	void SetEnabled(bool enabled) { isEnabled_ = enabled; }
	bool IsEnabled() const { return isEnabled_; }

	void SetGravityEnabled(bool enabled) {
		isGravityEnabled_ = enabled;
		if (!isGravityEnabled_) {
			gravityVelocity_ = 0.0f;
		}
	}
	bool IsGravityEnabled() const { return isGravityEnabled_; }
	void SetGravityStrength(float strength) { gravityStrength_ = strength < 0.0f ? 0.0f : strength; }
	float GetGravityStrength() const { return gravityStrength_; }
	void ResetGravityVelocity() { gravityVelocity_ = 0.0f; }
	void ApplyGravity(EulerTransform& transform, float deltaTime) {
		if (!isGravityEnabled_) {
			return;
		}
		gravityVelocity_ += gravityStrength_ * deltaTime;
		transform.translate.y -= gravityVelocity_ * deltaTime;
	}
	void ApplyCollisionResponse(const Vector3& collisionNormal) {
		if (!isGravityEnabled_ || gravityVelocity_ <= 0.0f) {
			return;
		}

		const float normalLength = Length(collisionNormal);
		if (normalLength <= MathConstants::kNormalizationEpsilon) {
			return;
		}

		const Vector3 normal = Normalize(collisionNormal);
		const float upwardRate = normal.y < 0.0f ? 0.0f : normal.y;
		gravityVelocity_ *= 1.0f - upwardRate;
		if (gravityVelocity_ < MathConstants::kDirectionEpsilon) {
			gravityVelocity_ = 0.0f;
		}
	}

private:
	GameObject* owner_ = nullptr;
	bool isEnabled_ = true;
	bool isGravityEnabled_ = false;
	float gravityStrength_ = 9.8f;
	float gravityVelocity_ = 0.0f;
};
