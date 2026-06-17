#include "GamePlayScene.h"
#include "SceneManager.h"
#include <ModelManager.h>

void GamePlayScene::Initialize() {
	std::vector<std::string> allTextures = {
			"Resources/uvChecker.png",
			"Resources/monsterball.png",
			"Resources/rostock_laage_airport_4k.dds",
			"Resources/gradationLine.png"
	};

	// ─── 2. 作った「allTextures」をここで使います ───
	availableTextures_.clear();

	for (const auto& path : allTextures) {
		// テクスチャをロード
		TextureManager::GetInstance()->LoadTexture(path);

		// ".png" が含まれているかチェックして、OKなら追加
		if (path.find(".png") != std::string::npos) {
			availableTextures_.push_back(path);
		}
	}
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
		}
		else {
			sprits->Initialize("Resources/monsterball.png");
		}
		sprites.push_back(std::move(sprits));
		Transform transform;
		transform.scale = { 50.0f, 50.0f, 1.0f };
		transform.translate = { 100.0f + i * 90.0f, 200.0f, 0.0f };
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
		axisObj->SetScale({ 1.0f, 1.0f, 1.0f });
		axisObj->SetTranslate({ float(i) * 2.0f, float(i) * 2.0f, 3.0f });
		axisObjects.push_back(std::move(axisObj));
	}

	sphereObject = std::make_unique<Object3d>();
	sphereObject->Initialize();
	ModelManager::GetInstance()->LoadModel("sphere.obj");
	sphereObject->SetModel("sphere.obj");
	sphereObject->SetTranslate({ 0.0f, 10.0f, 3.0f });
	sphereObject->SetScale({ 1.0f, 1.0f, 1.0f });
	sphereObject->SetRotate({ 0.0f, 0.0f, 0.0f });
	cylinderObject = std::make_unique<Object3d>();

	// ★ この初期化処理が抜けていると今回のエラーが起きます！
	cylinderObject->Initialize();

	// そのあとにシリンダーの形を作ったりテクスチャを貼る
	cylinderObject->CreateCylinder(1.0f, 2.0f, 16, true, true);
	cylinderObject->SetTexture("Resources/gradationLine.png");
	// =================================================
	// ▼ 追加: エミッタの作成
	// =================================================
	std::vector<std::string> particleTypes = { "Fire", "Dust", "Slash","Slash_Trace" };

	// 2. ループでエミッターを生成してリストにまとめる
	for (const auto& groupName : particleTypes) {
		auto newEmitter = std::make_unique<ParticleEmitter>();

		// グループ名を設定
		newEmitter->SetGroupName(groupName);

		// JSONファイルからパラメータ（emitParamやfrequencyなど）を自動ロード
		// (ParticleEmitter::LoadFromJsonの実装に合わせてパスを指定してください)
		newEmitter->LoadFromJson("Resources/Data/emit_status.json");
		std::string texPath = newEmitter->GetTextureFilePath();
		if (texPath == "") {
			texPath = "Resources/uvChecker.png"; // ※とりあえず市松模様を設定
			newEmitter->SetTexture(texPath);     // エミッターのパスも上書きしておく
		}

		// ─── ★超重要：ParticleManagerにグループを作成し、テクスチャを渡す ───
		ParticleManager::GetInstance()->CreateParticleGroup(groupName, texPath);
		// ※もし必要なら初期位置などもここで設定可能
		// newEmitter->SetTranslate({0.0f, 0.0f, 0.0f});
		ParticleManager::GetInstance()->SetGroupBlendMode(newEmitter->GetGroupName(), newEmitter->GetBlendMode());
		// リスト（vector）に格納
		emitters_.push_back(std::move(newEmitter));
	}

	// ※ここでは「1秒間に10回、1回につき5個発生」という設定にしています
