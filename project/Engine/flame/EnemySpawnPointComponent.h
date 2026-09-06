#pragma once
#include "../base/GameTime.h"
#include "camera/Camera.h"
#include "Component.h"
#include "MathConstants.h"
#include "GameObject.h"
#include "LineDrawer.h"
#include "Matrix.h"
#include "object/Object3dCommon.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

/// <summary>カメラ表示範囲の外側に敵の出現候補点を作り、時間帯別の生成要求を発行します。</summary>
class EnemySpawnPointComponent : public Component {
public:
	/// <summary>一定時間帯に適用する敵タイプ、生成間隔、生成数の設定です。</summary>
	struct SpawnSchedule {
		float startTimeSeconds = 0.0f;
		float endTimeSeconds = 60.0f;
		std::string enemyTypeName = "Default";
		int spawnIntervalFrames = 60;
		int spawnAmount = 1;
		/// <summary>trueの場合、時間帯内で最初の生成要求だけを発行します。</summary>
		bool spawnOnce = false;
		/// <summary>実行中だけ使用する生成間隔カウンターです。JSONには保存しません。</summary>
		int frameCounter = 0;
		/// <summary>spawnOnceスケジュールが生成済みかを表す実行時フラグです。</summary>
		bool hasSpawned = false;
	};
	/// <summary>シーン側へ渡す、生成位置が確定した敵生成要求です。</summary>
	struct ScheduledSpawnRequest {
		std::string enemyTypeName;
		Vector3 position{};
	};
	/// <summary>指定時刻に一度だけ発生するボス戦の設定です。</summary>
	struct BossEncounterSettings {
		bool enabled = false;
		float triggerTimeSeconds = 300.0f;
		std::string enemyTypeName = "MidBoss";
		Vector3 bossPosition{0.0f, 0.0f, 0.0f};
		Vector3 playerWarpPosition{0.0f, 0.0f, 6.0f};
	};

	void Update() override {
		// カメラの変化へ追従して候補点を更新し、生成が有効な間だけ経過時間を進める。
		RecalculateSpawnPoints();
		// ボス戦中は通常敵の出現時刻とフレーム間隔を停止地点のまま維持する。
		if (spawnEnabled_ && !bossEncounterActive_) {
			elapsedTimeSeconds_ += GameTime::GetDeltaTime();
		}
	}

	void Draw3D() override {
#ifdef USE_IMGUI
		if (!drawDebug_) {
			return;
		}

		const Vector4 ringColor{1.0f, 0.25f, 0.1f, 1.0f};
		const Vector4 pointColor{1.0f, 1.0f, 0.0f, 1.0f};
		const Vector4 viewColor{0.2f, 0.8f, 1.0f, 1.0f};
		if (groundViewCorners_.size() >= 4) {
			for (size_t index = 0; index < groundViewCorners_.size(); ++index) {
				const Vector3& a = groundViewCorners_[index];
				const Vector3& b = groundViewCorners_[(index + 1) % groundViewCorners_.size()];
				LineDrawer::GetInstance()->DrawLine(a, b, viewColor, true);
			}
		}
		if (spawnPoints_.size() >= 2) {
			for (size_t index = 0; index < spawnPoints_.size(); ++index) {
				const Vector3& a = spawnPoints_[index];
				const Vector3& b = spawnPoints_[(index + 1) % spawnPoints_.size()];
				LineDrawer::GetInstance()->DrawLine(a, b, ringColor, true);
			}
		}
		for (const Vector3& point : spawnPoints_) {
			const float size = debugPointSize_;
			LineDrawer::GetInstance()->DrawLine({point.x - size, point.y, point.z}, {point.x + size, point.y, point.z}, pointColor, true);
			LineDrawer::GetInstance()->DrawLine({point.x, point.y, point.z - size}, {point.x, point.y, point.z + size}, pointColor, true);
			LineDrawer::GetInstance()->DrawLine({point.x, point.y - size, point.z}, {point.x, point.y + size, point.z}, pointColor, true);
		}
#endif
	}

