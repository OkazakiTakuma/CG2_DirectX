#include "GamePlayScene.h"
#include "SceneManager.h"
#include <ModelManager.h>

void GamePlayScene::Initialize() {
	TextureManager::GetInstance()->LoadTexture("Resources/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("Resources/monsterball.png");
	TextureManager::GetInstance()->LoadTexture("Resources/rostock_laage_airport_4k.dds");

	sprite = std::make_unique<Sprite>();
	sprite->Initialize("Resources/uvChecker.png");
	Audio::GetInstance().Initialize();

#pragma endregion
#pragma region それぞれのリソースの生成
	skyBox = std::make_unique<SkyBox>();
	skyBox->Initialize("Resources/rostock_laage_airport_4k.dds");
	for (int i = 0; i < 5; i++) {
		std::unique_ptr<Sprite> sprits = std::make_unique<Sprite>();
		if (i == 1 || i == 3) {
			sprits->Initialize("Resources/uvChecker.png");
		} else {
			sprits->Initialize("Resources/monsterball.png");
		}
		sprites.push_back(std::move(sprits));
		Transform transform;
		transform.scale = {50.0f, 50.0f, 1.0f};
		transform.translate = {100.0f + i * 90.0f, 200.0f, 0.0f};
		// sprits->SetTransform(transform);
	}

	object3d = std::make_unique<Object3d>();
	object3d->Initialize();
	ModelManager::GetInstance()->LoadModel("plane.obj");
	object3d->SetModel("plane.obj");
	ModelManager::GetInstance()->LoadModel("axis.obj");
	const int axisCount = 5;
	for (int i = 0; i < axisCount; ++i) {
		std::unique_ptr<Object3d> axisObj = std::make_unique<Object3d>();
		axisObj->Initialize();
		axisObj->SetModel("axis.obj");
		// 位置をずらして配置
		axisObj->SetScale({1.0f, 1.0f, 1.0f});
		axisObj->SetTranslate({float(i) * 2.0f, float(i) * 2.0f, 3.0f});
		axisObjects.push_back(std::move(axisObj));
	}

	sphereObject = std::make_unique<Object3d>();
	sphereObject->Initialize();
	ModelManager::GetInstance()->LoadModel("sphere.obj");
	sphereObject->SetModel("sphere.obj");
	sphereObject->SetTranslate({0.0f, 10.0f, 3.0f});
	sphereObject->SetScale({1.0f, 1.0f, 1.0f});
	sphereObject->SetRotate({0.0f, 0.0f, 0.0f});

	// =================================================
	// ▼ 追加: エミッタの作成
	// =================================================
	// エミッタ用の座標設定
	Transform emitterTransform;
	emitterTransform.translate = {0.0f, 0.0f, 0.0f}; // 原点
	emitterTransform.rotate = {0.0f, 0.0f, 0.0f};
	emitterTransform.scale = {1.0f, 1.0f, 1.0f};

	// "Smoke" グループ用のエミッタを生成
	// (グループ名, Transform, 1回の発生数, 1秒間の発生頻度)
	fireEmitParam.scale = {2.0f, 2.0f, 2.0f}; // 大きくする
	fireEmitParam.baseVelocity = {0.0f, 0.5f, 0.0f}; // 上に向かって進む
	fireEmitParam.randomVelocityRange = {0.1f, 0.1f, 0.1f}; // 少しだけ横に揺らぐ
	fireEmitParam.randomPositionRange = {1.0f, 0.2f, 1.0f}; // 横に広く発生する
	fireEmitParam.lifeTime = 1.5f;                          // 1.5秒で消える

	emitter = std::make_unique<ParticleEmitter>();
	emitter->SetGroupName("Fire");
	emitter->LoadFromJson(); // JSONからステータスを読み込む
	emitter->SetTexture("Resources/circle.png");


	// ※ここでは「1秒間に10回、1回につき5個発生」という設定にしています
	const float clearColor[4] = {0.1f, 0.25f, 0.5f, 1.0f}; // 青色
	                                                       // メッセージループ
#pragma endregion

	bool useTexture = true;
	SoundData fanfare = {};
	Audio::GetInstance().LoadWave(L"Resources/fanfare.wav", fanfare);
	//Audio::GetInstance().Play(fanfare, 0);
}

void GamePlayScene::Update() {
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		SceneManager::GetInstance()->ChengeScene("TITLE");
	}
	skyBox->Update();

	// 更新
	emitter->Update(1.0f / 60.0f);
	ParticleManager::GetInstance()->Update();
	Object3dCommon::GetInstance()->GetDefaultCamera()->Update();
	Object3dCommon::GetInstance()->GetDefaultCamera()->Update();
	object3d->Update();
	for (auto& axisObj : axisObjects) {
		axisObj->Update();
	}

	sprite->Update();
	for (auto& s : sprites) {
		s->Update();
		s->SetSize({100.0f, 100.0f});
	}
	sphereObject->Update();

	ImGuiUpdate();
}

