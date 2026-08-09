#include "BaseScene.h"
#include "helpers/BaseSceneCollisionHelpers.h"
#include "helpers/BaseSceneEditorGeometry.h"
#include "repositories/EnemyStatusRepository.h"
#include "MathConstants.h"
#include "model/ModelManager.h"
#include "repositories/PlayerStatusRepository.h"
#include <filesystem>
#include <limits>
#include <random>
#include <unordered_set>
#include <Xinput.h>
#ifdef USE_IMGUI
#include "../../../imgui/ImGuizmo.h"
#endif

namespace {
bool ShouldSkipColliderPair(GameObject* objectA, GameObject* objectB) {
	if (!objectA || !objectB || objectA == objectB) {
		return true;
	}

	const bool isEnemyA = objectA->GetComponent<EnemyComponent>() != nullptr;
	const bool isEnemyB = objectB->GetComponent<EnemyComponent>() != nullptr;
	const bool isProjectileA =
	    objectA->GetComponent<EnemyProjectileComponent>() != nullptr ||
	    objectA->GetComponent<PlayerProjectileComponent>() != nullptr;
	const bool isProjectileB =
	    objectB->GetComponent<EnemyProjectileComponent>() != nullptr ||
	    objectB->GetComponent<PlayerProjectileComponent>() != nullptr;
	return (isEnemyA && isEnemyB) || isProjectileA || isProjectileB;
}

struct EnemyPlayerContact {
	EnemyComponent* enemy = nullptr;
	Player* player = nullptr;
};

constexpr std::array<int, 4> kExperienceDenominations = {1, 10, 50, 100};
constexpr float kExperienceCompressionDistance = 1.5f;

Vector4 GetExperienceColor(int denomination) {
	switch (denomination) {
	case 100: return {1.0f, 0.65f, 0.08f, 1.0f};
	case 50: return {0.72f, 0.25f, 1.0f, 1.0f};
	case 10: return {0.20f, 1.0f, 0.35f, 1.0f};
	default: return {0.15f, 0.75f, 1.0f, 1.0f};
	}
}

Vector4 GetArcHomingProjectileColor(int colorIndex) {
	static const std::array<Vector4, 6> kColors = {{
	    {1.0f, 0.08f, 0.04f, 1.0f},
	    {0.08f, 0.35f, 1.0f, 1.0f},
	    {0.08f, 1.0f, 0.20f, 1.0f},
	    {1.0f, 0.90f, 0.05f, 1.0f},
	    {1.0f, 1.0f, 1.0f, 1.0f},
	    {0.72f, 0.16f, 1.0f, 1.0f},
	}};
	return kColors[static_cast<size_t>((std::max)(0, colorIndex)) % kColors.size()];
}

Vector4 GetArcHomingTrailTailColor(int colorIndex) {
	Vector4 color = GetArcHomingProjectileColor(colorIndex);
	color.x *= 0.42f;
	color.y *= 0.42f;
	color.z *= 0.42f;
	color.w = 0.0f;
	return color;
}

float GetExperienceScale(int denomination) {
	switch (denomination) {
	case 100: return 0.55f;
	case 50: return 0.48f;
	case 10: return 0.41f;
	default: return 0.35f;
	}
}

bool RegisterEnemyPlayerContact(GameObject* objectA, GameObject* objectB, std::vector<EnemyPlayerContact>& contacts) {
	if (!objectA || !objectB) {
		return false;
	}

	EnemyComponent* enemy = objectA->GetComponent<EnemyComponent>();
	Player* player = objectB->GetComponent<Player>();
	if (!enemy || !player) {
		enemy = objectB->GetComponent<EnemyComponent>();
		player = objectA->GetComponent<Player>();
	}
	if (!enemy || !player) {
		return false;
	}

	if (enemy->IsEnabled() && player->IsEnabled() && enemy->GetCurrentHealth() > 0.0f && enemy->CanDealContactDamage()) {
		const auto duplicate = std::find_if(contacts.begin(), contacts.end(), [enemy, player](const EnemyPlayerContact& contact) {
			return contact.enemy == enemy && contact.player == player;
		});
		if (duplicate == contacts.end()) {
			contacts.push_back({enemy, player});
		}
	}
	return true;
}

bool IsPointOutsideView(const Vector3& worldPosition, float margin) {
	Camera* camera = Object3dCommon::GetInstance() ? Object3dCommon::GetInstance()->GetDefaultCamera() : nullptr;
	if (!camera) {
		return false;
	}

	const Matrix4x4& viewProjection = camera->GetViewProjectionMatrix();
	const float clipX =
	    worldPosition.x * viewProjection.m[0][0] +
	    worldPosition.y * viewProjection.m[1][0] +
	    worldPosition.z * viewProjection.m[2][0] +
	    viewProjection.m[3][0];
	const float clipY =
	    worldPosition.x * viewProjection.m[0][1] +
	    worldPosition.y * viewProjection.m[1][1] +
	    worldPosition.z * viewProjection.m[2][1] +
	    viewProjection.m[3][1];
	const float clipZ =
	    worldPosition.x * viewProjection.m[0][2] +
	    worldPosition.y * viewProjection.m[1][2] +
	    worldPosition.z * viewProjection.m[2][2] +
	    viewProjection.m[3][2];
	const float clipW =
	    worldPosition.x * viewProjection.m[0][3] +
	    worldPosition.y * viewProjection.m[1][3] +
	    worldPosition.z * viewProjection.m[2][3] +
	    viewProjection.m[3][3];
	if (clipW <= MathConstants::kDirectionEpsilon) {
		return true;
	}

	const float ndcX = clipX / clipW;
	const float ndcY = clipY / clipW;
	const float ndcZ = clipZ / clipW;
	const float safeMargin = margin < 0.0f ? 0.0f : margin;
	return ndcX < -1.0f - safeMargin ||
	    ndcX > 1.0f + safeMargin ||
	    ndcY < -1.0f - safeMargin ||
	    ndcY > 1.0f + safeMargin ||
	    ndcZ < -safeMargin ||
	    ndcZ > 1.0f + safeMargin;
}

bool ProjectToNdc(Camera* camera, const Vector3& worldPosition, Vector3& outNdc) {
	if (!camera) {
		return false;
	}
	const Matrix4x4& viewProjection = camera->GetViewProjectionMatrix();
	const float clipX = worldPosition.x * viewProjection.m[0][0] + worldPosition.y * viewProjection.m[1][0] + worldPosition.z * viewProjection.m[2][0] + viewProjection.m[3][0];
	const float clipY = worldPosition.x * viewProjection.m[0][1] + worldPosition.y * viewProjection.m[1][1] + worldPosition.z * viewProjection.m[2][1] + viewProjection.m[3][1];
	const float clipZ = worldPosition.x * viewProjection.m[0][2] + worldPosition.y * viewProjection.m[1][2] + worldPosition.z * viewProjection.m[2][2] + viewProjection.m[3][2];
	const float clipW = worldPosition.x * viewProjection.m[0][3] + worldPosition.y * viewProjection.m[1][3] + worldPosition.z * viewProjection.m[2][3] + viewProjection.m[3][3];
	if (clipW <= MathConstants::kDirectionEpsilon) {
		return false;
	}
	outNdc = {clipX / clipW, clipY / clipW, clipZ / clipW};
	return true;
}

bool IntersectScreenRayToHeight(Camera* camera, const Vector2& ndc, float height, Vector3& outPoint) {
	if (!camera) {
		return false;
	}
	const Matrix4x4 inverseViewProjection = Inverse(camera->GetViewProjectionMatrix());
	const Vector3 nearPoint = Transformation({ndc.x, ndc.y, 0.0f}, inverseViewProjection);
	const Vector3 farPoint = Transformation({ndc.x, ndc.y, 1.0f}, inverseViewProjection);
	const Vector3 ray = farPoint - nearPoint;
	if (std::fabs(ray.y) <= MathConstants::kNormalizationEpsilon) {
		return false;
	}
	const float t = (height - nearPoint.y) / ray.y;
	if (t < 0.0f) {
		return false;
	}
	outPoint = nearPoint + t * ray;
	return true;
}
}

void BaseScene::ApplyCamera(Camera* camera) {
	if (!camera) {
		return;
	}

	const float clientWidth = static_cast<float>(Input::GetInstance()->GetClientWidth());
	const float clientHeight = static_cast<float>(Input::GetInstance()->GetClientHeight());
	float aspectRatio = clientWidth > 0.0f && clientHeight > 0.0f ? clientWidth / clientHeight : 1.0f;
#ifdef USE_IMGUI
	const float gameViewAspectRatio = ImGuiManager::GetInstance()->GetGameViewAspectRatio();
	if (gameViewAspectRatio > 0.0f) {
		aspectRatio = gameViewAspectRatio;
	}
#endif
	camera->SetAspectRatio(aspectRatio);
	camera->Update();
	Object3dCommon::GetInstance()->SetDefaultCamera(camera);
	SkyBoxCommon::GetInstance()->SetDefaultCamera(camera);
	ParticleManager::GetInstance()->SetCamera(camera);
	for (const auto& object : sceneObjects_) {
		if (Object3dComponent* object3dComponent = object->GetComponent<Object3dComponent>()) {
			object3dComponent->SetCamera(camera);
			if (object3dComponent->GetObject3d()) {
				object3dComponent->GetObject3d()->Update();
			}
		}
	}
}

/// <summary>
/// 現在選択されているアクティブカメラを反映します。
/// </summary>
void BaseScene::ApplyActiveCamera() {
	if (activeCameraObjectName_.empty()) {
		ApplyCamera(fallbackCamera_);
		return;
	}

	GameObject* cameraObject = FindObjectByName(activeCameraObjectName_);
	if (!cameraObject) {
		activeCameraObjectName_.clear();
		ApplyCamera(fallbackCamera_);
		return;
	}

	CameraComponent* cameraComponent = cameraObject->GetComponent<CameraComponent>();
	if (!cameraComponent || !cameraComponent->IsEnabled() || !cameraComponent->GetCamera()) {
		activeCameraObjectName_.clear();
		ApplyCamera(fallbackCamera_);
		return;
	}

	cameraComponent->Update();
	ApplyCamera(cameraComponent->GetCamera());
}

/// <summary>
/// カメラの追従対象リンクを名前から解決します。
/// </summary>
void BaseScene::ResolveCameraLinks() {
	for (const auto& object : sceneObjects_) {
		CameraComponent* cameraComponent = object->GetComponent<CameraComponent>();
		if (!cameraComponent || !cameraComponent->IsEnabled() || cameraComponent->GetFollowTargetName().empty()) {
			continue;
		}

		GameObject* target = FindObjectByName(cameraComponent->GetFollowTargetName());
		if (target && target != object.get() && target != cameraComponent->GetFollowTarget()) {
			cameraComponent->SetFollowTarget(target);
		}
	}
}

void BaseScene::ResolveEnemySpawnPointLinks() {
	GameObject* firstPlayer = nullptr;
	for (const auto& object : sceneObjects_) {
		if (object->GetComponent<Player>()) {
			firstPlayer = object.get();
			break;
		}
	}

	for (const auto& object : sceneObjects_) {
		EnemySpawnPointComponent* spawnPoint = object->GetComponent<EnemySpawnPointComponent>();
		if (!spawnPoint || !spawnPoint->IsEnabled()) {
			continue;
		}

		GameObject* target = spawnPoint->GetTargetName().empty() ? firstPlayer : FindObjectByName(spawnPoint->GetTargetName());
		if (target && target != spawnPoint->GetTarget()) {
			spawnPoint->SetTarget(target);
			if (spawnPoint->GetTargetName().empty()) {
				spawnPoint->SetTargetName(target->GetName());
			}
		}

		Camera* camera = nullptr;
		if (!spawnPoint->GetCameraName().empty()) {
			GameObject* cameraObject = FindObjectByName(spawnPoint->GetCameraName());
			CameraComponent* cameraComponent = cameraObject ? cameraObject->GetComponent<CameraComponent>() : nullptr;
			if (cameraComponent && cameraComponent->IsEnabled()) {
				camera = cameraComponent->GetCamera();
			}
		}
		if (!camera && Object3dCommon::GetInstance()) {
			camera = Object3dCommon::GetInstance()->GetDefaultCamera();
		}
		spawnPoint->SetCamera(camera);
	}
}

void BaseScene::ResolveEnemyLinks() {
	GameObject* firstPlayer = nullptr;
	for (const auto& object : sceneObjects_) {
		if (object->GetComponent<Player>()) {
			firstPlayer = object.get();
			break;
		}
	}

	for (const auto& object : sceneObjects_) {
		EnemyComponent* enemy = object->GetComponent<EnemyComponent>();
		if (!enemy || !enemy->IsEnabled()) {
			continue;
		}

		GameObject* target = enemy->GetTargetName().empty() ? firstPlayer : FindObjectByName(enemy->GetTargetName());
		if (target && target != enemy->GetTarget()) {
			enemy->SetTarget(target);
			if (enemy->GetTargetName().empty()) {
				enemy->SetTargetName(target->GetName());
			}
		}
	}
}

