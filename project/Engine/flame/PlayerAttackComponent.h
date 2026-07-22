#pragma once
#include "Component.h"
#include "MathConstants.h"
#include "GameObject.h"
#include "../../Player/Player.h"
#include "Vector.h"
#include "../base/GameTime.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

/// <summary>攻撃1種類の特定レベルにおける発射・威力設定です。</summary>
struct PlayerAttackLevelStats {
	std::string level = "1";
	std::string choiceDescription;
	std::string choiceTextureFilePath;
	float attack = 100.0f;
	float speed = 0.3f;
	float size = 100.0f;
	int shotCount = 1;
	std::vector<float> angles = {0.0f};
	std::vector<Vector3> spawnOffsets = {{0.0f, 0.5f, 1.2f}};
	std::string modelFilePath = "sphere.obj";
	bool homing = false;
	float homingAccuracy = 1.0f;
	float attackInterval = 0.5f;
	float lifeTime = 3.0f;
	float travelDistance = 6.0f;
	int pierceCount = 0;
	bool infinitePierce = false;
};

/// <summary>攻撃名と、選択可能なレベル設定のまとまりです。</summary>
struct PlayerAttackStats {
	std::string name = "Straight";
	std::string choiceTextureFilePath;
	std::string superConditionStatusName;
	std::string superConditionStatusLevel = "1";
	std::vector<PlayerAttackLevelStats> levels;
};

/// <summary>プレイヤー弾の移動パターンです。</summary>
enum class PlayerProjectileMotionType {
	Linear,
	Orbit,
	SkyLaser,
	Boomerang,
	Ricochet,
	ClawSlash
};

/// <summary>シーン側で実体の弾へ変換する発射要求です。</summary>
struct PlayerAttackShotRequest {
	std::string attackName;
	std::string level;
	Vector3 position{};
	Vector3 direction{0.0f, 0.0f, 1.0f};
	float attack = 100.0f;
	float speed = 0.3f;
	float size = 1.0f;
	float lifeTime = 3.0f;
	int pierceCount = 0;
	bool infinitePierce = false;
	std::string modelFilePath = "sphere.obj";
	bool homing = false;
	float homingAccuracy = 1.0f;
	PlayerProjectileMotionType motionType = PlayerProjectileMotionType::Linear;
	GameObject* motionAnchor = nullptr;
	float orbitAngleRadians = 0.0f;
	float orbitRadius = 2.2f;
	float orbitHeight = 0.65f;
	float orbitAngularSpeed = 2.2f;
	float travelDistance = 6.0f;
	int clawSlashIndex = 0;
	int clawSlashCount = 3;
};

/// <summary>装備中の攻撃スロットを更新し、発射タイミングごとに弾生成要求を作ります。</summary>
class PlayerAttackComponent : public Component {
public:
	void Update() override {
		// 各攻撃スロットのクールダウンを進め、発射可能な攻撃を要求へ変換する。
		GameObject* owner = GetOwner();
		if (!owner) {
			return;
		}

		Player* player = owner->GetComponent<Player>();
		if (!player) {
			return;
		}

		const float playerAttackSpeedRate = (std::max)(0.01f, player->GetStats().attackSpeed / 100.0f);
		for (AttackSlotRuntime& slot : slots_) {
			if (!slot.enabled) {
				continue;
			}
			if (slot.attackTimer > 0.0f) {
				slot.attackTimer -= GameTime::GetDeltaTime();
			}
			if (slot.attackTimer > 0.0f) {
				continue;
			}
			const std::string currentLevel = slot.level;
			const PlayerAttackLevelStats levelStats = FindCurrentLevelStats(slot, currentLevel);
			CreateAttackByName(owner, *player, slot, levelStats, currentLevel);
			slot.attackTimer = levelStats.attackInterval / playerAttackSpeedRate;
		}
	}

	void ApplyAttackStats(const PlayerAttackStats& stats, const std::string& level) {
		ClearAttackSlots();
		AddAttackSlot(stats, level, true);
	}

