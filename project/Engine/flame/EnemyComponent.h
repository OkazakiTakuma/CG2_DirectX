#pragma once
#include "Component.h"
#include "MathConstants.h"
#include "GameObject.h"
#include "LineDrawer.h"
#include "../base/GameTime.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

/// <summary>敵が使用する移動・攻撃パターンです。</summary>
enum class EnemyBehaviorType {
	Chase,
	Shooter,
	Charger
};

/// <summary>シーンへ引き渡す敵弾生成パラメーターです。</summary>
struct EnemyShotRequest {
	Vector3 position{};
	Vector3 direction{0.0f, 0.0f, 1.0f};
	float speed = 0.12f;
	float attack = 1.0f;
	float size = 0.22f;
	float lifeTime = 6.0f;
};

/// <summary>敵タイプごとに読み込む能力・行動設定です。</summary>
struct EnemyStats {
	float health = 10.0f;
	float attack = 1.0f;
	float speed = 0.05f;
	float shootingInterval = 1.0f;
	float spawnsPerMinute = 12.0f;
	int experience = 1;
	std::string experienceModelFilePath = "sphere.obj";
	bool shoots = false;
	EnemyBehaviorType behavior = EnemyBehaviorType::Chase;
	float preferredDistance = 7.0f;
	float distanceTolerance = 1.5f;
	float projectileSpeed = 0.12f;
	float projectileSize = 0.22f;
	float projectileLifeTime = 6.0f;
	float chargeTriggerDistance = 7.0f;
	float chargeDuration = 1.2f;
	float dashSpeed = 0.28f;
	float dashDuration = 0.65f;
	float dashRecovery = 1.0f;
	float sizeScale = 1.0f;
};

/// <summary>敵の追跡、射撃、突進ステートと体力を管理します。</summary>
class EnemyComponent : public Component {
public:
	void Initialize() override {
		currentHealth_ = stats_.health;
	}

	void Update() override {
		// 設定された行動タイプに応じて、追跡または突進ステートを更新する。
		GameObject* owner = GetOwner();
		if (!owner || !target_) {
			return;
		}

		EulerTransform& transform = owner->GetTransform();
		const Vector3 targetPosition = target_->GetTransform().translate;
		Vector3 direction = targetPosition - transform.translate;
		direction.y = 0.0f;
		const float distance = Length(direction);
		const Vector3 normalized = distance > MathConstants::kDirectionEpsilon ? Normalize(direction) : Vector3{0.0f, 0.0f, 1.0f};
		transform.rotate.y = std::atan2(normalized.x, normalized.z);

		if (stats_.behavior == EnemyBehaviorType::Charger) {
			UpdateCharger(transform, normalized, distance);
		} else if (stats_.behavior == EnemyBehaviorType::Shooter || stats_.shoots) {
			UpdateShooter(transform, normalized, distance);
		} else if (distance > MathConstants::kDirectionEpsilon) {
			transform.translate = transform.translate + (stats_.speed * GameTime::GetFrameScale60()) * normalized;
		}
	}

	void Draw3D() override {
		if (!IsChargeWarningActive() || !GetOwner()) {
			return;
		}
		const Vector3 center = GetOwner()->GetTransform().translate + Vector3{0.0f, 0.06f, 0.0f};
		const float progress = GetChargeProgress();
		const float radius = 0.9f + 0.35f * progress;
		const Vector4 color{1.0f, 0.05f + 0.25f * progress, 0.02f, 1.0f};
		constexpr int segmentCount = 24;
		for (int index = 0; index < segmentCount; ++index) {
			const float angleA = static_cast<float>(index) * (2.0f * MathConstants::kPi / static_cast<float>(segmentCount));
			const float angleB = static_cast<float>(index + 1) * (2.0f * MathConstants::kPi / static_cast<float>(segmentCount));
			const Vector3 pointA = center + Vector3{std::cos(angleA) * radius, 0.0f, std::sin(angleA) * radius};
			const Vector3 pointB = center + Vector3{std::cos(angleB) * radius, 0.0f, std::sin(angleB) * radius};
			LineDrawer::GetInstance()->DrawLine(pointA, pointB, color, true);
		}
		if (target_) {
			Vector3 direction = target_->GetTransform().translate - center;
			direction.y = 0.0f;
			if (Length(direction) > MathConstants::kDirectionEpsilon) {
				LineDrawer::GetInstance()->DrawLine(center, center + 5.0f * Normalize(direction), color, true);
			}
		}
	}

	void ApplyStats(const EnemyStats& stats) {
		const bool isFirstStatsApplication = !hasAppliedStats_;
		const bool behaviorChanged = hasAppliedStats_ && stats_.behavior != stats.behavior;
		stats_ = stats;
		hasAppliedStats_ = true;
		if (behaviorChanged) {
			shootTimer_ = 0.0f;
			stateTimer_ = 0.0f;
			chargeState_ = ChargeState::Approach;
			pendingShotRequests_.clear();
		}
		if (isFirstStatsApplication || currentHealth_ <= 0.0f || currentHealth_ > stats_.health) {
			currentHealth_ = stats_.health;
		}
	}

