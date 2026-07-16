#include "BaseScene.h"
#include "BaseSceneHelpers.h"
#include <random>
#include <Xinput.h>

namespace {
bool ShouldSkipColliderPair(GameObject* objectA, GameObject* objectB) {
	if (!objectA || !objectB || objectA == objectB) {
		return true;
	}

	const bool isEnemyA = objectA->GetComponent<EnemyComponent>() != nullptr;
	const bool isEnemyB = objectB->GetComponent<EnemyComponent>() != nullptr;
	return isEnemyA && isEnemyB;
}

struct EnemyPlayerContact {
	EnemyComponent* enemy = nullptr;
	Player* player = nullptr;
};

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

	if (enemy->IsEnabled() && player->IsEnabled() && enemy->GetCurrentHealth() > 0.0f) {
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
	if (clipW <= 0.0001f) {
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
}

void BaseScene::ApplyCamera(Camera* camera) {
	if (!camera) {
		return;
	}

	const float clientWidth = static_cast<float>(Input::GetInstance()->GetClientWidth());
	const float clientHeight = static_cast<float>(Input::GetInstance()->GetClientHeight());
	if (clientWidth > 0.0f && clientHeight > 0.0f) {
		camera->SetAspectRatio(clientWidth / clientHeight);
	}
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
	struct SpawnRequest {
		std::string enemyTypeName;
		Vector3 position;
		GameObject* target = nullptr;
	};
	std::vector<SpawnRequest> spawnRequests;
	for (const auto& object : sceneObjects_) {
		EnemySpawnPointComponent* spawnPoint = object->GetComponent<EnemySpawnPointComponent>();
		if (!spawnPoint || !spawnPoint->IsEnabled()) {
			continue;
		}

		const std::string enemyTypeName = spawnPoint->GetEnemyTypeName();
		const EnemyStats stats = LoadEnemyStats(enemyTypeName);
		Vector3 spawnPosition{};
		if (spawnPoint->ConsumeSpawnRequest(stats.spawnsPerMinute, spawnPosition)) {
			spawnRequests.push_back({enemyTypeName, spawnPosition, spawnPoint->GetTarget()});
		}
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

	for (PlayerAttackShotRequest& request : shotRequests) {
		CreateRuntimePlayerProjectile(request);
	}
}

GameObject* BaseScene::CreateRuntimeEnemy(const std::string& enemyTypeName, const Vector3& position, GameObject* target) {
	auto object = std::make_unique<GameObject>();
	object->SetName(MakeUniqueObjectName(enemyTypeName.empty() ? "Enemy" : enemyTypeName));
	object->SetEditorType("Enemy");
	object->GetTransform().translate = position;
	object->GetTransform().scale = {0.75f, 0.75f, 0.75f};

	EnemyComponent* enemy = object->AddComponent<EnemyComponent>();
	enemy->SetEnemyTypeName(enemyTypeName.empty() ? "Default" : enemyTypeName);
	enemy->ApplyStats(LoadEnemyStats(enemy->GetEnemyTypeName()));
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

	auto object = std::make_unique<GameObject>();
	object->SetName(MakeUniqueObjectName("Experience"));
	object->SetEditorType("Experience");
	object->GetTransform().translate = position;
	object->GetTransform().scale = {0.35f, 0.35f, 0.35f};

	ExperienceComponent* experience = object->AddComponent<ExperienceComponent>();
	experience->SetExperience(enemyStats.experience);
	experience->SetModelFilePath(enemyStats.experienceModelFilePath);
	experience->SetTarget(target);

	const std::string modelFilePath = enemyStats.experienceModelFilePath.empty() ? "sphere.obj" : enemyStats.experienceModelFilePath;
	if (!ModelManager::GetInstance()->FindModel(modelFilePath)) {
		ModelManager::GetInstance()->LoadModel(modelFilePath);
	}
	Object3dComponent* object3d = object->AddComponent<Object3dComponent>();
	if (ModelManager::GetInstance()->FindModel(modelFilePath)) {
		object3d->SetModel(modelFilePath);
	} else {
		ModelManager::GetInstance()->LoadModel("sphere.obj");
		object3d->SetModel("sphere.obj");
	}

	object->Update();
	sceneObjects_.push_back(std::move(object));
	++nextObjectId_;
	return sceneObjects_.back().get();
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
	if (request.homing) {
		projectile->SetHomingTarget(FindNearestEnemy(request.position));
	}

	const std::string modelFilePath = request.modelFilePath.empty() ? "sphere.obj" : request.modelFilePath;
	if (!ModelManager::GetInstance()->FindModel(modelFilePath)) {
		ModelManager::GetInstance()->LoadModel(modelFilePath);
	}
	Object3dComponent* object3d = object->AddComponent<Object3dComponent>();
	if (ModelManager::GetInstance()->FindModel(modelFilePath)) {
		object3d->SetModel(modelFilePath);
	} else {
		ModelManager::GetInstance()->LoadModel("sphere.obj");
		object3d->SetModel("sphere.obj");
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
	std::vector<ExperienceDropRequest> experienceDropRequests;
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

			const float distance = Length(enemyObject->GetTransform().translate - projectileObject->GetTransform().translate);
			if (distance <= projectile->GetSize() + 0.5f) {
				enemy->SetCurrentHealth(enemy->GetCurrentHealth() - projectile->GetAttack());
				if (enemy->GetCurrentHealth() <= 0.0f) {
					experienceDropRequests.push_back({enemy->GetStats(), enemyObject->GetTransform().translate, enemy->GetTarget()});
				}
				projectile->RegisterHitObject(enemyObject.get());
				break;
			}
		}
	}

	for (const ExperienceDropRequest& request : experienceDropRequests) {
		CreateRuntimeExperience(request.stats, request.position, request.target);
	}
}

void BaseScene::CleanupExpiredPlayerProjectiles() {
	sceneObjects_.erase(
	    std::remove_if(sceneObjects_.begin(), sceneObjects_.end(), [](const std::unique_ptr<GameObject>& object) {
		    PlayerProjectileComponent* projectile = object->GetComponent<PlayerProjectileComponent>();
		    if (projectile) {
			    const float viewMargin = 0.05f + projectile->GetSize() * 0.02f;
			    if (projectile->IsExpired() || IsPointOutsideView(object->GetTransform().translate, viewMargin)) {
				    return true;
			    }
		    }
		    EnemyComponent* enemy = object->GetComponent<EnemyComponent>();
		    if (enemy && enemy->GetCurrentHealth() <= 0.0f) {
			    return true;
		    }
		    ExperienceComponent* experience = object->GetComponent<ExperienceComponent>();
		    return experience && experience->IsCollected();
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
				if (CalculateOBBOBBPushBack(colliderA, colliderB, direction, penetration)) {
					ApplyColliderPushBack(
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
				if (CalculateSphereSpherePushBack(sphereA, sphereB, direction, penetration)) {
					ApplyColliderPushBack(
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
				if (CalculateOBBSpherePushBack(obb, sphere, direction, penetration)) {
					ApplyColliderPushBack(
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
			playerDamageTotals.push_back({contact.player, contact.enemy->GetStats().attack});
		} else {
			total->second += contact.enemy->GetStats().attack;
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
	if (ImGui::GetIO().WantCaptureMouse || ImGuizmo::IsOver() || ImGuizmo::IsUsing()) {
		return;
	}

	Camera* camera = Object3dCommon::GetInstance()->GetDefaultCamera();
	if (!camera) {
		return;
	}

	const float width = static_cast<float>(input->GetClientWidth());
	const float height = static_cast<float>(input->GetClientHeight());
	const float mouseX = static_cast<float>(input->GetMouseClientX());
	const float mouseY = static_cast<float>(input->GetMouseClientY());
	if (mouseX < 0.0f || mouseX > width || mouseY < 0.0f || mouseY > height) {
		return;
	}

	const float ndcX = (mouseX / width) * 2.0f - 1.0f;
	const float ndcY = 1.0f - (mouseY / height) * 2.0f;
	const Matrix4x4 inverseViewProjection = Inverse(camera->GetViewProjectionMatrix());
	const Vector3 nearPoint = TransformCoord({ndcX, ndcY, 0.0f}, inverseViewProjection);
	const Vector3 farPoint = TransformCoord({ndcX, ndcY, 1.0f}, inverseViewProjection);
	const Vector3 rayDirection = Normalize(farPoint - nearPoint);

	int hitIndex = -1;
	float nearestDistance = 100000.0f;
	for (int index = 0; index < static_cast<int>(sceneObjects_.size()); ++index) {
		float distance = 0.0f;
		if (IntersectRayToOBB(nearPoint, rayDirection, MakePickOBB(sceneObjects_[index].get()), distance) && distance < nearestDistance) {
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
	} else if (TryGetCameraTransform(fallbackCamera_, cameraPosition, cameraRotate)) {
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
			if (Length(offset) > 0.0001f) {
				const Vector3 worldUp{0.0f, 1.0f, 0.0f};
				offset = RotateAroundAxis(offset, worldUp, yawDelta);

				Vector3 right = Cross(worldUp, Normalize(offset));
				if (Length(right) > 0.0001f) {
					offset = RotateAroundAxis(offset, right, pitchDelta);
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
#ifdef USE_IMGUI
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
#endif
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
	int emptyAttackSlot = -1;
	int emptyStatusSlot = -1;

	for (int index = 0; index < static_cast<int>(stats.attackSlots.size()); ++index) {
		const PlayerAttackSlot& slot = stats.attackSlots[index];
		if (slot.attackName.empty()) {
			if (emptyAttackSlot < 0) emptyAttackSlot = index;
			continue;
		}
		equippedAttacks.push_back(slot.attackName);
		const int level = std::atoi(slot.attackLevel.c_str());
		if (slot.attackLevel != "super" && level >= 1 && level < 5) {
			candidates.push_back({LevelUpChoiceType::AttackLevelUp, slot.attackName, slot.attackName, "Attack level " + slot.attackLevel + " -> " + std::to_string(level + 1), index});
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
				candidates.push_back({LevelUpChoiceType::AttackSuper, slot.attackName, slot.attackName + " SUPER", "Promote attack level 5 -> super", index});
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
			candidates.push_back({LevelUpChoiceType::StatusLevelUp, slot.statusName, slot.statusName, "Status level " + slot.level + " -> " + std::to_string(level + 1), index});
		}
	}

	if (emptyAttackSlot >= 0) {
		for (const std::string& attackName : LoadPlayerAttackNames()) {
			if (std::find(equippedAttacks.begin(), equippedAttacks.end(), attackName) == equippedAttacks.end()) {
				candidates.push_back({LevelUpChoiceType::NewAttack, attackName, attackName, "Add a new attack at level 1", emptyAttackSlot});
			}
		}
	}
	if (emptyStatusSlot >= 0) {
		for (const std::string& statusName : LoadPlayerStatusItemNames()) {
			if (std::find(equippedStatuses.begin(), equippedStatuses.end(), statusName) == equippedStatuses.end()) {
				candidates.push_back({LevelUpChoiceType::NewStatus, statusName, statusName, "Add a new status at level 1", emptyStatusSlot});
			}
		}
	}

	if (candidates.empty()) {
		return false;
	}
	static std::mt19937 randomEngine(std::random_device{}());
	std::shuffle(candidates.begin(), candidates.end(), randomEngine);
	for (int index = 0; index < 3; ++index) {
		levelUpChoices_.push_back(candidates[index % candidates.size()]);
	}
	return true;
}

void BaseScene::ApplyLevelUpChoice(int choiceIndex) {
	if (!levelUpPlayer_ || choiceIndex < 0 || choiceIndex >= static_cast<int>(levelUpChoices_.size())) return;
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
	}
	levelUpPlayer_->ApplyStats(stats, ApplyPlayerStatusItems(stats));
	if (GameObject* owner = levelUpPlayer_->GetOwner()) {
		ApplyPlayerAttackSlots(owner->GetComponent<PlayerAttackComponent>(), stats);
	}
	levelUpChoices_.clear();
	if (levelUpPlayer_->ConsumePendingLevelUp() && BuildLevelUpChoices(levelUpPlayer_)) return;
	isLevelUpSelectionActive_ = false;
	levelUpPlayer_ = nullptr;
	GameTime::SetPaused(false);
}

void BaseScene::DrawLevelUpSelectionImGui() {
#ifdef USE_IMGUI
	if (!isLevelUpSelectionActive_ || levelUpChoices_.empty()) return;
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::GetBackgroundDrawList(viewport)->AddRectFilled(viewport->Pos, ImVec2(viewport->Pos.x + viewport->Size.x, viewport->Pos.y + viewport->Size.y), IM_COL32(0, 0, 0, 150));
	const ImVec2 windowSize((std::min)(900.0f, viewport->WorkSize.x - 40.0f), 300.0f);
	ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x * 0.5f, viewport->WorkPos.y + viewport->WorkSize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
	int clickedChoice = -1;
	if (ImGui::Begin("LEVEL UP", nullptr, flags)) {
		const float titleWidth = ImGui::CalcTextSize("LEVEL UP!").x;
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - titleWidth) * 0.5f);
		ImGui::Text("LEVEL UP!");
		ImGui::TextDisabled("A / D or Pad: Select    Space or Pad A: Confirm");
		if (ImGui::BeginTable("LevelUpChoices", 3, ImGuiTableFlags_SizingStretchSame)) {
			for (int index = 0; index < static_cast<int>(levelUpChoices_.size()); ++index) {
				ImGui::TableNextColumn();
				ImGui::PushID(index);
				const bool selected = index == selectedLevelUpChoiceIndex_;
				if (selected) {
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.55f, 0.08f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.7f, 0.15f, 1.0f));
				}
				const std::string label = levelUpChoices_[index].title + "\n\n" + levelUpChoices_[index].description;
				if (ImGui::Button(label.c_str(), ImVec2(-1.0f, 155.0f))) clickedChoice = index;
				if (selected) ImGui::PopStyleColor(2);
				ImGui::PopID();
			}
			ImGui::EndTable();
		}
	}
	ImGui::End();
	if (clickedChoice >= 0) ApplyLevelUpChoice(clickedChoice);
#endif
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

