#include "GamePlayScene.h"
#include "SceneManager.h"
#include <cstring>
#include <ModelManager.h>
#include"InstancingModelCommon.h"

namespace {
ImVec2 GetPrimaryWorkPos() {
	const ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
	if (platformIO.Monitors.Size > 0) {
		return platformIO.Monitors[0].WorkPos;
	}
	return ImVec2(0.0f, 0.0f);
}

ImVec2 GetPrimaryWorkSize() {
	const ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
	if (platformIO.Monitors.Size > 0) {
		return platformIO.Monitors[0].WorkSize;
	}
	return ImVec2(1280.0f, 720.0f);
}

float ClampFloat(float value, float minValue, float maxValue) {
	if (maxValue < minValue) {
		return minValue;
	}
	if (value < minValue) {
		return minValue;
	}
	if (value > maxValue) {
		return maxValue;
	}
	return value;
}

ImVec2 ClampWindowPosToWorkArea(const ImVec2& pos, const ImVec2& size) {
	const ImVec2 workPos = GetPrimaryWorkPos();
	const ImVec2 workSize = GetPrimaryWorkSize();
	const float maxX = workPos.x + workSize.x - size.x;
	const float maxY = workPos.y + workSize.y - size.y;
	return ImVec2(
	    ClampFloat(pos.x, workPos.x, maxX),
	    ClampFloat(pos.y, workPos.y, maxY)
	);
}

struct EditorLayout {
	ImVec2 hierarchyPos;
	ImVec2 hierarchySize;
	ImVec2 inspectorPos;
	ImVec2 inspectorSize;
	ImVec2 particlePos;
	ImVec2 particleSize;
};

EditorLayout MakeEditorLayout() {
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	const float leftWidth = 270.0f;
	const float rightWidth = 360.0f;
	const float bottomHeight = 240.0f;
	const float width = displaySize.x > 0.0f ? displaySize.x : 1280.0f;
	const float height = displaySize.y > 0.0f ? displaySize.y : 720.0f;

	EditorLayout layout{};
	layout.hierarchyPos = ImVec2(0.0f, 0.0f);
	layout.hierarchySize = ImVec2(leftWidth, height - bottomHeight);
	layout.inspectorPos = ImVec2(width - rightWidth, 0.0f);
	layout.inspectorSize = ImVec2(rightWidth, height);
	layout.particlePos = ImVec2(leftWidth, height - bottomHeight);
	layout.particleSize = ImVec2(width - leftWidth - rightWidth, bottomHeight);
	return layout;
}

bool BeginEditorPanel(const char* name, const ImVec2& pos, const ImVec2& size) {
#ifdef IMGUI_HAS_DOCK
	(void)pos;
	(void)size;
	return ImGui::Begin(name);
#else
	const ImGuiWindowFlags flags =
	    ImGuiWindowFlags_NoMove |
	    ImGuiWindowFlags_NoCollapse |
	    ImGuiWindowFlags_NoSavedSettings;

	ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(size, ImGuiCond_Always);
	return ImGui::Begin(name, nullptr, flags);
#endif
}
}