#pragma endregion

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
	float deltaTime = 1.0f / 60.0f;

	// ─── リスト内のすべてのエミッターを一括更新 ───
	for (auto& emitter : emitters_) {
		emitter->Update(deltaTime);
	}
	ParticleManager::GetInstance()->Update();
	Object3dCommon::GetInstance()->GetDefaultCamera()->Update();
	object3d->Update();

	cylinderObject->Update();

	sprite->Update();
	for (auto& s : sprites) {
		s->Update();
		s->SetSize({ 100.0f, 100.0f });
	}
	sphereObject->Update();

	ImGuiUpdate();
}

void GamePlayScene::DrawSkyBox() { //skyBox->Draw(); 
}

void GamePlayScene::Draw2D() {
	sprite->Draw();
	for (auto& s : sprites) {
		s->Draw();
	}
}

void GamePlayScene::Draw3D() {
	object3d->Draw();
	cylinderObject->Draw();
	sphereObject->Draw();


	ParticleManager::GetInstance()->Draw(Object3dCommon::GetInstance()->GetDefaultCamera());

}

void GamePlayScene::Finalize() {}

void GamePlayScene::ImGuiUpdate() {
#ifdef USE_IMGUI
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
// --- ImGuiによるパーティクル編集エディタ ---
	ImGui::Begin("Particle Editor");

	// ① コンボボックス用の名前リストを作成する
	std::vector<const char*> emitterNames;
	for (const auto& emitter : emitters_) {
		emitterNames.push_back(emitter->GetGroupName().c_str());
	}

	// ② パーティクルを選択するコンボボックス
	ImGui::Combo("Target Particle", &currentParticleIndex_, emitterNames.data(), static_cast<int>(emitterNames.size()));

	ImGui::Separator(); // UIの区切り線

	// ③ 選択されたパーティクルのパラメータをすべて編集
	if (!emitters_.empty() && currentParticleIndex_ < emitters_.size()) {
		auto& targetEmitter = emitters_[currentParticleIndex_];

		// 現在の設定値を取得
		ParticleEmitParam param = targetEmitter->GetPalam();

		ImGui::Text("Editing: %s", targetEmitter->GetGroupName().c_str());

		// ─── 【基本設定】 ───
		if (ImGui::CollapsingHeader("1. Basic Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			// アクティブ（描画・更新オンオフ）
			bool isActive = targetEmitter->GetIsActive();
			if (ImGui::Checkbox("Active (Draw/Update ON)", &isActive)) {
				targetEmitter->SetIsActive(isActive);
			}

			// 一度に発生する数 (uint32_t なので int で受けてから戻す)
			int count = static_cast<int>(param.count);
			if (ImGui::DragInt("Emit Count", &count, 1, 0, 1000)) {
				param.count = static_cast<uint32_t>(count);
			}

			// 寿命
			ImGui::DragFloat("Life Time", &param.lifeTime, 0.01f, 0.1f, 20.0f);

			// ビルボード（常にカメラを向くか）
			ImGui::Checkbox("Is Billboard", &param.isBillboard);

			// 発生位置のばらつき
			ImGui::DragFloat3("Random Pos Range", &param.randomPositionRange.x, 0.01f);
			ImGui::Text("Texture");

			// コンボボックス用にC文字列(const char*)のリストを作成
			std::vector<const char*> textureNames;
			for (const auto& path : availableTextures_) {
				textureNames.push_back(path.c_str());
			}

			// 現在エミッターにセットされているテクスチャが、リストの何番目かを特定する
			int currentTexIndex = 0;
			std::string currentPath = targetEmitter->GetTextureFilePath();
			for (int i = 0; i < availableTextures_.size(); ++i) {
				if (availableTextures_[i] == currentPath) {
					currentTexIndex = i;
					break;
				}
			}

			// プルダウン（コンボボックス）を表示
			if (ImGui::Combo("Particle Texture", &currentTexIndex, textureNames.data(), static_cast<int>(textureNames.size()))) {
				// もし別の画像が選ばれたら、エミッターに新しいテクスチャをセットする
				targetEmitter->SetTexture(availableTextures_[currentTexIndex]);
			}

			ImGui::Separator();
			ImGui::Text("Blend Mode");
			// struct.h にある BlendMode enum の順番に合わせて名前リストを作る
			const char* blendModeNames[] = { "None", "Normal", "Add", "Subtract", "Multiply", "Screen" };

			// 現在のブレンドモードをint型として取得
			int currentBlendMode = static_cast<int>(targetEmitter->GetBlendMode());

			// プルダウンを表示
			if (ImGui::Combo("##BlendMode", &currentBlendMode, blendModeNames, IM_ARRAYSIZE(blendModeNames))) {
				// もし値が変更されたら、新しいブレンドモードをエミッターにセット
				BlendMode newMode = static_cast<BlendMode>(currentBlendMode);
				targetEmitter->SetBlendMode(newMode);

				// ★超重要：変更された瞬間、Manager側にも即座に反映させる！
				ParticleManager::GetInstance()->SetGroupBlendMode(targetEmitter->GetGroupName(), newMode);
			}
			ImGui::Separator();
		}

		// ─── 【動き（速度・加速度）】 ───
		if (ImGui::CollapsingHeader("2. Movement & Physics", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::DragFloat3("Base Velocity", &param.baseVelocity.x, 0.01f);
			ImGui::DragFloat3("Random Vel Range", &param.randomVelocityRange.x, 0.01f);

			// ★新機能：加速度（Yをマイナスにすれば重力、Xに入れれば風になる）
			ImGui::DragFloat3("Acceleration", &param.acceleration.x, 0.001f);
		}

		// ─── 【大きさと色】 ───
		if (ImGui::CollapsingHeader("3. Scale & Color", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Text("Scale");
			ImGui::DragFloat3("Start Scale", &param.scale.x, 0.01f);
			ImGui::DragFloat3("End Scale", &param.endScale.x, 0.01f);     // ★新機能
			ImGui::DragFloat3("Random Scale Range", &param.randomScaleRange.x, 0.01f);

			ImGui::Spacing();

			ImGui::Text("Color");
			ImGui::ColorEdit4("Start Color", &param.color.x);
			ImGui::ColorEdit4("End Color", &param.endColor.x);         // ★新機能
		}

		// ─── 【回転】 ───
		if (ImGui::CollapsingHeader("4. Rotation")) {
			ImGui::DragFloat3("Base Rotate", &param.baseRotate.x, 0.01f);
			ImGui::Checkbox("Enable Random Rotate", &param.isRandomRotate);
			if (param.isRandomRotate) {
				ImGui::DragFloat3("Random Rot Range", &param.randomRotateRange.x, 0.01f);
			}
		}

		// ─── 編集したパラメータをセットし直す ───
		targetEmitter->SetParam(param);

		ImGui::Separator();

		// ★便利機能：テスト放出ボタン
		if (ImGui::Button("Test Emit", ImVec2(120, 30))) {
			targetEmitter->Emit();
		}
	}

	ImGui::Separator();

	// ④ 保存ボタン
	if (ImGui::Button("Save All to JSON", ImVec2(150, 40))) {
		for (auto& emitter : emitters_) {
			emitter->SaveToJson("Resources/Data/emit_status.json");
		}
	}

	ImGui::End();
	ImGui::Begin("System Info");

	// 1. フレームレートとフレームタイム（1フレームにかかったミリ秒）の表示
	ImGui::Text("Frame Rate : %.1f FPS", ImGui::GetIO().Framerate);
	ImGui::Text("Frame Time : %.3f ms", 1000.0f / ImGui::GetIO().Framerate);

	ImGui::Separator(); // 見やすくするための区切り線

	// 2. 便利なデバッグツール（ImGui公式の機能カタログ）
	static bool showDemoWindow = false;
	ImGui::Checkbox("Show ImGui Demo Window", &showDemoWindow);
	if (showDemoWindow) {
		// ImGuiの全機能の使い方がわかるデモ画面を表示
		ImGui::ShowDemoWindow(&showDemoWindow);
	}

	ImGui::End();

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
#endif
}

