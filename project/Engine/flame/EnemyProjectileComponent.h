#pragma once

#include "Component.h"
#include "GameObject.h"
#include "MathConstants.h"
#include "../base/GameTime.h"
#include <algorithm>
#include <cmath>

/// <summary>敵弾が使用する移動軌道です。</summary>
enum class EnemyProjectileMotionType {
	Linear,
	ExpandingOrbit,
	ContractingOrbit,
	Homing
};

/// <summary>敵弾の移動、攻撃力、寿命、命中状態を管理します。</summary>
class EnemyProjectileComponent : public Component {
public:
	void Update() override {
		if (!GetOwner()) {
			return;
		}
		EulerTransform& transform = GetOwner()->GetTransform();
		if (motionType_ == EnemyProjectileMotionType::Homing) {
			if (homingTarget_) {
				Vector3 toTarget = homingTarget_->GetTransform().translate - transform.translate;
				toTarget.y = 0.0f;
				if (Length(toTarget) > MathConstants::kDirectionEpsilon) {
					direction_ = Normalize(toTarget);
				}
			}
			transform.translate = transform.translate +
				(speed_ * GameTime::GetFrameScale60()) * direction_;
			transform.rotate.y = std::atan2(direction_.x, direction_.z);
		} else if (motionType_ == EnemyProjectileMotionType::ExpandingOrbit ||
		    motionType_ == EnemyProjectileMotionType::ContractingOrbit) {
			const Vector3 previousPosition = transform.translate;
			const float radialDirection = motionType_ == EnemyProjectileMotionType::ContractingOrbit ? -1.0f : 1.0f;
			orbitRadius_ = (std::max)(0.15f, orbitRadius_ + radialDirection * orbitRadialSpeed_ * GameTime::GetFrameScale60());
			orbitAngle_ += orbitAngularSpeed_ * GameTime::GetDeltaTime();
			transform.translate = orbitCenter_ + Vector3{
				std::sin(orbitAngle_) * orbitRadius_,
				orbitHeight_,
				std::cos(orbitAngle_) * orbitRadius_
			};
			const Vector3 movement = transform.translate - previousPosition;
			if (Length(movement) > MathConstants::kDirectionEpsilon) {
				direction_ = Normalize(movement);
				transform.rotate.y = std::atan2(direction_.x, direction_.z);
			}
		} else {
			// 60FPS基準の速度を実際のフレーム時間に合わせて補正する。
			transform.translate = transform.translate +
				(speed_ * GameTime::GetFrameScale60()) * direction_;
		}
		elapsedTime_ += GameTime::GetDeltaTime();
	}

	void SetDirection(const Vector3& direction) {
		direction_ = Length(direction) > MathConstants::kDirectionEpsilon ? Normalize(direction) : Vector3{0.0f, 0.0f, 1.0f};
	}
	const Vector3& GetDirection() const { return direction_; }
	void SetMotionType(EnemyProjectileMotionType motionType) { motionType_ = motionType; }
	EnemyProjectileMotionType GetMotionType() const { return motionType_; }
	void SetExpandingOrbit(
		const Vector3& center, float angle, float initialRadius, float angularSpeed, float radialSpeed, float height) {
		motionType_ = EnemyProjectileMotionType::ExpandingOrbit;
		orbitCenter_ = center;
		orbitAngle_ = angle;
		orbitRadius_ = (std::max)(0.0f, initialRadius);
		orbitAngularSpeed_ = angularSpeed;
		orbitRadialSpeed_ = (std::max)(0.0f, radialSpeed);
		orbitHeight_ = height;
	}
	void SetContractingOrbit(
		const Vector3& center, float angle, float initialRadius, float angularSpeed, float radialSpeed, float height) {
		SetExpandingOrbit(center, angle, initialRadius, angularSpeed, radialSpeed, height);
		motionType_ = EnemyProjectileMotionType::ContractingOrbit;
	}
	void SetHomingTarget(GameObject* target) {
		motionType_ = EnemyProjectileMotionType::Homing;
		homingTarget_ = target;
	}
	void SetSpeed(float speed) { speed_ = (std::max)(0.0f, speed); }
	void SetAttack(float attack) { attack_ = (std::max)(0.0f, attack); }
	float GetAttack() const { return attack_; }
	void SetSize(float size) { size_ = (std::max)(0.01f, size); }
	float GetSize() const { return size_; }
	void SetLifeTime(float lifeTime) { lifeTime_ = (std::max)(0.0f, lifeTime); }
	bool IsExpired() const { return hit_ || elapsedTime_ >= lifeTime_; }
	void MarkHit() { hit_ = true; }

private:
	/// <summary>正規化された弾の進行方向です。</summary>
	Vector3 direction_{0.0f, 0.0f, 1.0f};
	EnemyProjectileMotionType motionType_ = EnemyProjectileMotionType::Linear;
	Vector3 orbitCenter_{};
	float orbitAngle_ = 0.0f;
	float orbitRadius_ = 0.0f;
	float orbitAngularSpeed_ = 0.0f;
	float orbitRadialSpeed_ = 0.0f;
	float orbitHeight_ = 0.0f;
	GameObject* homingTarget_ = nullptr;
	/// <summary>60FPS時の1フレームあたりの移動量です。</summary>
	float speed_ = 0.12f;
	/// <summary>プレイヤーへ与えるダメージ量です。</summary>
	float attack_ = 1.0f;
	/// <summary>表示と衝突判定に使用する弾の大きさです。</summary>
	float size_ = 0.22f;
	/// <summary>未命中でも弾を破棄するまでの秒数です。</summary>
	float lifeTime_ = 6.0f;
	/// <summary>生成後の経過秒数です。</summary>
	float elapsedTime_ = 0.0f;
	/// <summary>衝突処理済みかを表します。</summary>
	bool hit_ = false;
};