void GamePlayScene::Initialize() {
	LoadSceneModels();

	std::vector<std::string> allTextures = {
			"Resources/circle.png",
			"Resources/uvChecker.png",
			"Resources/monsterball.png",
			"Resources/rostock_laage_airport_4k.dds",
			"Resources/gradationLine.png"
	};

	availableTextures_.clear();

	for (const auto& path : allTextures) {
		TextureManager::GetInstance()->LoadTexture(path);

		if (path.find(".png") != std::string::npos) {
			availableTextures_.push_back(path);
		}
	}
	spriteObject_ = std::make_unique<GameObject>();
	sprite = spriteObject_->AddComponent<SpriteComponent>();
	sprite->Initialize("Resources/uvChecker.png");
	audio_ = std::make_unique<Audio>();
	audio_->Initialize();

#pragma endregion
#pragma region Setup
	skyBox = std::make_unique<SkyBox>();
	skyBox->Initialize("Resources/rostock_laage_airport_4k.dds");
	for (int i = 0; i < 5; i++) {
		auto spriteObject = std::make_unique<GameObject>();
		SpriteComponent* sprits = spriteObject->AddComponent<SpriteComponent>();
		if (i == 1 || i == 3) {
			sprits->Initialize("Resources/uvChecker.png");
		}
		else {
			sprits->Initialize("Resources/monsterball.png");
		}
		sprites.push_back(sprits);
		spriteObjects_.push_back(std::move(spriteObject));
		EulerTransform transform;
		transform.scale = { 50.0f, 50.0f, 1.0f };
		transform.translate = { 100.0f + i * 90.0f, 200.0f, 0.0f };
		// sprits->SetTransform(transform);
	}

	object3dObject_ = std::make_unique<GameObject>();
	object3d = object3dObject_->AddComponent<Object3dComponent>();
	object3d->SetModel("simpleSkin.gltf");

	sphereGameObject_ = std::make_unique<GameObject>();
	sphereObject = sphereGameObject_->AddComponent<Object3dComponent>();
	sphereObject->SetModel("sphere.obj");
	sphereObject->SetTranslate({ 0.0f, 10.0f, 3.0f });
	sphereObject->SetScale({ 1.0f, 1.0f, 1.0f });
	sphereObject->SetRotate({ 0.0f, 0.0f, 0.0f });
	cylinderGameObject_ = std::make_unique<GameObject>();
	cylinderObject = cylinderGameObject_->AddComponent<Object3dComponent>();

	cylinderObject->CreateCylinder(1.0f, 2.0f, 16, true, true);
	cylinderObject->SetTexture("Resources/gradationLine.png");
	// =================================================
	// =================================================
	std::vector<std::string> particleTypes = { "Fire", "Dust", "Slash","Slash_Trace","Lightning" };

	instancingModel_ = std::make_unique<InstancingModel>();
	instancingModel_->Initialize(ModelManager::GetInstance()->FindModel("sphere.obj"), 10);
	instancingModel_->SetEnvironmentMapPath("Resources/rostock_laage_airport_4k.dds");
	for (const auto& groupName : particleTypes) {
		auto emitterObject = std::make_unique<GameObject>();
		ParticleEmitterComponent* newEmitter = emitterObject->AddComponent<ParticleEmitterComponent>();

		newEmitter->SetGroupName(groupName);

		newEmitter->LoadFromJson("Resources/Data/emit_status.json");
		std::string texPath = newEmitter->GetTextureFilePath();
		//if (groupName == "Lightning") {
		//	if (texPath == "") {
		//		texPath = "Resources/circle.png";
		//	}

		//	ParticleEmitParam lightningParam = newEmitter->GetPalam();
		//	lightningParam.scale = {0.28f, 0.28f, 0.28f};
		//	lightningParam.endScale = {0.0f, 0.0f, 0.0f};
		//	lightningParam.color = {0.55f, 0.78f, 1.0f, 1.0f};
		//	lightningParam.endColor = {0.2f, 0.45f, 1.0f, 0.0f};
		//	lightningParam.lifeTime = 0.18f;
		//	lightningParam.randomPositionRange = {0.08f, 0.08f, 0.08f};
		//	lightningParam.randomScaleRange = {0.18f, 0.18f, 0.0f};
		//	lightningParam.count = 1;
		//	lightningParam.isBillboard = true;
		//	newEmitter->SetParam(lightningParam);
		//	newEmitter->SetBlendMode(kBlendModeAdd);
		//} else if (groupName == "Fire") {
		//	texPath = "Resources/circle.png";

		//	ParticleEmitParam fireParam = newEmitter->GetPalam();
		//	fireParam.scale = {0.7f, 0.7f, 0.7f};
		//	fireParam.endScale = {0.12f, 0.12f, 0.12f};
		//	fireParam.baseVelocity = {0.0f, 0.08f, 0.0f};
		//	fireParam.randomVelocityRange = {0.055f, 0.07f, 0.055f};
		//	fireParam.acceleration = {0.0f, 0.004f, 0.0f};
		//	fireParam.randomPositionRange = {0.28f, 0.04f, 0.28f};
		//	fireParam.lifeTime = 0.55f;
		//	fireParam.color = {1.0f, 0.55f, 0.08f, 0.95f};
		//	fireParam.endColor = {0.45f, 0.02f, 0.0f, 0.0f};
		//	fireParam.randomScaleRange = {0.28f, 0.28f, 0.0f};
		//	fireParam.count = 5;
		//	fireParam.isBillboard = true;
		//	newEmitter->SetParam(fireParam);
		//	newEmitter->SetBlendMode(kBlendModeAdd);
		//	newEmitter->SetFrequency(0.035f);
		//} else if (groupName == "Slash") {
		//	texPath = "Resources/circle.png";

		//	ParticleEmitParam slashParam = newEmitter->GetPalam();
		//	slashParam.scale = {0.28f, 0.28f, 0.28f};
		//	slashParam.endScale = {0.02f, 0.02f, 0.02f};
		//	slashParam.baseVelocity = {0.0f, 0.0f, 0.0f};
		//	slashParam.randomVelocityRange = {0.28f, 0.2f, 0.28f};
		//	slashParam.acceleration = {0.0f, -0.006f, 0.0f};
		//	slashParam.randomPositionRange = {0.16f, 0.16f, 0.16f};
		//	slashParam.lifeTime = 0.22f;
		//	slashParam.color = {1.0f, 0.9f, 0.45f, 1.0f};
		//	slashParam.endColor = {1.0f, 0.1f, 0.02f, 0.0f};
		//	slashParam.randomScaleRange = {0.18f, 0.18f, 0.0f};
		//	slashParam.count = 34;
		//	slashParam.isBillboard = true;
		//	newEmitter->SetParam(slashParam);
		//	newEmitter->SetBlendMode(kBlendModeAdd);
		//	newEmitter->SetFrequency(0.0f);
		//} else if (texPath == "") {
		//	texPath = "Resources/uvChecker.png";
		//}

		ParticleManager::GetInstance()->CreateParticleGroup(groupName, texPath, newEmitter->GetMeshType());
		newEmitter->SetTexture(texPath);
		// newEmitter->SetTranslate({0.0f, 0.0f, 0.0f});
		ParticleManager::GetInstance()->SetGroupBlendMode(newEmitter->GetGroupName(), newEmitter->GetBlendMode());
		emitters_.push_back(newEmitter);
		emitterObjects_.push_back(std::move(emitterObject));
	}

#pragma endregion

	SoundData fanfare = {};
	audio_->LoadWave(L"Resources/fanfare.wav", fanfare);
	//audio_->Play(fanfare, 0);
}

