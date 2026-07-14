#include "BaseScene.h"
#include "BaseSceneHelpers.h"

#ifdef USE_IMGUI
#include <cctype>

namespace {
constexpr float kProjectThumbnailSize = 64.0f;

void DrawProjectAssetDragSource(const std::string& label, const char* payloadType, const std::string& path) {
	ImGui::Selectable(label.c_str(), false);
	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
		ImGui::SetDragDropPayload(payloadType, path.c_str(), path.size() + 1);
		ImGui::Text("%s", label.c_str());
		ImGui::EndDragDropSource();
	}
}

void DrawProjectTextureDragSource(const std::string& texturePath) {
	TextureManager::GetInstance()->LoadTexture(texturePath);
	const D3D12_GPU_DESCRIPTOR_HANDLE textureHandle = TextureManager::GetInstance()->GetSRVHandleGPU(texturePath);
	const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetTextureMetadata(texturePath);
	const float width = static_cast<float>(metadata.width);
	const float height = static_cast<float>(metadata.height);
	const float maxSide = (std::max)(width, height);
	const ImVec2 imageSize = maxSide > 0.0f
	    ? ImVec2(kProjectThumbnailSize * width / maxSide, kProjectThumbnailSize * height / maxSide)
	    : ImVec2(kProjectThumbnailSize, kProjectThumbnailSize);
	const std::string fileName = std::filesystem::path(texturePath).filename().string();
	const std::string buttonId = "##SpriteTexture_" + texturePath;

	ImGui::BeginGroup();
	ImGui::PushID(texturePath.c_str());
	ImGui::BeginChild("ThumbnailFrame", ImVec2(kProjectThumbnailSize + 12.0f, kProjectThumbnailSize + 12.0f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	const ImVec2 cursor = ImGui::GetCursorPos();
	ImGui::SetCursorPos(ImVec2(
	    cursor.x + (kProjectThumbnailSize - imageSize.x) * 0.5f,
	    cursor.y + (kProjectThumbnailSize - imageSize.y) * 0.5f
	));
	ImGui::ImageButton(buttonId.c_str(), static_cast<ImTextureID>(textureHandle.ptr), imageSize);
	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
		ImGui::SetDragDropPayload("CG2_ASSET_SPRITE", texturePath.c_str(), texturePath.size() + 1);
		ImGui::Image(static_cast<ImTextureID>(textureHandle.ptr), imageSize);
		ImGui::Text("%s", texturePath.c_str());
		ImGui::EndDragDropSource();
	}
	if (ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();
		ImGui::Image(static_cast<ImTextureID>(textureHandle.ptr), ImVec2(kProjectThumbnailSize * 2.0f, kProjectThumbnailSize * 2.0f));
		ImGui::Text("%s", texturePath.c_str());
		ImGui::Text("%.0f x %.0f", width, height);
		ImGui::EndTooltip();
	}
	ImGui::EndChild();
	ImGui::TextWrapped("%s", fileName.c_str());
	ImGui::PopID();
	ImGui::EndGroup();
}

bool IsSpriteTexturePath(const std::string& path) {
	std::string extension = std::filesystem::path(path).extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char value) {
		return static_cast<char>(std::tolower(value));
	});
	return extension == ".png" || extension == ".jpg" || extension == ".jpeg";
}
}
#endif

GameObject* BaseScene::CreateEditorObject(EditorCreateType type, const std::string& modelFilePath) {
	auto object = std::make_unique<GameObject>();
	object->SetEditorType(EditorCreateTypeName(type));

	switch (type) {
	case EditorCreateType::Empty:
		object->SetName(MakeUniqueObjectName("Empty"));
		break;
	case EditorCreateType::Object3dSphere: {
		object->SetName(MakeUniqueObjectName("Sphere"));
		ModelManager::GetInstance()->LoadModel("sphere.obj");
		Object3dComponent* object3d = object->AddComponent<Object3dComponent>();
		object3d->SetModel("sphere.obj");
		break;
	}
	case EditorCreateType::Object3dCylinder: {
		object->SetName(MakeUniqueObjectName("Cylinder"));
		Object3dComponent* object3d = object->AddComponent<Object3dComponent>();
		object3d->CreateCylinder(1.0f, 2.0f, 16, true, true);
		object3d->SetTexture("Resources/gradationLine.png");
		break;
	}
	case EditorCreateType::Object3dCylinderOpen: {
		object->SetName(MakeUniqueObjectName("OpenCylinder"));
		Object3dComponent* object3d = object->AddComponent<Object3dComponent>();
		object3d->CreateCylinder(1.0f, 2.0f, 16, false, false);
		object3d->SetTexture("Resources/gradationLine.png");
		break;
	}
	case EditorCreateType::Sprite: {
		const std::string spriteBaseName = modelFilePath.empty() ? "Sprite" : std::filesystem::path(modelFilePath).stem().string();
		object->SetName(MakeUniqueObjectName(spriteBaseName.empty() ? "Sprite" : spriteBaseName));
		SpriteComponent* sprite = object->AddComponent<SpriteComponent>();
		const std::vector<std::string> textures = CollectResourceTexturePaths();
		const std::string textureFilePath = !modelFilePath.empty() ? modelFilePath : (textures.empty() ? "Resources/uvChecker.png" : textures[std::min(selectedTextureIndex_, static_cast<int>(textures.size()) - 1)]);
		TextureManager::GetInstance()->LoadTexture(textureFilePath);
		sprite->Initialize(textureFilePath);
		EulerTransform transform = object->GetTransform();
		transform.scale = {100.0f, 100.0f, 1.0f};
		object->GetTransform() = transform;
		sprite->SetSize({100.0f, 100.0f});
		break;
	}
	case EditorCreateType::LoadedModel: {
		if (modelFilePath.empty() || !ModelManager::GetInstance()->FindModel(modelFilePath)) {
			return nullptr;
		}
		object->SetName(MakeUniqueObjectName(std::filesystem::path(modelFilePath).stem().string()));
		object->SetEditorType("LoadedModel:" + modelFilePath);
		Object3dComponent* object3d = object->AddComponent<Object3dComponent>();
		object3d->SetModel(modelFilePath);
		break;
	}
	case EditorCreateType::AnimatedModel: {
		if (modelFilePath.empty() || !ModelManager::GetInstance()->FindModel(modelFilePath)) {
			return nullptr;
		}
		object->SetName(MakeUniqueObjectName(std::filesystem::path(modelFilePath).stem().string()));
		object->SetEditorType("AnimatedModel:" + modelFilePath);
		Object3dComponent* object3d = object->AddComponent<Object3dComponent>();
		object3d->SetModel(modelFilePath);
		object3d->SetDrawSkeleton(true);
		break;
	}
	case EditorCreateType::Camera: {
		object->SetName(MakeUniqueObjectName("Camera"));
		CameraComponent* camera = object->AddComponent<CameraComponent>();
		camera->SetFovY(0.45f);
		EulerTransform& transform = object->GetTransform();
		transform.translate = {0.0f, 4.0f, -10.0f};
		break;
	}
	case EditorCreateType::PointLight: {
		object->SetName(MakeUniqueObjectName("PointLight"));
		Object3dComponent* object3d = object->AddComponent<Object3dComponent>();
		ModelManager::GetInstance()->LoadModel("sphere.obj");
		object3d->SetModel("sphere.obj");
		object3d->IsPointLightSet(true);
		object3d->SetPointLight({1.0f, 1.0f, 1.0f, 1.0f}, object->GetTransform().translate, 1.0f, 10.0f, 1.0f);
		object->GetTransform().scale = {0.25f, 0.25f, 0.25f};
		break;
	}
	case EditorCreateType::ParticleEmitter: {
		object->SetName(MakeUniqueObjectName("ParticleEmitter"));
		ParticleEmitterComponent* emitter = object->AddComponent<ParticleEmitterComponent>();
		const std::string groupName = object->GetName();
		std::string textureFilePath = "Resources/circle.png";
		ParticleMeshType meshType = kMeshTypeQuad;
		GetParticlePresetResourceInfo(modelFilePath, textureFilePath, meshType);
		if (!ParticleManager::GetInstance()->GetGroup(groupName)) {
			ParticleManager::GetInstance()->CreateParticleGroup(groupName, textureFilePath, meshType);
		}
		emitter->SetGroupName(groupName);
		emitter->SetTexture(textureFilePath);
		emitter->SetMeshType(meshType);
		if (!modelFilePath.empty()) {
			ApplyParticlePreset(modelFilePath, emitter);
		} else {
			emitter->SetFrequency(0.0f);
			ParticleEmitParam param = emitter->GetPalam();
			param.count = 10;
			param.lifeTime = 1.0f;
			param.scale = {1.0f, 1.0f, 1.0f};
			param.endScale = {0.0f, 0.0f, 0.0f};
			param.randomPositionRange = {0.5f, 0.5f, 0.5f};
			param.randomVelocityRange = {0.5f, 0.5f, 0.5f};
			emitter->SetParam(param);
		}
		break;
	}
	case EditorCreateType::Player: {
		object->SetName(MakeUniqueObjectName("Player"));
		Player* player = object->AddComponent<Player>();
		player->SetSpawnPoint(object->GetTransform().translate);

		Object3dComponent* object3d = object->AddComponent<Object3dComponent>();
		std::string playerModelFilePath = modelFilePath;
		if (playerModelFilePath.empty() || !ModelManager::GetInstance()->FindModel(playerModelFilePath)) {
			ModelManager::GetInstance()->LoadModel("sphere.obj");
			playerModelFilePath = "sphere.obj";
		}
		object3d->SetModel(playerModelFilePath);
		Model* playerModel = ModelManager::GetInstance()->FindModel(playerModelFilePath);
		const bool isAnimationModel = playerModel && playerModel->GetIsAnimation();
		object3d->SetDrawSkeleton(isAnimationModel);
		player->SetModelFilePath(playerModelFilePath, isAnimationModel);

		OBBColliderComponent* collider = object->AddComponent<OBBColliderComponent>();
		collider->SetHalfSize({0.5f, 1.0f, 0.5f});
		collider->SetCenterOffset({0.0f, 1.0f, 0.0f});
		collider->SetPushBackEnabled(true);

		CameraComponent* camera = object->AddComponent<CameraComponent>();
		camera->SetLocalOffset({0.0f, 15.0f, 0.0f});
		camera->SetOverrideRotationEnabled(true);
		camera->SetOverrideRotation({kPi * 0.5f, 0.0f, 0.0f});
		camera->SetFovY(0.75f);
		camera->SetFarClip(1000.0f);
		break;
	}
	case EditorCreateType::EnemySpawnPoint: {
		object->SetName(MakeUniqueObjectName("EnemySpawnPoint"));
		object->AddComponent<EnemySpawnPointComponent>();
		break;
	}
	case EditorCreateType::Enemy: {
		const std::string enemyTypeName = modelFilePath.empty() ? "Default" : modelFilePath;
		object->SetName(MakeUniqueObjectName(enemyTypeName));
		object->SetEditorType("Enemy");
		EnemyComponent* enemy = object->AddComponent<EnemyComponent>();
		enemy->SetEnemyTypeName(enemyTypeName);
		enemy->ApplyStats(LoadEnemyStats(enemyTypeName));

		ModelManager::GetInstance()->LoadModel("sphere.obj");
		Object3dComponent* object3d = object->AddComponent<Object3dComponent>();
		object3d->SetModel("sphere.obj");
		object->GetTransform().scale = {0.75f, 0.75f, 0.75f};

		OBBColliderComponent* collider = object->AddComponent<OBBColliderComponent>();
		collider->SetHalfSize({0.4f, 0.4f, 0.4f});
		collider->SetPushBackEnabled(true);
		break;
	}
	default:
		return nullptr;
	}

	object->Update();
	sceneObjects_.push_back(std::move(object));
	selectedObjectIndex_ = static_cast<int>(sceneObjects_.size()) - 1;
	if (type == EditorCreateType::Player || (type == EditorCreateType::Camera && activeCameraObjectName_.empty())) {
		SetActiveCameraObject(sceneObjects_.back().get());
	}
	++nextObjectId_;
	return sceneObjects_.back().get();
}