void BaseScene::UpdateEnemySpawning() {
	if (!activeBossEncounterObjectName_.empty()) {
		GameObject* bossObject = FindObjectByName(activeBossEncounterObjectName_);
		EnemyComponent* bossEnemy = bossObject ? bossObject->GetComponent<EnemyComponent>() : nullptr;
		const bool bossIsAlive = bossEnemy && bossEnemy->IsEnabled() && bossEnemy->GetCurrentHealth() > 0.0f;
		for (const auto& object : sceneObjects_) {
			if (EnemySpawnPointComponent* spawnPoint = object->GetComponent<EnemySpawnPointComponent>()) {
				spawnPoint->SetBossEncounterActive(bossIsAlive);
			}
		}
		if (bossIsAlive) {
			return;
		}
		// Boss Encounter はステージの最後に一度だけ発生するため、撃破をクリア条件とする。
		isStageCleared_ = true;
		activeBossEncounterObjectName_.clear();
		return;
	}

	struct SpawnRequest {
		std::string enemyTypeName;
		Vector3 position;
		GameObject* target = nullptr;
	};
	std::vector<SpawnRequest> spawnRequests;
	EnemySpawnPointComponent* triggeredBossSpawnPoint = nullptr;
	for (const auto& object : sceneObjects_) {
		EnemySpawnPointComponent* spawnPoint = object->GetComponent<EnemySpawnPointComponent>();
		if (!spawnPoint || !spawnPoint->IsEnabled()) {
			continue;
		}

		if (spawnPoint->ConsumeBossEncounterRequest()) {
			triggeredBossSpawnPoint = spawnPoint;
			break;
		}

		if (!spawnPoint->GetSpawnSchedules().empty()) {
			const std::vector<EnemySpawnPointComponent::ScheduledSpawnRequest> scheduledRequests = spawnPoint->ConsumeScheduledSpawnRequests();
			for (const EnemySpawnPointComponent::ScheduledSpawnRequest& request : scheduledRequests) {
				spawnRequests.push_back({request.enemyTypeName, request.position, spawnPoint->GetTarget()});
			}
		} else {
			const std::string enemyTypeName = spawnPoint->GetEnemyTypeName();
			const EnemyStats stats = LoadEnemyStats(enemyTypeName);
			Vector3 spawnPosition{};
			if (spawnPoint->ConsumeSpawnRequest(stats.spawnsPerMinute, spawnPosition)) {
				spawnRequests.push_back({enemyTypeName, spawnPosition, spawnPoint->GetTarget()});
			}
		}
	}

	if (triggeredBossSpawnPoint) {
		const EnemySpawnPointComponent::BossEncounterSettings& bossSettings =
		    triggeredBossSpawnPoint->GetBossEncounterSettings();
		GameObject* player = triggeredBossSpawnPoint->GetTarget();
		if (player) {
			player->GetTransform().translate = bossSettings.playerWarpPosition;
			if (Player* playerComponent = player->GetComponent<Player>()) {
				playerComponent->ResetGravityVelocity();
			}
		}

		sceneObjects_.erase(
		    std::remove_if(sceneObjects_.begin(), sceneObjects_.end(), [](const std::unique_ptr<GameObject>& object) {
			    return object->GetComponent<EnemyComponent>() != nullptr;
		    }),
		    sceneObjects_.end()
		);
		selectedObjectIndex_ = -1;
		for (const auto& object : sceneObjects_) {
			if (EnemySpawnPointComponent* spawnPoint = object->GetComponent<EnemySpawnPointComponent>()) {
				spawnPoint->SetBossEncounterActive(true);
			}
		}

		GameObject* bossObject = CreateRuntimeEnemy(bossSettings.enemyTypeName, bossSettings.bossPosition, player);
		if (bossObject) {
			activeBossEncounterObjectName_ = bossObject->GetName();
		}
		return;
	}

	for (const SpawnRequest& request : spawnRequests) {
		CreateRuntimeEnemy(request.enemyTypeName, request.position, request.target);
	}
}

void BaseScene::UpdatePlayerAttacks() {
	std::vector<PlayerAttackShotRequest> shotRequests;
	for (const auto& object : sceneObjects_) {
		PlayerAttackComponent* attack = object->GetComponent<PlayerAttackComponent>();
		if (!attack || !attack->IsEnabled()) {
			continue;
		}

		std::vector<PlayerAttackShotRequest> requests = attack->ConsumeShotRequests();
		shotRequests.insert(shotRequests.end(), requests.begin(), requests.end());
	}

	std::vector<GameObject*> laserTargets;
	for (const auto& object : sceneObjects_) {
		EnemyComponent* enemy = object->GetComponent<EnemyComponent>();
		if (enemy && enemy->IsEnabled() && enemy->GetCurrentHealth() > 0.0f &&
		    !IsPointOutsideView(object->GetTransform().translate, 0.0f)) {
			laserTargets.push_back(object.get());
		}
	}
	static std::mt19937 laserRandomEngine(std::random_device{}());
	std::shuffle(laserTargets.begin(), laserTargets.end(), laserRandomEngine);
	size_t nextLaserTargetIndex = 0;

	for (PlayerAttackShotRequest& request : shotRequests) {
		if (request.motionType == PlayerProjectileMotionType::SkyLaser) {
			if (nextLaserTargetIndex >= laserTargets.size()) {
				continue;
			}
			const Vector3 targetPosition = laserTargets[nextLaserTargetIndex++]->GetTransform().translate;
			request.position = {targetPosition.x, targetPosition.y + 3.0f, targetPosition.z};
		}
		CreateRuntimePlayerProjectile(request);
	}
}

GameObject* BaseScene::CreateRuntimeEnemy(const std::string& enemyTypeName, const Vector3& position, GameObject* target) {
	auto object = std::make_unique<GameObject>();
	const std::string resolvedTypeName = enemyTypeName.empty() ? "Default" : enemyTypeName;
	const EnemyStats stats = LoadEnemyStats(resolvedTypeName);
	object->SetName(MakeUniqueObjectName(resolvedTypeName));
	object->SetEditorType("Enemy");
	object->GetTransform().translate = position;
	const float scale = 0.75f * stats.sizeScale;
	object->GetTransform().scale = {scale, scale, scale};

	EnemyComponent* enemy = object->AddComponent<EnemyComponent>();
	enemy->SetEnemyTypeName(resolvedTypeName);
	enemy->ApplyStats(stats);
	enemy->SetRuntimeSpawned(true);
	if (target) {
		enemy->SetTarget(target);
		enemy->SetTargetName(target->GetName());
	}

	ModelManager::GetInstance()->LoadModel("sphere.obj");
	Object3dComponent* object3d = object->AddComponent<Object3dComponent>();
	object3d->SetModel("sphere.obj");

	OBBColliderComponent* collider = object->AddComponent<OBBColliderComponent>();
	collider->SetHalfSize({0.4f, 0.4f, 0.4f});
	collider->SetPushBackEnabled(true);
	if (stats.behavior == EnemyBehaviorType::NightSlashBoss) {
		TrailRendererComponent* trail = object->AddComponent<TrailRendererComponent>();
		trail->SetWidth(1.15f * stats.sizeScale);
		trail->SetLifeTime(0.32f);
		trail->SetMinSegmentLength(0.08f);
		trail->SetHeadColor({0.92f, 0.20f, 1.0f, 0.92f});
		trail->SetTailColor({0.20f, 0.02f, 0.40f, 0.0f});
	}

	object->Update();
	sceneObjects_.push_back(std::move(object));
	++nextObjectId_;
	return sceneObjects_.back().get();
}

GameObject* BaseScene::CreateRuntimeExperience(const EnemyStats& enemyStats, const Vector3& position, GameObject* target) {
	if (enemyStats.experience <= 0) {
		return nullptr;
	}
	if (!target) {
		for (const auto& object : sceneObjects_) {
			if (object->GetComponent<Player>()) {
				target = object.get();
				break;
			}
		}
	}

	const std::string modelFilePath = enemyStats.experienceModelFilePath.empty() ? "sphere.obj" : enemyStats.experienceModelFilePath;
	if (!ModelManager::GetInstance()->FindModel(modelFilePath)) {
		ModelManager::GetInstance()->LoadModel(modelFilePath);
	}
	if (!ModelManager::GetInstance()->FindModel(modelFilePath)) {
		ModelManager::GetInstance()->LoadModel("sphere.obj");
	}
	const std::string resolvedModelFilePath = ModelManager::GetInstance()->FindModel(modelFilePath) ? modelFilePath : "sphere.obj";
	GameObject* firstExperienceObject = nullptr;
	int remainingExperience = enemyStats.experience;
	int particleIndex = 0;
	for (auto denominationIt = kExperienceDenominations.rbegin(); denominationIt != kExperienceDenominations.rend(); ++denominationIt) {
		const int denomination = *denominationIt;
		const int particleCount = remainingExperience / denomination;
		remainingExperience %= denomination;
		for (int index = 0; index < particleCount; ++index, ++particleIndex) {
			auto object = std::make_unique<GameObject>();
			object->SetName(MakeUniqueObjectName("Experience" + std::to_string(denomination)));
			object->SetEditorType("Experience");
			const float angle = static_cast<float>(particleIndex) * 2.39996323f;
			const float radius = particleIndex == 0 ? 0.0f : 0.18f + 0.07f * std::sqrt(static_cast<float>(particleIndex));
			object->GetTransform().translate = {
				position.x + std::cos(angle) * radius,
				position.y + 0.08f * static_cast<float>(particleIndex % 3),
				position.z + std::sin(angle) * radius
			};
			const float scale = GetExperienceScale(denomination);
			object->GetTransform().scale = {scale, scale, scale};

			ExperienceComponent* experience = object->AddComponent<ExperienceComponent>();
			experience->SetExperience(denomination);
			experience->SetModelFilePath(resolvedModelFilePath);
			experience->SetTarget(target);

			Object3dComponent* object3d = object->AddComponent<Object3dComponent>();
			object3d->SetModel(resolvedModelFilePath);
			object3d->SetColor(GetExperienceColor(denomination));

			object->Update();
			GameObject* createdObject = object.get();
			sceneObjects_.push_back(std::move(object));
			if (!firstExperienceObject) {
				firstExperienceObject = createdObject;
			}
			++nextObjectId_;
		}
	}
	return firstExperienceObject;
}

GameObject* BaseScene::CreateRuntimeItemDrop(
	ItemDropType type, const Vector3& position, GameObject* target, float healAmount) {
	if (!target) {
		for (const auto& object : sceneObjects_) {
			if (object->GetComponent<Player>()) {
				target = object.get();
				break;
			}
		}
	}

	ModelManager::GetInstance()->LoadModel("sphere.obj");
	auto object = std::make_unique<GameObject>();
	const bool isHealthItem = type == ItemDropType::Health;
	object->SetName(MakeUniqueObjectName(isHealthItem ? "HealthItem" : "ExperienceCollector"));
	object->SetEditorType(isHealthItem ? "HealthItem" : "ExperienceCollector");
	object->GetTransform().translate = position + Vector3{isHealthItem ? -0.35f : 0.35f, 0.35f, 0.0f};
	const float scale = isHealthItem ? 0.42f : 0.50f;
	object->GetTransform().scale = {scale, scale, scale};

	ItemDropComponent* item = object->AddComponent<ItemDropComponent>();
	item->SetType(type);
	item->SetTarget(target);
	item->SetHealAmount(healAmount);

	Object3dComponent* object3d = object->AddComponent<Object3dComponent>();
	object3d->SetModel("sphere.obj");
	object3d->SetColor(isHealthItem
		? Vector4{0.15f, 1.0f, 0.25f, 1.0f}
		: Vector4{1.0f, 0.85f, 0.10f, 1.0f});

	object->Update();
	sceneObjects_.push_back(std::move(object));
	++nextObjectId_;
	return sceneObjects_.back().get();
}

void BaseScene::CreateRuntimeBossUpgradeDrop(const Vector3& position, GameObject* target, int upgradeCount) {
	if (!target) {
		for (const auto& object : sceneObjects_) {
			if (object->GetComponent<Player>()) {
				target = object.get();
				break;
			}
		}
	}

	ModelManager::GetInstance()->LoadModel("sphere.obj");
	auto object = std::make_unique<GameObject>();
	object->SetName(MakeUniqueObjectName("BossUpgradeReward"));
	object->SetEditorType("BossUpgradeReward");
	object->GetTransform().translate = position + Vector3{0.0f, 0.45f, 0.0f};
	object->GetTransform().scale = {0.65f, 0.65f, 0.65f};

	ExperienceComponent* reward = object->AddComponent<ExperienceComponent>();
	reward->SetBossUpgradeReward(true);
	reward->SetBossUpgradeCount(upgradeCount);
	reward->SetTarget(target);
	reward->SetAttractDistance(12.0f);
	reward->SetAttractSpeed(0.06f);

	Object3dComponent* object3d = object->AddComponent<Object3dComponent>();
	object3d->SetModel("sphere.obj");
	object3d->SetColor({1.0f, 0.72f, 0.08f, 1.0f});

	object->Update();
	sceneObjects_.push_back(std::move(object));
	++nextObjectId_;
}

int BaseScene::ApplyRandomBossUpgrades(Player* player, int upgradeCount) {
	if (!player || upgradeCount <= 0) {
		return 0;
	}

	struct UpgradeCandidate {
		bool isAttack = false;
		int slotIndex = -1;
		bool promoteToSuper = false;
	};
	static std::mt19937 rewardRandomEngine(std::random_device{}());
	PlayerStats upgradedStats = player->GetBaseStats();
	int appliedCount = 0;
	for (; appliedCount < upgradeCount; ++appliedCount) {
		std::vector<UpgradeCandidate> candidates;
		for (int index = 0; index < static_cast<int>(upgradedStats.attackSlots.size()); ++index) {
			const PlayerAttackSlot& slot = upgradedStats.attackSlots[index];
			if (!slot.enabled || slot.attackName.empty() || slot.attackLevel == "super") {
				continue;
			}
			const int level = std::atoi(slot.attackLevel.c_str());
			if (level >= 1 && level < 5) {
				candidates.push_back({true, index, false});
				continue;
			}
			if (slot.attackLevel != "5") {
				continue;
			}
			const PlayerAttackStats attackStats = LoadPlayerAttackStats(slot.attackName);
			bool conditionMet = !attackStats.superConditionStatusName.empty();
			const int requiredLevel = (std::max)(1, std::atoi(attackStats.superConditionStatusLevel.c_str()));
			for (const PlayerStatusSlot& statusSlot : upgradedStats.statusSlots) {
				if (conditionMet && statusSlot.enabled &&
				    statusSlot.statusName == attackStats.superConditionStatusName &&
				    std::atoi(statusSlot.level.c_str()) >= requiredLevel) {
					candidates.push_back({true, index, true});
					break;
				}
			}
		}
		for (int index = 0; index < static_cast<int>(upgradedStats.statusSlots.size()); ++index) {
			const PlayerStatusSlot& slot = upgradedStats.statusSlots[index];
			const int level = std::atoi(slot.level.c_str());
			if (slot.enabled && !slot.statusName.empty() && level >= 1 && level < 5) {
				candidates.push_back({false, index, false});
			}
		}
		if (candidates.empty()) {
			break;
		}
		std::uniform_int_distribution<size_t> candidateDistribution(0, candidates.size() - 1);
		const UpgradeCandidate& selected = candidates[candidateDistribution(rewardRandomEngine)];
		if (selected.isAttack) {
			PlayerAttackSlot& slot = upgradedStats.attackSlots[selected.slotIndex];
			slot.attackLevel = selected.promoteToSuper
			    ? "super"
			    : std::to_string((std::min)(5, std::atoi(slot.attackLevel.c_str()) + 1));
		} else {
			PlayerStatusSlot& slot = upgradedStats.statusSlots[selected.slotIndex];
			slot.level = std::to_string((std::min)(5, std::atoi(slot.level.c_str()) + 1));
		}
	}

	if (appliedCount > 0) {
		player->ApplyStats(upgradedStats, ApplyPlayerStatusItems(upgradedStats));
		if (GameObject* owner = player->GetOwner()) {
			ApplyPlayerAttackSlots(owner->GetComponent<PlayerAttackComponent>(), upgradedStats);
		}
	}
	return appliedCount;
}