	void SetTarget(GameObject* target) { target_ = target; }
	GameObject* GetTarget() const { return target_; }
	void SetTargetName(const std::string& name) { targetName_ = name; }
	const std::string& GetTargetName() const { return targetName_; }

	void SetCamera(Camera* camera) { camera_ = camera; }
	void SetCameraName(const std::string& name) { cameraName_ = name; }
	const std::string& GetCameraName() const { return cameraName_; }

	void SetSpawnCount(int count) { spawnCount_ = std::clamp(count, 1, 64); }
	int GetSpawnCount() const { return spawnCount_; }
	void SetOuterMargin(float margin) { outerMargin_ = margin < 0.0f ? 0.0f : margin; }
	float GetOuterMargin() const { return outerMargin_; }
	void SetMinimumRadius(float radius) { minimumRadius_ = radius < 0.0f ? 0.0f : radius; }
	float GetMinimumRadius() const { return minimumRadius_; }
	void SetGroundY(float groundY) { groundY_ = groundY; }
	float GetGroundY() const { return groundY_; }
	void SetPointHeight(float height) { pointHeight_ = height; }
	float GetPointHeight() const { return pointHeight_; }
	void SetDrawDebug(bool drawDebug) { drawDebug_ = drawDebug; }
	bool GetDrawDebug() const { return drawDebug_; }
	void SetDebugPointSize(float size) { debugPointSize_ = size < 0.01f ? 0.01f : size; }
	float GetDebugPointSize() const { return debugPointSize_; }
	void SetEnemyTypeName(const std::string& enemyTypeName) { enemyTypeName_ = enemyTypeName.empty() ? "Default" : enemyTypeName; }
	const std::string& GetEnemyTypeName() const { return enemyTypeName_; }
	void SetSpawnEnabled(bool spawnEnabled) { spawnEnabled_ = spawnEnabled; }
	bool GetSpawnEnabled() const { return spawnEnabled_; }
	void ResetSpawnTimer() {
		// エディタの再生し直しや設定変更後に、一度限定イベントも含めて最初から再実行できる状態へ戻す。
		spawnTimerSeconds_ = 0.0f;
		elapsedTimeSeconds_ = 0.0f;
		nextSpawnIndex_ = 0;
		bossEncounterTriggered_ = false;
		for (SpawnSchedule& schedule : spawnSchedules_) {
			schedule.frameCounter = 0;
			schedule.hasSpawned = false;
		}
	}
	float GetElapsedTimeSeconds() const { return elapsedTimeSeconds_; }
	std::vector<SpawnSchedule>& GetSpawnSchedules() { return spawnSchedules_; }
	const std::vector<SpawnSchedule>& GetSpawnSchedules() const { return spawnSchedules_; }
	void SetSpawnSchedules(const std::vector<SpawnSchedule>& schedules) {
		// 保存データやインスペクターから受け取った値を正規化し、全スケジュールを先頭から再生する。
		spawnSchedules_ = schedules;
		for (SpawnSchedule& schedule : spawnSchedules_) {
			schedule.startTimeSeconds = (std::max)(0.0f, schedule.startTimeSeconds);
			schedule.endTimeSeconds = (std::max)(schedule.startTimeSeconds, schedule.endTimeSeconds);
			schedule.enemyTypeName = schedule.enemyTypeName.empty() ? "Default" : schedule.enemyTypeName;
			schedule.spawnIntervalFrames = std::clamp(schedule.spawnIntervalFrames, 1, 360000);
			schedule.spawnAmount = std::clamp(schedule.spawnAmount, 1, 64);
			schedule.frameCounter = 0;
			schedule.hasSpawned = false;
		}
		ResetSpawnTimer();
	}
	BossEncounterSettings& GetBossEncounterSettings() { return bossEncounterSettings_; }
	const BossEncounterSettings& GetBossEncounterSettings() const { return bossEncounterSettings_; }
	void SetBossEncounterSettings(const BossEncounterSettings& settings) {
		// JSONやImGuiから渡された値を安全な範囲へ補正し、変更後は再度発火可能にする。
		bossEncounterSettings_ = settings;
		bossEncounterSettings_.triggerTimeSeconds = (std::max)(0.0f, bossEncounterSettings_.triggerTimeSeconds);
		bossEncounterSettings_.enemyTypeName =
		    bossEncounterSettings_.enemyTypeName.empty() ? "MidBoss" : bossEncounterSettings_.enemyTypeName;
		bossEncounterTriggered_ = false;
	}
	bool ConsumeBossEncounterRequest() {
		// 時刻到達後の最初の呼び出しだけtrueを返し、毎フレームのボス多重生成を防ぐ。
		if (!spawnEnabled_ || !bossEncounterSettings_.enabled || bossEncounterTriggered_ ||
		    elapsedTimeSeconds_ < bossEncounterSettings_.triggerTimeSeconds) {
			return false;
		}
		bossEncounterTriggered_ = true;
		return true;
	}
	bool IsBossEncounterTriggered() const { return bossEncounterTriggered_; }
	void SetBossEncounterActive(bool active) { bossEncounterActive_ = active; }
	bool IsBossEncounterActive() const { return bossEncounterActive_; }

