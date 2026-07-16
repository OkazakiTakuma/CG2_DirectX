#pragma once
#include "Component.h"
#include "GameObject.h"
#include "Vector.h"
#include "PlayerAttackComponent.h"
#include "../base/GameTime.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <string>

class PlayerProjectileComponent : public Component {
public:
	void Update() override {
		GameObject* owner = GetOwner();
		if (!owner) {
			return;
		}

		const float deltaTime = GameTime::GetDeltaTime();
		const float frameScale = GameTime::GetFrameScale60();
		UpdateHitCooldowns(deltaTime);
		if (motionType_ == PlayerProjectileMotionType::Orbit) {
			if (!motionAnchor_) {
				lifeTime_ = 0.0f;
				return;
			}
			orbitAngleRadians_ += orbitAngularSpeed_ * deltaTime;
			const Vector3 anchorPosition = motionAnchor_->GetTransform().translate;
			owner->GetTransform().translate = {
				anchorPosition.x + std::cos(orbitAngleRadians_) * orbitRadius_,
				anchorPosition.y + 0.65f,
				anchorPosition.z + std::sin(orbitAngleRadians_) * orbitRadius_
			};
			owner->GetTransform().rotate.y = -orbitAngleRadians_;
			lifeTime_ -= deltaTime;
			constexpr float kOrbitShrinkDurationSeconds = 0.75f;
			visualScaleRate_ = (std::clamp)(lifeTime_ / kOrbitShrinkDurationSeconds, 0.0f, 1.0f);
			const float visualSize = size_ * visualScaleRate_;
			owner->GetTransform().scale = {visualSize, visualSize, visualSize};
			return;
		}
		if (motionType_ == PlayerProjectileMotionType::SkyLaser) {
			lifeTime_ -= deltaTime;
			return;
		}
		if (homingEnabled_ && homingTarget_) {
			Vector3 toTarget = homingTarget_->GetTransform().translate - owner->GetTransform().translate;
			toTarget.y = 0.0f;
			if (Length(toTarget) > 0.0001f) {
				const Vector3 targetDirection = NormalizeReturnVector(toTarget);
				const float accuracy = 1.0f - std::pow(1.0f - (std::clamp)(homingAccuracy_, 0.0f, 1.0f), frameScale);
				direction_ = NormalizeReturnVector(Leap(direction_, targetDirection, accuracy));
			}
		}

		owner->GetTransform().translate = owner->GetTransform().translate + (speed_ * frameScale) * direction_;
		owner->GetTransform().rotate.y = std::atan2(direction_.x, direction_.z);
		lifeTime_ -= deltaTime;
	}

	void SetAttackName(const std::string& attackName) { attackName_ = attackName; }
	const std::string& GetAttackName() const { return attackName_; }
	void SetLevel(const std::string& level) { level_ = level; }
	const std::string& GetLevel() const { return level_; }
	void SetDirection(const Vector3& direction) { direction_ = Length(direction) > 0.0001f ? NormalizeReturnVector(direction) : Vector3{0.0f, 0.0f, 1.0f}; }
	const Vector3& GetDirection() const { return direction_; }
	void SetSpeed(float speed) { speed_ = speed < 0.0f ? 0.0f : speed; }
	float GetSpeed() const { return speed_; }
	void SetAttack(float attack) { attack_ = attack < 0.0f ? 0.0f : attack; }
	float GetAttack() const { return attack_; }
	void SetSize(float size) { size_ = size < 0.01f ? 0.01f : size; }
	float GetSize() const { return size_ * visualScaleRate_; }
	void SetHomingEnabled(bool homingEnabled) { homingEnabled_ = homingEnabled; }
	bool IsHomingEnabled() const { return homingEnabled_; }
	void SetHomingAccuracy(float homingAccuracy) { homingAccuracy_ = (std::clamp)(homingAccuracy, 0.0f, 1.0f); }
	float GetHomingAccuracy() const { return homingAccuracy_; }
	void SetMotionType(PlayerProjectileMotionType motionType) { motionType_ = motionType; }
	PlayerProjectileMotionType GetMotionType() const { return motionType_; }
	void SetMotionAnchor(GameObject* motionAnchor) { motionAnchor_ = motionAnchor; }
	void SetOrbitAngleRadians(float angle) { orbitAngleRadians_ = angle; }
	void SetOrbitRadius(float radius) { orbitRadius_ = (std::max)(0.1f, radius); }
	void SetOrbitAngularSpeed(float speed) { orbitAngularSpeed_ = speed; }
	void SetHomingTarget(GameObject* target) { homingTarget_ = target; }
	GameObject* GetHomingTarget() const { return homingTarget_; }
	void SetLifeTime(float lifeTime) { lifeTime_ = lifeTime; }
	void Expire() { lifeTime_ = 0.0f; }
	bool IsExpired() const { return lifeTime_ <= 0.0f; }
	void SetPierceCount(int pierceCount) { pierceCount_ = pierceCount < 0 ? 0 : pierceCount; }
	int GetPierceCount() const { return pierceCount_; }
	void SetInfinitePierce(bool infinitePierce) { infinitePierce_ = infinitePierce; }
	bool IsInfinitePierce() const { return infinitePierce_; }
	void SetRepeatHitInterval(float intervalSeconds) { repeatHitIntervalSeconds_ = (std::max)(0.0f, intervalSeconds); }
	float GetRepeatHitInterval() const { return repeatHitIntervalSeconds_; }
	bool HasHitObject(GameObject* object) const {
		for (const HitRecord& hitRecord : hitRecords_) {
			if (hitRecord.object == object) {
				return true;
			}
		}
		return false;
	}
	void RegisterHitObject(GameObject* object) {
		if (!object || HasHitObject(object)) {
			return;
		}
		hitRecords_.push_back({object, repeatHitIntervalSeconds_ > 0.0f ? repeatHitIntervalSeconds_ : -1.0f});
		if (infinitePierce_) {
			return;
		}
		if (pierceCount_ <= 0) {
			Expire();
			return;
		}
		--pierceCount_;
	}

private:
	struct HitRecord {
		GameObject* object = nullptr;
		float cooldownSeconds = -1.0f;
	};

	void UpdateHitCooldowns(float deltaTime) {
		if (repeatHitIntervalSeconds_ <= 0.0f) {
			return;
		}
		for (HitRecord& hitRecord : hitRecords_) {
			hitRecord.cooldownSeconds -= deltaTime;
		}
		hitRecords_.erase(
			std::remove_if(hitRecords_.begin(), hitRecords_.end(), [](const HitRecord& hitRecord) {
				return hitRecord.cooldownSeconds <= 0.0f;
			}),
			hitRecords_.end());
	}

	std::string attackName_;
	std::string level_ = "1";
	Vector3 direction_{0.0f, 0.0f, 1.0f};
	float speed_ = 0.3f;
	float attack_ = 100.0f;
	float size_ = 1.0f;
	float visualScaleRate_ = 1.0f;
	float lifeTime_ = 3.0f;
	int pierceCount_ = 0;
	bool infinitePierce_ = false;
	bool homingEnabled_ = false;
	float homingAccuracy_ = 1.0f;
	PlayerProjectileMotionType motionType_ = PlayerProjectileMotionType::Linear;
	GameObject* motionAnchor_ = nullptr;
	float orbitAngleRadians_ = 0.0f;
	float orbitRadius_ = 2.2f;
	float orbitAngularSpeed_ = 2.2f;
	GameObject* homingTarget_ = nullptr;
	float repeatHitIntervalSeconds_ = 0.0f;
	std::vector<HitRecord> hitRecords_;
};
