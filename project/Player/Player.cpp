#include "Player.h"
#include "MathConstants.h"
#include "GameObject.h"
#include "Input.h"
#include "GameTime.h"
#include "object/Object3dComponent.h"
#include <cmath>
#include <limits>
#include <dinput.h>

namespace {
Vector3 MoveTowards(const Vector3& current, const Vector3& target, float maxDelta) {
	const Vector3 difference = target - current;
	const float distance = Length(difference);
	if (distance <= maxDelta || distance <= MathConstants::kNormalizationEpsilon) {
		return target;
	}

	const Vector3 direction = {difference.x / distance, difference.y / distance, difference.z / distance};
	return current + maxDelta * direction;
}
}

/// <summary>
/// 毎フレーム WASD 入力を見て、XZ 平面上でプレイヤーを移動します。
/// </summary>
void Player::Update() {
	const float deltaTime = GameTime::GetDeltaTime();
	const float frameScale = GameTime::GetFrameScale60();
	if (damageInvincibilityTimer_ > 0.0f) {
		damageInvincibilityTimer_ -= deltaTime;
		if (damageInvincibilityTimer_ < 0.0f) {
			damageInvincibilityTimer_ = 0.0f;
		}
	}

	GameObject* owner = GetOwner();
	if (!owner) {
		return;
	}

	Input* input = Input::GetInstance();
	Vector3 keyboardMove{0.0f, 0.0f, 0.0f};
	if (input->PushKey(DIK_W)) {
		keyboardMove.z += 1.0f;
	}
	if (input->PushKey(DIK_S)) {
		keyboardMove.z -= 1.0f;
	}
	if (input->PushKey(DIK_A)) {
		keyboardMove.x -= 1.0f;
	}
	if (input->PushKey(DIK_D)) {
		keyboardMove.x += 1.0f;
	}
	const float keyboardLength = Length(keyboardMove);
	if (keyboardLength > MathConstants::kNormalizationEpsilon) {
		keyboardMove = {keyboardMove.x / keyboardLength, 0.0f, keyboardMove.z / keyboardLength};
	}

	const Vector3 leftStick = input->GetGamepadLeftStick();
	Vector3 move{keyboardMove.x + leftStick.x, 0.0f, keyboardMove.z + leftStick.z};

	const float moveLength = Length(move);
	Vector3 targetVelocity{0.0f, 0.0f, 0.0f};
	const float effectiveMoveSpeed = GetEffectiveMoveSpeed();
	if (moveLength > MathConstants::kNormalizationEpsilon) {
		const Vector3 moveDirection = {move.x / moveLength, 0.0f, move.z / moveLength};
		const float moveAmount = moveLength > 1.0f ? 1.0f : moveLength;
		targetVelocity = (effectiveMoveSpeed * moveAmount) * moveDirection;
	}

	const bool hasMoveInput = Length(targetVelocity) > MathConstants::kNormalizationEpsilon;
	const float smoothingDelta = (hasMoveInput ? effectiveMoveSpeed * 0.18f : effectiveMoveSpeed * 0.14f) * frameScale;
	currentMoveVelocity_ = MoveTowards(currentMoveVelocity_, targetVelocity, smoothingDelta);

	const float currentSpeed = Length(currentMoveVelocity_);
	if (Object3dComponent* object3d = owner->GetComponent<Object3dComponent>()) {
		if (object3d->HasAnimation()) {
			object3d->SetAnimationPlaying(hasMoveInput || currentSpeed > 0.0005f);
		}
	}

	if (currentSpeed <= MathConstants::kNormalizationEpsilon) {
		return;
	}

	const Vector3 currentMoveDirection = {
	    currentMoveVelocity_.x / currentSpeed,
	    0.0f,
	    currentMoveVelocity_.z / currentSpeed
	};
	owner->GetTransform().rotate.y = std::atan2(currentMoveDirection.x, currentMoveDirection.z);
	owner->GetTransform().translate = owner->GetTransform().translate + frameScale * currentMoveVelocity_;
}