void BaseScene::UpdateBossUpgradeRewards() {
	for (const auto& object : sceneObjects_) {
		ExperienceComponent* reward = object->GetComponent<ExperienceComponent>();
		if (!reward || !reward->IsBossUpgradeReward() || !reward->IsCollected() || reward->IsBossUpgradeApplied()) {
			continue;
		}
		Player* player = reward->GetTarget() ? reward->GetTarget()->GetComponent<Player>() : nullptr;
		const int appliedCount = ApplyRandomBossUpgrades(player, reward->GetBossUpgradeCount());
		QueueBossAcquisitionOffers(player, reward->GetBossUpgradeCount() - appliedCount);
		reward->MarkBossUpgradeApplied();
	}
}

void BaseScene::QueueBossAcquisitionOffers(Player* player, int offerCount) {
	if (!player || offerCount <= 0) {
		return;
	}
	const PlayerStats& stats = player->GetBaseStats();
	const bool hasEmptyAttackSlot = std::any_of(stats.attackSlots.begin(), stats.attackSlots.end(), [](const PlayerAttackSlot& slot) {
		return slot.attackName.empty();
	});
	const bool hasEmptyStatusSlot = std::any_of(stats.statusSlots.begin(), stats.statusSlots.end(), [](const PlayerStatusSlot& slot) {
		return slot.statusName.empty();
	});

	std::unordered_set<std::string> ownedAttacks;
	std::unordered_set<std::string> ownedStatuses;
	for (const PlayerAttackSlot& slot : stats.attackSlots) {
		if (!slot.attackName.empty()) {
			ownedAttacks.insert(slot.attackName);
		}
	}
	for (const PlayerStatusSlot& slot : stats.statusSlots) {
		if (!slot.statusName.empty()) {
			ownedStatuses.insert(slot.statusName);
		}
	}

	std::vector<LevelUpChoice> offers;
	if (hasEmptyAttackSlot) {
		for (const std::string& attackName : LoadPlayerAttackNames()) {
			if (ownedAttacks.find(attackName) != ownedAttacks.end()) {
				continue;
			}
			const PlayerAttackStats attackStats = LoadPlayerAttackStats(attackName);
			std::string description = "Acquire this attack at level 1?";
			std::string texture = attackStats.choiceTextureFilePath;
			for (const PlayerAttackLevelStats& levelStats : attackStats.levels) {
				if (levelStats.level == "1") {
					if (!levelStats.choiceDescription.empty()) {
						description = levelStats.choiceDescription;
					}
					if (texture.empty()) {
						texture = levelStats.choiceTextureFilePath;
					}
					break;
				}
			}
			offers.push_back({LevelUpChoiceType::NewAttack, attackName, "Acquire " + attackName, description, -1, texture});
		}
	}
	if (hasEmptyStatusSlot) {
		for (const std::string& statusName : LoadPlayerStatusItemNames()) {
			if (ownedStatuses.find(statusName) != ownedStatuses.end()) {
				continue;
			}
			const PlayerStatusItemStats statusStats = LoadPlayerStatusItemStats(statusName);
			const std::string description = statusStats.levelDescriptions[0].empty()
			    ? "Acquire this item at level 1?"
			    : statusStats.levelDescriptions[0];
			offers.push_back({
				LevelUpChoiceType::NewStatus,
				statusName,
				"Acquire " + statusName,
				description,
				-1,
				statusStats.levelTextureFilePaths[0]
			});
		}
	}
	if (offers.empty()) {
		return;
	}

	static std::mt19937 offerRandomEngine(std::random_device{}());
	std::shuffle(offers.begin(), offers.end(), offerRandomEngine);
	const int queuedCount = (std::min)(offerCount, static_cast<int>(offers.size()));
	bossAcquisitionOfferQueue_.insert(
	    bossAcquisitionOfferQueue_.end(),
	    offers.begin(),
	    offers.begin() + queuedCount
	);
	bossAcquisitionPlayer_ = player;
	if (!isLevelUpSelectionActive_) {
		ShowNextBossAcquisitionOffer();
	}
}

bool BaseScene::ShowNextBossAcquisitionOffer() {
	while (bossAcquisitionPlayer_ && !bossAcquisitionOfferQueue_.empty()) {
		LevelUpChoice offer = bossAcquisitionOfferQueue_.front();
		bossAcquisitionOfferQueue_.erase(bossAcquisitionOfferQueue_.begin());
		const PlayerStats& stats = bossAcquisitionPlayer_->GetBaseStats();
		if (offer.type == LevelUpChoiceType::NewAttack) {
			const auto owned = std::find_if(stats.attackSlots.begin(), stats.attackSlots.end(), [&offer](const PlayerAttackSlot& slot) {
				return slot.attackName == offer.name;
			});
			const auto empty = std::find_if(stats.attackSlots.begin(), stats.attackSlots.end(), [](const PlayerAttackSlot& slot) {
				return slot.attackName.empty();
			});
			if (owned != stats.attackSlots.end() || empty == stats.attackSlots.end()) {
				continue;
			}
			offer.slotIndex = static_cast<int>(std::distance(stats.attackSlots.begin(), empty));
		} else if (offer.type == LevelUpChoiceType::NewStatus) {
			const auto owned = std::find_if(stats.statusSlots.begin(), stats.statusSlots.end(), [&offer](const PlayerStatusSlot& slot) {
				return slot.statusName == offer.name;
			});
			const auto empty = std::find_if(stats.statusSlots.begin(), stats.statusSlots.end(), [](const PlayerStatusSlot& slot) {
				return slot.statusName.empty();
			});
			if (owned != stats.statusSlots.end() || empty == stats.statusSlots.end()) {
				continue;
			}
			offer.slotIndex = static_cast<int>(std::distance(stats.statusSlots.begin(), empty));
		} else {
			continue;
		}

		levelUpPlayer_ = bossAcquisitionPlayer_;
		levelUpChoices_.clear();
		levelUpChoices_.push_back(offer);
		levelUpChoices_.push_back({
			LevelUpChoiceType::Decline,
			"",
			"Do not acquire",
			"Skip this reward.",
			-1,
			""
		});
		selectedLevelUpChoiceIndex_ = 0;
		isBossAcquisitionOfferActive_ = true;
		isLevelUpSelectionActive_ = true;
		GameTime::SetPaused(true);
		return true;
	}
	bossAcquisitionPlayer_ = nullptr;
	return false;
}

void BaseScene::UpdateEnemyAttacks() {
	std::vector<EnemyShotRequest> shotRequests;
	for (const auto& object : sceneObjects_) {
		EnemyComponent* enemy = object->GetComponent<EnemyComponent>();
		if (!enemy || !enemy->IsEnabled() || enemy->GetCurrentHealth() <= 0.0f) {
			continue;
		}
		if (enemy->ConsumeSelfDestructRequest()) {
			GameObject* target = enemy->GetTarget();
			Player* player = target ? target->GetComponent<Player>() : nullptr;
			if (player && player->IsEnabled() && player->GetCurrentHealth() > 0.0f) {
				Vector3 toPlayer = target->GetTransform().translate - object->GetTransform().translate;
				toPlayer.y = 0.0f;
				if (Length(toPlayer) <= enemy->GetStats().selfDestructRadius) {
					player->TakeDamage(enemy->GetStats().attack);
				}
			}
			continue;
		}
		std::vector<EnemyShotRequest> requests = enemy->ConsumeShotRequests();
		shotRequests.insert(shotRequests.end(), requests.begin(), requests.end());

		if (TrailRendererComponent* trail = object->GetComponent<TrailRendererComponent>();
			trail && enemy->GetStats().behavior == EnemyBehaviorType::NightSlashBoss) {
			trail->SetEmitting(enemy->IsNightSlashAttacking());
		}

		if (Object3dComponent* object3d = object->GetComponent<Object3dComponent>()) {
			if (enemy->IsTornadoWarningActive()) {
				const float pulse = 0.45f + 0.55f * std::sin(enemy->GetTornadoWarningProgress() * 16.0f * MathConstants::kPi);
				object3d->SetColor(enemy->GetTornadoPatternIndex() == 0
				    ? Vector4{0.08f, 0.60f + 0.35f * pulse, 1.0f, 1.0f}
				    : enemy->GetTornadoPatternIndex() == 1
				        ? Vector4{0.65f + 0.30f * pulse, 0.12f, 1.0f, 1.0f}
				        : Vector4{0.08f, 0.65f + 0.30f * pulse, 0.30f, 1.0f});
			} else if (enemy->IsBossRangedWarningActive()) {
				const float pulse = 0.45f + 0.55f * std::sin(enemy->GetBossRangedProgress() * 14.0f * MathConstants::kPi);
				object3d->SetColor({0.05f, 0.35f + 0.35f * pulse, 1.0f, 1.0f});
			} else if (enemy->IsBossRangedAttacking()) {
				object3d->SetColor({0.12f, 0.70f, 1.0f, 1.0f});
			} else if (enemy->IsNightSlashWarningActive()) {
				const float pulse = 0.45f + 0.55f * std::sin(enemy->GetNightSlashProgress() * 14.0f * MathConstants::kPi);
				object3d->SetColor({0.62f + 0.28f * pulse, 0.04f, 0.82f + 0.18f * pulse, 1.0f});
			} else if (enemy->IsNightSlashAttacking()) {
				object3d->SetColor({0.95f, 0.18f, 1.0f, 1.0f});
			} else if (enemy->GetStats().behavior == EnemyBehaviorType::NightSlashBoss) {
				object3d->SetColor({0.42f, 0.06f, 0.62f, 1.0f});
			} else if (enemy->GetStats().behavior == EnemyBehaviorType::TornadoBoss) {
				object3d->SetColor({0.08f, 0.48f, 0.78f, 1.0f});
			} else if (enemy->IsChargeWarningActive()) {
				const float pulse = 0.45f + 0.55f * std::sin(enemy->GetChargeProgress() * 18.0f * MathConstants::kPi);
				object3d->SetColor({1.0f, 0.05f + 0.25f * pulse, 0.02f, 1.0f});
			} else if (enemy->GetStats().behavior == EnemyBehaviorType::Shooter) {
				object3d->SetColor({0.25f, 0.55f, 1.0f, 1.0f});
			} else if (enemy->GetStats().behavior == EnemyBehaviorType::Charger) {
				object3d->SetColor({1.0f, 0.35f, 0.08f, 1.0f});
			} else if (enemy->GetStats().behavior == EnemyBehaviorType::SelfDestruct) {
				const float pulse = enemy->IsSelfDestructArmed()
				    ? 0.45f + 0.55f * std::sin(enemy->GetSelfDestructProgress() * 20.0f * MathConstants::kPi)
				    : 0.0f;
				object3d->SetColor({1.0f, 0.72f * (1.0f - pulse), 0.02f, 1.0f});
			} else if (enemy->GetEnemyTypeName() == "MidBoss") {
				object3d->SetColor({0.62f, 0.16f, 0.85f, 1.0f});
			}
		}
	}
	for (const EnemyShotRequest& request : shotRequests) {
		CreateRuntimeEnemyProjectile(request);
	}
}

GameObject* BaseScene::CreateRuntimeEnemyProjectile(const EnemyShotRequest& request) {
	auto object = std::make_unique<GameObject>();
	const bool isContractingTornado = request.motionType == EnemyProjectileMotionType::ContractingOrbit;
	const bool isGiantTornado = request.motionType == EnemyProjectileMotionType::Homing;
	const bool isTornado = request.motionType == EnemyProjectileMotionType::ExpandingOrbit ||
	    request.motionType == EnemyProjectileMotionType::ContractingOrbit || isGiantTornado;
	object->SetName(MakeUniqueObjectName(
	    isGiantTornado ? "BossGiantTornado"
	    : isContractingTornado ? "BossConvergingTornado" : isTornado ? "BossTornado" : "EnemyProjectile"));
	object->SetEditorType("EnemyProjectile");
	object->GetTransform().translate = request.position;
	object->GetTransform().scale = {request.size, request.size, request.size};
	object->GetTransform().rotate.y = std::atan2(request.direction.x, request.direction.z);

	EnemyProjectileComponent* projectile = object->AddComponent<EnemyProjectileComponent>();
	projectile->SetDirection(request.direction);
	if (request.motionType == EnemyProjectileMotionType::ExpandingOrbit) {
		projectile->SetExpandingOrbit(
		    request.orbitCenter,
		    request.orbitAngle,
		    request.orbitInitialRadius,
		    request.orbitAngularSpeed,
		    request.orbitRadialSpeed,
		    request.orbitHeight);
	} else if (request.motionType == EnemyProjectileMotionType::ContractingOrbit) {
		projectile->SetContractingOrbit(
		    request.orbitCenter,
		    request.orbitAngle,
		    request.orbitInitialRadius,
		    request.orbitAngularSpeed,
		    request.orbitRadialSpeed,
		    request.orbitHeight);
	} else if (isGiantTornado) {
		projectile->SetHomingTarget(request.homingTarget);
	}
	projectile->SetSpeed(request.speed);
	projectile->SetAttack(request.attack);
	projectile->SetSize(request.size);
	projectile->SetLifeTime(request.lifeTime);

	SphereColliderComponent* collider = object->AddComponent<SphereColliderComponent>();
	// Transform の scale が弾サイズなので、半径 1.0 の球を設定すると表示と同じ大きさになる。
	collider->SetRadius(1.0f);
	collider->SetPushBackEnabled(false);

	ModelManager::GetInstance()->LoadModel("sphere.obj");
	Object3dComponent* object3d = object->AddComponent<Object3dComponent>();
	object3d->SetModel("sphere.obj");
	object3d->SetColor(isGiantTornado
	    ? Vector4{0.20f, 1.0f, 0.48f, 1.0f}
	    : isContractingTornado
	    ? Vector4{0.78f, 0.35f, 1.0f, 1.0f}
	    : isTornado ? Vector4{0.50f, 0.92f, 1.0f, 1.0f} : Vector4{1.0f, 0.12f, 0.04f, 1.0f});
	TrailRendererComponent* trail = object->AddComponent<TrailRendererComponent>();
	trail->SetWidth((std::max)(0.10f, request.size * (isTornado ? 1.6f : 0.8f)));
	trail->SetLifeTime(isTornado ? 1.15f : 0.28f);
	trail->SetHeadColor(isGiantTornado
	    ? Vector4{0.72f, 1.0f, 0.82f, 0.98f}
	    : isContractingTornado
	    ? Vector4{0.95f, 0.75f, 1.0f, 0.95f}
	    : isTornado ? Vector4{0.80f, 1.0f, 1.0f, 0.95f} : Vector4{1.0f, 0.35f, 0.05f, 0.95f});
	trail->SetTailColor(isGiantTornado
	    ? Vector4{0.02f, 0.42f, 0.16f, 0.0f}
	    : isContractingTornado
	    ? Vector4{0.35f, 0.02f, 0.60f, 0.0f}
	    : isTornado ? Vector4{0.05f, 0.30f, 0.55f, 0.0f} : Vector4{0.70f, 0.02f, 0.01f, 0.0f});

	object->Update();
	sceneObjects_.push_back(std::move(object));
	++nextObjectId_;
	return sceneObjects_.back().get();
}