void GamePlayScene::LoadSceneModels() {
	ModelManager::GetInstance()->LoadModel("AnimatedCube.gltf", true, "/Cube");
	ModelManager::GetInstance()->LoadModel("simpleSkin.gltf", true, "/simpleSkin");
	ModelManager::GetInstance()->LoadModel("axis.obj");
	ModelManager::GetInstance()->LoadModel("sphere.obj");
}

void GamePlayScene::Update() {
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		sceneManager->ChengeScene("TITLE");
	}
	skyBox->Update();

	float deltaTime = 1.0f / 60.0f;

	for (auto& emitter : emitters_) {
		emitter->Update(deltaTime);
	}
	ParticleManager::GetInstance()->Update();
	Object3dCommon::GetInstance()->GetDefaultCamera()->Update();
	object3dObject_->Update();

	cylinderGameObject_->Update();for (int i = 0; i < 10; ++i) {
		for (int j = 0; j < 10; ++j) {
			EulerTransform transform;
			transform.scale = { 0.5f, 0.5f, 0.5f };
			transform.rotate = { 0.0f, 0.0f, 0.0f };
			transform.translate = { -20.0f + i * 4.0f, 0.0f, -20.0f + j * 4.0f };

			instancingModel_->AddInstance(transform);
		}
	}
	spriteObject_->Update();
	for (auto& spriteObject : spriteObjects_) {
		spriteObject->Update();
	}
	for (auto& s : sprites) {
		s->SetSize({ 100.0f, 100.0f });
	}
	sphereGameObject_->Update();
	if (Input::GetInstance()->TriggerKey(DIK_A)) {
		Vector3 targetPos = { 0.0f, -5.0f, 10.0f };

		for (auto& emitter : emitters_) {
			if (emitter->GetGroupName() == "Lightning") {
				emitter->SetTranslate({ 0.0f, 10.0f, 10.0f });
				emitter->EmitLightning(targetPos);
			}
		}
	}
	if (Input::GetInstance()->TriggerKey(DIK_S)) {
		for (auto& emitter : emitters_) {
			if (emitter->GetGroupName() == "Slash") {
				emitter->SetTranslate({0.0f, 2.0f, 5.0f});
				emitter->Emit();
			}
		}
	}
	ImGuiUpdate();
}

