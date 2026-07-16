#pragma once
#include "../base/GameTime.h"
#include "Camera.h"
#include "Component.h"
#include "GameObject.h"
#include "LineDrawer.h"
#include "Matrix.h"
#include "Object3dCommon.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

class EnemySpawnPointComponent : public Component {
public:
	struct SpawnSchedule {
		float startTimeSeconds = 0.0f;
		float endTimeSeconds = 60.0f;
		std::string enemyTypeName = "Default";
		int spawnIntervalFrames = 60;
		int spawnAmount = 1;
		int frameCounter = 0;
	};
	struct ScheduledSpawnRequest {
		std::string enemyTypeName;
		Vector3 position{};
	};

	void Update() override {
		RecalculateSpawnPoints();
		if (spawnEnabled_) {
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
		spawnTimerSeconds_ = 0.0f;
		elapsedTimeSeconds_ = 0.0f;
		nextSpawnIndex_ = 0;
		for (SpawnSchedule& schedule : spawnSchedules_) {
			schedule.frameCounter = 0;
		}
	}
	float GetElapsedTimeSeconds() const { return elapsedTimeSeconds_; }
	std::vector<SpawnSchedule>& GetSpawnSchedules() { return spawnSchedules_; }
	const std::vector<SpawnSchedule>& GetSpawnSchedules() const { return spawnSchedules_; }
	void SetSpawnSchedules(const std::vector<SpawnSchedule>& schedules) {
		spawnSchedules_ = schedules;
		for (SpawnSchedule& schedule : spawnSchedules_) {
			schedule.startTimeSeconds = (std::max)(0.0f, schedule.startTimeSeconds);
			schedule.endTimeSeconds = (std::max)(schedule.startTimeSeconds, schedule.endTimeSeconds);
			schedule.enemyTypeName = schedule.enemyTypeName.empty() ? "Default" : schedule.enemyTypeName;
			schedule.spawnIntervalFrames = std::clamp(schedule.spawnIntervalFrames, 1, 360000);
			schedule.spawnAmount = std::clamp(schedule.spawnAmount, 1, 64);
			schedule.frameCounter = 0;
		}
		ResetSpawnTimer();
	}

	std::vector<ScheduledSpawnRequest> ConsumeScheduledSpawnRequests() {
		std::vector<ScheduledSpawnRequest> requests;
		if (!spawnEnabled_ || spawnPoints_.empty()) {
			return requests;
		}
		for (SpawnSchedule& schedule : spawnSchedules_) {
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
		}
		return requests;
	}

	bool ConsumeSpawnRequest(float spawnsPerMinute, Vector3& outPosition) {
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

		const Matrix4x4 inverseViewProjection = Inverse(camera->GetViewProjectionMatrix());
		const Vector3 nearPoint = TransformCoord({ndc.x, ndc.y, 0.0f}, inverseViewProjection);
		const Vector3 farPoint = TransformCoord({ndc.x, ndc.y, 1.0f}, inverseViewProjection);
		const Vector3 direction = Normalize(farPoint - nearPoint);
		if (std::fabs(direction.y) <= 0.00001f) {
			return false;
		}

		const float t = (groundY_ - nearPoint.y) / direction.y;
		if (t < 0.0f) {
			return false;
		}

		outPoint = nearPoint + t * direction;
		return true;
	}

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

		constexpr float kPi = 3.14159265358979323846f;
		for (int index = 0; index < spawnCount_; ++index) {
			const float angle = (static_cast<float>(index) / static_cast<float>(spawnCount_)) * kPi * 2.0f;
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

	GameObject* target_ = nullptr;
	Camera* camera_ = nullptr;
	std::string enemyTypeName_ = "Default";
	std::string targetName_;
	std::string cameraName_;
	std::vector<Vector3> spawnPoints_;
	std::vector<Vector3> groundViewCorners_;
	size_t nextSpawnIndex_ = 0;
	int spawnCount_ = 8;
	float spawnTimerSeconds_ = 0.0f;
	float elapsedTimeSeconds_ = 0.0f;
	std::vector<SpawnSchedule> spawnSchedules_;
	float outerMargin_ = 5.0f;
	float minimumRadius_ = 8.0f;
	float groundY_ = 0.0f;
	float pointHeight_ = 0.2f;
	float debugPointSize_ = 0.5f;
	bool drawDebug_ = true;
	bool spawnEnabled_ = true;
};
