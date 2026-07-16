#pragma once
#include "Component.h"
#include "GameObject.h"
#include "Vector.h"
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
	float GetSize() const { return size_; }
	void SetHomingEnabled(bool homingEnabled) { homingEnabled_ = homingEnabled; }
	bool IsHomingEnabled() const { return homingEnabled_; }
	void SetHomingAccuracy(float homingAccuracy) { homingAccuracy_ = (std::clamp)(homingAccuracy, 0.0f, 1.0f); }
	float GetHomingAccuracy() const { return homingAccuracy_; }
	void SetHomingTarget(GameObject* target) { homingTarget_ = target; }
	GameObject* GetHomingTarget() const { return homingTarget_; }
	void SetLifeTime(float lifeTime) { lifeTime_ = lifeTime; }
	void Expire() { lifeTime_ = 0.0f; }
	bool IsExpired() const { return lifeTime_ <= 0.0f; }
	void SetPierceCount(int pierceCount) { pierceCount_ = pierceCount < 0 ? 0 : pierceCount; }
	int GetPierceCount() const { return pierceCount_; }
	void SetInfinitePierce(bool infinitePierce) { infinitePierce_ = infinitePierce; }
	bool IsInfinitePierce() const { return infinitePierce_; }
	bool HasHitObject(GameObject* object) const {
		for (const GameObject* hitObject : hitObjects_) {
			if (hitObject == object) {
				return true;
			}
		}
		return false;
	}
	void RegisterHitObject(GameObject* object) {
		if (!object || HasHitObject(object)) {
			return;
		}
		hitObjects_.push_back(object);
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
	std::string attackName_;
	std::string level_ = "1";
	Vector3 direction_{0.0f, 0.0f, 1.0f};
	float speed_ = 0.3f;
	float attack_ = 100.0f;
	float size_ = 1.0f;
	float lifeTime_ = 3.0f;
	int pierceCount_ = 0;
	bool infinitePierce_ = false;
	bool homingEnabled_ = false;
	float homingAccuracy_ = 1.0f;
	GameObject* homingTarget_ = nullptr;
	std::vector<GameObject*> hitObjects_;
};