void BaseScene::UpdateEnemyProjectileHits() {
	for (const auto& projectileObject : sceneObjects_) {
		EnemyProjectileComponent* projectile = projectileObject->GetComponent<EnemyProjectileComponent>();
		if (!projectile || !projectile->IsEnabled() || projectile->IsExpired()) {
			continue;
		}
		for (const auto& playerObject : sceneObjects_) {
			Player* player = playerObject->GetComponent<Player>();
			if (!player || !player->IsEnabled() || player->GetCurrentHealth() <= 0.0f) {
				continue;
			}

			const SphereColliderComponent* projectileCollider =
			    projectileObject->GetComponent<SphereColliderComponent>();
			const SphereColliderShape projectileSphere = projectileCollider
			    ? projectileCollider->GetWorldSphere()
			    : SphereColliderShape{projectileObject->GetTransform().translate, projectile->GetSize()};

			bool isHit = false;
			if (const OBBColliderComponent* playerCollider = playerObject->GetComponent<OBBColliderComponent>();
			    playerCollider && playerCollider->IsEnabled()) {
				isHit = IsCollisionOBBToSphere(playerCollider->GetWorldOBB(), projectileSphere);
			}
			if (!isHit) {
				if (const SphereColliderComponent* playerCollider = playerObject->GetComponent<SphereColliderComponent>();
				    playerCollider && playerCollider->IsEnabled()) {
					isHit = IsCollisionSphereToSphere(playerCollider->GetWorldSphere(), projectileSphere);
				}
			}
			if (!isHit &&
			    !playerObject->GetComponent<OBBColliderComponent>() &&
			    !playerObject->GetComponent<SphereColliderComponent>()) {
				const SphereColliderShape fallbackPlayerSphere{playerObject->GetTransform().translate, 0.5f};
				isHit = IsCollisionSphereToSphere(fallbackPlayerSphere, projectileSphere);
			}

			if (isHit) {
				player->TakeDamage(projectile->GetAttack());
				projectile->MarkHit();
				break;
			}
		}
	}
}

void BaseScene::UpdateExperienceCompression() {
	for (size_t denominationIndex = 0; denominationIndex + 1 < kExperienceDenominations.size(); ++denominationIndex) {
		const int denomination = kExperienceDenominations[denominationIndex];
		const int nextDenomination = kExperienceDenominations[denominationIndex + 1];
		const int requiredCount = nextDenomination / denomination;

		bool compressed = true;
		while (compressed) {
			compressed = false;
			for (const auto& anchorObject : sceneObjects_) {
				ExperienceComponent* anchor = anchorObject->GetComponent<ExperienceComponent>();
				if (!anchor || anchor->IsBossUpgradeReward() || anchor->IsCollected() || anchor->IsConsumedByCompression() ||
				    anchor->GetExperience() != denomination) {
					continue;
				}

				std::vector<GameObject*> nearbyObjects;
				nearbyObjects.reserve(requiredCount);
				nearbyObjects.push_back(anchorObject.get());
				const Vector3 anchorPosition = anchorObject->GetTransform().translate;
				for (const auto& candidateObject : sceneObjects_) {
					if (candidateObject.get() == anchorObject.get()) {
						continue;
					}
					ExperienceComponent* candidate = candidateObject->GetComponent<ExperienceComponent>();
					if (!candidate || candidate->IsBossUpgradeReward() || candidate->IsCollected() || candidate->IsConsumedByCompression() ||
					    candidate->GetExperience() != denomination) {
						continue;
					}
					if (Length(candidateObject->GetTransform().translate - anchorPosition) <= kExperienceCompressionDistance) {
						nearbyObjects.push_back(candidateObject.get());
						if (static_cast<int>(nearbyObjects.size()) == requiredCount) {
							break;
						}
					}
				}
				if (static_cast<int>(nearbyObjects.size()) < requiredCount) {
					continue;
				}

				Vector3 mergedPosition{};
				for (GameObject* object : nearbyObjects) {
					mergedPosition = mergedPosition + object->GetTransform().translate;
				}
				mergedPosition = (1.0f / static_cast<float>(requiredCount)) * mergedPosition;
				anchorObject->GetTransform().translate = mergedPosition;
				const float scale = GetExperienceScale(nextDenomination);
				anchorObject->GetTransform().scale = {scale, scale, scale};
				anchor->SetExperience(nextDenomination);
				if (Object3dComponent* object3d = anchorObject->GetComponent<Object3dComponent>()) {
					object3d->SetColor(GetExperienceColor(nextDenomination));
				}
				for (size_t index = 1; index < nearbyObjects.size(); ++index) {
					nearbyObjects[index]->GetComponent<ExperienceComponent>()->MarkConsumedByCompression();
				}
				compressed = true;
				break;
			}
		}
	}
}

void BaseScene::UpdateItemDrops() {
	for (const auto& itemObject : sceneObjects_) {
		ItemDropComponent* item = itemObject->GetComponent<ItemDropComponent>();
		if (!item || !item->IsCollected() || item->IsEffectApplied()) {
			continue;
		}

		if (item->GetType() == ItemDropType::CollectAllExperience) {
			for (const auto& experienceObject : sceneObjects_) {
				ExperienceComponent* experience = experienceObject->GetComponent<ExperienceComponent>();
				if (!experience || experience->IsBossUpgradeReward() || experience->IsCollected() ||
				    experience->IsConsumedByCompression()) {
					continue;
				}
				experience->SetAttractDistance((std::numeric_limits<float>::max)());
				experience->SetAttractSpeed(0.16f);
			}
		}
		item->MarkEffectApplied();
	}
}

GameObject* BaseScene::FindNearestEnemy(const Vector3& position) const {
	GameObject* nearest = nullptr;
	float nearestDistance = 0.0f;
	for (const auto& object : sceneObjects_) {
		EnemyComponent* enemy = object->GetComponent<EnemyComponent>();
		if (!enemy || !enemy->IsEnabled() || enemy->GetCurrentHealth() <= 0.0f) {
			continue;
		}

		const float distance = Length(object->GetTransform().translate - position);
		if (!nearest || distance < nearestDistance) {
			nearest = object.get();
			nearestDistance = distance;
		}
	}
	return nearest;
}

GameObject* BaseScene::CreateRuntimePlayerProjectile(const PlayerAttackShotRequest& request) {
	auto object = std::make_unique<GameObject>();
	object->SetName(MakeUniqueObjectName(request.attackName.empty() ? "PlayerAttack" : request.attackName));
	object->SetEditorType("PlayerProjectile");
	object->GetTransform().translate = request.position;
	object->GetTransform().scale = {request.size, request.size, request.size};

	PlayerProjectileComponent* projectile = object->AddComponent<PlayerProjectileComponent>();
	projectile->SetAttackName(request.attackName);
	projectile->SetLevel(request.level);
	projectile->SetDirection(request.direction);
	projectile->SetSpeed(request.speed);
	projectile->SetAttack(request.attack);
	projectile->SetSize(request.size);
	projectile->SetLifeTime(request.lifeTime);
	projectile->SetPierceCount(request.pierceCount);
	projectile->SetInfinitePierce(request.infinitePierce);
	projectile->SetHomingEnabled(request.homing);
	projectile->SetHomingAccuracy(request.homingAccuracy);
	projectile->SetMotionType(request.motionType);
	projectile->SetMotionAnchor(request.motionAnchor);
	projectile->SetOrbitAngleRadians(request.orbitAngleRadians);
	projectile->SetOrbitRadius(request.orbitRadius);
	projectile->SetOrbitHeight(request.orbitHeight);
	projectile->SetOrbitAngularSpeed(request.orbitAngularSpeed);
	projectile->SetTravelDistance(request.travelDistance);
	projectile->SetTravelOrigin(request.position);
	projectile->SetClawSlashIndex(request.clawSlashIndex);
	projectile->SetClawSlashCount(request.clawSlashCount);
	if (request.motionType == PlayerProjectileMotionType::Orbit) {
		projectile->SetRepeatHitInterval(0.75f);
	}
	if (request.homing) {
		projectile->SetHomingTarget(FindNearestEnemy(request.position));
	}
	if (request.motionType == PlayerProjectileMotionType::Boomerang) {
		if (GameObject* target = FindNearestEnemy(request.position)) {
			Vector3 toTarget = target->GetTransform().translate - request.position;
			toTarget.y = 0.0f;
			if (Length(toTarget) > MathConstants::kDirectionEpsilon) {
				projectile->SetDirection(toTarget);
			}
		}
	}

	const std::string modelFilePath = request.modelFilePath.empty() ? "sphere.obj" : request.modelFilePath;
	if (!ModelManager::GetInstance()->FindModel(modelFilePath)) {
		ModelManager::GetInstance()->LoadModel(modelFilePath);
	}
	if (request.motionType == PlayerProjectileMotionType::SkyLaser) {
		object->GetTransform().scale = {request.size * 0.30f, request.size * 5.0f, request.size * 0.30f};
	} else if (request.motionType == PlayerProjectileMotionType::ClawSlash) {
		object->GetTransform().scale = {request.size * 0.18f, request.size * 0.18f, request.size * 0.18f};
	}
	Object3dComponent* object3d = object->AddComponent<Object3dComponent>();
	if (ModelManager::GetInstance()->FindModel(modelFilePath)) {
		object3d->SetModel(modelFilePath);
	} else {
		ModelManager::GetInstance()->LoadModel("sphere.obj");
		object3d->SetModel("sphere.obj");
	}
	if (request.motionType == PlayerProjectileMotionType::ArcHoming) {
		object3d->SetModelTextureOverride("Resources/human/white.png");
		object3d->SetColor(GetArcHomingProjectileColor(request.colorIndex));
	} else if (request.motionType == PlayerProjectileMotionType::Orbit) {
		object3d->SetColor({0.35f, 0.75f, 1.0f, 1.0f});
	} else if (request.motionType == PlayerProjectileMotionType::SkyLaser) {
		object3d->SetColor({0.65f, 0.90f, 1.0f, 1.0f});
	} else if (request.motionType == PlayerProjectileMotionType::Boomerang) {
		object3d->SetColor({1.0f, 0.65f, 0.20f, 1.0f});
	} else if (request.motionType == PlayerProjectileMotionType::Ricochet) {
		object3d->SetColor({0.45f, 1.0f, 0.30f, 1.0f});
	} else if (request.motionType == PlayerProjectileMotionType::ClawSlash) {
		object3d->SetColor({1.0f, 0.22f, 0.10f, 1.0f});
	}
	if (request.motionType != PlayerProjectileMotionType::SkyLaser) {
		TrailRendererComponent* trail = object->AddComponent<TrailRendererComponent>();
		trail->SetWidth(request.motionType == PlayerProjectileMotionType::ClawSlash
		        ? (std::max)(0.10f, request.size * 0.22f)
		        : (std::max)(0.12f, request.size * 0.8f));
		trail->SetLifeTime(request.motionType == PlayerProjectileMotionType::Orbit
		        ? 0.45f
		        : request.motionType == PlayerProjectileMotionType::ClawSlash ? 0.20f : 0.32f);
		trail->SetMinSegmentLength((std::max)(0.025f, request.size * 0.08f));
		if (request.motionType == PlayerProjectileMotionType::ArcHoming) {
			Vector4 headColor = GetArcHomingProjectileColor(request.colorIndex);
			headColor.w = 0.95f;
			trail->SetHeadColor(headColor);
			trail->SetTailColor(GetArcHomingTrailTailColor(request.colorIndex));
		} else if (request.motionType == PlayerProjectileMotionType::Boomerang) {
			trail->SetHeadColor({1.0f, 0.72f, 0.20f, 0.95f});
			trail->SetTailColor({1.0f, 0.12f, 0.02f, 0.0f});
		} else if (request.motionType == PlayerProjectileMotionType::Orbit) {
			trail->SetPositionReference(request.motionAnchor);
			trail->SetHeadColor({0.45f, 0.90f, 1.0f, 0.9f});
			trail->SetTailColor({0.10f, 0.30f, 1.0f, 0.0f});
		} else if (request.motionType == PlayerProjectileMotionType::Ricochet) {
			trail->SetHeadColor({0.65f, 1.0f, 0.30f, 0.95f});
			trail->SetTailColor({0.05f, 0.80f, 0.15f, 0.0f});
		} else if (request.motionType == PlayerProjectileMotionType::ClawSlash) {
			trail->SetHeadColor({1.0f, 0.95f, 0.72f, 1.0f});
			trail->SetTailColor({1.0f, 0.05f, 0.01f, 0.0f});
		}
	}
	if (request.motionType == PlayerProjectileMotionType::ArcHoming) {
		constexpr const char* kGlowParticleGroup = "ArcHomingGlow";
		constexpr const char* kGlowTexture = "Resources/circle.png";
		if (!ParticleManager::GetInstance()->GetGroup(kGlowParticleGroup)) {
			ParticleManager::GetInstance()->CreateParticleGroup(kGlowParticleGroup, kGlowTexture, kMeshTypeQuad);
		}

		ParticleEmitterComponent* glowEmitter = object->AddComponent<ParticleEmitterComponent>();
		glowEmitter->SetGroupName(kGlowParticleGroup);
		glowEmitter->SetTexture(kGlowTexture);
		glowEmitter->SetBlendMode(kBlendModeAdd);
		glowEmitter->SetFrequency(0.020f);

		Vector4 glowColor = GetArcHomingProjectileColor(request.colorIndex);
		glowColor.x *= 1.35f;
		glowColor.y *= 1.35f;
		glowColor.z *= 1.35f;
		glowColor.w = 0.55f;

		ParticleEmitParam glowParam;
		const float glowScale = (std::max)(0.16f, request.size * 0.82f);
		glowParam.scale = {glowScale, glowScale, glowScale};
		glowParam.endScale = {glowScale * 0.12f, glowScale * 0.12f, glowScale * 0.12f};
		glowParam.baseVelocity = {0.0f, 0.008f, 0.0f};
		glowParam.randomVelocityRange = {0.018f, 0.018f, 0.018f};
		const float leakRadius = request.size * 0.42f;
		glowParam.randomPositionRange = {leakRadius, leakRadius, leakRadius};
		glowParam.lifeTime = 0.36f;
		glowParam.color = glowColor;
		glowParam.endColor = {glowColor.x * 0.55f, glowColor.y * 0.55f, glowColor.z * 0.55f, 0.0f};
		glowParam.randomScaleRange = {glowScale * 0.22f, glowScale * 0.22f, glowScale * 0.22f};
		glowParam.count = 2;
		glowParam.isBillboard = true;
		glowEmitter->SetParam(glowParam);
		// Component::Update より前に即時発生させるため、初回分も弾の生成位置へ同期しておく。
		glowEmitter->SetTranslate(request.position);
		glowEmitter->Emit();
	}

	object->Update();
	sceneObjects_.push_back(std::move(object));
	++nextObjectId_;
	return sceneObjects_.back().get();
}