	const EnemyStats& GetStats() const { return stats_; }
	void SetEnemyTypeName(const std::string& enemyTypeName) { enemyTypeName_ = enemyTypeName; }
	const std::string& GetEnemyTypeName() const { return enemyTypeName_; }
	void SetTarget(GameObject* target) { target_ = target; }
	GameObject* GetTarget() const { return target_; }
	void SetTargetName(const std::string& targetName) { targetName_ = targetName; }
	const std::string& GetTargetName() const { return targetName_; }
	void SetCurrentHealth(float health) { currentHealth_ = health < 0.0f ? 0.0f : health; }
	float GetCurrentHealth() const { return currentHealth_; }
	void SetRuntimeSpawned(bool runtimeSpawned) { runtimeSpawned_ = runtimeSpawned; }
	bool GetRuntimeSpawned() const { return runtimeSpawned_; }
	std::vector<EnemyShotRequest> ConsumeShotRequests() {
		std::vector<EnemyShotRequest> requests;
		requests.swap(pendingShotRequests_);
		return requests;
	}
	bool IsChargeWarningActive() const { return chargeState_ == ChargeState::Charging; }
	float GetChargeProgress() const {
		return stats_.chargeDuration > 0.0f ? (std::min)(1.0f, stateTimer_ / stats_.chargeDuration) : 1.0f;
	}

private:
	enum class ChargeState { Approach, Charging, Dashing, Recovering };

	void UpdateShooter(EulerTransform& transform, const Vector3& direction, float distance) {
		const float frameScale = GameTime::GetFrameScale60();
		const float minimumDistance = (std::max)(0.0f, stats_.preferredDistance - stats_.distanceTolerance);
		const float maximumDistance = stats_.preferredDistance + stats_.distanceTolerance;
		if (distance > maximumDistance) {
			transform.translate = transform.translate + (stats_.speed * frameScale) * direction;
		} else if (distance < minimumDistance) {
			transform.translate = transform.translate - (stats_.speed * frameScale) * direction;
		}

		if (stats_.shootingInterval <= 0.0f || distance <= MathConstants::kDirectionEpsilon || distance > maximumDistance) {
			return;
		}
		shootTimer_ += GameTime::GetDeltaTime();
		if (shootTimer_ >= stats_.shootingInterval) {
			shootTimer_ = 0.0f;
			pendingShotRequests_.push_back({
				transform.translate + Vector3{0.0f, 0.25f, 0.0f} + 0.65f * direction,
				direction,
				stats_.projectileSpeed,
				stats_.attack,
				stats_.projectileSize,
				stats_.projectileLifeTime
			});
		}
	}

	void UpdateCharger(EulerTransform& transform, const Vector3& direction, float distance) {
		const float deltaTime = GameTime::GetDeltaTime();
		const float frameScale = GameTime::GetFrameScale60();
		stateTimer_ += deltaTime;
		switch (chargeState_) {
		case ChargeState::Approach:
			if (distance <= stats_.chargeTriggerDistance) {
				chargeState_ = ChargeState::Charging;
				stateTimer_ = 0.0f;
			} else {
				transform.translate = transform.translate + (stats_.speed * frameScale) * direction;
			}
			break;
		case ChargeState::Charging:
			if (stateTimer_ >= stats_.chargeDuration) {
				dashDirection_ = direction;
				chargeState_ = ChargeState::Dashing;
				stateTimer_ = 0.0f;
			}
			break;
		case ChargeState::Dashing:
			transform.translate = transform.translate + (stats_.dashSpeed * frameScale) * dashDirection_;
			transform.rotate.y = std::atan2(dashDirection_.x, dashDirection_.z);
			if (stateTimer_ >= stats_.dashDuration) {
				chargeState_ = ChargeState::Recovering;
				stateTimer_ = 0.0f;
			}
			break;
		case ChargeState::Recovering:
			if (stateTimer_ >= stats_.dashRecovery) {
				chargeState_ = ChargeState::Approach;
				stateTimer_ = 0.0f;
			}
			break;
		}
	}

	std::string enemyTypeName_ = "Default";
	std::string targetName_;
	EnemyStats stats_;
	/// <summary>行動対象となるGameObjectへの非所有参照です。</summary>
	GameObject* target_ = nullptr;
	float currentHealth_ = 10.0f;
	float shootTimer_ = 0.0f;
	float stateTimer_ = 0.0f;
	Vector3 dashDirection_{0.0f, 0.0f, 1.0f};
	ChargeState chargeState_ = ChargeState::Approach;
	/// <summary>シーン側で敵弾へ変換される保留中の射撃要求です。</summary>
	std::vector<EnemyShotRequest> pendingShotRequests_;
	bool runtimeSpawned_ = false;
	bool hasAppliedStats_ = false;
};
