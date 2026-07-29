#pragma once
#include "../Engine/flame/Component.h"
#include "Vector.h"
#include <array>
#include <cmath>
#include <memory>
#include <string>

class Object3d;

struct PlayerAttackSlot {
	bool enabled = false;
	std::string attackName;
	std::string attackLevel = "1";
};

struct PlayerStatusSlot {
	bool enabled = false;
	std::string statusName;
	std::string level = "1";
};

struct PlayerStats {
	std::string name = "Default";
	float baseHealth = 100.0f;
	float health = 100.0f;
	float attack = 100.0f;
	float defense = 0.0f;
	float baseSpeed = 0.1f;
	float speed = 100.0f;
	float attackSpeed = 100.0f;
	float attackSize = 100.0f;
	float damageInvincibilityDuration = 1.0f;
	int level = 1;
	int experience = 0;
	float experienceCorrection = 100.0f;
	std::string modelFilePath;
	bool isAnimationModel = false;
	std::string initialAttackName = "Straight";
	std::string initialAttackLevel = "1";
	std::array<PlayerAttackSlot, 5> attackSlots{};
	std::array<PlayerStatusSlot, 5> statusSlots{};
};

class Player : public Component {
public:
	~Player() override;

	/// <summary>
	/// 毎フレーム WASD 入力を見て、XZ 平面上でプレイヤーを移動します。
	/// </summary>
	void Update() override;
	void Draw3D() override;
	void Finalize() override;

	/// <summary>
	/// 現在位置をスポーンポイントへ戻します。
	/// </summary>
	void ResetToSpawnPoint();
	void ApplyStats(const PlayerStats& stats);
	void ApplyStats(const PlayerStats& baseStats, const PlayerStats& effectiveStats);
	int TakeDamage(float rawDamage);
	float GetMaxHealth() const { return effectiveStats_.baseHealth * (effectiveStats_.health / 100.0f); }
	float GetEffectiveMoveSpeed() const { return effectiveStats_.baseSpeed * (effectiveStats_.speed / 100.0f); }
	const PlayerStats& GetStats() const { return effectiveStats_; }
	const PlayerStats& GetBaseStats() const { return stats_; }
	void AddExperience(int experience);
	bool ConsumePendingLevelUp() {
		if (pendingLevelUpCount_ <= 0) {
			return false;
		}
		--pendingLevelUpCount_;
		return true;
	}
	int GetPendingLevelUpCount() const { return pendingLevelUpCount_; }
	static int GetRequiredExperienceForNextLevel(int level);
	void SetExperience(int experience) {
		const int clampedExperience = experience < 0 ? 0 : experience;
		stats_.experience = clampedExperience;
		effectiveStats_.experience = clampedExperience;
	}
	void SetLevel(int level) {
		const int clampedLevel = level < 1 ? 1 : level;
		stats_.level = clampedLevel;
		effectiveStats_.level = clampedLevel;
	}
	void SetPlayerTypeName(const std::string& playerTypeName) { playerTypeName_ = playerTypeName.empty() ? "Default" : playerTypeName; }
	const std::string& GetPlayerTypeName() const { return playerTypeName_; }
	void SetCurrentHealth(float currentHealth) {
		const float maxHealth = GetMaxHealth();
		currentHealth_ = currentHealth < 0.0f ? 0.0f : (currentHealth > maxHealth ? maxHealth : currentHealth);
	}
	float GetCurrentHealth() const { return currentHealth_; }

	/// <summary>
	/// スポーンポイントを設定します。
	/// </summary>
	/// <param name="spawnPoint">リロード時やリセット時に戻す位置を指定します。</param>
	void SetSpawnPoint(const Vector3& spawnPoint) { spawnPoint_ = spawnPoint; }
	const Vector3& GetSpawnPoint() const { return spawnPoint_; }

	/// <summary>
	/// 1 フレームあたりの移動速度を設定します。
	/// </summary>
	/// <param name="moveSpeed">WASD 入力時に加算する移動量を指定します。</param>
	void SetMoveSpeed(float moveSpeed) { moveSpeed_ = moveSpeed < 0.0f ? 0.0f : moveSpeed; }
	float GetMoveSpeed() const { return moveSpeed_; }

	/// <summary>
	/// プレイヤー表示に使用するモデル情報を設定します。
	/// </summary>
	/// <param name="modelFilePath">ModelManager に読み込まれているモデル名を指定します。</param>
	/// <param name="isAnimationModel">アニメーションモデルの場合 true を指定します。</param>
	void SetModelFilePath(const std::string& modelFilePath, bool isAnimationModel) {
		modelFilePath_ = modelFilePath;
		isAnimationModel_ = isAnimationModel;
	}
	const std::string& GetModelFilePath() const { return modelFilePath_; }
	bool GetIsAnimationModel() const { return isAnimationModel_; }

private:
	void EnsureBladeEquipment();
	void UpdateLevelFromExperience();
	void UpdateHealthRegeneration(float deltaTime);
	Vector3 spawnPoint_{0.0f, 0.0f, 0.0f};
	Vector3 currentMoveVelocity_{0.0f, 0.0f, 0.0f};
	float moveSpeed_ = 0.1f;
	float currentHealth_ = 100.0f;
	float damageInvincibilityTimer_ = 0.0f;
	float healthRegenerationTimer_ = 0.0f;
	PlayerStats stats_;
	PlayerStats effectiveStats_;
	std::string playerTypeName_ = "Default";
	std::string modelFilePath_;
	bool isAnimationModel_ = false;
	int pendingLevelUpCount_ = 0;
	std::unique_ptr<Object3d> bladeObject_;
};