void BaseScene::UpdatePlayerProjectileHits() {
	struct ExperienceDropRequest {
		EnemyStats stats;
		Vector3 position;
		GameObject* target = nullptr;
	};
	struct BossUpgradeDropRequest {
		Vector3 position;
		GameObject* target = nullptr;
		int upgradeCount = 1;
	};
	struct ItemDropRequest {
		ItemDropType type = ItemDropType::Health;
		Vector3 position;
		GameObject* target = nullptr;
		float healAmount = 0.0f;
	};
	std::vector<ExperienceDropRequest> experienceDropRequests;
	std::vector<BossUpgradeDropRequest> bossUpgradeDropRequests;
	std::vector<ItemDropRequest> itemDropRequests;
	Camera* camera = Object3dCommon::GetInstance() ? Object3dCommon::GetInstance()->GetDefaultCamera() : nullptr;
	for (const auto& projectileObject : sceneObjects_) {
		PlayerProjectileComponent* projectile = projectileObject->GetComponent<PlayerProjectileComponent>();
		if (!projectile || projectile->IsExpired() ||
		    projectile->GetMotionType() != PlayerProjectileMotionType::Ricochet) {
			continue;
		}

		Vector3& position = projectileObject->GetTransform().translate;
		Vector3 currentNdc{};
		Vector3 aheadNdc{};
		if (camera && ProjectToNdc(camera, position, currentNdc) &&
		    ProjectToNdc(camera, position + projectile->GetDirection(), aheadNdc)) {
			const float edgeInset = (std::clamp)(0.025f + projectile->GetSize() * 0.02f, 0.025f, 0.20f);
			const float boundary = 1.0f - edgeInset;
			const bool hitHorizontalEdge = currentNdc.x < -boundary || currentNdc.x > boundary;
			const bool hitVerticalEdge = currentNdc.y < -boundary || currentNdc.y > boundary;
			if (hitHorizontalEdge || hitVerticalEdge) {
				Vector2 screenDirection{aheadNdc.x - currentNdc.x, aheadNdc.y - currentNdc.y};
				if (hitHorizontalEdge) {
					screenDirection.x = -screenDirection.x;
				}
				if (hitVerticalEdge) {
					screenDirection.y = -screenDirection.y;
				}
				const Vector2 clampedNdc{
				    (std::clamp)(currentNdc.x, -boundary, boundary),
				    (std::clamp)(currentNdc.y, -boundary, boundary)
				};
				const Vector2 reflectedNdc{clampedNdc.x + screenDirection.x, clampedNdc.y + screenDirection.y};
				Vector3 boundaryPoint{};
				Vector3 reflectedPoint{};
				if (IntersectScreenRayToHeight(camera, clampedNdc, position.y, boundaryPoint) &&
				    IntersectScreenRayToHeight(camera, reflectedNdc, position.y, reflectedPoint)) {
					position = boundaryPoint;
					projectile->SetDirection(reflectedPoint - boundaryPoint);
				}
			}
		}

		SphereColliderShape projectileSphere{position, projectile->GetSize()};
		for (const auto& obstacleObject : sceneObjects_) {
			if (obstacleObject.get() == projectileObject.get() ||
			    obstacleObject->GetComponent<Player>() ||
			    obstacleObject->GetComponent<EnemyComponent>() ||
			    obstacleObject->GetComponent<PlayerProjectileComponent>()) {
				continue;
			}

			Vector3 surfaceNormal{};
			float penetration = 0.0f;
			bool collided = false;
			if (OBBColliderComponent* obstacle = obstacleObject->GetComponent<OBBColliderComponent>();
			    obstacle && obstacle->IsEnabled() && obstacle->GetPushBackEnabled()) {
				collided = BaseSceneCollisionHelpers::CalculateOBBSpherePushBack(obstacle->GetWorldOBB(), projectileSphere, surfaceNormal, penetration);
			}
			if (!collided) {
				if (SphereColliderComponent* obstacle = obstacleObject->GetComponent<SphereColliderComponent>();
				    obstacle && obstacle->IsEnabled() && obstacle->GetPushBackEnabled()) {
					Vector3 projectileToObstacle{};
					collided = BaseSceneCollisionHelpers::CalculateSphereSpherePushBack(projectileSphere, obstacle->GetWorldSphere(), projectileToObstacle, penetration);
					surfaceNormal = -1.0f * projectileToObstacle;
				}
			}
			if (!collided) {
				continue;
			}

			surfaceNormal.y = 0.0f;
			if (Length(surfaceNormal) <= MathConstants::kDirectionEpsilon) {
				continue;
			}
			surfaceNormal = NormalizeReturnVector(surfaceNormal);
			const float incomingAmount = Dot(projectile->GetDirection(), surfaceNormal);
			if (incomingAmount >= 0.0f) {
				continue;
			}
			position = position + (penetration + 0.01f) * surfaceNormal;
			projectile->SetDirection(projectile->GetDirection() - (2.0f * incomingAmount) * surfaceNormal);
			projectileObject->GetTransform().rotate.y = std::atan2(projectile->GetDirection().x, projectile->GetDirection().z);
			break;
		}
	}
	for (const auto& projectileObject : sceneObjects_) {
		PlayerProjectileComponent* projectile = projectileObject->GetComponent<PlayerProjectileComponent>();
		if (!projectile || !projectile->IsEnabled() || projectile->IsExpired()) {
			continue;
		}

		for (const auto& enemyObject : sceneObjects_) {
			EnemyComponent* enemy = enemyObject->GetComponent<EnemyComponent>();
			if (!enemy || !enemy->IsEnabled() || enemy->GetCurrentHealth() <= 0.0f) {
				continue;
			}
			if (projectile->HasHitObject(enemyObject.get())) {
				continue;
			}

			float distance = Length(enemyObject->GetTransform().translate - projectileObject->GetTransform().translate);
			float hitRadius = projectile->GetSize() + 0.5f * enemy->GetStats().sizeScale;
			if (projectile->GetMotionType() == PlayerProjectileMotionType::SkyLaser) {
				Vector3 horizontalDifference = enemyObject->GetTransform().translate - projectileObject->GetTransform().translate;
				horizontalDifference.y = 0.0f;
				distance = Length(horizontalDifference);
				hitRadius = projectile->GetSize() * 0.35f + 0.5f * enemy->GetStats().sizeScale;
			}
			if (distance <= hitRadius) {
				enemy->SetCurrentHealth(enemy->GetCurrentHealth() - projectile->GetAttack());
				if (enemy->GetCurrentHealth() <= 0.0f) {
					++defeatedEnemyCount_;
					experienceDropRequests.push_back({enemy->GetStats(), enemyObject->GetTransform().translate, enemy->GetTarget()});
					static std::mt19937 itemDropRandomEngine(std::random_device{}());
					std::uniform_real_distribution<float> itemDropDistribution(0.0f, 1.0f);
					const EnemyStats& enemyStats = enemy->GetStats();
					if (itemDropDistribution(itemDropRandomEngine) < enemyStats.healthItemDropChance) {
						itemDropRequests.push_back({
							ItemDropType::Health, enemyObject->GetTransform().translate,
							enemy->GetTarget(), enemyStats.healthItemHealAmount
						});
					}
					if (itemDropDistribution(itemDropRandomEngine) < enemyStats.collectExperienceItemDropChance) {
						itemDropRequests.push_back({
							ItemDropType::CollectAllExperience, enemyObject->GetTransform().translate,
							enemy->GetTarget(), 0.0f
						});
					}
					const std::string& enemyTypeName = enemy->GetEnemyTypeName();
					const bool dropsBossUpgradeReward =
					    enemyTypeName == "MidBoss" ||
					    enemyTypeName == "ChaserMidBoss" ||
					    enemyTypeName == "ShooterMidBoss" ||
					    enemyTypeName == "ChargerMidBoss" ||
					    enemyTypeName == "Stage2Boss";
					if (dropsBossUpgradeReward) {
						static std::mt19937 bossRewardRandomEngine(std::random_device{}());
						std::uniform_int_distribution<int> rewardRollDistribution(1, 100);
						const int rewardRoll = rewardRollDistribution(bossRewardRandomEngine);
						const int upgradeCount = rewardRoll <= 70 ? 1 : rewardRoll <= 95 ? 3 : 5;
						bossUpgradeDropRequests.push_back({
							enemyObject->GetTransform().translate,
							enemy->GetTarget(),
							upgradeCount
						});
					}
				}
				projectile->RegisterHitObject(enemyObject.get());
				break;
			}
		}
	}

	for (const ExperienceDropRequest& request : experienceDropRequests) {
		CreateRuntimeExperience(request.stats, request.position, request.target);
	}
	for (const BossUpgradeDropRequest& request : bossUpgradeDropRequests) {
		CreateRuntimeBossUpgradeDrop(request.position, request.target, request.upgradeCount);
	}
	for (const ItemDropRequest& request : itemDropRequests) {
		CreateRuntimeItemDrop(request.type, request.position, request.target, request.healAmount);
	}
}

void BaseScene::CleanupExpiredPlayerProjectiles() {
	sceneObjects_.erase(
	    std::remove_if(sceneObjects_.begin(), sceneObjects_.end(), [](const std::unique_ptr<GameObject>& object) {
		    PlayerProjectileComponent* projectile = object->GetComponent<PlayerProjectileComponent>();
		    if (projectile) {
			    const float viewMargin = 0.05f + projectile->GetSize() * 0.02f;
			    const PlayerProjectileMotionType motionType = projectile->GetMotionType();
			    const bool shouldExpireOutsideView =
			        motionType == PlayerProjectileMotionType::Linear ||
			        motionType == PlayerProjectileMotionType::ArcHoming;
			    if (projectile->IsExpired() ||
				    (shouldExpireOutsideView && IsPointOutsideView(object->GetTransform().translate, viewMargin))) {
				    return true;
			    }
		    }
		    EnemyProjectileComponent* enemyProjectile = object->GetComponent<EnemyProjectileComponent>();
		    if (enemyProjectile && enemyProjectile->IsExpired()) {
			    return true;
		    }
		    EnemyComponent* enemy = object->GetComponent<EnemyComponent>();
		    if (enemy && enemy->GetCurrentHealth() <= 0.0f) {
			    return true;
		    }
		    ExperienceComponent* experience = object->GetComponent<ExperienceComponent>();
		    if (experience && (experience->IsCollected() || experience->IsConsumedByCompression())) {
			    return true;
		    }
		    ItemDropComponent* item = object->GetComponent<ItemDropComponent>();
		    return item && item->IsCollected() && item->IsEffectApplied();
	    }),
	    sceneObjects_.end()
	);
	if (selectedObjectIndex_ >= static_cast<int>(sceneObjects_.size())) {
		selectedObjectIndex_ = static_cast<int>(sceneObjects_.size()) - 1;
	}
}

void BaseScene::UpdatePlayerHealthHud() {
	Player* player = nullptr;
	for (const auto& object : sceneObjects_) {
		Player* candidate = object->GetComponent<Player>();
		if (candidate && candidate->IsEnabled()) {
			player = candidate;
			break;
		}
	}

	isPlayerHealthHudVisible_ = player != nullptr;
	if (!player) {
		return;
	}

	if (!playerHealthBarBackground_) {
		playerHealthBarBackground_ = std::make_unique<Sprite>();
		playerHealthBarBackground_->Initialize("Resources/human/white.png");
		playerHealthBarBackground_->SetColor({0.08f, 0.08f, 0.08f, 1.0f});
	}
	if (!playerHealthBarFill_) {
		playerHealthBarFill_ = std::make_unique<Sprite>();
		playerHealthBarFill_->Initialize("Resources/human/white.png");
	}

	const float screenWidth = static_cast<float>(Input::GetInstance()->GetClientWidth());
	constexpr float kRightMargin = 24.0f;
	constexpr float kTopMargin = 24.0f;
	constexpr float kBarWidth = 260.0f;
	constexpr float kBarHeight = 28.0f;
	constexpr float kBorderSize = 4.0f;
	const float left = (std::max)(kRightMargin, screenWidth - kRightMargin - kBarWidth);

	EulerTransform backgroundTransform = playerHealthBarBackground_->GetTransform();
	backgroundTransform.translate = {left, kTopMargin, 0.0f};
	playerHealthBarBackground_->SetTransform(backgroundTransform);
	playerHealthBarBackground_->SetSize({kBarWidth, kBarHeight});

	const float maxHealth = player->GetMaxHealth();
	float healthRate = maxHealth > 0.0f ? player->GetCurrentHealth() / maxHealth : 0.0f;
	healthRate = (std::max)(0.0f, (std::min)(1.0f, healthRate));
	EulerTransform fillTransform = playerHealthBarFill_->GetTransform();
	fillTransform.translate = {left + kBorderSize, kTopMargin + kBorderSize, 0.0f};
	playerHealthBarFill_->SetTransform(fillTransform);
	playerHealthBarFill_->SetSize({(kBarWidth - kBorderSize * 2.0f) * healthRate, kBarHeight - kBorderSize * 2.0f});
	playerHealthBarFill_->SetColor({1.0f - healthRate, healthRate, 0.08f, 1.0f});

	playerHealthBarBackground_->Update();
	playerHealthBarFill_->Update();
}