/// <summary>
/// 現在選択中のエディタオブジェクトを削除します。
/// </summary>
void BaseScene::DeleteSelectedEditorObject() {
	if (selectedObjectIndex_ < 0 || selectedObjectIndex_ >= static_cast<int>(sceneObjects_.size())) {
		return;
	}

	const bool deletesActiveCamera =
	    !activeCameraObjectName_.empty() &&
	    sceneObjects_[selectedObjectIndex_]->GetName() == activeCameraObjectName_;
	const std::string deletedObjectName = sceneObjects_[selectedObjectIndex_]->GetName();
	sceneObjects_.erase(sceneObjects_.begin() + selectedObjectIndex_);
	for (const auto& object : sceneObjects_) {
		if (object->GetParentName() == deletedObjectName) {
			object->SetParentName("");
		}
	}
	if (sceneObjects_.empty()) {
		selectedObjectIndex_ = -1;
	} else if (selectedObjectIndex_ >= static_cast<int>(sceneObjects_.size())) {
		selectedObjectIndex_ = static_cast<int>(sceneObjects_.size()) - 1;
	}

	if (deletesActiveCamera) {
		GameObject* nextCamera = FindFirstCameraObject();
		if (nextCamera) {
			SetActiveCameraObject(nextCamera);
		} else {
			activeCameraObjectName_.clear();
			ApplyCamera(fallbackCamera_);
		}
	}
}

/// <summary>
/// シーン内オブジェクトの階層ウィンドウを描画します。
/// </summary>
void BaseScene::DrawEditorHierarchy() {
#ifdef USE_IMGUI
	ImGui::Begin("Component Manager");

	DrawEditorSkyBoxControls();
	ImGui::Separator();
	DrawEditorCameraSelector();
	ImGui::Separator();

	const char* createLabels[] = {"Empty", "Sphere", "Cylinder Capped", "Cylinder Open", "Sprite", "Model", "Animation Model", "Camera", "Point Light", "Particle Emitter", "Player", "Enemy Spawn Point", "Enemy"};
	int createTypeIndex = static_cast<int>(createType_);
	if (ImGui::Combo("Type", &createTypeIndex, createLabels, _countof(createLabels))) {
		createType_ = static_cast<EditorCreateType>(createTypeIndex);
	}

	std::string selectedModelFilePath;
	std::string selectedParticlePresetName;
	std::string selectedEnemyTypeName = "Default";
	if (createType_ == EditorCreateType::Sprite) {
		const std::vector<std::string> textures = CollectResourceTexturePaths();
		if (textures.empty()) {
			ImGui::Text("No textures");
		} else {
			if (selectedTextureIndex_ >= static_cast<int>(textures.size())) {
				selectedTextureIndex_ = 0;
			}
			std::vector<const char*> textureLabels = MakeLabelPointers(textures);
			ImGui::Combo("Texture", &selectedTextureIndex_, textureLabels.data(), static_cast<int>(textureLabels.size()));
		}
	}
	if (createType_ == EditorCreateType::LoadedModel) {
		const std::vector<std::string> loadedModels = CollectLoadedModelNames(false);
		if (loadedModels.empty()) {
			ImGui::Text("No loaded models");
		} else {
			if (selectedLoadedModelIndex_ >= static_cast<int>(loadedModels.size())) {
				selectedLoadedModelIndex_ = 0;
			}

			std::vector<const char*> loadedModelLabels = MakeLabelPointers(loadedModels);
			ImGui::Combo("Model", &selectedLoadedModelIndex_, loadedModelLabels.data(), static_cast<int>(loadedModelLabels.size()));
			selectedModelFilePath = loadedModels[selectedLoadedModelIndex_];
		}
	}
	if (createType_ == EditorCreateType::AnimatedModel) {
		const std::vector<std::string> loadedModels = CollectLoadedModelNames(true);
		if (loadedModels.empty()) {
			ImGui::Text("No loaded animation models");
		} else {
			if (selectedAnimatedModelIndex_ >= static_cast<int>(loadedModels.size())) {
				selectedAnimatedModelIndex_ = 0;
			}

			std::vector<const char*> loadedModelLabels = MakeLabelPointers(loadedModels);
			ImGui::Combo("Animation Model", &selectedAnimatedModelIndex_, loadedModelLabels.data(), static_cast<int>(loadedModelLabels.size()));
			selectedModelFilePath = loadedModels[selectedAnimatedModelIndex_];
		}
	}
	if (createType_ == EditorCreateType::ParticleEmitter) {
		const std::vector<std::string> particlePresets = LoadParticlePresetNames();
		if (particlePresets.empty()) {
			ImGui::Text("No particle presets");
		} else {
			if (selectedParticlePresetIndex_ >= static_cast<int>(particlePresets.size())) {
				selectedParticlePresetIndex_ = 0;
			}

			std::vector<const char*> presetLabels;
			presetLabels.reserve(particlePresets.size());
			for (const std::string& presetName : particlePresets) {
				presetLabels.push_back(presetName.c_str());
			}
			ImGui::Combo("Particle Preset", &selectedParticlePresetIndex_, presetLabels.data(), static_cast<int>(presetLabels.size()));
			selectedParticlePresetName = particlePresets[selectedParticlePresetIndex_];
		}
	}
	if (createType_ == EditorCreateType::Player) {
		const std::vector<std::string> loadedModels = CollectAllLoadedModelNames();
		if (loadedModels.empty()) {
			ImGui::Text("No loaded models");
		} else {
			if (selectedPlayerModelIndex_ >= static_cast<int>(loadedModels.size())) {
				selectedPlayerModelIndex_ = 0;
			}

			std::vector<std::string> modelLabels;
			modelLabels.reserve(loadedModels.size());
			for (const std::string& modelName : loadedModels) {
				Model* model = ModelManager::GetInstance()->FindModel(modelName);
				modelLabels.push_back(model && model->GetIsAnimation() ? "[Anim] " + modelName : "[Model] " + modelName);
			}
			std::vector<const char*> loadedModelLabels = MakeLabelPointers(modelLabels);
			ImGui::Combo("Player Model", &selectedPlayerModelIndex_, loadedModelLabels.data(), static_cast<int>(loadedModelLabels.size()));
			selectedModelFilePath = loadedModels[selectedPlayerModelIndex_];
		}
	}
	if (createType_ == EditorCreateType::Enemy) {
		const std::vector<std::string> enemyTypes = LoadEnemyTypeNames();
		if (enemyTypes.empty()) {
			ImGui::Text("No enemy types");
		} else {
			if (selectedEnemyTypeIndex_ >= static_cast<int>(enemyTypes.size())) {
				selectedEnemyTypeIndex_ = 0;
			}
			std::vector<const char*> enemyTypeLabels = MakeLabelPointers(enemyTypes);
			ImGui::Combo("Enemy Type", &selectedEnemyTypeIndex_, enemyTypeLabels.data(), static_cast<int>(enemyTypeLabels.size()));
			selectedEnemyTypeName = enemyTypes[selectedEnemyTypeIndex_];
		}
	}

	if (ImGui::Button("Create")) {
		std::string createArgument = selectedModelFilePath;
		if (createType_ == EditorCreateType::ParticleEmitter) {
			createArgument = selectedParticlePresetName;
		} else if (createType_ == EditorCreateType::Enemy) {
			createArgument = selectedEnemyTypeName;
		}
		CreateEditorObject(createType_, createArgument);
	}
	ImGui::SameLine();
	const bool canDelete = selectedObjectIndex_ >= 0 && selectedObjectIndex_ < static_cast<int>(sceneObjects_.size());
	if (!canDelete) {
		ImGui::BeginDisabled();
	}
	if (ImGui::Button("Delete")) {
		DeleteSelectedEditorObject();
	}
	if (!canDelete) {
		ImGui::EndDisabled();
	}

	if (ImGui::Button("Save Layout")) {
		SaveEditorObjects();
	}
	ImGui::SameLine();
	if (ImGui::Button("Load Layout")) {
		LoadEditorObjects();
	}

	ImGui::Separator();
	std::function<void(const std::string&, int)> drawChildren = [&](const std::string& parentName, int depth) {
		for (int index = 0; index < static_cast<int>(sceneObjects_.size()); ++index) {
			if (sceneObjects_[index]->GetParentName() != parentName) {
				continue;
			}

			const bool isSelected = selectedObjectIndex_ == index;
			std::string label(depth * 2, ' ');
			label += sceneObjects_[index]->GetName();
			if (!activeCameraObjectName_.empty() && sceneObjects_[index]->GetName() == activeCameraObjectName_) {
				label += " [Active Camera]";
			}
			if (ImGui::Selectable(label.c_str(), isSelected)) {
				selectedObjectIndex_ = index;
				selectedInspectorComponentIndex_ = 0;
			}
			drawChildren(sceneObjects_[index]->GetName(), depth + 1);
		}
	};
	drawChildren("", 0);

	ImGui::End();
#endif
}

