#pragma once
#include "Component.h"
#include "MathConstants.h"
#include "GameObject.h"
#include "Vector.h"
#include "PlayerAttackComponent.h"
#include "../base/GameTime.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <string>

/// <summary>プレイヤー弾の移動方式、追尾、寿命、貫通・再ヒット制御を管理します。</summary>
class PlayerProjectileComponent : public Component {
public:
	void Update() override {
		// 移動方式ごとの位置更新後に、追尾補正と寿命更新を適用する。
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
				anchorPosition.y + orbitHeight_,
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
		if (motionType_ == PlayerProjectileMotionType::Boomerang) {
			UpdateBoomerang(owner, deltaTime, frameScale);
			return;
		}
		if (motionType_ == PlayerProjectileMotionType::ClawSlash) {
			UpdateClawSlash(owner, deltaTime);
			return;
		}
		if (homingEnabled_ && homingTarget_) {
			Vector3 toTarget = homingTarget_->GetTransform().translate - owner->GetTransform().translate;
			toTarget.y = 0.0f;
			if (Length(toTarget) > MathConstants::kDirectionEpsilon) {
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
	void SetDirection(const Vector3& direction) { direction_ = Length(direction) > MathConstants::kDirectionEpsilon ? NormalizeReturnVector(direction) : Vector3{0.0f, 0.0f, 1.0f}; }
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
	void SetOrbitHeight(float height) { orbitHeight_ = height; }
	void SetOrbitAngularSpeed(float speed) { orbitAngularSpeed_ = speed; }
	void SetTravelDistance(float distance) { travelDistance_ = (std::max)(0.1f, distance); }
	void SetTravelOrigin(const Vector3& origin) { travelOrigin_ = origin; }
	void SetClawSlashIndex(int index) { clawSlashIndex_ = (std::max)(0, index); }
	void SetClawSlashCount(int count) { clawSlashCount_ = (std::max)(1, count); }
	void SetHomingTarget(GameObject* target) { homingTarget_ = target; }
	GameObject* GetHomingTarget() const { return homingTarget_; }
	void SetLifeTime(float lifeTime) { lifeTime_ = lifeTime; initialLifeTime_ = lifeTime; }
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
	/// <summary>同じ対象への連続ヒットを抑制する対象別クールダウンです。</summary>
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

	void UpdateBoomerang(GameObject* owner, float deltaTime, float frameScale) {
		if (!motionAnchor_) {
			lifeTime_ = 0.0f;
			return;
		}

		Vector3& position = owner->GetTransform().translate;
		if (!returning_) {
			Vector3 traveled = position - travelOrigin_;
			traveled.y = 0.0f;
			if (Length(traveled) >= travelDistance_) {
				returnTarget_ = motionAnchor_->GetTransform().translate;
				returnTarget_.y = position.y;
				const Vector3 toPlayer = returnTarget_ - position;
				if (Length(toPlayer) <= MathConstants::kDirectionEpsilon) {
					lifeTime_ = 0.0f;
					return;
				}
				direction_ = NormalizeReturnVector(toPlayer);
				returning_ = true;
			}
		}

		const float movement = speed_ * frameScale;
		if (returning_) {
			const Vector3 toReturnTarget = returnTarget_ - position;
			if (Length(toReturnTarget) <= (std::max)(movement, 0.05f)) {
				position = returnTarget_;
				lifeTime_ = 0.0f;
				return;
			}
		}

		position = position + movement * direction_;
		owner->GetTransform().rotate.y = std::atan2(direction_.x, direction_.z);
		lifeTime_ -= deltaTime;
	}

	void UpdateClawSlash(GameObject* owner, float deltaTime) {
		if (!motionAnchor_) {
			lifeTime_ = 0.0f;
			return;
		}

		lifeTime_ -= deltaTime;
		const float duration = (std::max)(0.01f, initialLifeTime_);
		const float progress = (std::clamp)(1.0f - lifeTime_ / duration, 0.0f, 1.0f);
		if (!isClawSlashYawInitialized_) {
			clawSlashYaw_ = motionAnchor_->GetTransform().rotate.y;
			isClawSlashYawInitialized_ = true;
		}
		const Vector3 forward{std::sin(clawSlashYaw_), 0.0f, std::cos(clawSlashYaw_)};
		const Vector3 right{std::cos(clawSlashYaw_), 0.0f, -std::sin(clawSlashYaw_)};
		const float centeredIndex = static_cast<float>(clawSlashIndex_) - (static_cast<float>(clawSlashCount_) - 1.0f) * 0.5f;
		const float localX = -0.9f + 1.8f * progress + centeredIndex * 0.34f;
		const float localY = 1.15f - 0.70f * progress + centeredIndex * 0.18f;
		const float localZ = 1.55f + std::abs(centeredIndex) * 0.03f;

		owner->GetTransform().translate = motionAnchor_->GetTransform().translate +
		    localX * right + Vector3{0.0f, localY, 0.0f} + localZ * forward;
		owner->GetTransform().rotate.y = clawSlashYaw_;
		owner->GetTransform().rotate.z = -0.65f;
	}

	std::string attackName_;
	std::string level_ = "1";
	Vector3 direction_{0.0f, 0.0f, 1.0f};
	float speed_ = 0.3f;
	float attack_ = 100.0f;
	float size_ = 1.0f;
	float visualScaleRate_ = 1.0f;
	float lifeTime_ = 3.0f;
	float initialLifeTime_ = 3.0f;
	int pierceCount_ = 0;
	bool infinitePierce_ = false;
	bool homingEnabled_ = false;
	float homingAccuracy_ = 1.0f;
	PlayerProjectileMotionType motionType_ = PlayerProjectileMotionType::Linear;
	/// <summary>周回弾などの中心として使用する非所有参照です。</summary>
	GameObject* motionAnchor_ = nullptr;
	float orbitAngleRadians_ = 0.0f;
	float orbitRadius_ = 2.2f;
	float orbitHeight_ = 0.65f;
	float orbitAngularSpeed_ = 2.2f;
	float travelDistance_ = 6.0f;
	Vector3 travelOrigin_{};
	Vector3 returnTarget_{};
	bool returning_ = false;
	int clawSlashIndex_ = 0;
	int clawSlashCount_ = 3;
	float clawSlashYaw_ = 0.0f;
	bool isClawSlashYawInitialized_ = false;
	/// <summary>追尾対象となるGameObjectへの非所有参照です。</summary>
	GameObject* homingTarget_ = nullptr;
	/// <summary>同一対象へ再度命中可能になるまでの秒数です。</summary>
	float repeatHitIntervalSeconds_ = 0.0f;
	/// <summary>命中済み対象と残りクールダウンの一覧です。</summary>
	std::vector<HitRecord> hitRecords_;
};
