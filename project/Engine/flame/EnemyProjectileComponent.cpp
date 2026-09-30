#include "EnemyProjectileComponent.h"

void EnemyProjectileComponent::Update() {
		if (!GetOwner()) {
			return;
		}
		EulerTransform& transform = GetOwner()->GetTransform();
		previousPosition_ = transform.translate;
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

void EnemyProjectileComponent::SetDirection(const Vector3& direction) {
		direction_ = Length(direction) > MathConstants::kDirectionEpsilon ? Normalize(direction) : Vector3{0.0f, 0.0f, 1.0f};
	}

const Vector3& EnemyProjectileComponent::GetDirection() const { return direction_; }

const Vector3& EnemyProjectileComponent::GetPreviousPosition() const { return previousPosition_; }

void EnemyProjectileComponent::SetMotionType(EnemyProjectileMotionType motionType) { motionType_ = motionType; }

EnemyProjectileMotionType EnemyProjectileComponent::GetMotionType() const { return motionType_; }

void EnemyProjectileComponent::SetExpandingOrbit(
	const Vector3& center, float angle, float initialRadius, float angularSpeed, float radialSpeed, float height) {
	motionType_ = EnemyProjectileMotionType::ExpandingOrbit;
	orbitCenter_ = center;
	orbitAngle_ = angle;
	orbitRadius_ = (std::max)(0.0f, initialRadius);
	orbitAngularSpeed_ = angularSpeed;
	orbitRadialSpeed_ = (std::max)(0.0f, radialSpeed);
	orbitHeight_ = height;
}

void EnemyProjectileComponent::SetContractingOrbit(
	const Vector3& center, float angle, float initialRadius, float angularSpeed, float radialSpeed, float height) {
	SetExpandingOrbit(center, angle, initialRadius, angularSpeed, radialSpeed, height);
	motionType_ = EnemyProjectileMotionType::ContractingOrbit;
}

void EnemyProjectileComponent::SetHomingTarget(GameObject* target) {
		motionType_ = EnemyProjectileMotionType::Homing;
		homingTarget_ = target;
	}

void EnemyProjectileComponent::SetSpeed(float speed) { speed_ = (std::max)(0.0f, speed); }

void EnemyProjectileComponent::SetAttack(float attack) { attack_ = (std::max)(0.0f, attack); }

float EnemyProjectileComponent::GetAttack() const { return attack_; }

void EnemyProjectileComponent::SetSize(float size) { size_ = (std::max)(0.01f, size); }

float EnemyProjectileComponent::GetSize() const { return size_; }

void EnemyProjectileComponent::SetLifeTime(float lifeTime) { lifeTime_ = (std::max)(0.0f, lifeTime); }

bool EnemyProjectileComponent::IsExpired() const { return hit_ || elapsedTime_ >= lifeTime_; }

void EnemyProjectileComponent::MarkHit() { hit_ = true; }
