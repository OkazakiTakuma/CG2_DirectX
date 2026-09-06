#pragma once

#include "Component.h"
#include "GameObject.h"
#include "MathConstants.h"
#include "../base/GameTime.h"
#include <algorithm>
#include <cmath>

/// <summary>敵弾が使用する移動軌道です。</summary>
enum class EnemyProjectileMotionType {
	/// <summary>生成時に決めた方向へ直進します。</summary>
	Linear,
	/// <summary>固定中心の周囲を回りながら半径を広げます。</summary>
	ExpandingOrbit,
	/// <summary>固定中心の周囲を回りながら半径を狭めます。</summary>
	ContractingOrbit,
	/// <summary>対象の現在位置へ向きを更新しながら追跡します。</summary>
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
			// 巨大竜巻は急旋回の補間を行わず、毎フレーム対象の水平方向へ進行方向を合わせる。
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
			// radialSpeed は60FPS時の1フレーム量、angularSpeed はラジアン/秒として扱う。
			const float radialDirection = motionType_ == EnemyProjectileMotionType::ContractingOrbit ? -1.0f : 1.0f;
			// 収束竜巻が中心を通過して反対側へ跳ねないよう、最小半径を保持する。
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
	/// <summary>指定した中心へ収束する螺旋軌道を設定します。</summary>
	void SetContractingOrbit(
		const Vector3& center, float angle, float initialRadius, float angularSpeed, float radialSpeed, float height) {
		SetExpandingOrbit(center, angle, initialRadius, angularSpeed, radialSpeed, height);
		motionType_ = EnemyProjectileMotionType::ContractingOrbit;
	}
	/// <summary>巨大竜巻が追跡する対象を設定します。対象の所有権は保持しません。</summary>
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
	/// <summary>通常弾と3種類の竜巻軌道を切り替える移動方式です。</summary>
	EnemyProjectileMotionType motionType_ = EnemyProjectileMotionType::Linear;
	/// <summary>螺旋軌道の生成時に固定するワールド座標中心です。</summary>
	Vector3 orbitCenter_{};
	float orbitAngle_ = 0.0f;
	float orbitRadius_ = 0.0f;
	float orbitAngularSpeed_ = 0.0f;
	float orbitRadialSpeed_ = 0.0f;
	float orbitHeight_ = 0.0f;
	/// <summary>追尾対象への非所有参照です。</summary>
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