void BaseScene::UpdatePlayerExperienceHud() {
	Player* player = nullptr;
	for (const auto& object : sceneObjects_) {
		Player* candidate = object->GetComponent<Player>();
		if (candidate && candidate->IsEnabled()) {
			player = candidate;
			break;
		}
	}

	isPlayerExperienceHudVisible_ = player != nullptr;
	if (!player) {
		playerExperienceRate_ = 0.0f;
		return;
	}

	if (!playerExperienceBarBackground_) {
		playerExperienceBarBackground_ = std::make_unique<Sprite>();
		playerExperienceBarBackground_->Initialize("Resources/human/white.png");
	}
	if (!playerExperienceBarFill_) {
		playerExperienceBarFill_ = std::make_unique<Sprite>();
		playerExperienceBarFill_->Initialize("Resources/human/white.png");
	}
	if (!playerExperienceTextObject_) {
		playerExperienceTextObject_ = std::make_unique<GameObject>();
		TextComponent* text = playerExperienceTextObject_->AddComponent<TextComponent>();
		text->SetFontSize(17.0f);
		text->SetAnchor(TextComponent::Anchor::BottomCenter);
		text->SetColor({0.92f, 0.96f, 1.0f, 1.0f});
	}

	const PlayerStats& stats = player->GetBaseStats();
	const int level = (std::max)(1, stats.level);
	const int previousLevelExperience = level > 1 ? Player::GetRequiredExperienceForNextLevel(level - 1) : 0;
	const int nextLevelExperience = Player::GetRequiredExperienceForNextLevel(level);
	const long long requiredExperience = (std::max)(1LL, static_cast<long long>(nextLevelExperience) - previousLevelExperience);
	const long long currentExperience = (std::max)(0LL, static_cast<long long>(stats.experience) - previousLevelExperience);
	playerExperienceRate_ = static_cast<float>(currentExperience) / static_cast<float>(requiredExperience);
	playerExperienceRate_ = (std::max)(0.0f, (std::min)(1.0f, playerExperienceRate_));

	const float screenWidth = static_cast<float>(Input::GetInstance()->GetClientWidth());
	const float screenHeight = static_cast<float>(Input::GetInstance()->GetClientHeight());
	constexpr float kHorizontalMargin = 32.0f;
	constexpr float kBottomMargin = 18.0f;
	constexpr float kBarHeight = 18.0f;
	constexpr float kBorderSize = 3.0f;
	const float barWidth = (std::max)(160.0f, screenWidth - kHorizontalMargin * 2.0f);
	const float left = (screenWidth - barWidth) * 0.5f;
	const float top = (std::max)(0.0f, screenHeight - kBottomMargin - kBarHeight);

	EulerTransform backgroundTransform = playerExperienceBarBackground_->GetTransform();
	backgroundTransform.translate = {left, top, 0.0f};
	playerExperienceBarBackground_->SetTransform(backgroundTransform);
	playerExperienceBarBackground_->SetSize({barWidth, kBarHeight});
	playerExperienceBarBackground_->SetColor({0.025f, 0.04f, 0.07f, 0.94f});

	EulerTransform fillTransform = playerExperienceBarFill_->GetTransform();
	fillTransform.translate = {left + kBorderSize, top + kBorderSize, 0.0f};
	playerExperienceBarFill_->SetTransform(fillTransform);
	playerExperienceBarFill_->SetSize({(barWidth - kBorderSize * 2.0f) * playerExperienceRate_, kBarHeight - kBorderSize * 2.0f});
	playerExperienceBarFill_->SetColor({0.16f, 0.72f, 1.0f, 1.0f});

	playerExperienceBarBackground_->Update();
	playerExperienceBarFill_->Update();
	playerExperienceTextObject_->GetTransform().translate = {screenWidth * 0.5f, top - 3.0f, 0.0f};
	playerExperienceTextObject_->GetComponent<TextComponent>()->SetText(
	    "LV " + std::to_string(level) + "    EXP " + std::to_string(currentExperience) + " / " + std::to_string(requiredExperience));
}

void BaseScene::DrawPlayerExperienceHud() {
	if (!isPlayerExperienceHudVisible_ || !playerExperienceBarBackground_ || !playerExperienceBarFill_) return;
	SpriteCommon::GetInstance()->SetDraw(kBlendModeNormal);
	playerExperienceBarBackground_->Draw();
	if (playerExperienceRate_ > 0.0f) playerExperienceBarFill_->Draw();
	if (playerExperienceTextObject_) playerExperienceTextObject_->Draw2D();
}

void BaseScene::UpdatePlayerSlotHud() {
	Player* player = nullptr;
	for (const auto& object : sceneObjects_) {
		Player* candidate = object->GetComponent<Player>();
		if (candidate && candidate->IsEnabled()) {
			player = candidate;
			break;
		}
	}

	isPlayerSlotHudVisible_ = player != nullptr;
	if (!player) {
		playerAttackSlotIconVisible_.fill(false);
		playerStatusSlotIconVisible_.fill(false);
		return;
	}

	auto createSprite = []() {
		auto sprite = std::make_unique<Sprite>();
		sprite->Initialize("Resources/human/white.png");
		return sprite;
	};
	auto createLabel = [](const std::string& label) {
		auto object = std::make_unique<GameObject>();
		TextComponent* text = object->AddComponent<TextComponent>();
		text->SetText(label);
		text->SetFontSize(15.0f);
		text->SetAnchor(TextComponent::Anchor::CenterRight);
		text->SetColor({1.0f, 1.0f, 1.0f, 0.9f});
		return object;
	};
	for (int index = 0; index < 5; ++index) {
		if (!playerAttackSlotBackgroundSprites_[index]) playerAttackSlotBackgroundSprites_[index] = createSprite();
		if (!playerAttackSlotIconSprites_[index]) playerAttackSlotIconSprites_[index] = createSprite();
		if (!playerStatusSlotBackgroundSprites_[index]) playerStatusSlotBackgroundSprites_[index] = createSprite();
		if (!playerStatusSlotIconSprites_[index]) playerStatusSlotIconSprites_[index] = createSprite();
	}
	if (!playerAttackSlotLabelObject_) playerAttackSlotLabelObject_ = createLabel("ATTACK");
	if (!playerStatusSlotLabelObject_) playerStatusSlotLabelObject_ = createLabel("STATUS");

	const float screenWidth = static_cast<float>(Input::GetInstance()->GetClientWidth());
	constexpr float kRightMargin = 24.0f;
	constexpr float kTop = 68.0f;
	constexpr float kLabelWidth = 64.0f;
	constexpr float kSlotSize = 46.0f;
	constexpr float kSlotGap = 5.0f;
	constexpr float kRowGap = 7.0f;
	constexpr float kInnerMargin = 3.0f;
	const float slotsWidth = kSlotSize * 5.0f + kSlotGap * 4.0f;
	const float slotsLeft = (std::max)(kRightMargin + kLabelWidth, screenWidth - kRightMargin - slotsWidth);
	const float labelRight = slotsLeft - 8.0f;
	const float attackY = kTop;
	const float statusY = attackY + kSlotSize + kRowGap;
	playerAttackSlotLabelObject_->GetTransform().translate = {labelRight, attackY + kSlotSize * 0.5f, 0.0f};
	playerStatusSlotLabelObject_->GetTransform().translate = {labelRight, statusY + kSlotSize * 0.5f, 0.0f};

	const PlayerStats& stats = player->GetBaseStats();
	for (int index = 0; index < 5; ++index) {
		const float slotX = slotsLeft + static_cast<float>(index) * (kSlotSize + kSlotGap);
		const PlayerAttackSlot& attackSlot = stats.attackSlots[index];
		const PlayerStatusSlot& statusSlot = stats.statusSlots[index];
		const bool hasAttack = !attackSlot.attackName.empty();
		const bool hasStatus = !statusSlot.statusName.empty();

		auto updateBackground = [slotX, kSlotSize](Sprite* sprite, float y, const Vector4& color) {
			EulerTransform transform = sprite->GetTransform();
			transform.translate = {slotX, y, 0.0f};
			sprite->SetTransform(transform);
			sprite->SetSize({kSlotSize, kSlotSize});
			sprite->SetColor(color);
			sprite->Update();
		};
		updateBackground(playerAttackSlotBackgroundSprites_[index].get(), attackY,
		    hasAttack ? (attackSlot.enabled ? Vector4{0.12f, 0.28f, 0.52f, 0.95f} : Vector4{0.18f, 0.20f, 0.24f, 0.75f})
		              : Vector4{0.05f, 0.06f, 0.08f, 0.82f});
		updateBackground(playerStatusSlotBackgroundSprites_[index].get(), statusY,
		    hasStatus ? (statusSlot.enabled ? Vector4{0.10f, 0.42f, 0.25f, 0.95f} : Vector4{0.18f, 0.20f, 0.24f, 0.75f})
		              : Vector4{0.05f, 0.06f, 0.08f, 0.82f});

		const std::string attackTextureKey = hasAttack ? attackSlot.attackName + "#" + attackSlot.attackLevel : std::string{};
		const bool attackTextureChanged = playerAttackSlotTextureKeys_[index] != attackTextureKey;
		if (attackTextureChanged) {
			playerAttackSlotTextureKeys_[index] = attackTextureKey;
			playerAttackSlotTexturePaths_[index].clear();
			if (hasAttack) {
				const PlayerAttackStats attackStats = LoadPlayerAttackStats(attackSlot.attackName);
				playerAttackSlotTexturePaths_[index] = attackStats.choiceTextureFilePath;
				if (attackSlot.attackLevel == "super") {
					for (const PlayerAttackLevelStats& levelStats : attackStats.levels) {
						if (levelStats.level == "super") {
							playerAttackSlotTexturePaths_[index] = levelStats.choiceTextureFilePath;
							break;
						}
					}
				}
			}
		}
		const std::string statusTextureKey = hasStatus ? statusSlot.statusName + "#" + statusSlot.level : std::string{};
		const bool statusTextureChanged = playerStatusSlotTextureKeys_[index] != statusTextureKey;
		if (statusTextureChanged) {
			playerStatusSlotTextureKeys_[index] = statusTextureKey;
			playerStatusSlotTexturePaths_[index].clear();
			if (hasStatus) {
				const PlayerStatusItemStats statusStats = LoadPlayerStatusItemStats(statusSlot.statusName);
				playerStatusSlotTexturePaths_[index] = statusStats.levelTextureFilePaths[PlayerStatusSlotLevelToIndex(statusSlot.level)];
			}
		}
		const std::string& attackTexture = playerAttackSlotTexturePaths_[index];
		const std::string& statusTexture = playerStatusSlotTexturePaths_[index];

		auto updateIcon = [slotX, kSlotSize, kInnerMargin](Sprite* sprite, float y, const std::string& texture, bool enabled) {
			if (sprite->GetTextureFilePath() != texture) sprite->SetTexture(texture);
			EulerTransform transform = sprite->GetTransform();
			transform.translate = {slotX + kInnerMargin, y + kInnerMargin, 0.0f};
			sprite->SetTransform(transform);
			sprite->SetSize({kSlotSize - kInnerMargin * 2.0f, kSlotSize - kInnerMargin * 2.0f});
			sprite->SetColor(enabled ? Vector4{1.0f, 1.0f, 1.0f, 1.0f} : Vector4{0.55f, 0.55f, 0.55f, 0.55f});
			sprite->Update();
		};
		playerAttackSlotIconVisible_[index] = hasAttack && !attackTexture.empty() && std::filesystem::exists(attackTexture);
		playerStatusSlotIconVisible_[index] = hasStatus && !statusTexture.empty() && std::filesystem::exists(statusTexture);
		if (playerAttackSlotIconVisible_[index]) updateIcon(playerAttackSlotIconSprites_[index].get(), attackY, attackTexture, attackSlot.enabled);
		if (playerStatusSlotIconVisible_[index]) updateIcon(playerStatusSlotIconSprites_[index].get(), statusY, statusTexture, statusSlot.enabled);
	}
}

void BaseScene::DrawPlayerSlotHud() {
	if (!isPlayerSlotHudVisible_) return;
	SpriteCommon::GetInstance()->SetDraw(kBlendModeNormal);
	for (int index = 0; index < 5; ++index) {
		playerAttackSlotBackgroundSprites_[index]->Draw();
		if (playerAttackSlotIconVisible_[index]) playerAttackSlotIconSprites_[index]->Draw();
		playerStatusSlotBackgroundSprites_[index]->Draw();
		if (playerStatusSlotIconVisible_[index]) playerStatusSlotIconSprites_[index]->Draw();
	}
	if (playerAttackSlotLabelObject_) playerAttackSlotLabelObject_->Draw2D();
	if (playerStatusSlotLabelObject_) playerStatusSlotLabelObject_->Draw2D();
}

/// <summary>
/// 有効なコライダー同士の当たり判定と押し戻しを行います。
/// </summary>
void BaseScene::UpdateColliderCollisions() {
	std::vector<EnemyPlayerContact> enemyPlayerContacts;
	std::vector<OBBColliderComponent*> colliders;
	colliders.reserve(sceneObjects_.size());
	std::vector<SphereColliderComponent*> sphereColliders;
	sphereColliders.reserve(sceneObjects_.size());

	for (const auto& object : sceneObjects_) {
		OBBColliderComponent* collider = object->GetComponent<OBBColliderComponent>();
		if (collider && collider->IsEnabled()) {
			collider->SetColliding(false);
			colliders.push_back(collider);
		} else if (collider) {
			collider->SetColliding(false);
		}
		SphereColliderComponent* sphereCollider = object->GetComponent<SphereColliderComponent>();
		if (sphereCollider && sphereCollider->IsEnabled()) {
			sphereCollider->SetColliding(false);
			sphereColliders.push_back(sphereCollider);
		} else if (sphereCollider) {
			sphereCollider->SetColliding(false);
		}
	}

	for (size_t i = 0; i < colliders.size(); ++i) {
		for (size_t j = i + 1; j < colliders.size(); ++j) {
			GameObject* ownerA = colliders[i]->GetOwner();
			GameObject* ownerB = colliders[j]->GetOwner();
			if (ShouldSkipColliderPair(ownerA, ownerB)) {
				continue;
			}
			const OBBColliderShape colliderA = colliders[i]->GetWorldOBB();
			const OBBColliderShape colliderB = colliders[j]->GetWorldOBB();
			if (IsCollisionOBBToOBB(colliderA, colliderB)) {
				colliders[i]->SetColliding(true);
				colliders[j]->SetColliding(true);
				if (RegisterEnemyPlayerContact(ownerA, ownerB, enemyPlayerContacts)) {
					continue;
				}
				Vector3 direction{};
				float penetration = 0.0f;
				if (BaseSceneCollisionHelpers::CalculateOBBOBBPushBack(colliderA, colliderB, direction, penetration)) {
					BaseSceneCollisionHelpers::ApplyColliderPushBack(
					    ownerA,
					    colliders[i]->GetPushBackEnabled(),
					    ownerB,
					    colliders[j]->GetPushBackEnabled(),
					    direction,
					    penetration
					);
				}
			}
		}
	}

	for (size_t i = 0; i < sphereColliders.size(); ++i) {
		for (size_t j = i + 1; j < sphereColliders.size(); ++j) {
			GameObject* ownerA = sphereColliders[i]->GetOwner();
			GameObject* ownerB = sphereColliders[j]->GetOwner();
			if (ShouldSkipColliderPair(ownerA, ownerB)) {
				continue;
			}
			const SphereColliderShape sphereA = sphereColliders[i]->GetWorldSphere();
			const SphereColliderShape sphereB = sphereColliders[j]->GetWorldSphere();
			if (IsCollisionSphereToSphere(sphereA, sphereB)) {
				sphereColliders[i]->SetColliding(true);
				sphereColliders[j]->SetColliding(true);
				if (RegisterEnemyPlayerContact(ownerA, ownerB, enemyPlayerContacts)) {
					continue;
				}
				Vector3 direction{};
				float penetration = 0.0f;
				if (BaseSceneCollisionHelpers::CalculateSphereSpherePushBack(sphereA, sphereB, direction, penetration)) {
					BaseSceneCollisionHelpers::ApplyColliderPushBack(
					    ownerA,
					    sphereColliders[i]->GetPushBackEnabled(),
					    ownerB,
					    sphereColliders[j]->GetPushBackEnabled(),
					    direction,
					    penetration
					);
				}
			}
		}
	}

	for (OBBColliderComponent* collider : colliders) {
		for (SphereColliderComponent* sphereCollider : sphereColliders) {
			GameObject* obbOwner = collider->GetOwner();
			GameObject* sphereOwner = sphereCollider->GetOwner();
			if (ShouldSkipColliderPair(obbOwner, sphereOwner)) {
				continue;
			}

			const OBBColliderShape obb = collider->GetWorldOBB();
			const SphereColliderShape sphere = sphereCollider->GetWorldSphere();
			if (IsCollisionOBBToSphere(obb, sphere)) {
				collider->SetColliding(true);
				sphereCollider->SetColliding(true);
				if (RegisterEnemyPlayerContact(obbOwner, sphereOwner, enemyPlayerContacts)) {
					continue;
				}
				Vector3 direction{};
				float penetration = 0.0f;
				if (BaseSceneCollisionHelpers::CalculateOBBSpherePushBack(obb, sphere, direction, penetration)) {
					BaseSceneCollisionHelpers::ApplyColliderPushBack(
					    obbOwner,
					    collider->GetPushBackEnabled(),
					    sphereOwner,
					    sphereCollider->GetPushBackEnabled(),
					    direction,
					    penetration
					);
				}
			}
		}
	}

	std::vector<std::pair<Player*, float>> playerDamageTotals;
	for (const EnemyPlayerContact& contact : enemyPlayerContacts) {
		auto total = std::find_if(playerDamageTotals.begin(), playerDamageTotals.end(), [&contact](const std::pair<Player*, float>& entry) {
			return entry.first == contact.player;
		});
		if (total == playerDamageTotals.end()) {
			playerDamageTotals.push_back({contact.player, contact.enemy->GetContactAttackDamage()});
		} else {
			total->second += contact.enemy->GetContactAttackDamage();
		}
	}
	for (const auto& [player, totalDamage] : playerDamageTotals) {
		player->TakeDamage(totalDamage);
	}
}