	void ClearAttackSlots() {
		slots_.clear();
	}

	void AddAttackSlot(const PlayerAttackStats& stats, const std::string& level, bool enabled) {
		AttackSlotRuntime slot;
		slot.stats = stats;
		slot.level = NormalizeLevel(level);
		slot.enabled = enabled;
		slots_.push_back(slot);
	}
	void UpdateAttackStatsByName(const std::string& attackName, const PlayerAttackStats& stats) {
		for (AttackSlotRuntime& slot : slots_) {
			if (slot.stats.name == attackName) {
				slot.stats = stats;
			}
		}
	}

	const PlayerAttackStats& GetAttackStats() const { return slots_.empty() ? emptyStats_ : slots_.front().stats; }
	const std::string& GetAttackName() const { return slots_.empty() ? emptyStats_.name : slots_.front().stats.name; }
	const std::string& GetLevel() const { return slots_.empty() ? defaultLevel_ : slots_.front().level; }
	void SetLevel(const std::string& level) {
		if (!slots_.empty()) {
			slots_.front().level = NormalizeLevel(level);
		}
	}

	std::vector<PlayerAttackShotRequest> ConsumeShotRequests() {
		std::vector<PlayerAttackShotRequest> requests = shotRequests_;
		shotRequests_.clear();
		return requests;
	}

private:
	/// <summary>装備スロットごとのレベル、クールダウン、利用可否を保持します。</summary>
	struct AttackSlotRuntime {
		PlayerAttackStats stats;
		std::string level = "1";
		float attackTimer = 0.0f;
		bool enabled = true;
	};

	static std::string NormalizeLevel(const std::string& level) {
		if (level == "1" || level == "2" || level == "3" || level == "4" || level == "5" || level == "super") {
			return level;
		}
		return "1";
	}

	PlayerAttackLevelStats FindCurrentLevelStats(const AttackSlotRuntime& slot, const std::string& level) const {
		for (const PlayerAttackLevelStats& levelStats : slot.stats.levels) {
			if (levelStats.level == level) {
				return levelStats;
			}
		}
		if (!slot.stats.levels.empty()) {
			return slot.stats.levels.front();
		}
		return {};
	}

	static Vector3 RotateYaw(const Vector3& direction, float degrees) {
		const float radians = degrees * 3.14159265358979323846f / 180.0f;
		const float c = std::cos(radians);
		const float s = std::sin(radians);
		const Vector3 rotated = {direction.x * c + direction.z * s, 0.0f, -direction.x * s + direction.z * c};
		return NormalizeReturnVector(rotated);
	}

	static Vector3 GetShotSpawnOffset(const PlayerAttackLevelStats& levelStats, int shotIndex) {
		if (shotIndex >= 0 && shotIndex < static_cast<int>(levelStats.spawnOffsets.size())) {
			return levelStats.spawnOffsets[shotIndex];
		}
		if (!levelStats.spawnOffsets.empty()) {
			return levelStats.spawnOffsets.back();
		}
		return {0.0f, 0.5f, 1.2f};
	}