void Player::ApplyStats(const PlayerStats& stats) {
	ApplyStats(stats, stats);
}

void Player::ApplyStats(const PlayerStats& baseStats, const PlayerStats& effectiveStats) {
	stats_ = baseStats;
	effectiveStats_ = effectiveStats;
	effectiveStats_.level = stats_.level;
	effectiveStats_.experience = stats_.experience;
	moveSpeed_ = effectiveStats_.baseSpeed;
	SetModelFilePath(stats_.modelFilePath, stats_.isAnimationModel);
	const float maxHealth = GetMaxHealth();
	if (currentHealth_ <= 0.0f || currentHealth_ > maxHealth) {
		currentHealth_ = maxHealth;
	}
}

int Player::TakeDamage(float rawDamage) {
	if (rawDamage <= 0.0f || currentHealth_ <= 0.0f || damageInvincibilityTimer_ > 0.0f) {
		return 0;
	}

	const float damageRate = 1.0f - (effectiveStats_.defense * 0.01f);
	const float clampedRate = damageRate < 0.0f ? 0.0f : damageRate;
	const int damage = static_cast<int>(std::ceil(rawDamage * clampedRate));
	if (damage <= 0) {
		return 0;
	}

	SetCurrentHealth(currentHealth_ - static_cast<float>(damage));
	damageInvincibilityTimer_ = effectiveStats_.damageInvincibilityDuration;
	return damage;
}

void Player::AddExperience(int experience) {
	if (experience <= 0) {
		return;
	}
	const float correctionRate = effectiveStats_.experienceCorrection / 100.0f;
	const double correctedValue = std::ceil(static_cast<double>(experience) * static_cast<double>(correctionRate));
	const int maxExperience = (std::numeric_limits<int>::max)();
	const int correctedExperience = correctedValue >= static_cast<double>(maxExperience)
	                                    ? maxExperience
	                                    : static_cast<int>(correctedValue);
	if (correctedExperience <= 0) {
		return;
	}
	stats_.experience = correctedExperience > maxExperience - stats_.experience
	                        ? maxExperience
	                        : stats_.experience + correctedExperience;
	effectiveStats_.experience = stats_.experience;
	UpdateLevelFromExperience();
}

int Player::GetRequiredExperienceForNextLevel(int level) {
	const int safeLevel = level < 1 ? 1 : level;
	const long long maxExperience = static_cast<long long>((std::numeric_limits<int>::max)());
	long long requiredExperience = 0;
	long long levelExperience = 4;
	for (int currentLevel = 1; currentLevel <= safeLevel; ++currentLevel) {
		requiredExperience += levelExperience;
		if (requiredExperience >= maxExperience) {
			return static_cast<int>(maxExperience);
		}
		if (currentLevel == safeLevel) {
			return static_cast<int>(requiredExperience);
		}
		if (levelExperience > maxExperience / 2) {
			return static_cast<int>(maxExperience);
		}
		levelExperience *= 2;
	}
	return static_cast<int>(requiredExperience);
}

void Player::UpdateLevelFromExperience() {
	const int maxExperience = (std::numeric_limits<int>::max)();
	const int previousLevel = stats_.level;
	while (true) {
		const int requiredExperience = GetRequiredExperienceForNextLevel(stats_.level);
		if (requiredExperience == maxExperience || stats_.experience < requiredExperience) {
			break;
		}
		++stats_.level;
	}
	effectiveStats_.level = stats_.level;
	pendingLevelUpCount_ += stats_.level - previousLevel;
}

/// <summary>
/// 現在位置をスポーンポイントへ戻します。
/// </summary>
void Player::ResetToSpawnPoint() {
	GameObject* owner = GetOwner();
	if (!owner) {
		return;
	}

	owner->GetTransform().translate = spawnPoint_;
	currentMoveVelocity_ = {0.0f, 0.0f, 0.0f};
}
