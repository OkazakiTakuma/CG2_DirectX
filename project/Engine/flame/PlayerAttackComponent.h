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
	/// <summary>JSON内で識別するレベル名です。通常は1～5またはsuperを指定します。</summary>
	std::string level = "1";
	/// <summary>レベルアップ選択画面に表示する説明文です。</summary>
	std::string choiceDescription;
	/// <summary>選択肢カードに表示する画像パスです。</summary>
	std::string choiceTextureFilePath;
	/// <summary>このレベルの基礎攻撃力です。プレイヤーの攻撃倍率を掛けて弾へ渡します。</summary>
	float attack = 100.0f;
	/// <summary>60FPS時の1フレームあたりの移動量、または特殊攻撃の速度パラメーターです。</summary>
	float speed = 0.3f;
	/// <summary>100を基準とした弾の表示・判定サイズです。</summary>
	float size = 100.0f;
	/// <summary>1回の発射で生成する弾数です。</summary>
	int shotCount = 1;
	/// <summary>弾番号と同じ添字に対応する、プレイヤー前方基準の発射角度です。</summary>
	std::vector<float> angles = {0.0f};
	/// <summary>弾ごとの発射位置です。X=右、Y=上、Z=前のプレイヤーローカル座標で指定します。</summary>
	std::vector<Vector3> spawnOffsets = {{0.0f, 0.5f, 1.2f}};
	/// <summary>弾の表示に使用するモデルファイル名です。</summary>
	std::string modelFilePath = "sphere.obj";
	/// <summary>trueの場合、生成後に最寄りの敵へ追尾対象を設定します。</summary>
	bool homing = false;
	/// <summary>0～1で表す追尾の曲がりやすさです。</summary>
	float homingAccuracy = 1.0f;
	/// <summary>次にこの攻撃を発射できるまでの基本秒数です。</summary>
	float attackInterval = 0.5f;
	/// <summary>弾が自動消滅するまでの秒数です。</summary>
	float lifeTime = 3.0f;
	/// <summary>ブーメランなど、距離で挙動を切り替える攻撃に使用する移動距離です。</summary>
	float travelDistance = 6.0f;
	/// <summary>何体まで貫通できるかを指定します。0なら命中時に消滅します。</summary>
	int pierceCount = 0;
	/// <summary>trueの場合、命中しても貫通回数を消費せず寿命まで残ります。</summary>
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
	// 発射方向へ一度膨らんでから、時間経過で追尾力を強めて敵へ収束する弾。
	ArcHoming,
	Orbit,
	SkyLaser,
	Boomerang,
	Ricochet,
	ClawSlash
};

