#include "BaseScene.h"
#include "BaseSceneHelpers.h"

namespace {
void SaveComponentGravity(nlohmann::json& componentJson, Component* component) {
	if (!component) {
		return;
	}
	componentJson["gravity"]["enabled"] = component->IsGravityEnabled();
	componentJson["gravity"]["strength"] = component->GetGravityStrength();
}

void LoadComponentGravity(const nlohmann::json& componentJson, Component* component) {
	if (!component) {
		return;
	}
	const nlohmann::json gravityJson = componentJson.value("gravity", nlohmann::json::object());
	component->SetGravityEnabled(gravityJson.value("enabled", component->IsGravityEnabled()));
	component->SetGravityStrength(gravityJson.value("strength", component->GetGravityStrength()));
	component->ResetGravityVelocity();
}
}

void BaseScene::SaveEditorObjects() {
	nlohmann::json root;
	root["scene"] = sceneName_;
	root["nextObjectId"] = nextObjectId_;
	root["activeCamera"] = activeCameraObjectName_;
	root["skyBox"]["enabled"] = isEditorSkyBoxEnabled_;
	root["skyBox"]["textureFilePath"] = skyBoxTextureFilePath_;
	root["objects"] = nlohmann::json::array();

	std::function<nlohmann::json(GameObject*)> makeObjectJson = [&](GameObject* object) {
		const EulerTransform& transform = object->GetTransform();

		nlohmann::json objectJson;
		objectJson["name"] = object->GetName();
		objectJson["type"] = object->GetEditorType();
		if (object->GetEditorType().starts_with("LoadedModel:")) {
			objectJson["type"] = "LoadedModel";
			objectJson["model"] = object->GetEditorType().substr(std::string("LoadedModel:").size());
		}
		if (object->GetEditorType().starts_with("AnimatedModel:")) {
			objectJson["type"] = "AnimatedModel";
			objectJson["model"] = object->GetEditorType().substr(std::string("AnimatedModel:").size());
		}
		objectJson["parent"] = object->GetParentName();
		if (Player* player = object->GetComponent<Player>()) {
			objectJson["type"] = "Player";
			objectJson["player"]["enabled"] = player->IsEnabled();
			SaveComponentGravity(objectJson["player"], player);
			objectJson["player"]["typeName"] = player->GetPlayerTypeName();
			objectJson["player"]["spawnPoint"] = Vector3ToJson(player->GetSpawnPoint());
			objectJson["player"]["currentHealth"] = player->GetCurrentHealth();
			objectJson["player"]["level"] = player->GetStats().level;
			objectJson["player"]["experience"] = player->GetStats().experience;
			objectJson["player"]["moveSpeed"] = player->GetMoveSpeed();
			objectJson["player"]["model"] = player->GetModelFilePath();
			objectJson["player"]["isAnimationModel"] = player->GetIsAnimationModel();
			if (PlayerAttackComponent* attack = object->GetComponent<PlayerAttackComponent>()) {
				objectJson["playerAttack"]["enabled"] = attack->IsEnabled();
				SaveComponentGravity(objectJson["playerAttack"], attack);
			}
		}
		if (EnemyComponent* enemy = object->GetComponent<EnemyComponent>()) {
			objectJson["type"] = "Enemy";
			objectJson["enemy"]["enabled"] = enemy->IsEnabled();
			SaveComponentGravity(objectJson["enemy"], enemy);
			objectJson["enemy"]["typeName"] = enemy->GetEnemyTypeName();
			objectJson["enemy"]["currentHealth"] = enemy->GetCurrentHealth();
			objectJson["enemy"]["targetName"] = enemy->GetTargetName();
		}
		if (SpriteComponent* spriteComponent = object->GetComponent<SpriteComponent>()) {
			objectJson["sprite"]["enabled"] = spriteComponent->IsEnabled();
			SaveComponentGravity(objectJson["sprite"], spriteComponent);
			objectJson["sprite"]["textureFilePath"] = spriteComponent->GetTextureFilePath();
			objectJson["sprite"]["color"] = Vector4ToJson(spriteComponent->GetColor());
			objectJson["sprite"]["size"] = nlohmann::json::array({spriteComponent->GetSize().x, spriteComponent->GetSize().y});
		}
		if (TextComponent* textComponent = object->GetComponent<TextComponent>()) {
			objectJson["type"] = "Text";
			objectJson["text"]["enabled"] = textComponent->IsEnabled();
			SaveComponentGravity(objectJson["text"], textComponent);
			objectJson["text"]["value"] = textComponent->GetText();
			objectJson["text"]["fontName"] = textComponent->GetFontName();
			objectJson["text"]["fontSize"] = textComponent->GetFontSize();
			objectJson["text"]["anchor"] = static_cast<int>(textComponent->GetAnchor());
			objectJson["text"]["color"] = Vector4ToJson(textComponent->GetColor());
		}
		if (CameraComponent* cameraComponent = object->GetComponent<CameraComponent>()) {
			objectJson["camera"]["enabled"] = cameraComponent->IsEnabled();
			SaveComponentGravity(objectJson["camera"], cameraComponent);
			objectJson["camera"]["fovY"] = cameraComponent->GetFovY();
			objectJson["camera"]["nearClip"] = cameraComponent->GetNearClip();
			objectJson["camera"]["farClip"] = cameraComponent->GetFarClip();
			objectJson["camera"]["followTarget"] = cameraComponent->GetFollowTargetName();
			objectJson["camera"]["followOffset"] = Vector3ToJson(cameraComponent->GetFollowOffset());
			objectJson["camera"]["localOffset"] = Vector3ToJson(cameraComponent->GetLocalOffset());
			objectJson["camera"]["overrideRotationEnabled"] = cameraComponent->GetOverrideRotationEnabled();
			objectJson["camera"]["overrideRotation"] = Vector3ToJson(cameraComponent->GetOverrideRotation());
		}
		if (OBBColliderComponent* collider = object->GetComponent<OBBColliderComponent>()) {
			objectJson["obbCollider"]["enabled"] = collider->IsEnabled();
			SaveComponentGravity(objectJson["obbCollider"], collider);
			objectJson["obbCollider"]["centerOffset"] = Vector3ToJson(collider->GetCenterOffset());
			objectJson["obbCollider"]["halfSize"] = Vector3ToJson(collider->GetHalfSize());
			objectJson["obbCollider"]["drawDebug"] = collider->GetDrawDebug();
			objectJson["obbCollider"]["pushBack"] = collider->GetPushBackEnabled();
		}
		if (SphereColliderComponent* collider = object->GetComponent<SphereColliderComponent>()) {
			objectJson["sphereCollider"]["enabled"] = collider->IsEnabled();
			SaveComponentGravity(objectJson["sphereCollider"], collider);
			objectJson["sphereCollider"]["centerOffset"] = Vector3ToJson(collider->GetCenterOffset());
			objectJson["sphereCollider"]["radius"] = collider->GetRadius();
			objectJson["sphereCollider"]["drawDebug"] = collider->GetDrawDebug();
			objectJson["sphereCollider"]["pushBack"] = collider->GetPushBackEnabled();
		}
		if (Object3dComponent* object3dComponent = object->GetComponent<Object3dComponent>()) {
			objectJson["object3d"]["enabled"] = object3dComponent->IsEnabled();
			SaveComponentGravity(objectJson["object3d"], object3dComponent);
			objectJson["object3d"]["modelTextureFilePath"] = object3dComponent->GetModelTextureFilePath();
			objectJson["object3d"]["drawSkeleton"] = object3dComponent->GetDrawSkeleton();
			objectJson["object3d"]["animationPlaying"] = object3dComponent->GetAnimationPlaying();
			objectJson["object3d"]["isPointLight"] = object3dComponent->GetIsPointLightSet();
			objectJson["object3d"]["pointLight"]["color"] = Vector4ToJson(object3dComponent->GetPointLightColor());
			objectJson["object3d"]["pointLight"]["position"] = Vector3ToJson(object3dComponent->GetPointLightPosition());
			objectJson["object3d"]["pointLight"]["intensity"] = object3dComponent->GetPointLightIntensity();
			objectJson["object3d"]["pointLight"]["radius"] = object3dComponent->GetPointLightRadius();
			objectJson["object3d"]["pointLight"]["decay"] = object3dComponent->GetPointLightDecay();
		}
		if (ParticleEmitterComponent* emitter = object->GetComponent<ParticleEmitterComponent>()) {
			const ParticleEmitParam param = emitter->GetPalam();
			objectJson["particleEmitter"]["enabled"] = emitter->IsEnabled();
			SaveComponentGravity(objectJson["particleEmitter"], emitter);
			objectJson["particleEmitter"]["groupName"] = emitter->GetGroupName();
			objectJson["particleEmitter"]["textureFilePath"] = emitter->GetTextureFilePath();
			objectJson["particleEmitter"]["isActive"] = emitter->GetIsActive();
			objectJson["particleEmitter"]["frequency"] = emitter->GetFrequency();
			objectJson["particleEmitter"]["blendMode"] = static_cast<int>(emitter->GetBlendMode());
			objectJson["particleEmitter"]["meshType"] = static_cast<int>(emitter->GetMeshType());
			objectJson["particleEmitter"]["param"]["count"] = param.count;
			objectJson["particleEmitter"]["param"]["lifeTime"] = param.lifeTime;
			objectJson["particleEmitter"]["param"]["scale"] = Vector3ToJson(param.scale);
			objectJson["particleEmitter"]["param"]["endScale"] = Vector3ToJson(param.endScale);
			objectJson["particleEmitter"]["param"]["baseVelocity"] = Vector3ToJson(param.baseVelocity);
			objectJson["particleEmitter"]["param"]["randomVelocityRange"] = Vector3ToJson(param.randomVelocityRange);
			objectJson["particleEmitter"]["param"]["acceleration"] = Vector3ToJson(param.acceleration);
			objectJson["particleEmitter"]["param"]["randomPositionRange"] = Vector3ToJson(param.randomPositionRange);
			objectJson["particleEmitter"]["param"]["baseRotate"] = Vector3ToJson(param.baseRotate);
			objectJson["particleEmitter"]["param"]["isRandomRotate"] = param.isRandomRotate;
			objectJson["particleEmitter"]["param"]["randomRotateRange"] = Vector3ToJson(param.randomRotateRange);
			objectJson["particleEmitter"]["param"]["color"] = Vector4ToJson(param.color);
			objectJson["particleEmitter"]["param"]["endColor"] = Vector4ToJson(param.endColor);
			objectJson["particleEmitter"]["param"]["randomScaleRange"] = Vector3ToJson(param.randomScaleRange);
			objectJson["particleEmitter"]["param"]["isBillboard"] = param.isBillboard;
		}
		if (EnemySpawnPointComponent* enemySpawnPoint = object->GetComponent<EnemySpawnPointComponent>()) {
			objectJson["type"] = "EnemySpawnPoint";
			objectJson["enemySpawnPoint"]["enabled"] = enemySpawnPoint->IsEnabled();
			SaveComponentGravity(objectJson["enemySpawnPoint"], enemySpawnPoint);
			objectJson["enemySpawnPoint"]["targetName"] = enemySpawnPoint->GetTargetName();
			objectJson["enemySpawnPoint"]["cameraName"] = enemySpawnPoint->GetCameraName();
			objectJson["enemySpawnPoint"]["enemyTypeName"] = enemySpawnPoint->GetEnemyTypeName();
			objectJson["enemySpawnPoint"]["spawnEnabled"] = enemySpawnPoint->GetSpawnEnabled();
			objectJson["enemySpawnPoint"]["spawnCount"] = enemySpawnPoint->GetSpawnCount();
			objectJson["enemySpawnPoint"]["outerMargin"] = enemySpawnPoint->GetOuterMargin();
			objectJson["enemySpawnPoint"]["minimumRadius"] = enemySpawnPoint->GetMinimumRadius();
			objectJson["enemySpawnPoint"]["groundY"] = enemySpawnPoint->GetGroundY();
			objectJson["enemySpawnPoint"]["pointHeight"] = enemySpawnPoint->GetPointHeight();
			objectJson["enemySpawnPoint"]["drawDebug"] = enemySpawnPoint->GetDrawDebug();
			objectJson["enemySpawnPoint"]["debugPointSize"] = enemySpawnPoint->GetDebugPointSize();
		}
		objectJson["transform"]["scale"] = Vector3ToJson(transform.scale);
		objectJson["transform"]["rotate"] = Vector3ToJson(transform.rotate);
		objectJson["transform"]["translate"] = Vector3ToJson(transform.translate);

		objectJson["children"] = nlohmann::json::array();
		for (const auto& childObject : sceneObjects_) {
			if (childObject->GetParentName() == object->GetName()) {
				objectJson["children"].push_back(makeObjectJson(childObject.get()));
			}
		}
		return objectJson;
	};

	for (const auto& object : sceneObjects_) {
		if (EnemyComponent* enemy = object->GetComponent<EnemyComponent>(); enemy && enemy->GetRuntimeSpawned()) {
			continue;
		}
		if (object->GetComponent<ExperienceComponent>()) {
			continue;
		}
		if (object->GetParentName().empty() || !FindObjectByName(object->GetParentName())) {
			root["objects"].push_back(makeObjectJson(object.get()));
		}
	}

	const std::string filePath = GetSceneObjectFilePath();
	std::filesystem::create_directories(std::filesystem::path(filePath).parent_path());

	std::ofstream ofs(filePath);
	if (!ofs) {
		return;
	}
	ofs << root.dump(4);
}