/// <summary>
/// マウスクリックによるエディタオブジェクト選択を更新します。
/// </summary>
void BaseScene::UpdateEditorObjectPicking() {
#ifdef USE_IMGUI
	constexpr int kLeftMouseButton = 0;
	Input* input = Input::GetInstance();
	if (!input->TriggerMouseButton(kLeftMouseButton)) {
		return;
	}
	if (ImGuizmo::IsOver() || ImGuizmo::IsUsing()) {
		return;
	}

	Camera* camera = Object3dCommon::GetInstance()->GetDefaultCamera();
	if (!camera) {
		return;
	}

	ImGuiViewport* mainViewport = ImGui::GetMainViewport();
	const ImVec2 gameViewPosition = ImGuiManager::GetInstance()->GetGameViewContentPosition();
	const ImVec2 gameViewSize = ImGuiManager::GetInstance()->GetGameViewContentSize();
	const float viewportOriginX = mainViewport ? mainViewport->Pos.x : 0.0f;
	const float viewportOriginY = mainViewport ? mainViewport->Pos.y : 0.0f;
	const float viewLeft = gameViewSize.x > 1.0f ? gameViewPosition.x - viewportOriginX : 0.0f;
	const float viewTop = gameViewSize.y > 1.0f ? gameViewPosition.y - viewportOriginY : 0.0f;
	const float width = gameViewSize.x > 1.0f ? gameViewSize.x : static_cast<float>(input->GetClientWidth());
	const float height = gameViewSize.y > 1.0f ? gameViewSize.y : static_cast<float>(input->GetClientHeight());
	const float mouseX = static_cast<float>(input->GetMouseClientX());
	const float mouseY = static_cast<float>(input->GetMouseClientY());
	if (mouseX < viewLeft || mouseX > viewLeft + width || mouseY < viewTop || mouseY > viewTop + height) {
		return;
	}

	const float ndcX = ((mouseX - viewLeft) / width) * 2.0f - 1.0f;
	const float ndcY = 1.0f - ((mouseY - viewTop) / height) * 2.0f;
	const Matrix4x4 inverseViewProjection = Inverse(camera->GetViewProjectionMatrix());
	const Vector3 nearPoint = BaseSceneEditorGeometry::TransformCoord({ndcX, ndcY, 0.0f}, inverseViewProjection);
	const Vector3 farPoint = BaseSceneEditorGeometry::TransformCoord({ndcX, ndcY, 1.0f}, inverseViewProjection);
	const Vector3 rayDirection = Normalize(farPoint - nearPoint);

	int hitIndex = -1;
	float nearestDistance = 100000.0f;
	for (int index = 0; index < static_cast<int>(sceneObjects_.size()); ++index) {
		float distance = 0.0f;
		if (BaseSceneEditorGeometry::IntersectRayToOBB(nearPoint, rayDirection, BaseSceneEditorGeometry::MakePickOBB(sceneObjects_[index].get()), distance) && distance < nearestDistance) {
			nearestDistance = distance;
			hitIndex = index;
		}
	}

	if (hitIndex >= 0) {
		selectedObjectIndex_ = hitIndex;
	}
#endif
}

/// <summary>
/// エディタ用カメラのマウス操作を更新します。
/// </summary>
void BaseScene::UpdateEditorCameraControl() {
#ifdef USE_IMGUI
	constexpr int kLeftMouseButton = 0;
	constexpr int kMiddleMouseButton = 2;
	constexpr float kRotateSpeed = 0.004f;
	constexpr float kPanSpeed = 0.0015f;

	Input* input = Input::GetInstance();
	const bool isLeftDragging = input->PushMouseButton(kLeftMouseButton);
	const bool isMiddleDragging = input->PushMouseButton(kMiddleMouseButton);
	if (ImGui::GetIO().WantCaptureMouse || (!isLeftDragging && !isMiddleDragging)) {
		return;
	}
	if (isLeftDragging && (ImGuizmo::IsOver() || ImGuizmo::IsUsing())) {
		return;
	}

	const float moveX = static_cast<float>(input->GetMouseMoveX());
	const float moveY = static_cast<float>(input->GetMouseMoveY());
	if (moveX == 0.0f && moveY == 0.0f) {
		return;
	}

	GameObject* selectedObject =
	    selectedObjectIndex_ >= 0 && selectedObjectIndex_ < static_cast<int>(sceneObjects_.size())
	        ? sceneObjects_[selectedObjectIndex_].get()
	        : nullptr;

	GameObject* activeCameraObject = FindObjectByName(activeCameraObjectName_);
	EulerTransform* cameraTransform = nullptr;
	Camera* fallbackCamera = nullptr;
	Vector3 cameraPosition{};
	Vector3 cameraRotate{};

	CameraComponent* activeCameraComponent =
	    activeCameraObject ? activeCameraObject->GetComponent<CameraComponent>() : nullptr;
	if (activeCameraComponent && activeCameraComponent->IsEnabled()) {
		cameraTransform = &activeCameraObject->GetTransform();
		cameraPosition = cameraTransform->translate;
		cameraRotate = cameraTransform->rotate;
	} else if (BaseSceneEditorGeometry::TryGetCameraTransform(fallbackCamera_, cameraPosition, cameraRotate)) {
		fallbackCamera = fallbackCamera_;
	} else {
		return;
	}

	if (isMiddleDragging) {
		const float pitchDelta = moveY * kRotateSpeed;
		const float yawDelta = moveX * kRotateSpeed;
		if (selectedObject) {
			const Vector3 target = selectedObject->GetTransform().translate;
			Vector3 offset = cameraPosition - target;
			if (Length(offset) > MathConstants::kDirectionEpsilon) {
				const Vector3 worldUp{0.0f, 1.0f, 0.0f};
				offset = BaseSceneEditorGeometry::RotateAroundAxis(offset, worldUp, yawDelta);

				Vector3 right = Cross(worldUp, Normalize(offset));
				if (Length(right) > MathConstants::kDirectionEpsilon) {
					offset = BaseSceneEditorGeometry::RotateAroundAxis(offset, right, pitchDelta);
				}
				cameraPosition = target + offset;
			}
		}

		cameraRotate.x += pitchDelta;
		cameraRotate.y += yawDelta;
	} else if (isLeftDragging) {
		const Matrix4x4 rotateMatrix = MakeRotateXYZMatrix(cameraRotate);
		const Vector3 rightAxis{rotateMatrix.m[0][0], rotateMatrix.m[0][1], rotateMatrix.m[0][2]};
		const Vector3 upAxis{rotateMatrix.m[1][0], rotateMatrix.m[1][1], rotateMatrix.m[1][2]};
		const Vector3 right = Normalize(rightAxis);
		const Vector3 up = Normalize(upAxis);
		float distanceScale = 10.0f;
		if (selectedObject) {
			distanceScale = Length(cameraPosition - selectedObject->GetTransform().translate);
			if (distanceScale < 1.0f) {
				distanceScale = 1.0f;
			}
		}
		cameraPosition = cameraPosition + (moveX * distanceScale * kPanSpeed) * right - (moveY * distanceScale * kPanSpeed) * up;
	}

	if (cameraTransform) {
		cameraTransform->translate = cameraPosition;
		cameraTransform->rotate = cameraRotate;
	} else if (fallbackCamera) {
		fallbackCamera->SetTranslate(cameraPosition);
		fallbackCamera->SetRotate(cameraRotate);
	}
#endif
}

/// <summary>
/// 指定したオブジェクトのカメラをアクティブカメラに設定します。
/// </summary>
void BaseScene::UpdateLevelUpSelection() {
	Input* input = Input::GetInstance();
	if (isLevelUpSelectionActive_) {
		if (levelUpChoices_.empty()) {
			isLevelUpSelectionActive_ = false;
			levelUpPlayer_ = nullptr;
			GameTime::SetPaused(false);
			return;
		}
		if (input->TriggerKey(DIK_A) || input->TriggerGamepadLeft()) {
			selectedLevelUpChoiceIndex_ = (selectedLevelUpChoiceIndex_ + static_cast<int>(levelUpChoices_.size()) - 1) % static_cast<int>(levelUpChoices_.size());
		}
		if (input->TriggerKey(DIK_D) || input->TriggerGamepadRight()) {
			selectedLevelUpChoiceIndex_ = (selectedLevelUpChoiceIndex_ + 1) % static_cast<int>(levelUpChoices_.size());
		}
		if (input->TriggerKey(DIK_SPACE) || input->TriggerGamepadButton(XINPUT_GAMEPAD_A)) {
			ApplyLevelUpChoice(selectedLevelUpChoiceIndex_);
		}
		return;
	}

	if (ShowNextBossAcquisitionOffer()) {
		return;
	}

	for (const auto& object : sceneObjects_) {
		Player* player = object->GetComponent<Player>();
		if (!player || !player->ConsumePendingLevelUp()) {
			continue;
		}
		levelUpPlayer_ = player;
		if (BuildLevelUpChoices(player)) {
			isLevelUpSelectionActive_ = true;
			GameTime::SetPaused(true);
		} else {
			levelUpPlayer_ = nullptr;
		}
		break;
	}
}