	void QueueShot(GameObject* owner, const Player& player, const AttackSlotRuntime& slot, const PlayerAttackLevelStats& levelStats, const std::string& currentLevel, float angleDegrees, int shotIndex) {
		const float playerAttackRate = player.GetStats().attack / 100.0f;
		const float playerAttackSizeRate = player.GetStats().attackSize / 100.0f;
		const Vector3 spawnOffset = GetShotSpawnOffset(levelStats, shotIndex);
		Vector3 forward = {
		    std::sin(owner->GetTransform().rotate.y),
		    0.0f,
		    std::cos(owner->GetTransform().rotate.y)
		};
		forward = NormalizeReturnVector(forward);
		const Vector3 right = {
		    std::cos(owner->GetTransform().rotate.y),
		    0.0f,
		    -std::sin(owner->GetTransform().rotate.y)
		};

		PlayerAttackShotRequest request;
		request.attackName = slot.stats.name;
		request.level = currentLevel;
		request.position = owner->GetTransform().translate +
		    spawnOffset.x * right +
		    Vector3{0.0f, spawnOffset.y, 0.0f} +
		    spawnOffset.z * forward;
		request.direction = RotateYaw(forward, angleDegrees);
		request.attack = levelStats.attack * playerAttackRate;
		request.speed = levelStats.speed;
		request.size = (levelStats.size / 100.0f) * playerAttackSizeRate;
		request.lifeTime = levelStats.lifeTime;
		request.pierceCount = levelStats.pierceCount;
		request.infinitePierce = levelStats.infinitePierce;
		request.modelFilePath = levelStats.modelFilePath;
		request.homing = levelStats.homing;
		request.homingAccuracy = levelStats.homingAccuracy;
		shotRequests_.push_back(request);
	}

	void CreateStraightAttack(GameObject* owner, const Player& player, const AttackSlotRuntime& slot, const PlayerAttackLevelStats& levelStats, const std::string& currentLevel) {
		QueueShot(owner, player, slot, levelStats, currentLevel, 0.0f, 0);
	}

	void CreateSpreadAttack(GameObject* owner, const Player& player, const AttackSlotRuntime& slot, const PlayerAttackLevelStats& levelStats, const std::string& currentLevel) {
		const int shotCount = (std::max)(1, levelStats.shotCount);
		for (int index = 0; index < shotCount; ++index) {
			const float angle = index < static_cast<int>(levelStats.angles.size()) ? levelStats.angles[index] : 0.0f;
			QueueShot(owner, player, slot, levelStats, currentLevel, angle, index);
		}
	}

	void CreateHomingAttack(GameObject* owner, const Player& player, const AttackSlotRuntime& slot, PlayerAttackLevelStats levelStats, const std::string& currentLevel) {
		levelStats.homing = true;
		CreateSpreadAttack(owner, player, slot, levelStats, currentLevel);
	}

	void CreateOrbitAttack(GameObject* owner, const Player& player, const AttackSlotRuntime& slot, const PlayerAttackLevelStats& levelStats, const std::string& currentLevel) {
		const int shotCount = (std::max)(1, levelStats.shotCount);
		constexpr float kTwoPi = 6.28318530717958647692f;
		for (int index = 0; index < shotCount; ++index) {
			QueueShot(owner, player, slot, levelStats, currentLevel, 0.0f, index);
			PlayerAttackShotRequest& request = shotRequests_.back();
			const Vector3 spawnOffset = GetShotSpawnOffset(levelStats, index);
			const float horizontalRadius = std::sqrt(
			    spawnOffset.x * spawnOffset.x + spawnOffset.z * spawnOffset.z);
			const float localStartAngle = horizontalRadius > MathConstants::kDirectionEpsilon
			    ? std::atan2(spawnOffset.z, spawnOffset.x)
			    : kTwoPi * static_cast<float>(index) / static_cast<float>(shotCount);
			request.motionType = PlayerProjectileMotionType::Orbit;
			request.motionAnchor = owner;
			request.orbitAngleRadians = localStartAngle - owner->GetTransform().rotate.y;
			request.orbitRadius = (std::max)(0.1f, horizontalRadius);
			request.orbitHeight = spawnOffset.y;
			request.orbitAngularSpeed = (std::max)(0.1f, levelStats.speed);
			request.speed = 0.0f;
		}
	}

	void CreateSkyLaserAttack(GameObject* owner, const Player& player, const AttackSlotRuntime& slot, const PlayerAttackLevelStats& levelStats, const std::string& currentLevel) {
		const int targetCount = (std::max)(1, levelStats.shotCount);
		for (int index = 0; index < targetCount; ++index) {
			QueueShot(owner, player, slot, levelStats, currentLevel, 0.0f, index);
			PlayerAttackShotRequest& request = shotRequests_.back();
			request.motionType = PlayerProjectileMotionType::SkyLaser;
			request.direction = {0.0f, -1.0f, 0.0f};
			request.speed = 0.0f;
		}
	}

