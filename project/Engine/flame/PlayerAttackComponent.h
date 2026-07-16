#pragma once
#include "Component.h"
#include "GameObject.h"
#include "../../Player/Player.h"
#include "Vector.h"
#include "../base/GameTime.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

struct PlayerAttackLevelStats {
	std::string level = "1";
	float attack = 100.0f;
	float speed = 0.3f;
	float size = 100.0f;
	int shotCount = 1;
	std::vector<float> angles = {0.0f};
	std::string modelFilePath = "sphere.obj";
	bool homing = false;
	float homingAccuracy = 1.0f;
	float attackInterval = 0.5f;
	float lifeTime = 3.0f;
	int pierceCount = 0;
	bool infinitePierce = false;
};

struct PlayerAttackStats {
	std::string name = "Straight";
	std::string superConditionStatusName;
	std::string superConditionStatusLevel = "1";
	std::vector<PlayerAttackLevelStats> levels;
};

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
};

class PlayerAttackComponent : public Component {
public:
	void Update() override {
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

	void QueueShot(GameObject* owner, const Player& player, const AttackSlotRuntime& slot, const PlayerAttackLevelStats& levelStats, const std::string& currentLevel, float angleDegrees) {
		const float playerAttackRate = player.GetStats().attack / 100.0f;
		const float playerAttackSizeRate = player.GetStats().attackSize / 100.0f;
		Vector3 forward = {
		    std::sin(owner->GetTransform().rotate.y),
		    0.0f,
		    std::cos(owner->GetTransform().rotate.y)
		};
		forward = NormalizeReturnVector(forward);

		PlayerAttackShotRequest request;
		request.attackName = slot.stats.name;
		request.level = currentLevel;
		request.position = owner->GetTransform().translate + 1.2f * forward;
		request.position.y += 0.5f;
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
		QueueShot(owner, player, slot, levelStats, currentLevel, 0.0f);
	}

	void CreateSpreadAttack(GameObject* owner, const Player& player, const AttackSlotRuntime& slot, const PlayerAttackLevelStats& levelStats, const std::string& currentLevel) {
		const int shotCount = (std::max)(1, levelStats.shotCount);
		for (int index = 0; index < shotCount; ++index) {
			const float angle = index < static_cast<int>(levelStats.angles.size()) ? levelStats.angles[index] : 0.0f;
			QueueShot(owner, player, slot, levelStats, currentLevel, angle);
		}
	}

	void CreateHomingAttack(GameObject* owner, const Player& player, const AttackSlotRuntime& slot, PlayerAttackLevelStats levelStats, const std::string& currentLevel) {
		levelStats.homing = true;
		CreateSpreadAttack(owner, player, slot, levelStats, currentLevel);
	}

	void CreateAttackByName(GameObject* owner, const Player& player, const AttackSlotRuntime& slot, const PlayerAttackLevelStats& levelStats, const std::string& currentLevel) {
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
	std::vector<AttackSlotRuntime> slots_;
	std::vector<PlayerAttackShotRequest> shotRequests_;
};