/// <summary>
/// JSONからエディタ配置オブジェクトを読み込みます。
/// </summary>
void BaseScene::LoadEditorObjects() {
	const std::string filePath = GetSceneObjectFilePath();
	std::ifstream ifs(filePath);
	if (!ifs) {
		return;
	}

	nlohmann::json root;
	ifs >> root;

	sceneObjects_.clear();
	editorSkyBox_.reset();
	selectedObjectIndex_ = -1;
	nextObjectId_ = root.value("nextObjectId", 1);
	const bool hasSavedActiveCamera = root.contains("activeCamera");
	activeCameraObjectName_ = root.value("activeCamera", "");
	const std::string requestedActiveCameraName = activeCameraObjectName_;
	const nlohmann::json skyBoxJson = root.value("skyBox", nlohmann::json::object());
	isEditorSkyBoxEnabled_ = skyBoxJson.value("enabled", false);
	skyBoxTextureFilePath_ = skyBoxJson.value("textureFilePath", std::string());
	if (!skyBoxTextureFilePath_.empty()) {
		CreateOrReloadEditorSkyBox(skyBoxTextureFilePath_);
	}

	std::vector<nlohmann::json> flatObjects;
	std::function<void(nlohmann::json, const std::string&)> collectObjects = [&](nlohmann::json objectJson, const std::string& parentName) {
		if (!parentName.empty()) {
			objectJson["parent"] = parentName;
		}
		flatObjects.push_back(objectJson);
		const std::string currentName = objectJson.value("name", "");
		const nlohmann::json children = objectJson.value("children", nlohmann::json::array());
		for (const auto& childJson : children) {
			collectObjects(childJson, currentName);
		}
	};

	const nlohmann::json objects = root.value("objects", nlohmann::json::array());
	for (const auto& objectJson : objects) {
		collectObjects(objectJson, objectJson.value("parent", ""));
	}

	for (const auto& objectJson : flatObjects) {
		const std::string typeName = objectJson.value("type", "Empty");
		std::string modelFilePath = objectJson.value("model", "");
		if (typeName == "Player") {
			const nlohmann::json playerJson = objectJson.value("player", nlohmann::json::object());
			modelFilePath = playerJson.value("typeName", playerJson.value("model", modelFilePath));
		}
		if (typeName == "Enemy") {
			const nlohmann::json enemyJson = objectJson.value("enemy", nlohmann::json::object());
			modelFilePath = enemyJson.value("typeName", std::string("Default"));
		}
		GameObject* object = CreateEditorObject(EditorCreateTypeFromName(typeName), modelFilePath);
		if (!object) {
			continue;
		}

		object->SetName(objectJson.value("name", object->GetName()));
		if (typeName == "LoadedModel" && !modelFilePath.empty()) {
			object->SetEditorType("LoadedModel:" + modelFilePath);
		} else if (typeName == "AnimatedModel" && !modelFilePath.empty()) {
			object->SetEditorType("AnimatedModel:" + modelFilePath);
		} else {
			object->SetEditorType(typeName);
		}
		object->SetParentName(objectJson.value("parent", ""));

		const nlohmann::json transformJson = objectJson.value("transform", nlohmann::json::object());
		EulerTransform& transform = object->GetTransform();
		transform.scale = JsonToVector3(transformJson.value("scale", nlohmann::json::array()), transform.scale);
		transform.rotate = JsonToVector3(transformJson.value("rotate", nlohmann::json::array()), transform.rotate);
		transform.translate = JsonToVector3(transformJson.value("translate", nlohmann::json::array()), transform.translate);
		if (Player* player = object->GetComponent<Player>()) {
			const nlohmann::json playerJson = objectJson.value("player", nlohmann::json::object());
			player->SetEnabled(playerJson.value("enabled", player->IsEnabled()));
			LoadComponentGravity(playerJson, player);
			const std::string playerTypeName = playerJson.value("typeName", player->GetPlayerTypeName());
			player->SetPlayerTypeName(playerTypeName);
			PlayerStats playerStats = LoadPlayerStats(playerTypeName);
			player->ApplyStats(playerStats, ApplyPlayerStatusItems(playerStats));
			PlayerAttackComponent* attack = object->GetComponent<PlayerAttackComponent>();
			if (!attack) {
				attack = object->AddComponent<PlayerAttackComponent>();
			}
			ApplyPlayerAttackSlots(attack, playerStats);
			const nlohmann::json attackJson = objectJson.value("playerAttack", nlohmann::json::object());
			attack->SetEnabled(attackJson.value("enabled", attack->IsEnabled()));
			LoadComponentGravity(attackJson, attack);
			const Vector3 spawnPoint = JsonToVector3(playerJson.value("spawnPoint", nlohmann::json::array()), transform.translate);
			player->SetSpawnPoint(spawnPoint);
			const std::string playerModelFilePath = playerJson.value("model", player->GetModelFilePath());
			Model* playerModel = ModelManager::GetInstance()->FindModel(playerModelFilePath);
			const bool isAnimationModel = playerJson.value("isAnimationModel", playerModel && playerModel->GetIsAnimation());
			player->SetModelFilePath(playerModelFilePath, isAnimationModel);
			if (Object3dComponent* object3dComponent = object->GetComponent<Object3dComponent>()) {
				if (playerModel) {
					object3dComponent->SetModel(playerModelFilePath);
				}
				object3dComponent->SetDrawSkeleton(isAnimationModel);
			}
			player->ResetToSpawnPoint();
			player->SetCurrentHealth(playerJson.value("currentHealth", player->GetMaxHealth()));
			player->SetLevel(playerJson.value("level", player->GetStats().level));
			player->SetExperience(playerJson.value("experience", player->GetStats().experience));
			CameraComponent* playerCamera = object->GetComponent<CameraComponent>();
			if (!playerCamera) {
				playerCamera = object->AddComponent<CameraComponent>();
			}
			playerCamera->SetLocalOffset({0.0f, 15.0f, 0.0f});
			playerCamera->SetFollowOffset({0.0f, 15.0f, 0.0f});
			playerCamera->SetOverrideRotationEnabled(true);
			playerCamera->SetOverrideRotation({kPi * 0.5f, 0.0f, 0.0f});
			playerCamera->SetFovY(0.75f);
			playerCamera->SetFarClip(1000.0f);
			activeCameraObjectName_ = object->GetName();
		}
		if (EnemyComponent* enemy = object->GetComponent<EnemyComponent>()) {
			const nlohmann::json enemyJson = objectJson.value("enemy", nlohmann::json::object());
			enemy->SetEnabled(enemyJson.value("enabled", enemy->IsEnabled()));
			LoadComponentGravity(enemyJson, enemy);
			const std::string enemyTypeName = enemyJson.value("typeName", enemy->GetEnemyTypeName());
			enemy->SetEnemyTypeName(enemyTypeName);
			enemy->ApplyStats(LoadEnemyStats(enemyTypeName));
			enemy->SetCurrentHealth(enemyJson.value("currentHealth", enemy->GetCurrentHealth()));
			enemy->SetTargetName(enemyJson.value("targetName", std::string()));
			enemy->SetRuntimeSpawned(false);
		}

		if (SpriteComponent* spriteComponent = object->GetComponent<SpriteComponent>()) {
			const nlohmann::json spriteJson = objectJson.value("sprite", nlohmann::json::object());
			spriteComponent->SetEnabled(spriteJson.value("enabled", spriteComponent->IsEnabled()));
			LoadComponentGravity(spriteJson, spriteComponent);
			const std::string textureFilePath = spriteJson.value("textureFilePath", spriteComponent->GetTextureFilePath());
			if (!textureFilePath.empty()) {
				spriteComponent->SetTexture(textureFilePath);
			}
			if (spriteJson.contains("color")) {
				spriteComponent->SetColor(JsonToVector4(spriteJson.value("color", nlohmann::json::array()), spriteComponent->GetColor()));
			}
			const nlohmann::json sizeJson = spriteJson.value("size", nlohmann::json::array());
			if (sizeJson.is_array() && sizeJson.size() >= 2) {
				spriteComponent->SetSize({sizeJson.at(0).get<float>(), sizeJson.at(1).get<float>()});
			}
		}
		if (TextComponent* textComponent = object->GetComponent<TextComponent>()) {
			const nlohmann::json textJson = objectJson.value("text", nlohmann::json::object());
			textComponent->SetEnabled(textJson.value("enabled", textComponent->IsEnabled()));
			LoadComponentGravity(textJson, textComponent);
			textComponent->SetText(textJson.value("value", textComponent->GetText()));
			textComponent->SetFontName(textJson.value("fontName", textComponent->GetFontName()));
			textComponent->SetFontSize(textJson.value("fontSize", textComponent->GetFontSize()));
			textComponent->SetAnchor(static_cast<TextComponent::Anchor>(textJson.value("anchor", static_cast<int>(textComponent->GetAnchor()))));
			textComponent->SetColor(JsonToVector4(textJson.value("color", nlohmann::json::array()), textComponent->GetColor()));
		}
		if (Object3dComponent* object3dComponent = object->GetComponent<Object3dComponent>()) {
			const nlohmann::json object3dJson = objectJson.value("object3d", nlohmann::json::object());
			object3dComponent->SetEnabled(object3dJson.value("enabled", object3dComponent->IsEnabled()));
			LoadComponentGravity(object3dJson, object3dComponent);
			const std::string textureFilePath = object3dJson.value("modelTextureFilePath", std::string());
			if (!textureFilePath.empty()) {
				object3dComponent->SetModelTexture(textureFilePath);
			}
			object3dComponent->SetDrawSkeleton(object3dJson.value("drawSkeleton", object3dComponent->GetDrawSkeleton()));
			object3dComponent->SetAnimationPlaying(object3dJson.value("animationPlaying", object3dComponent->GetAnimationPlaying()));
			object3dComponent->IsPointLightSet(object3dJson.value("isPointLight", object3dComponent->GetIsPointLightSet()));
			const nlohmann::json pointLightJson = object3dJson.value("pointLight", nlohmann::json::object());
			object3dComponent->SetPointLight(
			    JsonToVector4(pointLightJson.value("color", nlohmann::json::array()), object3dComponent->GetPointLightColor()),
			    JsonToVector3(pointLightJson.value("position", nlohmann::json::array()), object3dComponent->GetPointLightPosition()),
			    pointLightJson.value("intensity", object3dComponent->GetPointLightIntensity()),
			    pointLightJson.value("radius", object3dComponent->GetPointLightRadius()),
			    pointLightJson.value("decay", object3dComponent->GetPointLightDecay())
			);
		}
		if (CameraComponent* cameraComponent = object->GetComponent<CameraComponent>()) {
			const nlohmann::json cameraJson = objectJson.value("camera", nlohmann::json::object());
			cameraComponent->SetEnabled(cameraJson.value("enabled", cameraComponent->IsEnabled()));
			LoadComponentGravity(cameraJson, cameraComponent);
			cameraComponent->SetFovY(cameraJson.value("fovY", cameraComponent->GetFovY()));
			cameraComponent->SetNearClip(cameraJson.value("nearClip", cameraComponent->GetNearClip()));
			cameraComponent->SetFarClip(cameraJson.value("farClip", cameraComponent->GetFarClip()));
			cameraComponent->SetFollowTargetName(cameraJson.value("followTarget", ""));
			cameraComponent->SetFollowOffset(JsonToVector3(cameraJson.value("followOffset", nlohmann::json::array()), cameraComponent->GetFollowOffset()));
			cameraComponent->SetLocalOffset(JsonToVector3(cameraJson.value("localOffset", nlohmann::json::array()), cameraComponent->GetLocalOffset()));
			cameraComponent->SetOverrideRotationEnabled(cameraJson.value("overrideRotationEnabled", cameraComponent->GetOverrideRotationEnabled()));
			cameraComponent->SetOverrideRotation(JsonToVector3(cameraJson.value("overrideRotation", nlohmann::json::array()), cameraComponent->GetOverrideRotation()));
		}
		if (objectJson.contains("obbCollider")) {
			const nlohmann::json colliderJson = objectJson.value("obbCollider", nlohmann::json::object());
			OBBColliderComponent* collider = object->GetComponent<OBBColliderComponent>();
			if (!collider) {
				collider = object->AddComponent<OBBColliderComponent>();
			}
			collider->SetEnabled(colliderJson.value("enabled", collider->IsEnabled()));
			LoadComponentGravity(colliderJson, collider);
			collider->SetCenterOffset(JsonToVector3(colliderJson.value("centerOffset", nlohmann::json::array()), collider->GetCenterOffset()));
			collider->SetHalfSize(JsonToVector3(colliderJson.value("halfSize", nlohmann::json::array()), collider->GetHalfSize()));
			collider->SetDrawDebug(colliderJson.value("drawDebug", collider->GetDrawDebug()));
			collider->SetPushBackEnabled(colliderJson.value("pushBack", collider->GetPushBackEnabled()));
		}
		if (objectJson.contains("sphereCollider")) {
			const nlohmann::json colliderJson = objectJson.value("sphereCollider", nlohmann::json::object());
			SphereColliderComponent* collider = object->GetComponent<SphereColliderComponent>();
			if (!collider) {
				collider = object->AddComponent<SphereColliderComponent>();
			}
			collider->SetEnabled(colliderJson.value("enabled", collider->IsEnabled()));
			LoadComponentGravity(colliderJson, collider);
			collider->SetCenterOffset(JsonToVector3(colliderJson.value("centerOffset", nlohmann::json::array()), collider->GetCenterOffset()));
			collider->SetRadius(colliderJson.value("radius", collider->GetRadius()));
			collider->SetDrawDebug(colliderJson.value("drawDebug", collider->GetDrawDebug()));
			collider->SetPushBackEnabled(colliderJson.value("pushBack", collider->GetPushBackEnabled()));
		}
		if (ParticleEmitterComponent* emitter = object->GetComponent<ParticleEmitterComponent>()) {
			const nlohmann::json emitterJson = objectJson.value("particleEmitter", nlohmann::json::object());
			emitter->SetEnabled(emitterJson.value("enabled", emitter->IsEnabled()));
			LoadComponentGravity(emitterJson, emitter);
			const std::string groupName = emitterJson.value("groupName", object->GetName());
			const std::string textureFilePath = emitterJson.value("textureFilePath", std::string("Resources/circle.png"));
			const ParticleMeshType meshType = static_cast<ParticleMeshType>(emitterJson.value("meshType", static_cast<int>(emitter->GetMeshType())));

			if (!ParticleManager::GetInstance()->GetGroup(groupName)) {
				ParticleManager::GetInstance()->CreateParticleGroup(groupName, textureFilePath, meshType);
			}
			emitter->SetGroupName(groupName);
			emitter->SetTexture(textureFilePath);
			emitter->SetIsActive(emitterJson.value("isActive", emitter->GetIsActive()));
			emitter->SetFrequency(emitterJson.value("frequency", emitter->GetFrequency()));
			emitter->SetBlendMode(static_cast<BlendMode>(emitterJson.value("blendMode", static_cast<int>(emitter->GetBlendMode()))));
			emitter->SetMeshType(meshType);

			const nlohmann::json paramJson = emitterJson.value("param", nlohmann::json::object());
			ParticleEmitParam param = emitter->GetPalam();
			param.count = paramJson.value("count", param.count);
			param.lifeTime = paramJson.value("lifeTime", param.lifeTime);
			param.scale = JsonToVector3(paramJson.value("scale", nlohmann::json::array()), param.scale);
			param.endScale = JsonToVector3(paramJson.value("endScale", nlohmann::json::array()), param.endScale);
			param.baseVelocity = JsonToVector3(paramJson.value("baseVelocity", nlohmann::json::array()), param.baseVelocity);
			param.randomVelocityRange = JsonToVector3(paramJson.value("randomVelocityRange", nlohmann::json::array()), param.randomVelocityRange);
			param.acceleration = JsonToVector3(paramJson.value("acceleration", nlohmann::json::array()), param.acceleration);
			param.randomPositionRange = JsonToVector3(paramJson.value("randomPositionRange", nlohmann::json::array()), param.randomPositionRange);
			param.baseRotate = JsonToVector3(paramJson.value("baseRotate", nlohmann::json::array()), param.baseRotate);
			param.isRandomRotate = paramJson.value("isRandomRotate", param.isRandomRotate);
			param.randomRotateRange = JsonToVector3(paramJson.value("randomRotateRange", nlohmann::json::array()), param.randomRotateRange);
			param.color = JsonToVector4(paramJson.value("color", nlohmann::json::array()), param.color);
			param.endColor = JsonToVector4(paramJson.value("endColor", nlohmann::json::array()), param.endColor);
			param.randomScaleRange = JsonToVector3(paramJson.value("randomScaleRange", nlohmann::json::array()), param.randomScaleRange);
			param.isBillboard = paramJson.value("isBillboard", param.isBillboard);
			emitter->SetParam(param);
		}
		if (objectJson.contains("enemySpawnPoint")) {
			const nlohmann::json spawnJson = objectJson.value("enemySpawnPoint", nlohmann::json::object());
			EnemySpawnPointComponent* enemySpawnPoint = object->GetComponent<EnemySpawnPointComponent>();
			if (!enemySpawnPoint) {
				enemySpawnPoint = object->AddComponent<EnemySpawnPointComponent>();
			}
			enemySpawnPoint->SetEnabled(spawnJson.value("enabled", enemySpawnPoint->IsEnabled()));
			LoadComponentGravity(spawnJson, enemySpawnPoint);
			enemySpawnPoint->SetTargetName(spawnJson.value("targetName", std::string()));
			enemySpawnPoint->SetCameraName(spawnJson.value("cameraName", std::string()));
			enemySpawnPoint->SetEnemyTypeName(spawnJson.value("enemyTypeName", enemySpawnPoint->GetEnemyTypeName()));
			enemySpawnPoint->SetSpawnEnabled(spawnJson.value("spawnEnabled", enemySpawnPoint->GetSpawnEnabled()));
			enemySpawnPoint->SetSpawnCount(spawnJson.value("spawnCount", enemySpawnPoint->GetSpawnCount()));
			enemySpawnPoint->SetOuterMargin(spawnJson.value("outerMargin", enemySpawnPoint->GetOuterMargin()));
			enemySpawnPoint->SetMinimumRadius(spawnJson.value("minimumRadius", enemySpawnPoint->GetMinimumRadius()));
			enemySpawnPoint->SetGroundY(spawnJson.value("groundY", enemySpawnPoint->GetGroundY()));
			enemySpawnPoint->SetPointHeight(spawnJson.value("pointHeight", enemySpawnPoint->GetPointHeight()));
			enemySpawnPoint->SetDrawDebug(spawnJson.value("drawDebug", enemySpawnPoint->GetDrawDebug()));
			enemySpawnPoint->SetDebugPointSize(spawnJson.value("debugPointSize", enemySpawnPoint->GetDebugPointSize()));
		}
	}

	ResolveCameraLinks();
	ResolveEnemySpawnPointLinks();
	ResolveEnemyLinks();
	if (!requestedActiveCameraName.empty()) {
		GameObject* requestedCameraObject = FindObjectByName(requestedActiveCameraName);
		CameraComponent* requestedCameraComponent =
		    requestedCameraObject ? requestedCameraObject->GetComponent<CameraComponent>() : nullptr;
		if (requestedCameraComponent && requestedCameraComponent->IsEnabled()) {
			activeCameraObjectName_ = requestedActiveCameraName;
		} else {
			activeCameraObjectName_.clear();
		}
	} else if (hasSavedActiveCamera) {
		activeCameraObjectName_.clear();
	}
	if (activeCameraObjectName_.empty() && !hasSavedActiveCamera) {
		if (GameObject* firstCamera = FindFirstCameraObject()) {
			activeCameraObjectName_ = firstCamera->GetName();
		}
	}
	ApplyActiveCamera();

	if (!sceneObjects_.empty()) {
		selectedObjectIndex_ = 0;
	}
	nextObjectId_ = root.value("nextObjectId", nextObjectId_);
}

