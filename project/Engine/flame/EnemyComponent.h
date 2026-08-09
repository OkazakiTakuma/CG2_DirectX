#pragma once
#include "Component.h"
#include "MathConstants.h"
#include "GameObject.h"
#include "LineDrawer.h"
#include "EnemyProjectileComponent.h"
#include "../base/GameTime.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

/// <summary>敵が使用する移動・攻撃パターンです。</summary>
enum class EnemyBehaviorType {
	Chase,
	Shooter,
	Charger,
	NightSlashBoss,
	SelfDestruct,
	TornadoBoss
};

/// <summary>シーンへ引き渡す敵弾生成パラメーターです。</summary>
struct EnemyShotRequest {
	Vector3 position{};
	Vector3 direction{0.0f, 0.0f, 1.0f};
	float speed = 0.12f;
	float attack = 1.0f;
	float size = 0.22f;
	float lifeTime = 6.0f;
	EnemyProjectileMotionType motionType = EnemyProjectileMotionType::Linear;
	Vector3 orbitCenter{};
	float orbitAngle = 0.0f;
	float orbitInitialRadius = 0.0f;
	float orbitAngularSpeed = 0.0f;
	float orbitRadialSpeed = 0.0f;
	float orbitHeight = 0.0f;
	GameObject* homingTarget = nullptr;
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
	float healthItemDropChance = 0.05f;
	float collectExperienceItemDropChance = 0.05f;
	float healthItemHealAmount = 25.0f;
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
	float comboTriggerDistance = 8.0f;
	float comboWindup = 0.8f;
	float comboDashSpeed = 0.48f;
	float comboDashDuration = 0.24f;
	float comboSlashPause = 0.16f;
	float comboRecovery = 1.8f;
	float comboSideOffset = 1.8f;
	int comboDashCount = 4;
	float finisherSpeedMultiplier = 1.35f;
	float bossRangedWindup = 0.75f;
	float bossRangedInterval = 0.32f;
	int bossRangedWaves = 3;
	int bossRadialShotCount = 12;
	int bossAimedShotCount = 5;
	float bossAimedSpreadAngle = 0.22f;
	float bossProjectileAttackMultiplier = 0.65f;
	int bossTornadoCount = 4;
	float bossTornadoInitialRadius = 1.4f;
	float bossTornadoAngularSpeed = 2.4f;
	float bossTornadoRadialSpeed = 0.035f;
	float bossTornadoSize = 0.65f;
	float bossTornadoLifeTime = 5.0f;
	float bossTornadoAttackMultiplier = 0.8f;
	float bossTornadoTriggerDistance = 10.0f;
	float bossTornadoWindup = 1.0f;
	float bossTornadoRecovery = 2.0f;
	int bossConvergingTornadoCount = 8;
	float bossConvergingTornadoInitialRadius = 9.0f;
	float bossConvergingTornadoAngularSpeed = -2.0f;
	float bossConvergingTornadoRadialSpeed = 0.035f;
	float bossConvergingTornadoSize = 0.55f;
	float bossConvergingTornadoLifeTime = 4.0f;
	float bossConvergingTornadoAttackMultiplier = 0.55f;
	float bossGiantTornadoSpeed = 0.035f;
	float bossGiantTornadoSize = 2.2f;
	float bossGiantTornadoLifeTime = 7.0f;
	float bossGiantTornadoAttackMultiplier = 1.2f;
	float bossGiantTornadoSpawnOffset = 2.2f;
	float selfDestructTriggerDistance = 2.2f;
	float selfDestructFuseDuration = 0.8f;
	float selfDestructRadius = 3.0f;
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