void GamePlayScene::DrawSkyBox() {
	if (isShowSkyBox_) {
		 skyBox->Draw();
	}
}

void GamePlayScene::Draw2D() {
	if (isShowSprite_) {
		spriteObject_->Draw();
	}

	if (isShowSprites_) {
		for (auto& spriteObject : spriteObjects_) {
			spriteObject->Draw();
		}
	}
}

void GamePlayScene::Draw3D() {
	if (isShowObject3D_) {
		object3dObject_->Draw();
	}

	if (isShowCylinder_) {
		cylinderGameObject_->Draw();
	}

	if (isShowSphere_) {
		sphereGameObject_->Draw();
	}

	if (isShowInstancing_) {
		InstancingModelCommon::GetInstance()->SetDraw();

		instancingModel_->Draw(Object3dCommon::GetInstance()->GetDefaultCamera());
	}

	if (isShowParticles_) {
		ParticleManager::GetInstance()->Draw(Object3dCommon::GetInstance()->GetDefaultCamera());
	}
}

void GamePlayScene::Finalize() {
	BaseScene::Finalize();
	if (audio_) {
		audio_->Finalize();
		audio_.reset();
	}
	sprite = nullptr;
	sprites.clear();
	emitters_.clear();
	object3d = nullptr;
	sphereObject = nullptr;
	cylinderObject = nullptr;
	spriteObject_.reset();
	spriteObjects_.clear();
	emitterObjects_.clear();
	ParticleManager::GetInstance()->ClearGroups();
	object3dObject_.reset();
	sphereGameObject_.reset();
	cylinderGameObject_.reset();
}