void BaseScene::DrawEditorProjectAssets() {
#ifdef USE_IMGUI
	if (!ImGui::Begin("Project")) {
		ImGui::End();
		return;
	}

	if (ImGui::CollapsingHeader("Loaded Models", ImGuiTreeNodeFlags_DefaultOpen)) {
		const std::vector<std::string> loadedModels = CollectAllLoadedModelNames();
		if (loadedModels.empty()) {
			ImGui::Text("No loaded models");
		}
		for (const std::string& modelName : loadedModels) {
			Model* model = ModelManager::GetInstance()->FindModel(modelName);
			const bool isAnimation = model && model->GetIsAnimation();
			const std::string label = std::string(isAnimation ? "[Anim] " : "[Model] ") + modelName;
			DrawProjectAssetDragSource(label, isAnimation ? "CG2_ASSET_ANIM_MODEL" : "CG2_ASSET_MODEL", modelName);
		}
	}

	if (ImGui::CollapsingHeader("Sprites / Textures", ImGuiTreeNodeFlags_DefaultOpen)) {
		const std::vector<std::string> textures = CollectResourceTexturePaths();
		bool hasSpriteTexture = false;
		for (const std::string& texturePath : textures) {
			if (!IsSpriteTexturePath(texturePath)) {
				continue;
			}
			hasSpriteTexture = true;
			DrawProjectTextureDragSource(texturePath);
			const float windowContentRight = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
			const float nextItemRight = ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x + kProjectThumbnailSize + 12.0f;
			if (nextItemRight < windowContentRight) {
				ImGui::SameLine();
			}
		}
		if (!hasSpriteTexture) {
			ImGui::Text("No sprite textures");
		}
	}

	ImGui::End();
#endif
}

void BaseScene::HandleGameViewAssetDrop() {
#ifdef USE_IMGUI
	ImGuiManager::DroppedAssetPayload payload;
	while (ImGuiManager::GetInstance()->ConsumeDroppedAsset(payload)) {
		GameObject* createdObject = nullptr;
		switch (payload.type) {
		case ImGuiManager::DroppedAssetPayload::Type::Model:
			createdObject = CreateEditorObject(EditorCreateType::LoadedModel, payload.path);
			break;
		case ImGuiManager::DroppedAssetPayload::Type::AnimatedModel:
			createdObject = CreateEditorObject(EditorCreateType::AnimatedModel, payload.path);
			break;
		case ImGuiManager::DroppedAssetPayload::Type::SpriteTexture:
			createdObject = CreateEditorObject(EditorCreateType::Sprite, payload.path);
			break;
		case ImGuiManager::DroppedAssetPayload::Type::None:
		default:
			break;
		}

		if (createdObject) {
			createdObject->Update();
		}
	}
#endif
}