		if (stats_.behavior == EnemyBehaviorType::TornadoBoss) {
			UpdateTornadoBoss(transform, normalized, distance);
		} else if (stats_.behavior == EnemyBehaviorType::NightSlashBoss) {
			UpdateNightSlashBoss(transform, normalized, distance);
		} else if (stats_.behavior == EnemyBehaviorType::SelfDestruct) {
			UpdateSelfDestruct(transform, normalized, distance);
		} else if (stats_.behavior == EnemyBehaviorType::Charger) {
			UpdateCharger(transform, normalized, distance);
		} else if (stats_.behavior == EnemyBehaviorType::Shooter || stats_.shoots) {
			UpdateShooter(transform, normalized, distance);
		} else if (distance > MathConstants::kDirectionEpsilon) {
			transform.translate = transform.translate + (stats_.speed * GameTime::GetFrameScale60()) * normalized;
		}
	}

	void Draw3D() override {
		if ((!IsChargeWarningActive() && !IsSelfDestructArmed() && !IsTornadoWarningActive() &&
		     !IsNightSlashWarningActive() && !IsNightSlashAttacking() &&
		     !IsBossRangedWarningActive() && !IsBossRangedAttacking()) ||
		    !GetOwner()) {
			return;
		}
		Vector3 center = GetOwner()->GetTransform().translate + Vector3{0.0f, 0.06f, 0.0f};
		const bool isNightSlash = stats_.behavior == EnemyBehaviorType::NightSlashBoss;
		const bool isSelfDestruct = stats_.behavior == EnemyBehaviorType::SelfDestruct;
		const bool isTornadoBoss = stats_.behavior == EnemyBehaviorType::TornadoBoss;
		if (isTornadoBoss && tornadoPatternIndex_ == 1 && target_) {
			center = target_->GetTransform().translate + Vector3{0.0f, 0.06f, 0.0f};
		}
		const bool isRangedPattern = IsBossRangedWarningActive() || IsBossRangedAttacking();
		const float progress = isRangedPattern
		    ? GetBossRangedProgress()
		    : isNightSlash ? GetNightSlashProgress()
		    : isSelfDestruct ? GetSelfDestructProgress()
		    : isTornadoBoss ? GetTornadoWarningProgress() : GetChargeProgress();
		const float radius = isNightSlash
		    ? 1.35f + 0.45f * progress
		    : isSelfDestruct ? stats_.selfDestructRadius * (0.75f + 0.25f * progress)
		    : isTornadoBoss ? (tornadoPatternIndex_ == 0
		        ? stats_.bossTornadoInitialRadius + 0.6f * progress
		        : tornadoPatternIndex_ == 1
		            ? stats_.bossConvergingTornadoInitialRadius
		            : stats_.bossGiantTornadoSize * (1.0f + 0.25f * progress))
		    : 0.9f + 0.35f * progress;
		const Vector4 color = isRangedPattern
			? Vector4{0.12f, 0.45f + 0.45f * progress, 1.0f, 1.0f}
			: isNightSlash ? Vector4{0.82f, 0.08f + 0.30f * progress, 1.0f, 1.0f}
			: isSelfDestruct ? Vector4{1.0f, 0.85f * (1.0f - progress), 0.02f, 1.0f}
			: isTornadoBoss ? (tornadoPatternIndex_ == 0
			    ? Vector4{0.20f, 0.75f + 0.25f * progress, 1.0f, 1.0f}
			    : tornadoPatternIndex_ == 1
			        ? Vector4{0.75f + 0.25f * progress, 0.25f, 1.0f, 1.0f}
			        : Vector4{0.20f, 1.0f, 0.40f + 0.35f * progress, 1.0f})
			: Vector4{1.0f, 0.05f + 0.25f * progress, 0.02f, 1.0f};
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
				if (isNightSlash && !isRangedPattern) {
					const Vector3 targetCenter = target_->GetTransform().translate + Vector3{0.0f, 0.08f, 0.0f};
					const Vector3 right{direction.z, 0.0f, -direction.x};
					const Vector3 normalizedRight = Length(right) > MathConstants::kDirectionEpsilon ? Normalize(right) : Vector3{1.0f, 0.0f, 0.0f};
					const float slashRadius = 1.25f + 0.35f * progress;
					LineDrawer::GetInstance()->DrawLine(
						targetCenter - slashRadius * normalizedRight - Vector3{0.0f, 0.0f, slashRadius},
						targetCenter + slashRadius * normalizedRight + Vector3{0.0f, 0.0f, slashRadius},
						color,
						true
					);
					LineDrawer::GetInstance()->DrawLine(
						targetCenter + slashRadius * normalizedRight - Vector3{0.0f, 0.0f, slashRadius},
						targetCenter - slashRadius * normalizedRight + Vector3{0.0f, 0.0f, slashRadius},
						color,
						true
					);
				}
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
			nightSlashState_ = NightSlashState::Approach;
			tornadoBossState_ = TornadoBossState::Approach;
			tornadoPatternIndex_ = 0;
			comboDashIndex_ = 0;
			bossPatternIndex_ = 0;
			rangedWaveIndex_ = 0;
			selfDestructArmed_ = false;
			selfDestructRequested_ = false;
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
	bool IsNightSlashWarningActive() const { return nightSlashState_ == NightSlashState::Windup; }
	bool IsSelfDestructArmed() const { return selfDestructArmed_; }
	float GetSelfDestructProgress() const {
		return stats_.selfDestructFuseDuration > 0.0f
		    ? (std::min)(1.0f, stateTimer_ / stats_.selfDestructFuseDuration)
		    : 1.0f;
	}
	bool IsTornadoWarningActive() const { return tornadoBossState_ == TornadoBossState::Windup; }
	int GetTornadoPatternIndex() const { return tornadoPatternIndex_; }
	float GetTornadoWarningProgress() const {
		return stats_.bossTornadoWindup > 0.0f
		    ? (std::min)(1.0f, stateTimer_ / stats_.bossTornadoWindup)
		    : 1.0f;
	}
	bool ConsumeSelfDestructRequest() {
		if (!selfDestructRequested_) {
			return false;
		}
		selfDestructRequested_ = false;
		currentHealth_ = 0.0f;
		return true;
	}
	bool IsBossRangedWarningActive() const { return nightSlashState_ == NightSlashState::RangedWindup; }
	bool IsBossRangedAttacking() const { return nightSlashState_ == NightSlashState::RangedFiring; }
	float GetBossRangedProgress() const {
		if (nightSlashState_ == NightSlashState::RangedWindup) {
			return stats_.bossRangedWindup > 0.0f
			    ? (std::min)(1.0f, stateTimer_ / stats_.bossRangedWindup)
			    : 1.0f;
		}
		return stats_.bossRangedWaves > 0
		    ? (std::min)(1.0f, static_cast<float>(rangedWaveIndex_ + 1) / static_cast<float>(stats_.bossRangedWaves))
		    : 1.0f;
	}
	bool IsNightSlashAttacking() const {
		return nightSlashState_ == NightSlashState::Dashing || nightSlashState_ == NightSlashState::Slashing;
	}
	float GetNightSlashProgress() const {
		if (nightSlashState_ == NightSlashState::Windup) {
			return stats_.comboWindup > 0.0f ? (std::min)(1.0f, stateTimer_ / stats_.comboWindup) : 1.0f;
		}
		return stats_.comboDashCount > 0
			? (std::min)(1.0f, static_cast<float>(comboDashIndex_ + 1) / static_cast<float>(stats_.comboDashCount))
			: 1.0f;
	}
	bool CanDealContactDamage() const {
		if (stats_.behavior == EnemyBehaviorType::SelfDestruct || stats_.behavior == EnemyBehaviorType::TornadoBoss) {
			return false;
		}
		return stats_.behavior != EnemyBehaviorType::NightSlashBoss || IsNightSlashAttacking();
	}
	float GetContactAttackDamage() const {
		const bool isNightSlashFinisher =
			stats_.behavior == EnemyBehaviorType::NightSlashBoss &&
			IsNightSlashAttacking() &&
			comboDashIndex_ + 1 >= (std::max)(1, stats_.comboDashCount);
		return stats_.attack * (isNightSlashFinisher ? 1.6f : 1.0f);
	}

private:
	enum class ChargeState { Approach, Charging, Dashing, Recovering };
	enum class NightSlashState { Approach, Windup, Dashing, Slashing, RangedWindup, RangedFiring, Recovering };
	enum class TornadoBossState { Approach, Windup, Recovering };

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

	void UpdateSelfDestruct(EulerTransform& transform, const Vector3& direction, float distance) {
		if (selfDestructRequested_) {
			return;
		}
		if (!selfDestructArmed_) {
			if (distance <= stats_.selfDestructTriggerDistance) {
				selfDestructArmed_ = true;
				stateTimer_ = 0.0f;
			} else if (distance > MathConstants::kDirectionEpsilon) {
				transform.translate = transform.translate + (stats_.speed * GameTime::GetFrameScale60()) * direction;
			}
			return;
		}

		stateTimer_ += GameTime::GetDeltaTime();
		if (stateTimer_ >= stats_.selfDestructFuseDuration) {
			selfDestructRequested_ = true;
		}
	}

	void UpdateTornadoBoss(EulerTransform& transform, const Vector3& direction, float distance) {
		stateTimer_ += GameTime::GetDeltaTime();
		switch (tornadoBossState_) {
		case TornadoBossState::Approach:
			if (distance <= stats_.bossTornadoTriggerDistance) {
				tornadoBossState_ = TornadoBossState::Windup;
				stateTimer_ = 0.0f;
			} else {
				transform.translate = transform.translate + (stats_.speed * GameTime::GetFrameScale60()) * direction;
			}
			break;
		case TornadoBossState::Windup:
			if (stateTimer_ >= stats_.bossTornadoWindup) {
				if (tornadoPatternIndex_ == 0) {
					EmitBossTornadoPattern(transform);
				} else if (tornadoPatternIndex_ == 1) {
					EmitBossConvergingTornadoPattern(transform);
				} else {
					EmitBossGiantTornadoPattern(transform);
				}
				tornadoPatternIndex_ = (tornadoPatternIndex_ + 1) % 3;
				tornadoBossState_ = TornadoBossState::Recovering;
				stateTimer_ = 0.0f;
			}
			break;
		case TornadoBossState::Recovering:
			if (stateTimer_ >= stats_.bossTornadoRecovery) {
				tornadoBossState_ = TornadoBossState::Approach;
				stateTimer_ = 0.0f;
			}
			break;
		}
	}

	void UpdateNightSlashBoss(EulerTransform& transform, const Vector3& direction, float distance) {
		const float deltaTime = GameTime::GetDeltaTime();
		const float frameScale = GameTime::GetFrameScale60();
		stateTimer_ += deltaTime;

		switch (nightSlashState_) {
		case NightSlashState::Approach:
			if (distance <= stats_.comboTriggerDistance) {
				nightSlashState_ = bossPatternIndex_ == 0
				    ? NightSlashState::Windup
				    : NightSlashState::RangedWindup;
				stateTimer_ = 0.0f;
				comboDashIndex_ = 0;
				rangedWaveIndex_ = 0;
			} else {
				transform.translate = transform.translate + (stats_.speed * frameScale) * direction;
			}
			break;
		case NightSlashState::Windup:
			if (stateTimer_ >= stats_.comboWindup) {
				BeginNightSlashDash(transform);
			}
			break;
		case NightSlashState::Dashing: {
			const bool isFinisher = comboDashIndex_ + 1 >= (std::max)(1, stats_.comboDashCount);
			const float speedMultiplier = isFinisher ? stats_.finisherSpeedMultiplier : 1.0f;
			transform.translate = transform.translate + (stats_.comboDashSpeed * speedMultiplier * frameScale) * dashDirection_;
			transform.rotate.y = std::atan2(dashDirection_.x, dashDirection_.z);
			if (stateTimer_ >= stats_.comboDashDuration) {
				nightSlashState_ = NightSlashState::Slashing;
				stateTimer_ = 0.0f;
			}
			break;
		}
		case NightSlashState::Slashing:
			if (stateTimer_ >= stats_.comboSlashPause) {
				++comboDashIndex_;
				if (comboDashIndex_ >= (std::max)(1, stats_.comboDashCount)) {
					nightSlashState_ = NightSlashState::Recovering;
					stateTimer_ = 0.0f;
				} else {
					BeginNightSlashDash(transform);
				}
			}
			break;
		case NightSlashState::RangedWindup:
			if (stateTimer_ >= stats_.bossRangedWindup) {
				nightSlashState_ = NightSlashState::RangedFiring;
				stateTimer_ = 0.0f;
				rangedWaveIndex_ = 0;
				EmitBossRangedWave(transform);
			}
			break;
		case NightSlashState::RangedFiring:
			if (stateTimer_ >= stats_.bossRangedInterval) {
				stateTimer_ = 0.0f;
				++rangedWaveIndex_;
				if (rangedWaveIndex_ >= (std::max)(1, stats_.bossRangedWaves)) {
					nightSlashState_ = NightSlashState::Recovering;
				} else {
					EmitBossRangedWave(transform);
				}
			}
			break;
		case NightSlashState::Recovering:
			if (stateTimer_ >= stats_.comboRecovery) {
				nightSlashState_ = NightSlashState::Approach;
				stateTimer_ = 0.0f;
				bossPatternIndex_ = (bossPatternIndex_ + 1) % 3;
			}
			break;
		}
	}

	void EmitBossRangedWave(const EulerTransform& transform) {
		const Vector3 shotPosition = transform.translate + Vector3{0.0f, 0.35f, 0.0f};
		const float shotAttack = stats_.attack * stats_.bossProjectileAttackMultiplier;
		if (bossPatternIndex_ == 1) {
			const int shotCount = (std::max)(1, stats_.bossRadialShotCount);
			const float waveRotation = static_cast<float>(rangedWaveIndex_) * 0.16f;
			for (int index = 0; index < shotCount; ++index) {
				const float angle =
				    (static_cast<float>(index) / static_cast<float>(shotCount)) * 2.0f * MathConstants::kPi + waveRotation;
				const Vector3 direction{std::sin(angle), 0.0f, std::cos(angle)};
				pendingShotRequests_.push_back({
					shotPosition, direction, stats_.projectileSpeed, shotAttack,
					stats_.projectileSize, stats_.projectileLifeTime
				});
			}
			return;
		}

		Vector3 forward = target_->GetTransform().translate - transform.translate;
		forward.y = 0.0f;
		forward = Length(forward) > MathConstants::kDirectionEpsilon
		    ? Normalize(forward)
		    : Vector3{0.0f, 0.0f, 1.0f};
		const int shotCount = (std::max)(1, stats_.bossAimedShotCount);
		const float centerIndex = static_cast<float>(shotCount - 1) * 0.5f;
		for (int index = 0; index < shotCount; ++index) {
			const float angle = (static_cast<float>(index) - centerIndex) * stats_.bossAimedSpreadAngle;
			const float sine = std::sin(angle);
			const float cosine = std::cos(angle);
			const Vector3 direction{
				forward.x * cosine + forward.z * sine,
				0.0f,
				forward.z * cosine - forward.x * sine
			};
			pendingShotRequests_.push_back({
				shotPosition, direction, stats_.projectileSpeed, shotAttack,
				stats_.projectileSize, stats_.projectileLifeTime
			});
		}
	}

	void EmitBossTornadoPattern(const EulerTransform& transform) {
		const int tornadoCount = (std::max)(1, stats_.bossTornadoCount);
		for (int index = 0; index < tornadoCount; ++index) {
			const float angle =
			    (static_cast<float>(index) / static_cast<float>(tornadoCount)) * 2.0f * MathConstants::kPi;
			EnemyShotRequest request{};
			request.position = transform.translate + Vector3{
				std::sin(angle) * stats_.bossTornadoInitialRadius,
				0.35f,
				std::cos(angle) * stats_.bossTornadoInitialRadius
			};
			request.direction = {std::cos(angle), 0.0f, -std::sin(angle)};
			request.attack = stats_.attack * stats_.bossTornadoAttackMultiplier;
			request.size = stats_.bossTornadoSize;
			request.lifeTime = stats_.bossTornadoLifeTime;
			request.motionType = EnemyProjectileMotionType::ExpandingOrbit;
			request.orbitCenter = transform.translate;
			request.orbitAngle = angle;
			request.orbitInitialRadius = stats_.bossTornadoInitialRadius;
			request.orbitAngularSpeed = stats_.bossTornadoAngularSpeed;
			request.orbitRadialSpeed = stats_.bossTornadoRadialSpeed;
			request.orbitHeight = 0.35f;
			pendingShotRequests_.push_back(request);
		}
	}

	void EmitBossConvergingTornadoPattern(const EulerTransform& transform) {
		const Vector3 center = target_ ? target_->GetTransform().translate : transform.translate;
		const int tornadoCount = (std::max)(1, stats_.bossConvergingTornadoCount);
		for (int index = 0; index < tornadoCount; ++index) {
			const float angle =
			    (static_cast<float>(index) / static_cast<float>(tornadoCount)) * 2.0f * MathConstants::kPi;
			EnemyShotRequest request{};
			request.position = center + Vector3{
				std::sin(angle) * stats_.bossConvergingTornadoInitialRadius,
				0.35f,
				std::cos(angle) * stats_.bossConvergingTornadoInitialRadius
			};
			request.direction = {-std::cos(angle), 0.0f, std::sin(angle)};
			request.attack = stats_.attack * stats_.bossConvergingTornadoAttackMultiplier;
			request.size = stats_.bossConvergingTornadoSize;
			request.lifeTime = stats_.bossConvergingTornadoLifeTime;
			request.motionType = EnemyProjectileMotionType::ContractingOrbit;
			request.orbitCenter = center;
			request.orbitAngle = angle;
			request.orbitInitialRadius = stats_.bossConvergingTornadoInitialRadius;
			request.orbitAngularSpeed = stats_.bossConvergingTornadoAngularSpeed;
			request.orbitRadialSpeed = stats_.bossConvergingTornadoRadialSpeed;
			request.orbitHeight = 0.35f;
			pendingShotRequests_.push_back(request);
		}
	}

	void EmitBossGiantTornadoPattern(const EulerTransform& transform) {
		Vector3 direction = target_ ? target_->GetTransform().translate - transform.translate : Vector3{0.0f, 0.0f, 1.0f};
		direction.y = 0.0f;
		direction = Length(direction) > MathConstants::kDirectionEpsilon
		    ? Normalize(direction)
		    : Vector3{0.0f, 0.0f, 1.0f};
		EnemyShotRequest request{};
		request.position = transform.translate + stats_.bossGiantTornadoSpawnOffset * direction + Vector3{0.0f, 0.5f, 0.0f};
		request.direction = direction;
		request.speed = stats_.bossGiantTornadoSpeed;
		request.attack = stats_.attack * stats_.bossGiantTornadoAttackMultiplier;
		request.size = stats_.bossGiantTornadoSize;
		request.lifeTime = stats_.bossGiantTornadoLifeTime;
		request.motionType = EnemyProjectileMotionType::Homing;
		request.homingTarget = target_;
		pendingShotRequests_.push_back(request);
	}

	void BeginNightSlashDash(const EulerTransform& transform) {
		Vector3 toTarget = target_->GetTransform().translate - transform.translate;
		toTarget.y = 0.0f;
		const Vector3 forward = Length(toTarget) > MathConstants::kDirectionEpsilon
			? Normalize(toTarget)
			: Vector3{0.0f, 0.0f, 1.0f};
		const Vector3 right{forward.z, 0.0f, -forward.x};
		const float side = comboDashIndex_ % 2 == 0 ? 1.0f : -1.0f;
		Vector3 dashTarget = target_->GetTransform().translate + side * stats_.comboSideOffset * right;
		if (comboDashIndex_ + 1 >= (std::max)(1, stats_.comboDashCount)) {
			dashTarget = target_->GetTransform().translate + 1.2f * forward;
		}
		Vector3 dashVector = dashTarget - transform.translate;
		dashVector.y = 0.0f;
		dashDirection_ = Length(dashVector) > MathConstants::kDirectionEpsilon ? Normalize(dashVector) : forward;
		nightSlashState_ = NightSlashState::Dashing;
		stateTimer_ = 0.0f;
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
	NightSlashState nightSlashState_ = NightSlashState::Approach;
	TornadoBossState tornadoBossState_ = TornadoBossState::Approach;
	int tornadoPatternIndex_ = 0;
	int comboDashIndex_ = 0;
	int bossPatternIndex_ = 0;
	int rangedWaveIndex_ = 0;
	bool selfDestructArmed_ = false;
	bool selfDestructRequested_ = false;
	/// <summary>シーン側で敵弾へ変換される保留中の射撃要求です。</summary>
	std::vector<EnemyShotRequest> pendingShotRequests_;
	bool runtimeSpawned_ = false;
	bool hasAppliedStats_ = false;
};