	std::vector<ScheduledSpawnRequest> ConsumeScheduledSpawnRequests() {
		// 有効時間帯に達し、指定間隔を満たしたスケジュールだけ要求へ変換する。
		std::vector<ScheduledSpawnRequest> requests;
		if (!spawnEnabled_ || spawnPoints_.empty()) {
			return requests;
		}
		for (SpawnSchedule& schedule : spawnSchedules_) {
			// 中ボスなどの一度限定スケジュールは、時間帯が続いていても再生成しない。
			if (schedule.spawnOnce && schedule.hasSpawned) {
				continue;
			}
			const bool isActive = elapsedTimeSeconds_ >= schedule.startTimeSeconds && elapsedTimeSeconds_ <= schedule.endTimeSeconds;
			if (!isActive) {
				schedule.frameCounter = 0;
				continue;
			}
			schedule.spawnIntervalFrames = std::clamp(schedule.spawnIntervalFrames, 1, 360000);
			schedule.spawnAmount = std::clamp(schedule.spawnAmount, 1, 64);
			++schedule.frameCounter;
			if (schedule.frameCounter < schedule.spawnIntervalFrames) {
				continue;
			}
			schedule.frameCounter -= schedule.spawnIntervalFrames;
			for (int count = 0; count < schedule.spawnAmount; ++count) {
				requests.push_back({
					schedule.enemyTypeName.empty() ? "Default" : schedule.enemyTypeName,
					spawnPoints_[nextSpawnIndex_ % spawnPoints_.size()]
				});
				++nextSpawnIndex_;
			}
			// 通常スケジュールでも値を保持してよいが、spawnOnceがtrueのときだけ次回判定に使用する。
			schedule.hasSpawned = true;
		}
		return requests;
	}

	bool ConsumeSpawnRequest(float spawnsPerMinute, Vector3& outPosition) {
		// スケジュール未設定シーンとの互換用。毎分生成数を秒間隔へ変換して1体ずつ要求する。
		if (!spawnEnabled_ || spawnPoints_.empty() || spawnsPerMinute <= 0.0f) {
			return false;
		}

		spawnTimerSeconds_ += GameTime::GetDeltaTime();
		const float spawnInterval = 60.0f / spawnsPerMinute;
		if (spawnTimerSeconds_ < spawnInterval) {
			return false;
		}

		spawnTimerSeconds_ -= spawnInterval;
		outPosition = spawnPoints_[nextSpawnIndex_ % spawnPoints_.size()];
		++nextSpawnIndex_;
		return true;
	}

	const std::vector<Vector3>& GetSpawnPoints() const { return spawnPoints_; }

private:
	Vector3 TransformCoord(const Vector3& vector, const Matrix4x4& matrix) const {
		return Transformation(vector, matrix);
	}