/// <summary>
/// 使用するカメラを選択するUIを描画します。
/// </summary>
void BaseScene::DrawEditorCameraSelector() {
#ifdef USE_IMGUI
	if (!ImGui::CollapsingHeader("Main Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
		return;
	}

	std::vector<std::string> cameraNames;
	cameraNames.push_back("Fallback Camera");
	int currentCameraIndex = 0;
	for (const auto& object : sceneObjects_) {
		CameraComponent* cameraComponent = object ? object->GetComponent<CameraComponent>() : nullptr;
		if (!cameraComponent || !cameraComponent->IsEnabled()) {
			continue;
		}
		cameraNames.push_back(object->GetName());
		if (!activeCameraObjectName_.empty() && object->GetName() == activeCameraObjectName_) {
			currentCameraIndex = static_cast<int>(cameraNames.size()) - 1;
		}
	}

	std::vector<const char*> cameraLabels = MakeLabelPointers(cameraNames);
	if (ImGui::Combo("Active Camera", &currentCameraIndex, cameraLabels.data(), static_cast<int>(cameraLabels.size()))) {
		if (currentCameraIndex == 0) {
			activeCameraObjectName_.clear();
			ApplyCamera(fallbackCamera_);
		} else {
			GameObject* cameraObject = FindObjectByName(cameraNames[currentCameraIndex]);
			SetActiveCameraObject(cameraObject);
			selectedObjectIndex_ = -1;
			for (int index = 0; index < static_cast<int>(sceneObjects_.size()); ++index) {
				if (sceneObjects_[index].get() == cameraObject) {
					selectedObjectIndex_ = index;
					selectedInspectorComponentIndex_ = 0;
					break;
				}
			}
		}
	}

	if (activeCameraObjectName_.empty()) {
		ImGui::Text("Current: Fallback Camera");
	} else {
		ImGui::Text("Current: %s", activeCameraObjectName_.c_str());
	}
#endif
}

void BaseScene::DrawEditorInspector() {
#ifdef USE_IMGUI
	ImGui::Begin("Component Inspector");

	if (selectedObjectIndex_ < 0 || selectedObjectIndex_ >= static_cast<int>(sceneObjects_.size())) {
		ImGui::Text("No object selected");
		ImGui::End();
		return;
	}

	GameObject* selectedObject = sceneObjects_[selectedObjectIndex_].get();
	ImGui::Text("%s", selectedObject->GetName().c_str());
	ImGui::Separator();

	EulerTransform& transform = selectedObject->GetTransform();
	ImGui::DragFloat3("Position", &transform.translate.x, 0.1f);
	ImGui::SliderAngle("Rotate X", &transform.rotate.x);
	ImGui::SliderAngle("Rotate Y", &transform.rotate.y);
	ImGui::SliderAngle("Rotate Z", &transform.rotate.z);
	ImGui::DragFloat3("Scale", &transform.scale.x, 0.1f, 0.01f, 1000.0f);
	ImGui::Checkbox("Enable Gizmo", &isGizmoEnabled_);
	const char* gizmoOperationLabels[] = {"Move", "Rotate", "Scale"};
	ImGui::Combo("Gizmo Operation", &gizmoOperationIndex_, gizmoOperationLabels, _countof(gizmoOperationLabels));

	std::vector<std::string> parentLabels;
	parentLabels.push_back("None");
	int currentParentIndex = 0;
	for (const auto& object : sceneObjects_) {
		if (object.get() == selectedObject) {
			continue;
		}
		parentLabels.push_back(object->GetName());
		if (selectedObject->GetParentName() == object->GetName()) {
			currentParentIndex = static_cast<int>(parentLabels.size()) - 1;
		}
	}
	std::vector<const char*> parentLabelPtrs = MakeLabelPointers(parentLabels);
	if (ImGui::Combo("Parent", &currentParentIndex, parentLabelPtrs.data(), static_cast<int>(parentLabelPtrs.size()))) {
		selectedObject->SetParentName(currentParentIndex == 0 ? "" : parentLabels[currentParentIndex]);
	}

	std::vector<std::string> componentLabels;
	componentLabels.push_back("Transform");
	if (selectedObject->GetComponent<Object3dComponent>()) {
		componentLabels.push_back("Object3d");
	}
	if (selectedObject->GetComponent<SpriteComponent>()) {
		componentLabels.push_back("Sprite");
	}
	if (selectedObject->GetComponent<CameraComponent>()) {
		componentLabels.push_back("Camera");
	}
	if (selectedObject->GetComponent<ParticleEmitterComponent>()) {
		componentLabels.push_back("ParticleEmitter");
	}
	if (selectedObject->GetComponent<Player>()) {
		componentLabels.push_back("Player");
	}
	if (selectedObject->GetComponent<EnemySpawnPointComponent>()) {
		componentLabels.push_back("EnemySpawnPoint");
	}
	if (selectedObject->GetComponent<EnemyComponent>()) {
		componentLabels.push_back("Enemy");
	}
	if (selectedObject->GetComponent<OBBColliderComponent>()) {
		componentLabels.push_back("OBBCollider");
	}
	if (selectedObject->GetComponent<SphereColliderComponent>()) {
		componentLabels.push_back("SphereCollider");
	}
	if (selectedInspectorComponentIndex_ >= static_cast<int>(componentLabels.size())) {
		selectedInspectorComponentIndex_ = 0;
	}
	std::vector<const char*> componentLabelPtrs = MakeLabelPointers(componentLabels);
	ImGui::Combo("Edit Component", &selectedInspectorComponentIndex_, componentLabelPtrs.data(), static_cast<int>(componentLabelPtrs.size()));

	const std::string selectedComponentLabel = componentLabels[selectedInspectorComponentIndex_];

	auto drawComponentEnabledCheckbox = [](const char* label, Component* component) {
		if (!component) {
			return;
		}
		bool isComponentEnabled = component->IsEnabled();
		if (ImGui::Checkbox(label, &isComponentEnabled)) {
			component->SetEnabled(isComponentEnabled);
		}
	};
	auto drawComponentGravityControls = [](const char* label, Component* component) {
		if (!component) {
			return;
		}
		if (ImGui::TreeNode(label)) {
			bool isGravityEnabled = component->IsGravityEnabled();
			if (ImGui::Checkbox("Use Gravity", &isGravityEnabled)) {
				component->SetGravityEnabled(isGravityEnabled);
			}
			float gravityStrength = component->GetGravityStrength();
			if (ImGui::DragFloat("Gravity Strength", &gravityStrength, 0.1f, 0.0f, 200.0f)) {
				component->SetGravityStrength(gravityStrength);
			}
			if (ImGui::Button("Reset Gravity Velocity")) {
				component->ResetGravityVelocity();
			}
			ImGui::TreePop();
		}
	};
	ImGui::Separator();
	ImGui::Text("Component Enabled");
	drawComponentEnabledCheckbox("Object3d Enabled", selectedObject->GetComponent<Object3dComponent>());
	drawComponentEnabledCheckbox("Sprite Enabled", selectedObject->GetComponent<SpriteComponent>());
	drawComponentEnabledCheckbox("Camera Enabled", selectedObject->GetComponent<CameraComponent>());
	drawComponentEnabledCheckbox("ParticleEmitter Enabled", selectedObject->GetComponent<ParticleEmitterComponent>());
	drawComponentEnabledCheckbox("Player Enabled", selectedObject->GetComponent<Player>());
	drawComponentEnabledCheckbox("EnemySpawnPoint Enabled", selectedObject->GetComponent<EnemySpawnPointComponent>());
	drawComponentEnabledCheckbox("Enemy Enabled", selectedObject->GetComponent<EnemyComponent>());
	drawComponentEnabledCheckbox("OBBCollider Enabled", selectedObject->GetComponent<OBBColliderComponent>());
	drawComponentEnabledCheckbox("SphereCollider Enabled", selectedObject->GetComponent<SphereColliderComponent>());
	ImGui::Separator();
	ImGui::Text("Component Gravity");
	drawComponentGravityControls("Object3d Gravity", selectedObject->GetComponent<Object3dComponent>());
	drawComponentGravityControls("Sprite Gravity", selectedObject->GetComponent<SpriteComponent>());
	drawComponentGravityControls("Camera Gravity", selectedObject->GetComponent<CameraComponent>());
	drawComponentGravityControls("ParticleEmitter Gravity", selectedObject->GetComponent<ParticleEmitterComponent>());
	drawComponentGravityControls("Player Gravity", selectedObject->GetComponent<Player>());
	drawComponentGravityControls("EnemySpawnPoint Gravity", selectedObject->GetComponent<EnemySpawnPointComponent>());
	drawComponentGravityControls("Enemy Gravity", selectedObject->GetComponent<EnemyComponent>());
	drawComponentGravityControls("OBBCollider Gravity", selectedObject->GetComponent<OBBColliderComponent>());
	drawComponentGravityControls("SphereCollider Gravity", selectedObject->GetComponent<SphereColliderComponent>());

	if (componentLabels.size() == 1) {
		ImGui::Text("No optional components");
	}
	if (Object3dComponent* object3dComponent = selectedObject->GetComponent<Object3dComponent>()) {
		if (selectedComponentLabel == "Object3d" || object3dComponent->HasModel() || object3dComponent->HasSkeleton()) {
			ImGui::Separator();
			const bool object3dHeaderOpen = ImGui::CollapsingHeader("Object3d / Model Settings", ImGuiTreeNodeFlags_DefaultOpen);
			if (object3dHeaderOpen) {
				if (object3dComponent->HasSkeleton()) {
					ImGui::Text("Skeleton: Available");
					bool isDrawSkeleton = object3dComponent->GetDrawSkeleton();
					if (ImGui::Checkbox("Draw Bone Debug", &isDrawSkeleton)) {
						object3dComponent->SetDrawSkeleton(isDrawSkeleton);
					}
				} else {
					ImGui::Text("Skeleton: None");
				}
				if (object3dComponent->HasAnimation()) {
					bool isAnimationPlaying = object3dComponent->GetAnimationPlaying();
					if (ImGui::Checkbox("Play Animation", &isAnimationPlaying)) {
						object3dComponent->SetAnimationPlaying(isAnimationPlaying);
					}
					ImGui::SameLine();
					if (ImGui::Button("Restart Animation")) {
						object3dComponent->RestartAnimation();
					}
					ImGui::SameLine();
					if (ImGui::Button("Initial Pose")) {
						object3dComponent->SetAnimationPlaying(false);
						object3dComponent->ResetAnimationPoseToInitial();
					}
					ImGui::Text(
					    "Animation Time: %.2f / %.2f",
					    object3dComponent->GetAnimationTime(),
					    object3dComponent->GetAnimationDuration()
					);
				}

				const std::vector<std::string> textures = CollectResourceTexturePaths();
				if (!textures.empty()) {
					int textureIndex = 0;
					const std::string currentTexture = object3dComponent->GetModelTextureFilePath();
					for (int index = 0; index < static_cast<int>(textures.size()); index++) {
						if (textures[index] == currentTexture) {
							textureIndex = index;
							break;
						}
					}
					std::vector<const char*> textureLabels = MakeLabelPointers(textures);
					if (ImGui::Combo("Model Texture", &textureIndex, textureLabels.data(), static_cast<int>(textureLabels.size()))) {
						object3dComponent->SetModelTexture(textures[textureIndex]);
					}
					if (currentTexture.empty()) {
						ImGui::Text("Current Model Texture: None");
					} else {
						ImGui::Text("Current Model Texture: %s", currentTexture.c_str());
					}
				} else {
					ImGui::Text("No selectable textures");
				}

				if (selectedComponentLabel == "Object3d") {
					bool isPointLight = object3dComponent->GetIsPointLightSet();
					if (ImGui::Checkbox("Use Point Light", &isPointLight)) {
						object3dComponent->IsPointLightSet(isPointLight);
					}
					Vector4 pointColor = object3dComponent->GetPointLightColor();
					Vector3 pointPosition = object3dComponent->GetPointLightPosition();
					float pointIntensity = object3dComponent->GetPointLightIntensity();
					float pointRadius = object3dComponent->GetPointLightRadius();
					float pointDecay = object3dComponent->GetPointLightDecay();
					bool pointChanged = false;
					pointChanged |= ImGui::ColorEdit4("Point Color", &pointColor.x);
					pointChanged |= ImGui::DragFloat3("Point Position", &pointPosition.x, 0.05f);
					pointChanged |= ImGui::DragFloat("Point Intensity", &pointIntensity, 0.05f, 0.0f, 100.0f);
					pointChanged |= ImGui::DragFloat("Point Radius", &pointRadius, 0.05f, 0.0f, 1000.0f);
					pointChanged |= ImGui::DragFloat("Point Decay", &pointDecay, 0.01f, 0.0f, 10.0f);
					if (pointChanged) {
						object3dComponent->SetPointLight(pointColor, pointPosition, pointIntensity, pointRadius, pointDecay);
					}
				}
			}
		}
	}
	if (SpriteComponent* spriteComponent = selectedObject->GetComponent<SpriteComponent>(); spriteComponent && selectedComponentLabel == "Sprite") {
		ImGui::Separator();
		ImGui::Text("SpriteComponent");
		const std::vector<std::string> textures = CollectResourceTexturePaths();
		if (!textures.empty()) {
			int textureIndex = 0;
			for (int index = 0; index < static_cast<int>(textures.size()); index++) {
				if (textures[index] == spriteComponent->GetTextureFilePath()) {
					textureIndex = index;
					break;
				}
			}
			std::vector<const char*> textureLabels = MakeLabelPointers(textures);
			if (ImGui::Combo("Texture", &textureIndex, textureLabels.data(), static_cast<int>(textureLabels.size()))) {
				spriteComponent->SetTexture(textures[textureIndex]);
			}
		}
		Vector4 color = spriteComponent->GetColor();
		if (ImGui::ColorEdit4("Color", &color.x)) {
			spriteComponent->SetColor(color);
		}
		Vector2 size = spriteComponent->GetSize();
		if (ImGui::DragFloat2("Size", &size.x, 1.0f, 1.0f, 4096.0f)) {
			spriteComponent->SetSize(size);
		}
	}
	if (selectedObject->GetComponent<CameraComponent>() && selectedComponentLabel == "Camera") {
		DrawCameraInspector(selectedObject);
	}
	if (selectedObject->GetComponent<ParticleEmitterComponent>()) {
		DrawParticleEmitterInspector(selectedObject);
	}
	if (Player* player = selectedObject->GetComponent<Player>()) {
		ImGui::Separator();
		ImGui::Text("PlayerComponent");

		Vector3 spawnPoint = player->GetSpawnPoint();
		if (ImGui::DragFloat3("Spawn Point", &spawnPoint.x, 0.1f)) {
			player->SetSpawnPoint(spawnPoint);
		}
		float moveSpeed = player->GetMoveSpeed();
		if (ImGui::DragFloat("Move Speed", &moveSpeed, 0.01f, 0.0f, 100.0f)) {
			player->SetMoveSpeed(moveSpeed);
		}
		if (ImGui::Button("Set Spawn To Current")) {
			player->SetSpawnPoint(selectedObject->GetTransform().translate);
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset To Spawn")) {
			player->ResetToSpawnPoint();
		}
		if (ImGui::Button("Save Spawn Point")) {
			SaveEditorObjects();
		}

		const std::vector<std::string> loadedModels = CollectAllLoadedModelNames();
		if (!loadedModels.empty()) {
			int modelIndex = 0;
			for (int index = 0; index < static_cast<int>(loadedModels.size()); index++) {
				if (loadedModels[index] == player->GetModelFilePath()) {
					modelIndex = index;
					break;
				}
			}

			std::vector<std::string> modelLabels;
			modelLabels.reserve(loadedModels.size());
			for (const std::string& modelName : loadedModels) {
				Model* model = ModelManager::GetInstance()->FindModel(modelName);
				modelLabels.push_back(model && model->GetIsAnimation() ? "[Anim] " + modelName : "[Model] " + modelName);
			}
			std::vector<const char*> loadedModelLabels = MakeLabelPointers(modelLabels);
			if (ImGui::Combo("Player Model", &modelIndex, loadedModelLabels.data(), static_cast<int>(loadedModelLabels.size()))) {
				const std::string& modelFilePath = loadedModels[modelIndex];
				Model* model = ModelManager::GetInstance()->FindModel(modelFilePath);
				const bool isAnimationModel = model && model->GetIsAnimation();
				player->SetModelFilePath(modelFilePath, isAnimationModel);
				if (Object3dComponent* object3dComponent = selectedObject->GetComponent<Object3dComponent>()) {
					object3dComponent->SetModel(modelFilePath);
					object3dComponent->SetDrawSkeleton(isAnimationModel);
				}
			}
			ImGui::Text("Current Player Model: %s", player->GetModelFilePath().empty() ? "None" : player->GetModelFilePath().c_str());
		} else {
			ImGui::Text("No loaded models");
		}
	}
	if (EnemyComponent* enemy = selectedObject->GetComponent<EnemyComponent>()) {
		ImGui::Separator();
		ImGui::Text("EnemyComponent");

		std::vector<std::string> enemyTypes = LoadEnemyTypeNames();
		int currentEnemyTypeIndex = 0;
		for (int index = 0; index < static_cast<int>(enemyTypes.size()); ++index) {
			if (enemyTypes[index] == enemy->GetEnemyTypeName()) {
				currentEnemyTypeIndex = index;
				break;
			}
		}
		if (!enemyTypes.empty()) {
			std::vector<const char*> enemyTypeLabels = MakeLabelPointers(enemyTypes);
			if (ImGui::Combo("Enemy Type", &currentEnemyTypeIndex, enemyTypeLabels.data(), static_cast<int>(enemyTypeLabels.size()))) {
				enemy->SetEnemyTypeName(enemyTypes[currentEnemyTypeIndex]);
				enemy->ApplyStats(LoadEnemyStats(enemyTypes[currentEnemyTypeIndex]));
			}
		}

		EnemyStats stats = enemy->GetStats();
		bool statsChanged = false;
		statsChanged |= ImGui::DragFloat("Health", &stats.health, 0.1f, 0.0f, 10000.0f);
		statsChanged |= ImGui::DragFloat("Attack", &stats.attack, 0.1f, 0.0f, 10000.0f);
		statsChanged |= ImGui::DragFloat("Speed", &stats.speed, 0.001f, 0.0f, 100.0f);
		statsChanged |= ImGui::Checkbox("Shoots", &stats.shoots);
		statsChanged |= ImGui::DragFloat("Shoot Interval", &stats.shootingInterval, 0.01f, 0.0f, 1000.0f);
		statsChanged |= ImGui::DragFloat("Spawns Per Minute", &stats.spawnsPerMinute, 0.1f, 0.0f, 10000.0f);
		if (statsChanged) {
			enemy->ApplyStats(stats);
		}
		float currentHealth = enemy->GetCurrentHealth();
		if (ImGui::DragFloat("Current Health", &currentHealth, 0.1f, 0.0f, stats.health)) {
			enemy->SetCurrentHealth(currentHealth);
		}

		if (enemyTypeNameBuffer_[0] == '\0') {
			const std::string currentTypeName = enemy->GetEnemyTypeName();
			const size_t copyLength = currentTypeName.size() < enemyTypeNameBuffer_.size() - 1 ? currentTypeName.size() : enemyTypeNameBuffer_.size() - 1;
			std::memcpy(enemyTypeNameBuffer_.data(), currentTypeName.data(), copyLength);
			enemyTypeNameBuffer_[copyLength] = '\0';
		}
		ImGui::InputText("Save Type Name", enemyTypeNameBuffer_.data(), enemyTypeNameBuffer_.size());
		if (ImGui::Button("Save Enemy Type")) {
			const std::string saveTypeName = enemyTypeNameBuffer_.data();
			enemy->SetEnemyTypeName(saveTypeName.empty() ? enemy->GetEnemyTypeName() : saveTypeName);
			SaveEnemyStats(enemy->GetEnemyTypeName(), enemy->GetStats());
		}
	}
	if (EnemySpawnPointComponent* enemySpawnPoint = selectedObject->GetComponent<EnemySpawnPointComponent>()) {
		ImGui::Separator();
		ImGui::Text("EnemySpawnPointComponent");

		std::vector<std::string> targetLabels;
		targetLabels.push_back("Auto First Player");
		int currentTargetIndex = 0;
		for (const auto& object : sceneObjects_) {
			if (!object->GetComponent<Player>()) {
				continue;
			}
			targetLabels.push_back(object->GetName());
			if (enemySpawnPoint->GetTargetName() == object->GetName()) {
				currentTargetIndex = static_cast<int>(targetLabels.size()) - 1;
			}
		}
		std::vector<const char*> targetLabelPtrs = MakeLabelPointers(targetLabels);
		if (ImGui::Combo("Target Player", &currentTargetIndex, targetLabelPtrs.data(), static_cast<int>(targetLabelPtrs.size()))) {
			enemySpawnPoint->SetTargetName(currentTargetIndex == 0 ? "" : targetLabels[currentTargetIndex]);
			enemySpawnPoint->SetTarget(nullptr);
		}

		std::vector<std::string> cameraLabels;
		cameraLabels.push_back("Active Camera");
		int currentCameraIndex = 0;
		for (const auto& object : sceneObjects_) {
			if (!object->GetComponent<CameraComponent>()) {
				continue;
			}
			cameraLabels.push_back(object->GetName());
			if (enemySpawnPoint->GetCameraName() == object->GetName()) {
				currentCameraIndex = static_cast<int>(cameraLabels.size()) - 1;
			}
		}
		std::vector<const char*> cameraLabelPtrs = MakeLabelPointers(cameraLabels);
		if (ImGui::Combo("Reference Camera", &currentCameraIndex, cameraLabelPtrs.data(), static_cast<int>(cameraLabelPtrs.size()))) {
			enemySpawnPoint->SetCameraName(currentCameraIndex == 0 ? "" : cameraLabels[currentCameraIndex]);
			enemySpawnPoint->SetCamera(nullptr);
		}

		std::vector<std::string> enemyTypes = LoadEnemyTypeNames();
		int currentEnemyTypeIndex = 0;
		for (int index = 0; index < static_cast<int>(enemyTypes.size()); ++index) {
			if (enemyTypes[index] == enemySpawnPoint->GetEnemyTypeName()) {
				currentEnemyTypeIndex = index;
				break;
			}
		}
		if (!enemyTypes.empty()) {
			std::vector<const char*> enemyTypeLabels = MakeLabelPointers(enemyTypes);
			if (ImGui::Combo("Spawn Enemy Type", &currentEnemyTypeIndex, enemyTypeLabels.data(), static_cast<int>(enemyTypeLabels.size()))) {
				enemySpawnPoint->SetEnemyTypeName(enemyTypes[currentEnemyTypeIndex]);
				enemySpawnPoint->ResetSpawnTimer();
			}
		}
		bool spawnEnabled = enemySpawnPoint->GetSpawnEnabled();
		if (ImGui::Checkbox("Spawn Enabled", &spawnEnabled)) {
			enemySpawnPoint->SetSpawnEnabled(spawnEnabled);
			enemySpawnPoint->ResetSpawnTimer();
		}

		int spawnCount = enemySpawnPoint->GetSpawnCount();
		if (ImGui::DragInt("Spawn Count", &spawnCount, 1.0f, 1, 64)) {
			enemySpawnPoint->SetSpawnCount(spawnCount);
		}
		float outerMargin = enemySpawnPoint->GetOuterMargin();
		if (ImGui::DragFloat("Outside Margin", &outerMargin, 0.1f, 0.0f, 1000.0f)) {
			enemySpawnPoint->SetOuterMargin(outerMargin);
		}
		float minimumRadius = enemySpawnPoint->GetMinimumRadius();
		if (ImGui::DragFloat("Minimum Radius", &minimumRadius, 0.1f, 0.0f, 1000.0f)) {
			enemySpawnPoint->SetMinimumRadius(minimumRadius);
		}
		float groundY = enemySpawnPoint->GetGroundY();
		if (ImGui::DragFloat("Ground Y", &groundY, 0.1f, -1000.0f, 1000.0f)) {
			enemySpawnPoint->SetGroundY(groundY);
		}
		float pointHeight = enemySpawnPoint->GetPointHeight();
		if (ImGui::DragFloat("Point Height", &pointHeight, 0.05f, -1000.0f, 1000.0f)) {
			enemySpawnPoint->SetPointHeight(pointHeight);
		}
		bool drawDebug = enemySpawnPoint->GetDrawDebug();
		if (ImGui::Checkbox("Draw Spawn Debug", &drawDebug)) {
			enemySpawnPoint->SetDrawDebug(drawDebug);
		}
		float debugPointSize = enemySpawnPoint->GetDebugPointSize();
		if (ImGui::DragFloat("Debug Point Size", &debugPointSize, 0.05f, 0.01f, 100.0f)) {
			enemySpawnPoint->SetDebugPointSize(debugPointSize);
		}
		ImGui::Text("Spawn Points: %d", static_cast<int>(enemySpawnPoint->GetSpawnPoints().size()));
	}
	DrawOBBColliderInspector(selectedObject);

	ImGui::End();
#endif
}

/// <summary>
/// パーティクルエミッターコンポーネントの編集UIを描画します。
/// </summary>
void BaseScene::DrawParticleEmitterInspector(GameObject* selectedObject) {
#ifdef USE_IMGUI
	if (!selectedObject) {
		return;
	}

	ParticleEmitterComponent* emitter = selectedObject->GetComponent<ParticleEmitterComponent>();
	if (!emitter) {
		return;
	}

	ImGui::Separator();
	ImGui::Text("ParticleEmitterComponent");

	const std::vector<std::string> particlePresets = LoadParticlePresetNames();
	if (!particlePresets.empty()) {
		if (selectedParticlePresetIndex_ >= static_cast<int>(particlePresets.size())) {
			selectedParticlePresetIndex_ = 0;
		}

		std::vector<const char*> presetLabels;
		presetLabels.reserve(particlePresets.size());
		for (const std::string& presetName : particlePresets) {
			presetLabels.push_back(presetName.c_str());
		}

		ImGui::Combo("Preset", &selectedParticlePresetIndex_, presetLabels.data(), static_cast<int>(presetLabels.size()));
		if (ImGui::Button("Apply Preset")) {
			ApplyParticlePreset(particlePresets[selectedParticlePresetIndex_], emitter);
		}
	} else {
		ImGui::Text("No particle presets");
	}

	if (particlePresetNameBuffer_[0] == '\0') {
		const std::string currentGroupName = emitter->GetGroupName();
		const size_t copyLength = currentGroupName.size() < particlePresetNameBuffer_.size() - 1 ? currentGroupName.size() : particlePresetNameBuffer_.size() - 1;
		std::memcpy(particlePresetNameBuffer_.data(), currentGroupName.data(), copyLength);
		particlePresetNameBuffer_[copyLength] = '\0';
	}
	ImGui::InputText("Preset Name", particlePresetNameBuffer_.data(), particlePresetNameBuffer_.size());
	if (ImGui::Button("Save Preset")) {
		SaveParticlePreset(particlePresetNameBuffer_.data(), emitter);
	}

	const std::vector<std::string> textures = CollectResourceTexturePaths();
	if (!textures.empty()) {
		int textureIndex = 0;
		for (int index = 0; index < static_cast<int>(textures.size()); index++) {
			if (textures[index] == emitter->GetTextureFilePath()) {
				textureIndex = index;
				break;
			}
		}
		std::vector<const char*> textureLabels = MakeLabelPointers(textures);
		if (ImGui::Combo("Particle Texture", &textureIndex, textureLabels.data(), static_cast<int>(textureLabels.size()))) {
			emitter->SetTexture(textures[textureIndex]);
		}
	}

	bool isActive = emitter->GetIsActive();
	if (ImGui::Checkbox("Emitter Active", &isActive)) {
		emitter->SetIsActive(isActive);
	}

	float frequency = emitter->GetFrequency();
	if (ImGui::DragFloat("Emit Frequency", &frequency, 0.01f, 0.0f, 10.0f)) {
		emitter->SetFrequency(frequency);
	}

	ParticleEmitParam param = emitter->GetPalam();
	int count = static_cast<int>(param.count);
	if (ImGui::DragInt("Emit Count", &count, 1.0f, 0, 1000)) {
		param.count = static_cast<uint32_t>(count);
		emitter->SetParam(param);
	}
	if (ImGui::DragFloat("Life Time", &param.lifeTime, 0.01f, 0.01f, 30.0f)) {
		emitter->SetParam(param);
	}
	if (ImGui::DragFloat3("Particle Scale", &param.scale.x, 0.05f, 0.0f, 100.0f)) {
		emitter->SetParam(param);
	}
	if (ImGui::DragFloat3("End Scale", &param.endScale.x, 0.05f, 0.0f, 100.0f)) {
		emitter->SetParam(param);
	}
	if (ImGui::DragFloat3("Random Position", &param.randomPositionRange.x, 0.05f, 0.0f, 100.0f)) {
		emitter->SetParam(param);
	}
	if (ImGui::DragFloat3("Base Velocity", &param.baseVelocity.x, 0.05f, -100.0f, 100.0f)) {
		emitter->SetParam(param);
	}
	if (ImGui::DragFloat3("Random Velocity", &param.randomVelocityRange.x, 0.05f, 0.0f, 100.0f)) {
		emitter->SetParam(param);
	}
	if (ImGui::DragFloat3("Acceleration", &param.acceleration.x, 0.05f, -100.0f, 100.0f)) {
		emitter->SetParam(param);
	}
	if (ImGui::DragFloat3("Base Rotate", &param.baseRotate.x, 0.05f, -10.0f, 10.0f)) {
		emitter->SetParam(param);
	}
	if (ImGui::Checkbox("Random Rotate", &param.isRandomRotate)) {
		emitter->SetParam(param);
	}
	if (ImGui::DragFloat3("Random Rotate Range", &param.randomRotateRange.x, 0.05f, 0.0f, 10.0f)) {
		emitter->SetParam(param);
	}
	if (ImGui::DragFloat3("Random Scale", &param.randomScaleRange.x, 0.05f, 0.0f, 100.0f)) {
		emitter->SetParam(param);
	}
	if (ImGui::Checkbox("Billboard", &param.isBillboard)) {
		emitter->SetParam(param);
	}
	if (ImGui::ColorEdit4("Start Color", &param.color.x)) {
		emitter->SetParam(param);
	}
	if (ImGui::ColorEdit4("End Color", &param.endColor.x)) {
		emitter->SetParam(param);
	}
	emitter->SetParam(param);

	int meshType = static_cast<int>(emitter->GetMeshType());
	const char* meshTypeLabels[] = {"Quad", "Ring", "Cylinder"};
	if (ImGui::Combo("Mesh Type", &meshType, meshTypeLabels, _countof(meshTypeLabels))) {
		emitter->SetMeshType(static_cast<ParticleMeshType>(meshType));
	}

	int blendMode = static_cast<int>(emitter->GetBlendMode());
	const char* blendModeLabels[] = {"None", "Normal", "Add", "Subtract", "Multiply", "Screen"};
	if (ImGui::Combo("Blend Mode", &blendMode, blendModeLabels, _countof(blendModeLabels))) {
		emitter->SetBlendMode(static_cast<BlendMode>(blendMode));
	}

	if (ImGui::Button("Test Emit")) {
		emitter->Emit();
	}
#else
	(void)selectedObject;
#endif
}

/// <summary>
/// コライダーコンポーネントの編集UIを描画します。
/// </summary>
void BaseScene::DrawOBBColliderInspector(GameObject* selectedObject) {
#ifdef USE_IMGUI
	if (!selectedObject) {
		return;
	}

	OBBColliderComponent* collider = selectedObject->GetComponent<OBBColliderComponent>();
	SphereColliderComponent* sphereCollider = selectedObject->GetComponent<SphereColliderComponent>();
	ImGui::Separator();
	if (!collider) {
		if (ImGui::Button("Create OBB Collider")) {
			selectedObject->AddComponent<OBBColliderComponent>();
		}
	} else {
		ImGui::Text("OBBColliderComponent");
		if (ImGui::Button("Destroy OBB Collider")) {
			selectedObject->RemoveComponent<OBBColliderComponent>();
			collider = nullptr;
		}
	}

	if (!sphereCollider) {
		if (ImGui::Button("Create Sphere Collider")) {
			selectedObject->AddComponent<SphereColliderComponent>();
		}
	} else {
		ImGui::Text("SphereColliderComponent");
		if (ImGui::Button("Destroy Sphere Collider")) {
			selectedObject->RemoveComponent<SphereColliderComponent>();
			sphereCollider = nullptr;
		}
	}

	if (!collider && !sphereCollider) {
		return;
	}

	if (collider) {
		ImGui::Separator();
		ImGui::Text("OBB Settings");
		Vector3 centerOffset = collider->GetCenterOffset();
		if (ImGui::DragFloat3("OBB Center", &centerOffset.x, 0.05f)) {
			collider->SetCenterOffset(centerOffset);
		}

		Vector3 halfSize = collider->GetHalfSize();
		if (ImGui::DragFloat3("OBB Half Size", &halfSize.x, 0.05f, 0.0f, 1000.0f)) {
			collider->SetHalfSize(halfSize);
		}
		Vector3 fullSize = {halfSize.x * 2.0f, halfSize.y * 2.0f, halfSize.z * 2.0f};
		if (ImGui::DragFloat3("OBB Size", &fullSize.x, 0.1f, 0.0f, 2000.0f)) {
			collider->SetHalfSize({fullSize.x * 0.5f, fullSize.y * 0.5f, fullSize.z * 0.5f});
		}

		bool isDrawDebug = collider->GetDrawDebug();
		if (ImGui::Checkbox("Draw OBB Collider", &isDrawDebug)) {
			collider->SetDrawDebug(isDrawDebug);
		}
		bool isPushBackEnabled = collider->GetPushBackEnabled();
		if (ImGui::Checkbox("OBB Push Back", &isPushBackEnabled)) {
			collider->SetPushBackEnabled(isPushBackEnabled);
		}
		ImGui::Text("OBB Collision: %s", collider->IsColliding() ? "Hit" : "None");
	}

	if (sphereCollider) {
		ImGui::Separator();
		ImGui::Text("Sphere Settings");
		Vector3 centerOffset = sphereCollider->GetCenterOffset();
		if (ImGui::DragFloat3("Sphere Center", &centerOffset.x, 0.05f)) {
			sphereCollider->SetCenterOffset(centerOffset);
		}

		float radius = sphereCollider->GetRadius();
		if (ImGui::DragFloat("Sphere Radius", &radius, 0.05f, 0.0f, 1000.0f)) {
			sphereCollider->SetRadius(radius);
		}

		bool isDrawDebug = sphereCollider->GetDrawDebug();
		if (ImGui::Checkbox("Draw Sphere Collider", &isDrawDebug)) {
			sphereCollider->SetDrawDebug(isDrawDebug);
		}
		bool isPushBackEnabled = sphereCollider->GetPushBackEnabled();
		if (ImGui::Checkbox("Sphere Push Back", &isPushBackEnabled)) {
			sphereCollider->SetPushBackEnabled(isPushBackEnabled);
		}
		ImGui::Text("Sphere Collision: %s", sphereCollider->IsColliding() ? "Hit" : "None");
	}
#else
	(void)selectedObject;
#endif
}

/// <summary>
/// カメラコンポーネントの編集UIを描画します。
/// </summary>
void BaseScene::DrawCameraInspector(GameObject* selectedObject) {
#ifdef USE_IMGUI
	CameraComponent* cameraComponent = selectedObject->GetComponent<CameraComponent>();
	if (!cameraComponent) {
		return;
	}

	ImGui::Separator();
	ImGui::Text("CameraComponent");

	const bool isActive = selectedObject->GetName() == activeCameraObjectName_;
	if (isActive) {
		ImGui::Text("Active Camera");
	} else if (ImGui::Button("Set Active Camera")) {
		SetActiveCameraObject(selectedObject);
	}

	float fovY = cameraComponent->GetFovY();
	if (ImGui::SliderAngle("FOV", &fovY, 1.0f, 120.0f)) {
		cameraComponent->SetFovY(fovY);
	}
	float nearClip = cameraComponent->GetNearClip();
	if (ImGui::DragFloat("Near Clip", &nearClip, 0.01f, 0.001f, 100.0f)) {
		cameraComponent->SetNearClip(nearClip);
	}
	float farClip = cameraComponent->GetFarClip();
	if (ImGui::DragFloat("Far Clip", &farClip, 1.0f, 1.0f, 10000.0f)) {
		cameraComponent->SetFarClip(farClip);
	}
	Vector3 localOffset = cameraComponent->GetLocalOffset();
	if (ImGui::DragFloat3("Camera Local Offset", &localOffset.x, 0.1f)) {
		cameraComponent->SetLocalOffset(localOffset);
	}
	Vector3 followOffset = cameraComponent->GetFollowOffset();
	if (ImGui::DragFloat3("Follow Offset", &followOffset.x, 0.1f)) {
		cameraComponent->SetFollowOffset(followOffset);
	}
	bool overrideRotationEnabled = cameraComponent->GetOverrideRotationEnabled();
	if (ImGui::Checkbox("Use Fixed Camera Rotation", &overrideRotationEnabled)) {
		cameraComponent->SetOverrideRotationEnabled(overrideRotationEnabled);
	}
	Vector3 overrideRotation = cameraComponent->GetOverrideRotation();
	bool overrideRotationChanged = false;
	overrideRotationChanged |= ImGui::SliderAngle("Fixed Rotate X", &overrideRotation.x, -180.0f, 180.0f);
	overrideRotationChanged |= ImGui::SliderAngle("Fixed Rotate Y", &overrideRotation.y, -180.0f, 180.0f);
	overrideRotationChanged |= ImGui::SliderAngle("Fixed Rotate Z", &overrideRotation.z, -180.0f, 180.0f);
	if (overrideRotationChanged) {
		cameraComponent->SetOverrideRotation(overrideRotation);
	}
	if (ImGui::Button("Set Top Down View")) {
		cameraComponent->SetLocalOffset({0.0f, 15.0f, 0.0f});
		cameraComponent->SetFollowOffset({0.0f, 15.0f, 0.0f});
		cameraComponent->SetOverrideRotationEnabled(true);
		cameraComponent->SetOverrideRotation({kPi * 0.5f, 0.0f, 0.0f});
		cameraComponent->SetFovY(0.75f);
		cameraComponent->SetFarClip(1000.0f);
	}

	std::vector<std::string> linkLabels;
	linkLabels.push_back("None");
	int currentLinkIndex = 0;
	for (const auto& object : sceneObjects_) {
		if (object.get() == selectedObject) {
			continue;
		}
		linkLabels.push_back(object->GetName());
		if (cameraComponent->GetFollowTarget() == object.get() || cameraComponent->GetFollowTargetName() == object->GetName()) {
			currentLinkIndex = static_cast<int>(linkLabels.size()) - 1;
		}
	}

	std::vector<const char*> linkLabelPtrs;
	linkLabelPtrs.reserve(linkLabels.size());
	for (const std::string& label : linkLabels) {
		linkLabelPtrs.push_back(label.c_str());
	}

	if (ImGui::Combo("Follow Object", &currentLinkIndex, linkLabelPtrs.data(), static_cast<int>(linkLabelPtrs.size()))) {
		if (currentLinkIndex == 0) {
			cameraComponent->SetFollowTarget(nullptr);
		} else {
			cameraComponent->SetFollowTarget(FindObjectByName(linkLabels[currentLinkIndex]));
		}
	}
#endif
}

/// <summary>
/// 選択中オブジェクトを操作するギズモを描画します。
/// </summary>
void BaseScene::DrawEditorGizmo() {
#ifdef USE_IMGUI
	if (!isGizmoEnabled_ || selectedObjectIndex_ < 0 || selectedObjectIndex_ >= static_cast<int>(sceneObjects_.size())) {
		return;
	}

	Camera* camera = Object3dCommon::GetInstance()->GetDefaultCamera();
	if (!camera) {
		return;
	}

	GameObject* selectedObject = sceneObjects_[selectedObjectIndex_].get();
	EulerTransform& transform = selectedObject->GetTransform();

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	const ImVec2 viewportPos = viewport ? viewport->Pos : ImVec2(0.0f, 0.0f);
	const ImVec2 viewportSize = viewport ? viewport->Size : ImGui::GetIO().DisplaySize;

	Matrix4x4 objectMatrix{};
	float translation[3] = {transform.translate.x, transform.translate.y, transform.translate.z};
	float rotation[3] = {ToDegrees(transform.rotate.x), ToDegrees(transform.rotate.y), ToDegrees(transform.rotate.z)};
	float scale[3] = {transform.scale.x, transform.scale.y, transform.scale.z};
	ImGuizmo::RecomposeMatrixFromComponents(translation, rotation, scale, &objectMatrix.m[0][0]);

	ImGuizmo::BeginFrame();
	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList(viewport));
	ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);

	const Matrix4x4& viewMatrix = camera->GetViewMatrix();
	const Matrix4x4& projectionMatrix = camera->GetProjectionMatrix();
	const ImGuizmo::OPERATION gizmoOperations[] = {
	    ImGuizmo::TRANSLATE,
	    ImGuizmo::ROTATE,
	    ImGuizmo::SCALE
	};
	const int operationIndex = gizmoOperationIndex_ >= 0 && gizmoOperationIndex_ < _countof(gizmoOperations) ? gizmoOperationIndex_ : 0;
	const ImGuizmo::MODE gizmoMode = gizmoOperations[operationIndex] == ImGuizmo::SCALE ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

	if (ImGuizmo::Manipulate(&viewMatrix.m[0][0], &projectionMatrix.m[0][0], gizmoOperations[operationIndex], gizmoMode, &objectMatrix.m[0][0])) {
		ImGuizmo::DecomposeMatrixToComponents(&objectMatrix.m[0][0], translation, rotation, scale);
		transform.translate = {translation[0], translation[1], translation[2]};
		transform.rotate = {ToRadians(rotation[0]), ToRadians(rotation[1]), ToRadians(rotation[2])};
		transform.scale = {scale[0], scale[1], scale[2]};
	}
