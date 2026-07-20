#pragma once
#include "Component.h"
#include "MathConstants.h"
#include "GameObject.h"
#include "../base/GameTime.h"
#include <cmath>
#include <string>

struct EnemyStats {
	float health = 10.0f;
	float attack = 1.0f;
	float speed = 0.05f;
	float shootingInterval = 1.0f;
	float spawnsPerMinute = 12.0f;
	int experience = 1;
	std::string experienceModelFilePath = "sphere.obj";
	bool shoots = false;
};

class EnemyComponent : public Component {
public:
	void Initialize() override {
		currentHealth_ = stats_.health;
	}

	void Update() override {
		GameObject* owner = GetOwner();
		if (!owner || !target_) {
			return;
		}

		EulerTransform& transform = owner->GetTransform();
		const Vector3 targetPosition = target_->GetTransform().translate;
		Vector3 direction = targetPosition - transform.translate;
		direction.y = 0.0f;
		const float distance = Length(direction);
		if (distance > MathConstants::kDirectionEpsilon) {
			const Vector3 normalized = Normalize(direction);
			transform.translate = transform.translate + (stats_.speed * GameTime::GetFrameScale60()) * normalized;
			transform.rotate.y = std::atan2(normalized.x, normalized.z);
		}

		if (stats_.shoots && stats_.shootingInterval > 0.0f) {
			shootTimer_ += GameTime::GetDeltaTime();
			if (shootTimer_ >= stats_.shootingInterval) {
				shootTimer_ = 0.0f;
			}
		}
	}

	void ApplyStats(const EnemyStats& stats) {
		const bool isFirstStatsApplication = !hasAppliedStats_;
		stats_ = stats;
		hasAppliedStats_ = true;
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

private:
	std::string enemyTypeName_ = "Default";
	std::string targetName_;
	EnemyStats stats_;
	GameObject* target_ = nullptr;
	float currentHealth_ = 10.0f;
	float shootTimer_ = 0.0f;
	bool runtimeSpawned_ = false;
	bool hasAppliedStats_ = false;
};