	bool IntersectCameraRayToGround(Camera* camera, const Vector2& ndc, Vector3& outPoint) const {
		if (!camera) {
			return false;
		}

		// NDCの手前・奥をワールド座標へ戻し、その2点から地面へ向かうカメラレイを作る。
		const Matrix4x4 inverseViewProjection = Inverse(camera->GetViewProjectionMatrix());
		const Vector3 nearPoint = TransformCoord({ndc.x, ndc.y, 0.0f}, inverseViewProjection);
		const Vector3 farPoint = TransformCoord({ndc.x, ndc.y, 1.0f}, inverseViewProjection);
		const Vector3 direction = Normalize(farPoint - nearPoint);
		if (std::fabs(direction.y) <= MathConstants::kNormalizationEpsilon) {
			return false;
		}

		// ray(t) = near + direction * t と水平面 y = groundY_ の交点を求める。
		const float t = (groundY_ - nearPoint.y) / direction.y;
		if (t < 0.0f) {
			return false;
		}

		outPoint = nearPoint + t * direction;
		return true;
	}

	/// <summary>カメラの地面投影範囲を基に、画面外の生成候補点を再計算します。</summary>
	void RecalculateSpawnPoints() {
		spawnPoints_.clear();
		groundViewCorners_.clear();

		GameObject* owner = GetOwner();
		GameObject* target = target_ ? target_ : owner;
		if (!owner || !target) {
			return;
		}

		Camera* camera = camera_ ? camera_ : Object3dCommon::GetInstance()->GetDefaultCamera();
		const Vector3 targetPosition = target->GetTransform().translate;
		owner->GetTransform().translate = {targetPosition.x, groundY_ + pointHeight_, targetPosition.z};

		const Vector2 ndcCorners[4] = {
		    {-1.0f, -1.0f},
		    {1.0f, -1.0f},
		    {1.0f, 1.0f},
		    {-1.0f, 1.0f}
		};
		for (const Vector2& corner : ndcCorners) {
			Vector3 groundPoint{};
			if (IntersectCameraRayToGround(camera, corner, groundPoint)) {
				groundViewCorners_.push_back(groundPoint);
			}
		}

		for (int index = 0; index < spawnCount_; ++index) {
			// 各方向に対して視錐台の最遠点を投影し、さらに外側マージンを足して画面外へ配置する。
			const float angle = (static_cast<float>(index) / static_cast<float>(spawnCount_)) * MathConstants::kPi * 2.0f;
			const Vector3 direction{std::cos(angle), 0.0f, std::sin(angle)};
			float visibleRadiusInDirection = minimumRadius_;
			for (const Vector3& corner : groundViewCorners_) {
				const Vector3 targetToCorner = corner - targetPosition;
				visibleRadiusInDirection = (std::max)(visibleRadiusInDirection, Dot(targetToCorner, direction));
			}
			const float radius = visibleRadiusInDirection + outerMargin_;
			spawnPoints_.push_back({
			    targetPosition.x + direction.x * radius,
			    groundY_ + pointHeight_,
			    targetPosition.z + direction.z * radius
			});
		}
	}

	/// <summary>生成位置の基準となる対象への非所有参照です。</summary>
	GameObject* target_ = nullptr;
	Camera* camera_ = nullptr;
	std::string enemyTypeName_ = "Default";
	std::string targetName_;
	std::string cameraName_;
	/// <summary>画面外周に配置した敵生成候補位置です。</summary>
	std::vector<Vector3> spawnPoints_;
	/// <summary>カメラ視錐台を地面へ投影した四隅です。</summary>
	std::vector<Vector3> groundViewCorners_;
	/// <summary>候補点を均等に使用するための次回位置です。</summary>
	size_t nextSpawnIndex_ = 0;
	int spawnCount_ = 8;
	float spawnTimerSeconds_ = 0.0f;
	float elapsedTimeSeconds_ = 0.0f;
	/// <summary>ゲーム時間帯ごとの敵生成設定です。</summary>
	std::vector<SpawnSchedule> spawnSchedules_;
	BossEncounterSettings bossEncounterSettings_;
	bool bossEncounterTriggered_ = false;
	bool bossEncounterActive_ = false;
	float outerMargin_ = 5.0f;
	float minimumRadius_ = 8.0f;
	float groundY_ = 0.0f;
	float pointHeight_ = 0.2f;
	float debugPointSize_ = 0.5f;
	bool drawDebug_ = true;
	bool spawnEnabled_ = true;
};