/// <summary>
/// 現在のシーンに対応する配置JSONファイルパスを返します。
/// </summary>
std::string BaseScene::GetSceneObjectFilePath() const {
	return "Resources/Data/Scenes/" + sceneName_ + "_objects.json";
}

/// <summary>
/// 文字列からエディタ生成タイプへ変換します。
/// </summary>
BaseScene::EditorCreateType BaseScene::EditorCreateTypeFromName(const std::string& typeName) const {
	if (typeName == "Object3dSphere") {
		return EditorCreateType::Object3dSphere;
	}
	if (typeName == "Object3dCylinder") {
		return EditorCreateType::Object3dCylinder;
	}
	if (typeName == "Object3dCylinderOpen") {
		return EditorCreateType::Object3dCylinderOpen;
	}
	if (typeName == "Sprite") {
		return EditorCreateType::Sprite;
	}
	if (typeName == "Text") {
		return EditorCreateType::Text;
	}
	if (typeName == "LoadedModel" || typeName.starts_with("LoadedModel:")) {
		return EditorCreateType::LoadedModel;
	}
	if (typeName == "AnimatedModel" || typeName.starts_with("AnimatedModel:")) {
		return EditorCreateType::AnimatedModel;
	}
	if (typeName == "Camera") {
		return EditorCreateType::Camera;
	}
	if (typeName == "PointLight") {
		return EditorCreateType::PointLight;
	}
	if (typeName == "ParticleEmitter") {
		return EditorCreateType::ParticleEmitter;
	}
	if (typeName == "Player") {
		return EditorCreateType::Player;
	}
	if (typeName == "EnemySpawnPoint") {
		return EditorCreateType::EnemySpawnPoint;
	}
	if (typeName == "Enemy") {
		return EditorCreateType::Enemy;
	}
	return EditorCreateType::Empty;
}