	void CreateBoomerangAttack(GameObject* owner, const Player& player, const AttackSlotRuntime& slot, const PlayerAttackLevelStats& levelStats, const std::string& currentLevel) {
		QueueShot(owner, player, slot, levelStats, currentLevel, 0.0f, 0);
		PlayerAttackShotRequest& request = shotRequests_.back();
		request.motionType = PlayerProjectileMotionType::Boomerang;
		request.motionAnchor = owner;
		request.travelDistance = levelStats.travelDistance;
		request.homing = false;
	}

	void CreateRicochetAttack(GameObject* owner, const Player& player, const AttackSlotRuntime& slot, const PlayerAttackLevelStats& levelStats, const std::string& currentLevel) {
		const int shotCount = (std::max)(1, levelStats.shotCount);
		for (int index = 0; index < shotCount; ++index) {
			const float angle = index < static_cast<int>(levelStats.angles.size()) ? levelStats.angles[index] : 0.0f;
			QueueShot(owner, player, slot, levelStats, currentLevel, angle, index);
			PlayerAttackShotRequest& request = shotRequests_.back();
			request.motionType = PlayerProjectileMotionType::Ricochet;
			request.homing = false;
		}
	}

	void CreateClawSlashAttack(GameObject* owner, const Player& player, const AttackSlotRuntime& slot, const PlayerAttackLevelStats& levelStats, const std::string& currentLevel) {
		const int slashCount = (std::max)(3, levelStats.shotCount);
		for (int index = 0; index < slashCount; ++index) {
			QueueShot(owner, player, slot, levelStats, currentLevel, 0.0f, index);
			PlayerAttackShotRequest& request = shotRequests_.back();
			request.motionType = PlayerProjectileMotionType::ClawSlash;
			request.motionAnchor = owner;
			request.speed = 0.0f;
			request.homing = false;
			request.attack /= static_cast<float>(slashCount);
			request.clawSlashIndex = index;
			request.clawSlashCount = slashCount;
		}
	}

	void CreateAttackByName(GameObject* owner, const Player& player, const AttackSlotRuntime& slot, const PlayerAttackLevelStats& levelStats, const std::string& currentLevel) {
		if (slot.stats.name == "ClawSlash") {
			CreateClawSlashAttack(owner, player, slot, levelStats, currentLevel);
			return;
		}
		if (slot.stats.name == "Ricochet") {
			CreateRicochetAttack(owner, player, slot, levelStats, currentLevel);
			return;
		}
		if (slot.stats.name == "Boomerang") {
			CreateBoomerangAttack(owner, player, slot, levelStats, currentLevel);
			return;
		}
		if (slot.stats.name == "Orbit") {
			CreateOrbitAttack(owner, player, slot, levelStats, currentLevel);
			return;
		}
		if (slot.stats.name == "SkyLaser") {
			CreateSkyLaserAttack(owner, player, slot, levelStats, currentLevel);
			return;
		}
		if (levelStats.homing) {
			CreateHomingAttack(owner, player, slot, levelStats, currentLevel);
			return;
		}
		if (levelStats.shotCount > 1) {
			CreateSpreadAttack(owner, player, slot, levelStats, currentLevel);
			return;
		}
		CreateStraightAttack(owner, player, slot, levelStats, currentLevel);
	}

	PlayerAttackStats emptyStats_;
	std::string defaultLevel_ = "1";
	/// <summary>同時に更新する攻撃スロットの実行時状態です。</summary>
	std::vector<AttackSlotRuntime> slots_;
	/// <summary>次にシーンが回収する未処理の発射要求です。</summary>
	std::vector<PlayerAttackShotRequest> shotRequests_;
};