bool BaseScene::BuildLevelUpChoices(Player* player) {
	levelUpChoices_.clear();
	selectedLevelUpChoiceIndex_ = 0;
	if (!player) {
		return false;
	}
	const PlayerStats& stats = player->GetBaseStats();
	std::vector<LevelUpChoice> candidates;
	std::vector<std::string> equippedAttacks;
	std::vector<std::string> equippedStatuses;
	std::unordered_set<std::string> otherPlayerInitialAttacks;
	int emptyAttackSlot = -1;
	int emptyStatusSlot = -1;
	for (const std::string& playerTypeName : LoadPlayerTypeNames()) {
		if (playerTypeName == player->GetPlayerTypeName()) {
			continue;
		}
		const PlayerStats otherPlayerStats = LoadPlayerStats(playerTypeName);
		for (const PlayerAttackSlot& slot : otherPlayerStats.attackSlots) {
			if (slot.enabled && !slot.attackName.empty()) {
				otherPlayerInitialAttacks.insert(slot.attackName);
			}
		}
	}
	auto getAttackDescription = [](const std::string& attackName, const std::string& targetLevel, const std::string& fallback) {
		const PlayerAttackStats attackStats = LoadPlayerAttackStats(attackName);
		for (const PlayerAttackLevelStats& levelStats : attackStats.levels) {
			if (levelStats.level == targetLevel && !levelStats.choiceDescription.empty()) {
				return levelStats.choiceDescription;
			}
		}
		return fallback;
	};
	auto getStatusDescription = [](const std::string& statusName, int targetLevelIndex, const std::string& fallback) {
		const PlayerStatusItemStats statusStats = LoadPlayerStatusItemStats(statusName);
		if (targetLevelIndex >= 0 && targetLevelIndex < static_cast<int>(statusStats.levelDescriptions.size()) &&
		    !statusStats.levelDescriptions[targetLevelIndex].empty()) {
			return statusStats.levelDescriptions[targetLevelIndex];
		}
		return fallback;
	};
	auto getAttackTexture = [](const std::string& attackName, const std::string& targetLevel) {
		const PlayerAttackStats attackStats = LoadPlayerAttackStats(attackName);
		if (targetLevel != "super") {
			return attackStats.choiceTextureFilePath;
		}
		for (const PlayerAttackLevelStats& levelStats : attackStats.levels) {
			if (levelStats.level == targetLevel) {
				return levelStats.choiceTextureFilePath;
			}
		}
		return std::string{};
	};
	auto getStatusTexture = [](const std::string& statusName, int targetLevelIndex) {
		const PlayerStatusItemStats statusStats = LoadPlayerStatusItemStats(statusName);
		if (targetLevelIndex >= 0 && targetLevelIndex < static_cast<int>(statusStats.levelTextureFilePaths.size())) {
			return statusStats.levelTextureFilePaths[targetLevelIndex];
		}
		return std::string{};
	};

	for (int index = 0; index < static_cast<int>(stats.attackSlots.size()); ++index) {
		const PlayerAttackSlot& slot = stats.attackSlots[index];
		if (slot.attackName.empty()) {
			if (emptyAttackSlot < 0) emptyAttackSlot = index;
			continue;
		}
		equippedAttacks.push_back(slot.attackName);
		const int level = std::atoi(slot.attackLevel.c_str());
		if (slot.attackLevel != "super" && level >= 1 && level < 5) {
			const std::string targetLevel = std::to_string(level + 1);
			const std::string fallback = "Attack level " + slot.attackLevel + " -> " + targetLevel;
			candidates.push_back({LevelUpChoiceType::AttackLevelUp, slot.attackName, slot.attackName, getAttackDescription(slot.attackName, targetLevel, fallback), index, getAttackTexture(slot.attackName, targetLevel)});
		}
		if (slot.attackLevel == "5") {
			const PlayerAttackStats attackStats = LoadPlayerAttackStats(slot.attackName);
			bool conditionMet = !attackStats.superConditionStatusName.empty();
			const int requiredLevel = (std::max)(1, std::atoi(attackStats.superConditionStatusLevel.c_str()));
			if (conditionMet) {
				conditionMet = false;
				for (const PlayerStatusSlot& statusSlot : stats.statusSlots) {
					if (statusSlot.enabled && statusSlot.statusName == attackStats.superConditionStatusName && std::atoi(statusSlot.level.c_str()) >= requiredLevel) {
						conditionMet = true;
						break;
					}
				}
			}
			if (conditionMet) {
				candidates.push_back({LevelUpChoiceType::AttackSuper, slot.attackName, slot.attackName + " SUPER", getAttackDescription(slot.attackName, "super", "Promote attack level 5 -> super"), index, getAttackTexture(slot.attackName, "super")});
			}
		}
	}

	for (int index = 0; index < static_cast<int>(stats.statusSlots.size()); ++index) {
		const PlayerStatusSlot& slot = stats.statusSlots[index];
		if (slot.statusName.empty()) {
			if (emptyStatusSlot < 0) emptyStatusSlot = index;
			continue;
		}
		equippedStatuses.push_back(slot.statusName);
		const int level = std::atoi(slot.level.c_str());
		if (level >= 1 && level < 5) {
			const std::string targetLevel = std::to_string(level + 1);
			const std::string fallback = "Status level " + slot.level + " -> " + targetLevel;
			candidates.push_back({LevelUpChoiceType::StatusLevelUp, slot.statusName, slot.statusName, getStatusDescription(slot.statusName, level, fallback), index, getStatusTexture(slot.statusName, level)});
		}
	}

	if (emptyAttackSlot >= 0) {
		for (const std::string& attackName : LoadPlayerAttackNames()) {
			if (std::find(equippedAttacks.begin(), equippedAttacks.end(), attackName) == equippedAttacks.end() &&
			    !otherPlayerInitialAttacks.contains(attackName)) {
				candidates.push_back({LevelUpChoiceType::NewAttack, attackName, attackName, getAttackDescription(attackName, "1", "Add a new attack at level 1"), emptyAttackSlot, getAttackTexture(attackName, "1")});
			}
		}
	}
	if (emptyStatusSlot >= 0) {
		for (const std::string& statusName : LoadPlayerStatusItemNames()) {
			if (std::find(equippedStatuses.begin(), equippedStatuses.end(), statusName) == equippedStatuses.end()) {
				candidates.push_back({LevelUpChoiceType::NewStatus, statusName, statusName, getStatusDescription(statusName, 0, "Add a new status at level 1"), emptyStatusSlot, getStatusTexture(statusName, 0)});
			}
		}
	}

	if (candidates.empty()) {
		return false;
	}
	static std::mt19937 randomEngine(std::random_device{}());
	std::shuffle(candidates.begin(), candidates.end(), randomEngine);
	auto isEquippedUpgrade = [](const LevelUpChoice& choice) {
		return choice.type == LevelUpChoiceType::AttackLevelUp ||
		       choice.type == LevelUpChoiceType::AttackSuper ||
		       choice.type == LevelUpChoiceType::StatusLevelUp;
	};
	const auto equippedUpgrade = std::find_if(candidates.begin(), candidates.end(), isEquippedUpgrade);
	if (candidates.size() > 3 && equippedUpgrade >= candidates.begin() + 3 && equippedUpgrade != candidates.end()) {
		std::uniform_int_distribution<int> selectionSlotDistribution(0, 2);
		std::iter_swap(candidates.begin() + selectionSlotDistribution(randomEngine), equippedUpgrade);
	}
	for (int index = 0; index < 3; ++index) {
		levelUpChoices_.push_back(candidates[index % candidates.size()]);
	}
	return true;
}

void BaseScene::ApplyLevelUpChoice(int choiceIndex) {
	if (!levelUpPlayer_ || choiceIndex < 0 || choiceIndex >= static_cast<int>(levelUpChoices_.size())) return;
	const bool wasBossAcquisitionOffer = isBossAcquisitionOfferActive_;
	const LevelUpChoice choice = levelUpChoices_[choiceIndex];
	PlayerStats stats = levelUpPlayer_->GetBaseStats();
	switch (choice.type) {
	case LevelUpChoiceType::AttackLevelUp:
		stats.attackSlots[choice.slotIndex].enabled = true;
		stats.attackSlots[choice.slotIndex].attackLevel = std::to_string((std::min)(5, std::atoi(stats.attackSlots[choice.slotIndex].attackLevel.c_str()) + 1));
		break;
	case LevelUpChoiceType::AttackSuper:
		stats.attackSlots[choice.slotIndex].enabled = true;
		stats.attackSlots[choice.slotIndex].attackLevel = "super";
		break;
	case LevelUpChoiceType::NewAttack:
		stats.attackSlots[choice.slotIndex] = {true, choice.name, "1"};
		break;
	case LevelUpChoiceType::StatusLevelUp:
		stats.statusSlots[choice.slotIndex].enabled = true;
		stats.statusSlots[choice.slotIndex].level = std::to_string((std::min)(5, std::atoi(stats.statusSlots[choice.slotIndex].level.c_str()) + 1));
		break;
	case LevelUpChoiceType::NewStatus:
		stats.statusSlots[choice.slotIndex] = {true, choice.name, "1"};
		break;
	case LevelUpChoiceType::Decline:
		break;
	}
	if (choice.type != LevelUpChoiceType::Decline) {
		levelUpPlayer_->ApplyStats(stats, ApplyPlayerStatusItems(stats));
		if (GameObject* owner = levelUpPlayer_->GetOwner()) {
			ApplyPlayerAttackSlots(owner->GetComponent<PlayerAttackComponent>(), stats);
		}
	}
	levelUpChoices_.clear();
	if (wasBossAcquisitionOffer) {
		isBossAcquisitionOfferActive_ = false;
		if (ShowNextBossAcquisitionOffer()) {
			return;
		}
	}
	if (levelUpPlayer_->ConsumePendingLevelUp() && BuildLevelUpChoices(levelUpPlayer_)) return;
	isLevelUpSelectionActive_ = false;
	levelUpPlayer_ = nullptr;
	GameTime::SetPaused(false);
}

void BaseScene::EnsureLevelUpSelectionSprites() {
	if (levelUpOverlaySprite_) return;
	const std::string whiteTexture = "Resources/human/white.png";
	auto createSprite = [&whiteTexture]() {
		auto sprite = std::make_unique<Sprite>();
		sprite->Initialize(whiteTexture);
		return sprite;
	};
	levelUpOverlaySprite_ = createSprite();
	levelUpPanelSprite_ = createSprite();
	for (int index = 0; index < 3; ++index) {
		levelUpChoiceBorderSprites_[index] = createSprite();
		levelUpChoiceSprites_[index] = createSprite();
		levelUpChoiceIconSprites_[index] = createSprite();
	}

	auto createTextObject = [](const std::string& text, float fontSize) {
		auto object = std::make_unique<GameObject>();
		TextComponent* textComponent = object->AddComponent<TextComponent>();
		textComponent->SetText(text);
		textComponent->SetFontSize(fontSize);
		textComponent->SetAnchor(TextComponent::Anchor::Center);
		return object;
	};
	levelUpTitleTextObject_ = createTextObject("LEVEL UP!", 46.0f);
	levelUpInstructionTextObject_ = createTextObject("A / D or Pad: Select    Space or Pad A: Confirm", 21.0f);
	for (auto& object : levelUpChoiceTextObjects_) {
		object = createTextObject("", 23.0f);
	}
}

void BaseScene::DrawLevelUpSelection2D() {
	if (!isLevelUpSelectionActive_ || levelUpChoices_.empty()) return;
	EnsureLevelUpSelectionSprites();
	DirectXCommon* dxCommon = SpriteCommon::GetInstance()->GetDxCommon();
	if (!dxCommon) return;
	const float screenWidth = static_cast<float>(dxCommon->GetRenderWidth());
	const float screenHeight = static_cast<float>(dxCommon->GetRenderHeight());
	const float panelWidth = (std::min)(screenWidth - 48.0f, 1120.0f);
	const float panelHeight = (std::min)(screenHeight - 48.0f, 440.0f);
	const float panelX = (screenWidth - panelWidth) * 0.5f;
	const float panelY = (screenHeight - panelHeight) * 0.5f;
	const float gap = 24.0f;
	const float cardWidth = (panelWidth - 64.0f - gap * 2.0f) / 3.0f;
	const float cardHeight = panelHeight - 145.0f;
	const float cardY = panelY + 92.0f;

	auto drawSprite = [](Sprite* sprite, float x, float y, float width, float height, const Vector4& color) {
		EulerTransform transform = sprite->GetTransform();
		transform.translate = {x, y, 0.0f};
		sprite->SetTransform(transform);
		sprite->SetSize({width, height});
		sprite->SetColor(color);
		sprite->Update();
		sprite->Draw();
	};
	auto choiceColor = [](LevelUpChoiceType type) {
		switch (type) {
		case LevelUpChoiceType::AttackSuper: return Vector4{0.72f, 0.42f, 0.05f, 0.98f};
		case LevelUpChoiceType::NewAttack: return Vector4{0.55f, 0.18f, 0.08f, 0.98f};
		case LevelUpChoiceType::StatusLevelUp: return Vector4{0.08f, 0.35f, 0.20f, 0.98f};
		case LevelUpChoiceType::NewStatus: return Vector4{0.06f, 0.32f, 0.38f, 0.98f};
		case LevelUpChoiceType::Decline: return Vector4{0.22f, 0.22f, 0.25f, 0.98f};
		case LevelUpChoiceType::AttackLevelUp:
		default: return Vector4{0.08f, 0.22f, 0.48f, 0.98f};
		}
	};

	SpriteCommon::GetInstance()->SetDraw(kBlendModeNormal);
	drawSprite(levelUpOverlaySprite_.get(), 0.0f, 0.0f, screenWidth, screenHeight, {0.0f, 0.0f, 0.0f, 0.68f});
	drawSprite(levelUpPanelSprite_.get(), panelX, panelY, panelWidth, panelHeight, {0.035f, 0.045f, 0.075f, 0.98f});
	for (int index = 0; index < static_cast<int>(levelUpChoices_.size()) && index < 3; ++index) {
		const float cardX = panelX + 32.0f + static_cast<float>(index) * (cardWidth + gap);
		const bool selected = index == selectedLevelUpChoiceIndex_;
		drawSprite(levelUpChoiceBorderSprites_[index].get(), cardX - 5.0f, cardY - 5.0f, cardWidth + 10.0f, cardHeight + 10.0f,
			selected ? Vector4{1.0f, 0.78f, 0.12f, 1.0f} : Vector4{0.20f, 0.23f, 0.30f, 1.0f});
		Vector4 color = choiceColor(levelUpChoices_[index].type);
		if (selected) {
			color.x = (std::min)(1.0f, color.x + 0.14f);
			color.y = (std::min)(1.0f, color.y + 0.14f);
			color.z = (std::min)(1.0f, color.z + 0.14f);
		}
		drawSprite(levelUpChoiceSprites_[index].get(), cardX, cardY, cardWidth, cardHeight, color);
		const std::string& textureFilePath = levelUpChoices_[index].textureFilePath;
		const bool hasTexture = !textureFilePath.empty() && std::filesystem::exists(textureFilePath);
		if (hasTexture) {
			Sprite* iconSprite = levelUpChoiceIconSprites_[index].get();
			if (iconSprite->GetTextureFilePath() != textureFilePath) {
				iconSprite->SetTexture(textureFilePath);
			}
			const float iconMargin = 18.0f;
			const float iconHeight = (std::min)(cardHeight * 0.43f, 125.0f);
			drawSprite(iconSprite, cardX + iconMargin, cardY + iconMargin, cardWidth - iconMargin * 2.0f, iconHeight,
			    selected ? Vector4{1.0f, 1.0f, 1.0f, 1.0f} : Vector4{0.88f, 0.88f, 0.88f, 1.0f});
		}

		GameObject* textObject = levelUpChoiceTextObjects_[index].get();
		const float textCenterY = hasTexture ? cardY + cardHeight * 0.72f : cardY + cardHeight * 0.5f;
		textObject->GetTransform().translate = {cardX + cardWidth * 0.5f, textCenterY, 0.0f};
		TextComponent* text = textObject->GetComponent<TextComponent>();
		text->SetText(levelUpChoices_[index].title + "\n\n" + levelUpChoices_[index].description);
		text->SetColor(selected ? Vector4{1.0f, 0.92f, 0.55f, 1.0f} : Vector4{1.0f, 1.0f, 1.0f, 1.0f});
	}

	if (TextComponent* titleText = levelUpTitleTextObject_->GetComponent<TextComponent>()) {
		titleText->SetText(isBossAcquisitionOfferActive_ ? "BOSS REWARD!" : "LEVEL UP!");
	}
	levelUpTitleTextObject_->GetTransform().translate = {screenWidth * 0.5f, panelY + 35.0f, 0.0f};
	levelUpInstructionTextObject_->GetTransform().translate = {screenWidth * 0.5f, panelY + panelHeight - 24.0f, 0.0f};
	levelUpTitleTextObject_->Draw2D();
	levelUpInstructionTextObject_->Draw2D();
	for (int index = 0; index < static_cast<int>(levelUpChoices_.size()) && index < 3; ++index) {
		levelUpChoiceTextObjects_[index]->Draw2D();
	}
}

void BaseScene::SetActiveCameraObject(GameObject* object) {
	CameraComponent* cameraComponent = object ? object->GetComponent<CameraComponent>() : nullptr;
	if (!cameraComponent || !cameraComponent->IsEnabled()) {
		return;
	}

	activeCameraObjectName_ = object->GetName();
	ApplyActiveCamera();
}

/// <summary>
/// 最初に見つかった有効なカメラオブジェクトを返します。
/// </summary>
GameObject* BaseScene::FindFirstCameraObject() {
	for (const auto& object : sceneObjects_) {
		CameraComponent* cameraComponent = object->GetComponent<CameraComponent>();
		if (cameraComponent && cameraComponent->IsEnabled()) {
			return object.get();
		}
	}
	return nullptr;
}

/// <summary>
/// 名前に一致するシーンオブジェクトを検索します。
/// </summary>
GameObject* BaseScene::FindObjectByName(const std::string& name) const {
	if (name.empty()) {
		return nullptr;
	}

	for (const auto& object : sceneObjects_) {
		if (object->GetName() == name) {
			return object.get();
		}
	}
	return nullptr;
}