void GamePlayScene::ImGuiUpdate() {
#ifdef USE_IMGUI
	cameraPosition = Object3dCommon::GetInstance()->GetDefaultCamera()->GetTranslate();
	cameraRotate = Object3dCommon::GetInstance()->GetDefaultCamera()->GetRotate();
	if (!ImGui::GetIO().WantCaptureMouse) {
		const float wheelZoomSpeed = 0.01f;
		cameraPosition.z = ClampFloat(cameraPosition.z + static_cast<float>(Input::GetInstance()->GetMouseWheelDelta()) * wheelZoomSpeed, -100.0f, -1.0f);
	}

	trsprite = sprite->GetTransform();
	trspriteUV = sprite->GetUVTransform();
	spriteColor = sprite->GetColor();
	spriteSize = sprite->GetSize();
	anchor = sprite->GetAnchorPoint();
	isFlipX = sprite->GetIsFlipX();
	isFlipY = sprite->GetIsFlipY();
	textureLeftTop = sprite->GetTextureLeftTop();
	textureSize = sprite->GetTextureSize();

	modelPosition = object3d->GetTranslate();
	modelRotate = object3d->GetRotate();
	modelScale = object3d->GetScale();
	lightColor = object3d->GetLightColor();
	lightDirection = object3d->GetLightDirection();
	lightIntensity = object3d->GetLightIntensity();
	float	environmentMultiplier = object3d->GetEnvironmentMultiplier();

	Vector3 spherepos = sphereObject->GetTranslate();
	Vector3 sphererot = sphereObject->GetRotate();
	Vector3 spherescl = sphereObject->GetScale();

	Vector4 pointLightColor = object3d->GetPointLightColor();
	Vector3 pointLightPosition = object3d->GetPointLightPosition();
	float pointLightIntensity = object3d->GetPointLightIntensity();
	float pointLightRadius = object3d->GetPointLightRadius();
	float pointLightDecay = object3d->GetPointLightDecay();
	bool isPointLightSet = object3d->GetIsPointLightSet();
	const EditorLayout layout = MakeEditorLayout();

	if (BeginEditorPanel("Hierarchy", layout.hierarchyPos, layout.hierarchySize)) {
		ImGui::Text("Scene Visibility");
		ImGui::Separator();

		ImGui::Checkbox("Show SkyBox", &isShowSkyBox_);
		ImGui::Checkbox("Show Main Sprite", &isShowSprite_);
		ImGui::Checkbox("Show Array Sprites", &isShowSprites_);
		ImGui::Checkbox("Show Plane", &isShowObject3D_);
		ImGui::Checkbox("Show InstancingModel", &isShowInstancing_);
		ImGui::Checkbox("Show Sphere", &isShowSphere_);
		ImGui::Checkbox("Show Cylinder", &isShowCylinder_);
		ImGui::Checkbox("Show Particles", &isShowParticles_);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Text("System Info");
		ImGui::Text("Frame Rate : %.1f FPS", ImGui::GetIO().Framerate);
		ImGui::Text("Frame Time : %.3f ms", 1000.0f / ImGui::GetIO().Framerate);

		static bool showDemoWindow = false;
		ImGui::Checkbox("Show ImGui Demo Window", &showDemoWindow);
		if (showDemoWindow) {
			ImGui::ShowDemoWindow(&showDemoWindow);
		}
	}
	ImGui::End();

	if (BeginEditorPanel("Inspector", layout.inspectorPos, layout.inspectorSize)) {
		if (ImGui::CollapsingHeader("Camera Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::DragFloat3("camera pos", &cameraPosition.x, 0.1f);
			ImGui::SliderAngle("camera rotate x", &cameraRotate.x);
			ImGui::SliderAngle("camera rotate y", &cameraRotate.y);
			ImGui::SliderAngle("camera rotate z", &cameraRotate.z);
		}

		if (ImGui::CollapsingHeader("Model Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::DragFloat3("model pos", &modelPosition.x, 0.1f);
			ImGui::SliderAngle("model rotate x", &modelRotate.x);
			ImGui::SliderAngle("model rotate y", &modelRotate.y);
			ImGui::SliderAngle("model rotate z", &modelRotate.z);
			ImGui::DragFloat3("model scale", &modelScale.x, 0.1f);
			ImGui::DragFloat("environment multiplier", &environmentMultiplier, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat3("sphere pos", &spherepos.x, 0.1f);
			ImGui::DragFloat3("sphere rotate", &sphererot.x, 0.1f);
			ImGui::DragFloat3("sphere scale", &spherescl.x, 0.1f);
		}

		if (ImGui::CollapsingHeader("Sprite Settings")) {
			ImGui::DragFloat2("sprite pos", &trsprite.translate.x, 0.3f);
			ImGui::SliderAngle("sprite rotate", &trsprite.rotate.z);
			ImGui::DragFloat2("sprite scale", &spriteSize.x, 0.3f);
			ImGui::ColorEdit4("sprite color", &spriteColor.x);
			ImGui::DragFloat2("anchor point", &anchor.x, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat2("texture left top", &textureLeftTop.x, 1.0f, 0.0f, 512.0f);
			ImGui::DragFloat2("texture size", &textureSize.x, 1.0f, 0.0f, 512.0f);
			ImGui::Checkbox("Flip X", &isFlipX);
			ImGui::Checkbox("Flip Y", &isFlipY);
			ImGui::DragFloat2("UV translate", &trspriteUV.translate.x, 0.01f, -10.0f, 10.0f);
			ImGui::DragFloat2("UV scale", &trspriteUV.scale.x, 0.01f, 0.0f, 10.0f);
			ImGui::SliderAngle("UV rotate", &trspriteUV.rotate.z);
		}

		if (ImGui::CollapsingHeader("Light Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::ColorEdit4("light color", &lightColor.x);
			ImGui::DragFloat3("light direction", &lightDirection.x, 0.1f, -1.0f, 1.0f);
			ImGui::DragFloat("light intensity", &lightIntensity, 0.1f, 0.0f, 10.0f);
			ImGui::Checkbox("point light set", &isPointLightSet);
			ImGui::ColorEdit4("point light color", &pointLightColor.x);
			ImGui::DragFloat3("point light position", &pointLightPosition.x, 0.1f);
			ImGui::DragFloat("point light intensity", &pointLightIntensity, 0.1f, 0.0f, 10.0f);
			ImGui::DragFloat("point light radius", &pointLightRadius, 0.1f, 0.0f, 20.0f);
			ImGui::DragFloat("point light decay", &pointLightDecay, 0.1f, 0.0f, 10.0f);
			ImGui::DragFloat("environment multiplier", &environmentMultiplier, 0.01f, 0.0f, 1.0f);
		}
	}
	ImGui::End();

	BeginEditorPanel("Particle Editor", layout.particlePos, layout.particleSize);

	std::vector<const char*> emitterNames;
	for (const auto& emitter : emitters_) {
		emitterNames.push_back(emitter->GetGroupName().c_str());
	}

	ImGui::Combo("Target Particle", &currentParticleIndex_, emitterNames.data(), static_cast<int>(emitterNames.size()));

	ImGui::Separator();

	if (!emitters_.empty() && currentParticleIndex_ < emitters_.size()) {
		auto& targetEmitter = emitters_[currentParticleIndex_];

		ParticleEmitParam param = targetEmitter->GetPalam();

		ImGui::Text("Editing: %s", targetEmitter->GetGroupName().c_str());

		if (ImGui::CollapsingHeader("1. Basic Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			bool isActive = targetEmitter->GetIsActive();
			if (ImGui::Checkbox("Active (Draw/Update ON)", &isActive)) {
				targetEmitter->SetIsActive(isActive);
			}

			int count = static_cast<int>(param.count);
			if (ImGui::DragInt("Emit Count", &count, 1, 0, 1000)) {
				param.count = static_cast<uint32_t>(count);
			}

			float frequency = targetEmitter->GetFrequency();
			if (ImGui::DragFloat("Emit Frequency (sec)", &frequency, 0.01f, 0.01f, 10.0f)) {
				targetEmitter->SetFrequency(frequency);
			}
			ImGui::Separator();

			ImGui::DragFloat("Life Time", &param.lifeTime, 0.01f, 0.1f, 20.0f);
			
			ImGui::Checkbox("Is Billboard", &param.isBillboard);

			ImGui::DragFloat3("Random Pos Range", &param.randomPositionRange.x, 0.01f);
			ImGui::Text("Texture");

			std::vector<const char*> textureNames;
			for (const auto& path : availableTextures_) {
				textureNames.push_back(path.c_str());
			}

			int currentTexIndex = 0;
			std::string currentPath = targetEmitter->GetTextureFilePath();
			for (int i = 0; i < availableTextures_.size(); ++i) {
				if (availableTextures_[i] == currentPath) {
					currentTexIndex = i;
					break;
				}
			}

			if (ImGui::Combo("Particle Texture", &currentTexIndex, textureNames.data(), static_cast<int>(textureNames.size()))) {
				targetEmitter->SetTexture(availableTextures_[currentTexIndex]);
			}

			ImGui::Separator();
			ImGui::Text("Blend Mode");
			const char* blendModeNames[] = { "None", "Normal", "Add", "Subtract", "Multiply", "Screen" };

			int currentBlendMode = static_cast<int>(targetEmitter->GetBlendMode());

			if (ImGui::Combo("##BlendMode", &currentBlendMode, blendModeNames, IM_ARRAYSIZE(blendModeNames))) {
				BlendMode newMode = static_cast<BlendMode>(currentBlendMode);
				targetEmitter->SetBlendMode(newMode);

				ParticleManager::GetInstance()->SetGroupBlendMode(targetEmitter->GetGroupName(), newMode);
			}
			ImGui::Text("Mesh Type");
			const char* meshTypeNames[] = { "Quad (Square)", "Ring" };

			int currentMeshType = static_cast<int>(targetEmitter->GetMeshType());

			if (ImGui::Combo("##MeshType", &currentMeshType, meshTypeNames, IM_ARRAYSIZE(meshTypeNames))) {
				ParticleMeshType newType = static_cast<ParticleMeshType>(currentMeshType);
				targetEmitter->SetMeshType(newType);

			}
			ImGui::Separator();
		}

		if (ImGui::CollapsingHeader("2. Movement & Physics", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::DragFloat3("Base Velocity", &param.baseVelocity.x, 0.01f);
			ImGui::DragFloat3("Random Vel Range", &param.randomVelocityRange.x, 0.01f);

			ImGui::DragFloat3("Acceleration", &param.acceleration.x, 0.001f);
		}

		if (ImGui::CollapsingHeader("3. Scale & Color", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Text("Scale");
			ImGui::DragFloat3("Start Scale", &param.scale.x, 0.01f);
			ImGui::DragFloat3("End Scale", &param.endScale.x, 0.01f);
			ImGui::DragFloat3("Random Scale Range", &param.randomScaleRange.x, 0.01f);

			ImGui::Spacing();

			ImGui::Text("Color");
			ImGui::ColorEdit4("Start Color", &param.color.x);
			ImGui::ColorEdit4("End Color", &param.endColor.x);
		}

		if (ImGui::CollapsingHeader("4. Rotation")) {
			ImGui::DragFloat3("Base Rotate", &param.baseRotate.x, 0.01f);
			ImGui::Checkbox("Enable Random Rotate", &param.isRandomRotate);
			if (param.isRandomRotate) {
				ImGui::DragFloat3("Random Rot Range", &param.randomRotateRange.x, 0.01f);
			}
		}

		targetEmitter->SetParam(param);

		ImGui::Separator();

		if (ImGui::Button("Test Emit", ImVec2(120, 30))) {
			targetEmitter->Emit();
		}
	}

	ImGui::Separator();

	if (ImGui::Button("Save All to JSON", ImVec2(150, 40))) {
		for (auto& emitter : emitters_) {
			emitter->SaveToJson("Resources/Data/emit_status.json");
		}
	}

	ImGui::End();

	sphereObject->SetTranslate(spherepos);
	sphereObject->SetRotate(sphererot);
	sphereObject->SetScale(spherescl);
	object3d->IsPointLightSet(isPointLightSet);
	sphereObject->IsPointLightSet(isPointLightSet);

	object3d->SetPointLight(pointLightColor, pointLightPosition, pointLightIntensity, pointLightRadius, pointLightDecay);
	sphereObject->SetPointLight(pointLightColor, pointLightPosition, pointLightIntensity, pointLightRadius, pointLightDecay);
	object3d->SetEnvironmentMultiplier(environmentMultiplier);

	Object3dCommon::GetInstance()->GetDefaultCamera()->SetTranslate(cameraPosition);
	Object3dCommon::GetInstance()->GetDefaultCamera()->SetRotate(cameraRotate);
	object3d->SetTranslate(modelPosition);
	object3d->SetRotate(modelRotate);
	object3d->SetScale(modelScale);
	sprite->SetIsFlipX(isFlipX);
	sprite->SetIsFlipY(isFlipY);
	sprite->SetAnchorPoint(anchor);
	sprite->SetTransform(trsprite);
	sprite->SetUVTransform(trspriteUV);
	sprite->SetColor(spriteColor);
	sprite->SetSize(spriteSize);
	sprite->SetTextureLeftTop(textureLeftTop);
	sprite->SetTextureSize(textureSize);
	object3d->SetDirectionalLight(lightColor, lightDirection, lightIntensity);
	sphereObject->SetDirectionalLight(lightColor, lightDirection, lightIntensity);
#endif
}