#endif
}

/// <summary>
/// エディタ用スカイボックスを生成または再読み込みします。
/// </summary>
void BaseScene::CreateOrReloadEditorSkyBox(const std::string& textureFilePath) {
	if (textureFilePath.empty()) {
		editorSkyBox_.reset();
		skyBoxTextureFilePath_.clear();
		return;
	}

	TextureManager::GetInstance()->LoadTexture(textureFilePath);
	auto skyBox = std::make_unique<SkyBox>();
	skyBox->Initialize(textureFilePath);
	editorSkyBox_ = std::move(skyBox);
	skyBoxTextureFilePath_ = textureFilePath;
}

void BaseScene::DrawEditorSkyBoxControls() {
#ifdef USE_IMGUI
	if (!ImGui::CollapsingHeader("SkyBox", ImGuiTreeNodeFlags_DefaultOpen)) {
		return;
	}

	ImGui::Checkbox("Enable SkyBox", &isEditorSkyBoxEnabled_);

	const std::vector<std::string> ddsTextures = CollectResourceDdsTexturePaths();
	if (ddsTextures.empty()) {
		ImGui::Text("No DDS textures in Resources");
		if (ImGui::Button("Clear SkyBox")) {
			CreateOrReloadEditorSkyBox("");
			isEditorSkyBoxEnabled_ = false;
		}
		return;
	}

	if (!skyBoxTextureFilePath_.empty()) {
		for (int index = 0; index < static_cast<int>(ddsTextures.size()); ++index) {
			if (ddsTextures[index] == skyBoxTextureFilePath_) {
				selectedSkyBoxTextureIndex_ = index;
				break;
			}
		}
	}
	if (selectedSkyBoxTextureIndex_ >= static_cast<int>(ddsTextures.size())) {
		selectedSkyBoxTextureIndex_ = 0;
	}

	std::vector<const char*> textureLabels = MakeLabelPointers(ddsTextures);
	ImGui::Combo("DDS Texture", &selectedSkyBoxTextureIndex_, textureLabels.data(), static_cast<int>(textureLabels.size()));

	if (ImGui::Button("Create SkyBox")) {
		CreateOrReloadEditorSkyBox(ddsTextures[selectedSkyBoxTextureIndex_]);
		isEditorSkyBoxEnabled_ = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Remove SkyBox")) {
		CreateOrReloadEditorSkyBox("");
		isEditorSkyBoxEnabled_ = false;
	}

	if (!skyBoxTextureFilePath_.empty()) {
		ImGui::Text("Current: %s", skyBoxTextureFilePath_.c_str());
	} else {
		ImGui::Text("Current: None");
	}
#endif
}

std::string BaseScene::MakeUniqueObjectName(const std::string& baseName) const {
	return baseName + "_" + std::to_string(nextObjectId_);
}