void GamePlayScene::DrawSkyBox() { skyBox->Draw(); }

void GamePlayScene::Draw2D() {
	sprite->Draw();
	for (auto& s : sprites) {
		s->Draw();
	}
	ImGuiManager::GetInstance()->Draw();
}

void GamePlayScene::Draw3D() {
	// object3d->Draw();
	//   複数axis.obj描画
	for (auto& axisObj : axisObjects) {
		axisObj->Draw();
	}
	object3d->Draw();
	sphereObject->Draw();

	if (isParticleEmit) {
		ParticleManager::GetInstance()->Draw(Object3dCommon::GetInstance()->GetDefaultCamera());
	}
}

void GamePlayScene::Finalize() {}

void GamePlayScene::ImGuiUpdate() {
#ifdef USE_IMGUI
	ImGuiManager::GetInstance()->Begin();
	cameraPosition = Object3dCommon::GetInstance()->GetDefaultCamera()->GetTranslate();
	cameraRotate = Object3dCommon::GetInstance()->GetDefaultCamera()->GetRotate();

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

	fireEmitParam.scale = emitter->GetScale();
	fireEmitParam.baseVelocity = emitter->GetBaseVelocity();
	fireEmitParam.randomVelocityRange = emitter->GetRandomVelocityRange();
	fireEmitParam.randomPositionRange = emitter->GetRandomPositionRange();
	fireEmitParam.lifeTime = emitter->GetLifeTime();
	ImGui::Begin("Camera Settings");
	ImGui::DragFloat3("camera pos", &cameraPosition.x, 0.1f);
	ImGui::SliderAngle("camera rotate x", &cameraRotate.x);
	ImGui::SliderAngle("camera rotate y", &cameraRotate.y);
	ImGui::SliderAngle("camera rotate z", &cameraRotate.z);
	ImGui::End();
	ImGui::Begin("Model Settings");
	ImGui::DragFloat3("model pos", &modelPosition.x, 0.1f);
	ImGui::SliderAngle("model rotate x", &modelRotate.x);
	ImGui::SliderAngle("model rotate y", &modelRotate.y);
	ImGui::SliderAngle("model rotate z", &modelRotate.z);
	ImGui::DragFloat3("model scale", &modelScale.x, 0.1f);
	ImGui::DragFloat("environment multiplier", &environmentMultiplier, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat3("sphere pos", &spherepos.x, 0.1f);
	ImGui::DragFloat3("sphere rotate", &sphererot.x, 0.1f);
	ImGui::DragFloat3("sphere scale", &spherescl.x, 0.1f);
	ImGui::End();

	ImGui::Begin("Sprite Settings");
	ImGui::DragFloat2("sprite pos", &trsprite.translate.x, 0.3f);
	ImGui::SliderAngle("sprite rotate", &trsprite.rotate.z);
	ImGui::DragFloat2("sprite scale", &spriteSize.x, 0.3f);
	ImGui::ColorEdit4("sprite color", &spriteColor.x, 1.0f); // クリアカラーの編集
	ImGui::DragFloat2("anchor point", &anchor.x, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat2("texture left top", &textureLeftTop.x, 1.0f, 0.0f, 512.0f);
	ImGui::DragFloat2("texture size", &textureSize.x, 1.0f, 0.0f, 512.0f);
	ImGui::Checkbox("Flip X", &isFlipX);
	ImGui::Checkbox("Flip Y", &isFlipY);
	ImGui::DragFloat2("UV translate", &trspriteUV.translate.x, 0.01f, -10.0f, 10.0f);
	ImGui::DragFloat2("UV scale", &trspriteUV.scale.x, 0.01f, 0.0f, 10.0f);
	ImGui::SliderAngle("UV rotate", &trspriteUV.rotate.z);
	ImGui::End();

	ImGui::Begin("Light Settings");

	ImGui::ColorEdit4("light color", &lightColor.x, 1.0f);
	ImGui::DragFloat3("light direction", &lightDirection.x, 0.1f, -1.0f, 1.0f);
	ImGui::DragFloat("light intensity", &lightIntensity, 0.1f, 0.0f, 10.0f);
	ImGui::Checkbox("point light set", &isPointLightSet);
	ImGui::ColorEdit4("point light color", &pointLightColor.x, 1.0f);
	ImGui::DragFloat3("point light position", &pointLightPosition.x, 0.1f);
	ImGui::DragFloat("point light intensity", &pointLightIntensity, 0.1f, 0.0f, 10.0f);
	ImGui::DragFloat("point light radius", &pointLightRadius, 0.1f, 0.0f, 20.0f);
	ImGui::DragFloat("point light decay", &pointLightDecay, 0.1f, 0.0f, 10.0f);
	ImGui::DragFloat("environment multiplier", &environmentMultiplier, 0.01f, 0.0f, 1.0f);
	
	ImGui::End();
	//  ImGuiのウィンドウを作成
	
	ImGui::Begin("Emitter Settings");
	ImGui::Checkbox("Particle Emit", &isParticleEmit);
	ImGui::DragFloat3("Emitter Scale", &fireEmitParam.scale.x, 0.1f);
	ImGui::DragFloat3("Emitter Base Velocity", &fireEmitParam.baseVelocity.x, 0.1f);
	ImGui::DragFloat3("Emitter Random Velocity Range", &fireEmitParam.randomVelocityRange.x, 0.1f);
	ImGui::DragFloat3("Emitter Random Position Range", &fireEmitParam.randomPositionRange.x, 0.1f);
	ImGui::DragFloat("Emitter Life Time", &fireEmitParam.lifeTime, 0.1f, 0.1f, 10.0f);
	ImGui::End();
	ImGuiManager::GetInstance()->End();

	emitter->SetScale(fireEmitParam.scale);
	emitter->SetBaseVelocity(fireEmitParam.baseVelocity);
	emitter->SetRandomVelocityRange(fireEmitParam.randomVelocityRange);
	emitter->SetRandomPositionRange(fireEmitParam.randomPositionRange);
	emitter->SetLifeTime(fireEmitParam.lifeTime);

	sphereObject->SetTranslate(spherepos);
	sphereObject->SetRotate(sphererot);
	sphereObject->SetScale(spherescl);
	object3d->IsPointLightSet(isPointLightSet);
	sphereObject->IsPointLightSet(isPointLightSet);
	for (auto& axisObj : axisObjects) {
		axisObj->IsPointLightSet(isPointLightSet);
	}

	object3d->SetPointLight(pointLightColor, pointLightPosition, pointLightIntensity, pointLightRadius, pointLightDecay);
	sphereObject->SetPointLight(pointLightColor, pointLightPosition, pointLightIntensity, pointLightRadius, pointLightDecay);
	for (auto& axisObj : axisObjects) {
		axisObj->SetPointLight(pointLightColor, pointLightPosition, pointLightIntensity, pointLightRadius, pointLightDecay);
		axisObj->SetEnvironmentMultiplier(environmentMultiplier);
	}
	object3d->SetEnvironmentMultiplier(environmentMultiplier);

#endif
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
	for (auto& axisObj : axisObjects) {
		axisObj->SetDirectionalLight(lightColor, lightDirection, lightIntensity);
	}
}