/// <summary>シーン側で実体の弾へ変換する発射要求です。</summary>
struct PlayerAttackShotRequest {
	/// <summary>生成元の攻撃名です。見た目や特殊処理の分岐に使用します。</summary>
	std::string attackName;
	/// <summary>生成元の攻撃レベルです。</summary>
	std::string level;
	/// <summary>弾を生成するワールド座標です。</summary>
	Vector3 position{};
	/// <summary>弾の初期進行方向です。</summary>
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
	/// <summary>周回弾や爪攻撃の基準にするGameObjectです。所有権は持ちません。</summary>
	GameObject* motionAnchor = nullptr;
	/// <summary>周回弾の現在角度です。</summary>
	float orbitAngleRadians = 0.0f;
	/// <summary>周回弾が基準点から離れる水平半径です。</summary>
	float orbitRadius = 2.2f;
	/// <summary>周回弾の基準点からの高さです。</summary>
	float orbitHeight = 0.65f;
	/// <summary>周回弾の角速度です。</summary>
	float orbitAngularSpeed = 2.2f;
	float travelDistance = 6.0f;
	int clawSlashIndex = 0;
	int clawSlashCount = 3;
	// アークホーミングの弾本体・トレイル・発光へ同じ6色を割り当てる番号。
	int colorIndex = 0;
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
		// 弾番号に対応する値を優先し、不足時は末尾値を複製したものとして扱う。
		if (shotIndex >= 0 && shotIndex < static_cast<int>(levelStats.spawnOffsets.size())) {
			return levelStats.spawnOffsets[shotIndex];
		}
		if (!levelStats.spawnOffsets.empty()) {
			return levelStats.spawnOffsets.back();
		}
		// データが空でも従来と同じ「少し前方・上方」から発射できる安全値を返す。
		return {0.0f, 0.5f, 1.2f};
	}

	void QueueShot(GameObject* owner, const Player& player, const AttackSlotRuntime& slot, const PlayerAttackLevelStats& levelStats, const std::string& currentLevel, float angleDegrees, int shotIndex, bool alignSpawnToShotAngle = false) {
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
		const Vector3 shotDirection = RotateYaw(forward, angleDegrees);
		const Vector3 shotRight = {shotDirection.z, 0.0f, -shotDirection.x};
		// アークホーミングでは発射角ごとに銃口位置も回し、複数弾が同一点から重ならないようにする。
		const Vector3& spawnForward = alignSpawnToShotAngle ? shotDirection : forward;
		const Vector3& spawnRight = alignSpawnToShotAngle ? shotRight : right;

		PlayerAttackShotRequest request;
		request.attackName = slot.stats.name;
		request.level = currentLevel;
		// ローカルオフセットをプレイヤーの右・上・前ベクトルへ分解してワールド座標へ変換する。
		request.position = owner->GetTransform().translate +
		    spawnOffset.x * spawnRight +
		    Vector3{0.0f, spawnOffset.y, 0.0f} +
		    spawnOffset.z * spawnForward;
		request.direction = shotDirection;
		request.attack = levelStats.attack * playerAttackRate;
		request.speed = levelStats.speed;
		request.size = (levelStats.size / 100.0f) * playerAttackSizeRate;
		request.lifeTime = levelStats.lifeTime;
		request.pierceCount = levelStats.pierceCount;
		request.infinitePierce = levelStats.infinitePierce;
		request.modelFilePath = levelStats.modelFilePath;
		request.homing = levelStats.homing;
		request.homingAccuracy = levelStats.homingAccuracy;
		request.colorIndex = shotIndex % 6;
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

	void CreateArcHomingAttack(GameObject* owner, const Player& player, const AttackSlotRuntime& slot, PlayerAttackLevelStats levelStats, const std::string& currentLevel) {
		levelStats.homing = true;
		const int shotCount = (std::max)(1, levelStats.shotCount);
		for (int index = 0; index < shotCount; ++index) {
			// JSONの角度配列により、3発=120度、4発=90度、6発=60度間隔で全周へ発射する。
			const float angle = index < static_cast<int>(levelStats.angles.size()) ? levelStats.angles[index] : 0.0f;
			QueueShot(owner, player, slot, levelStats, currentLevel, angle, index, true);
			PlayerAttackShotRequest& request = shotRequests_.back();
			request.motionType = PlayerProjectileMotionType::ArcHoming;
		}
	}

	void CreateOrbitAttack(GameObject* owner, const Player& player, const AttackSlotRuntime& slot, const PlayerAttackLevelStats& levelStats, const std::string& currentLevel) {
		// 弾数分を円周上へ等間隔に配置し、以後の位置更新に使う中心・角度・半径を要求へ記録する。
		const int shotCount = (std::max)(1, levelStats.shotCount);
		constexpr float kTwoPi = 6.28318530717958647692f;
		for (int index = 0; index < shotCount; ++index) {
			QueueShot(owner, player, slot, levelStats, currentLevel, 0.0f, index);
			PlayerAttackShotRequest& request = shotRequests_.back();
			const Vector3 spawnOffset = GetShotSpawnOffset(levelStats, index);
			const float horizontalRadius = std::sqrt(
			    spawnOffset.x * spawnOffset.x + spawnOffset.z * spawnOffset.z);
			// JSONに個別オフセットがあればその角度を優先し、未設定時は自動的に等間隔へ並べる。
			const float localStartAngle = horizontalRadius > MathConstants::kDirectionEpsilon
			    ? std::atan2(spawnOffset.z, spawnOffset.x)
			    : kTwoPi * static_cast<float>(index) / static_cast<float>(shotCount);
			request.motionType = PlayerProjectileMotionType::Orbit;
			request.motionAnchor = owner;
			// プレイヤーの現在回転を除き、保存したローカル開始角度をワールド周回角へ変換する。
			request.orbitAngleRadians = localStartAngle - owner->GetTransform().rotate.y;
			request.orbitRadius = (std::max)(0.1f, horizontalRadius);
			request.orbitHeight = spawnOffset.y;
			request.orbitAngularSpeed = (std::max)(0.1f, levelStats.speed);
			request.speed = 0.0f;
		}
	}

	void CreateSkyLaserAttack(GameObject* owner, const Player& player, const AttackSlotRuntime& slot, const PlayerAttackLevelStats& levelStats, const std::string& currentLevel) {
		// 実際の敵座標への置き換えは、画面内の敵を列挙できるBaseScene側で行う。
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
		// 発射時の進行方向はBaseSceneで最寄りの敵へ向けるが、飛行中は追尾しない。
		QueueShot(owner, player, slot, levelStats, currentLevel, 0.0f, 0);
		PlayerAttackShotRequest& request = shotRequests_.back();
		request.motionType = PlayerProjectileMotionType::Boomerang;
		request.motionAnchor = owner;
		request.travelDistance = levelStats.travelDistance;
		request.homing = false;
	}

	void CreateRicochetAttack(GameObject* owner, const Player& player, const AttackSlotRuntime& slot, const PlayerAttackLevelStats& levelStats, const std::string& currentLevel) {
		// 画面端・障害物での反射判定は、シーン内のカメラとコライダーを参照できるBaseScene側で行う。
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
		// 通常は3本、JSONでそれ以上の本数が指定された場合（Superなど）はその本数で生成する。
		const int slashCount = (std::max)(3, levelStats.shotCount);
		for (int index = 0; index < slashCount; ++index) {
			QueueShot(owner, player, slot, levelStats, currentLevel, 0.0f, index);
			PlayerAttackShotRequest& request = shotRequests_.back();
			request.motionType = PlayerProjectileMotionType::ClawSlash;
			request.motionAnchor = owner;
			request.speed = 0.0f;
			request.homing = false;
			// 全ての爪が同じ敵へ命中しても、JSONのAttack値が合計ダメージになるよう均等に分割する。
			request.attack /= static_cast<float>(slashCount);
			request.clawSlashIndex = index;
			request.clawSlashCount = slashCount;
		}
	}

	void CreateAttackByName(GameObject* owner, const Player& player, const AttackSlotRuntime& slot, const PlayerAttackLevelStats& levelStats, const std::string& currentLevel) {
		// 専用挙動を持つ攻撃は名前で生成方式を振り分け、その他は設定値から通常・拡散・追尾を選ぶ。
		if (slot.stats.name == "ArcHoming") {
			CreateArcHomingAttack(owner, player, slot, levelStats, currentLevel);
			return;
		}
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
