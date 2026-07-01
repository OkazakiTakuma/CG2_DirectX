#include "GamePlayScene.h"
#include "SceneManager.h"
#include <ModelManager.h>
#include"InstancingModelCommon.h"
#include"SkinnedObject3dCommon.h"

void GamePlayScene::Initialize() {
	std::vector<std::string> allTextures = {
			"Resources/circle.png",
			"Resources/uvChecker.png",
			"Resources/monsterball.png",
			"Resources/rostock_laage_airport_4k.dds",
			"Resources/gradationLine.png"
	};

	// 笏笏笏 2. 菴懊▲縺溘径llTextures縲阪ｒ縺薙％縺ｧ菴ｿ縺・∪縺・笏笏笏
	availableTextures_.clear();

	for (const auto& path : allTextures) {
		// 繝・け繧ｹ繝√Ε繧偵Ο繝ｼ繝・
		TextureManager::GetInstance()->LoadTexture(path);

		// ".png" 縺悟性縺ｾ繧後※縺・ｋ縺九メ繧ｧ繝・け縺励※縲＾K縺ｪ繧芽ｿｽ蜉
		if (path.find(".png") != std::string::npos) {
			availableTextures_.push_back(path);
		}
	}
	sprite = std::make_unique<Sprite>();
	sprite->Initialize("Resources/uvChecker.png");
	Audio::GetInstance().Initialize();

#pragma endregion
#pragma region 縺昴ｌ縺槭ｌ縺ｮ繝ｪ繧ｽ繝ｼ繧ｹ縺ｮ逕滓・
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

	sphereObject = std::make_unique<Object3d>();
	sphereObject->Initialize();
	ModelManager::GetInstance()->LoadModel("sphere.obj");
	sphereObject->SetModel("sphere.obj");
	sphereObject->SetTranslate({ 0.0f, 10.0f, 3.0f });
	sphereObject->SetScale({ 1.0f, 1.0f, 1.0f });
	sphereObject->SetRotate({ 0.0f, 0.0f, 0.0f });
	cylinderObject = std::make_unique<Object3d>();

	skinnedModel = std::make_unique<SkinnedModel>();

	// ※注意: 第1引数の ModelCommon の渡し方は、プロジェクトの環境に合わせて変更してください。
	// 例: ModelCommon* modelCommon = ModelCommon::GetInstance();
	skinnedModel->Initialize(ModelManager::GetInstance()->GetModelCommon(), "Resources/simpleSkin", "simpleskin.gltf");

	// アニメーションデータの読み込み
	skinAnimation = SkinnedModel::LoadAnimationFile("Resources/simpleSkin", "simpleskin.gltf");

	// 画面に表示するオブジェクトの作成と設定
	skinnedObject = std::make_unique<SkinnedObject3d>();
	skinnedObject->Initialize();
	skinnedObject->SetModel(skinnedModel.get());
	skinnedObject->SetAnimation(&skinAnimation); // アニメーションをセットして再生準備

	// 表示する位置や大きさを調整します（必要に応じて数値を変更してください）
	skinnedObject->SetTranslate({ 0.0f, 0.0f, 0.0f });
	skinnedObject->SetScale({ 1.0f, 1.0f, 1.0f });

	


	// 笘・縺薙・蛻晄悄蛹門・逅・′謚懊￠縺ｦ縺・ｋ縺ｨ莉雁屓縺ｮ繧ｨ繝ｩ繝ｼ縺瑚ｵｷ縺阪∪縺呻ｼ・
	cylinderObject->Initialize();

	// 縺昴・縺ゅ→縺ｫ繧ｷ繝ｪ繝ｳ繝繝ｼ縺ｮ蠖｢繧剃ｽ懊▲縺溘ｊ繝・け繧ｹ繝√Ε繧定ｲｼ繧・
	cylinderObject->CreateCylinder(1.0f, 2.0f, 16, true, true);
	cylinderObject->SetTexture("Resources/gradationLine.png");
	// =================================================
	// 笆ｼ 霑ｽ蜉: 繧ｨ繝溘ャ繧ｿ縺ｮ菴懈・
	// =================================================
	std::vector<std::string> particleTypes = { "Fire", "Dust", "Slash","Slash_Trace","Lightning" };

	ModelManager::GetInstance()->LoadModel("sphere.obj");

	// インスタンシングモデルの初期化（最大1000個描画できるように確保）
	instancingModel_ = std::make_unique<InstancingModel>();
	instancingModel_->Initialize(ModelManager::GetInstance()->FindModel("sphere.obj"), 1000);

	// 2. 繝ｫ繝ｼ繝励〒繧ｨ繝溘ャ繧ｿ繝ｼ繧堤函謌舌＠縺ｦ繝ｪ繧ｹ繝医↓縺ｾ縺ｨ繧√ｋ
	for (const auto& groupName : particleTypes) {
		auto newEmitter = std::make_unique<ParticleEmitter>();

		// 繧ｰ繝ｫ繝ｼ繝怜錐繧定ｨｭ螳・
		newEmitter->SetGroupName(groupName);

		// JSON繝輔ぃ繧､繝ｫ縺九ｉ繝代Λ繝｡繝ｼ繧ｿ・・mitParam繧・requency縺ｪ縺ｩ・峨ｒ閾ｪ蜍輔Ο繝ｼ繝・
		// (ParticleEmitter::LoadFromJson縺ｮ螳溯｣・↓蜷医ｏ縺帙※繝代せ繧呈欠螳壹＠縺ｦ縺上□縺輔＞)
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

		// 笏笏笏 笘・ｶ・㍾隕・ｼ啀articleManager縺ｫ繧ｰ繝ｫ繝ｼ繝励ｒ菴懈・縺励√ユ繧ｯ繧ｹ繝√Ε繧呈ｸ｡縺・笏笏笏
		ParticleManager::GetInstance()->CreateParticleGroup(groupName, texPath, newEmitter->GetMeshType());// 竊舌％繧後ｒ霑ｽ蜉);
		newEmitter->SetTexture(texPath);
		// 窶ｻ繧ゅ＠蠢・ｦ√↑繧牙・譛滉ｽ咲ｽｮ縺ｪ縺ｩ繧ゅ％縺薙〒險ｭ螳壼庄閭ｽ
		// newEmitter->SetTranslate({0.0f, 0.0f, 0.0f});
		ParticleManager::GetInstance()->SetGroupBlendMode(newEmitter->GetGroupName(), newEmitter->GetBlendMode());
		// 繝ｪ繧ｹ繝茨ｼ・ector・峨↓譬ｼ邏・
		emitters_.push_back(std::move(newEmitter));
	}

	// 窶ｻ縺薙％縺ｧ縺ｯ縲・遘帝俣縺ｫ10蝗槭・蝗槭↓縺､縺・蛟狗匱逕溘阪→縺・≧險ｭ螳壹↓縺励※縺・∪縺・
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

	// 譖ｴ譁ｰ
	float deltaTime = 1.0f / 60.0f;

	// 笏笏笏 繝ｪ繧ｹ繝亥・縺ｮ縺吶∋縺ｦ縺ｮ繧ｨ繝溘ャ繧ｿ繝ｼ繧剃ｸ諡ｬ譖ｴ譁ｰ 笏笏笏
	for (auto& emitter : emitters_) {
		emitter->Update(deltaTime);
	}
	ParticleManager::GetInstance()->Update();
	Object3dCommon::GetInstance()->GetDefaultCamera()->Update();
	object3d->Update();

	cylinderObject->Update();for (int i = 0; i < 10; ++i) {
		for (int j = 0; j < 10; ++j) {
			Transform transform;
			transform.scale = { 0.5f, 0.5f, 0.5f }; // 少し小さめに
			transform.rotate = { 0.0f, 0.0f, 0.0f };
			// 等間隔に並べる
			transform.translate = { -20.0f + i * 4.0f, 0.0f, -20.0f + j * 4.0f };

			// 座標を追加（Drawを呼ぶとリセットされる仕組みです）
			instancingModel_->AddInstance(transform);
		}
	}
	sprite->Update();
	for (auto& s : sprites) {
		s->Update();
		s->SetSize({ 100.0f, 100.0f });
	}
	sphereObject->Update();
	if (Input::GetInstance()->TriggerKey(DIK_A)) {
		// 繧ｿ繝ｼ繧ｲ繝・ヨ縺ｮ蠎ｧ讓呻ｼ井ｾ具ｼ夂岼縺ｮ蜑阪・蝨ｰ髱｢・・
		Vector3 targetPos = { 0.0f, -5.0f, 10.0f };

		// 繝ｪ繧ｹ繝医・荳ｭ縺九ｉ "Lightning" 繧ｨ繝溘ャ繧ｿ繝ｼ繧呈爾縺励※蜻ｼ縺ｳ蜃ｺ縺・
		for (auto& emitter : emitters_) {
			if (emitter->GetGroupName() == "Lightning") {
				// 繧ｨ繝溘ャ繧ｿ繝ｼ縺ｮ菴咲ｽｮ繧剃ｸ顔ｩｺ縺ｫ繧ｻ繝・ヨ
				emitter->SetTranslate({ 0.0f, 10.0f, 10.0f });
				// 髮ｷ繧堤匱蟆・ｼ・ｼ・
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
	if (skinnedObject) {
		skinnedObject->Update();
	}
	ImGuiUpdate();
}

void GamePlayScene::DrawSkyBox() {
	// スカイボックスの表示
	if (isShowSkyBox_) {
		 skyBox->Draw(); // 必要な場合はコメントアウトを外してください
	}
}

void GamePlayScene::Draw2D() {
	// メインのスプライト
	if (isShowSprite_) {
		sprite->Draw();
	}

	// 配列の複数のスプライト
	if (isShowSprites_) {
		for (auto& s : sprites) {
			s->Draw();
		}
	}
}

void GamePlayScene::Draw3D() {
	// 平面モデル
	if (isShowObject3D_) {
		object3d->Draw();
	}

	// シリンダーモデル
	if (isShowCylinder_) {
		cylinderObject->Draw();
	}

	// スフィア（球体）モデル
	if (isShowSphere_) {
		sphereObject->Draw();
	}

	if (skinnedObject) {
		SkinnedObject3dCommon::GetInstance()->SetDraw();
		skinnedObject->Draw();
	}

	// 軸モデル（元のコードには無かったので追加しました！）
	if (isShowInstancing_) {
		InstancingModelCommon::GetInstance()->SetDraw();

		// 一括描画を実行！
		instancingModel_->Draw(Object3dCommon::GetInstance()->GetDefaultCamera());
	}

	// パーティクル全体
	if (isShowParticles_) {
		ParticleManager::GetInstance()->Draw(Object3dCommon::GetInstance()->GetDefaultCamera());
	}
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

	ImGui::Begin("Display Settings"); // ウィンドウの名前

	ImGui::Checkbox("Show SkyBox", &isShowSkyBox_);
	ImGui::Checkbox("Show Main Sprite", &isShowSprite_);
	ImGui::Checkbox("Show Array Sprites", &isShowSprites_);
	ImGui::Checkbox("Show Plane", &isShowObject3D_);
	ImGui::Checkbox("Show InstancingModel", &isShowInstancing_);
	ImGui::Checkbox("Show Sphere", &isShowSphere_);
	ImGui::Checkbox("Show Cylinder", &isShowCylinder_);
	ImGui::Checkbox("Show Particles", &isShowParticles_);

	ImGui::End();

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
	ImGui::ColorEdit4("sprite color", &spriteColor.x, 1.0f); // 繧ｯ繝ｪ繧｢繧ｫ繝ｩ繝ｼ縺ｮ邱ｨ髮・
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
	//  ImGui縺ｮ繧ｦ繧｣繝ｳ繝峨え繧剃ｽ懈・
// --- ImGui縺ｫ繧医ｋ繝代・繝・ぅ繧ｯ繝ｫ邱ｨ髮・お繝・ぅ繧ｿ ---
	ImGui::Begin("Particle Editor");

	// 竭 繧ｳ繝ｳ繝懊・繝・け繧ｹ逕ｨ縺ｮ蜷榊燕繝ｪ繧ｹ繝医ｒ菴懈・縺吶ｋ
	std::vector<const char*> emitterNames;
	for (const auto& emitter : emitters_) {
		emitterNames.push_back(emitter->GetGroupName().c_str());
	}

	// 竭｡ 繝代・繝・ぅ繧ｯ繝ｫ繧帝∈謚槭☆繧九さ繝ｳ繝懊・繝・け繧ｹ
	ImGui::Combo("Target Particle", &currentParticleIndex_, emitterNames.data(), static_cast<int>(emitterNames.size()));

	ImGui::Separator(); // UI縺ｮ蛹ｺ蛻・ｊ邱・

	// 竭｢ 驕ｸ謚槭＆繧後◆繝代・繝・ぅ繧ｯ繝ｫ縺ｮ繝代Λ繝｡繝ｼ繧ｿ繧偵☆縺ｹ縺ｦ邱ｨ髮・
	if (!emitters_.empty() && currentParticleIndex_ < emitters_.size()) {
		auto& targetEmitter = emitters_[currentParticleIndex_];

		// 迴ｾ蝨ｨ縺ｮ險ｭ螳壼､繧貞叙蠕・
		ParticleEmitParam param = targetEmitter->GetPalam();

		ImGui::Text("Editing: %s", targetEmitter->GetGroupName().c_str());

		// 笏笏笏 縲仙渕譛ｬ險ｭ螳壹・笏笏笏
		if (ImGui::CollapsingHeader("1. Basic Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			// 繧｢繧ｯ繝・ぅ繝厄ｼ域緒逕ｻ繝ｻ譖ｴ譁ｰ繧ｪ繝ｳ繧ｪ繝包ｼ・
			bool isActive = targetEmitter->GetIsActive();
			if (ImGui::Checkbox("Active (Draw/Update ON)", &isActive)) {
				targetEmitter->SetIsActive(isActive);
			}

			// 荳蠎ｦ縺ｫ逋ｺ逕溘☆繧区焚 (uint32_t 縺ｪ縺ｮ縺ｧ int 縺ｧ蜿励￠縺ｦ縺九ｉ謌ｻ縺・
			int count = static_cast<int>(param.count);
			if (ImGui::DragInt("Emit Count", &count, 1, 0, 1000)) {
				param.count = static_cast<uint32_t>(count);
			}

			// 逋ｺ蟆・俣髫・
			float frequency = targetEmitter->GetFrequency();
			// 0.01遘・・・10.0遘・縺ｮ髢薙〒隱ｿ謨ｴ縺ｧ縺阪ｋ繧医≧縺ｫ縺吶ｋ
			if (ImGui::DragFloat("Emit Frequency (sec)", &frequency, 0.01f, 0.01f, 10.0f)) {
				targetEmitter->SetFrequency(frequency);
			}
			ImGui::Separator();

			// 蟇ｿ蜻ｽ
			ImGui::DragFloat("Life Time", &param.lifeTime, 0.01f, 0.1f, 20.0f);
			
			// 繝薙Ν繝懊・繝会ｼ亥ｸｸ縺ｫ繧ｫ繝｡繝ｩ繧貞髄縺上°・・
			ImGui::Checkbox("Is Billboard", &param.isBillboard);

			// 逋ｺ逕滉ｽ咲ｽｮ縺ｮ縺ｰ繧峨▽縺・
			ImGui::DragFloat3("Random Pos Range", &param.randomPositionRange.x, 0.01f);
			ImGui::Text("Texture");

			// 繧ｳ繝ｳ繝懊・繝・け繧ｹ逕ｨ縺ｫC譁・ｭ怜・(const char*)縺ｮ繝ｪ繧ｹ繝医ｒ菴懈・
			std::vector<const char*> textureNames;
			for (const auto& path : availableTextures_) {
				textureNames.push_back(path.c_str());
			}

			// 迴ｾ蝨ｨ繧ｨ繝溘ャ繧ｿ繝ｼ縺ｫ繧ｻ繝・ヨ縺輔ｌ縺ｦ縺・ｋ繝・け繧ｹ繝√Ε縺後√Μ繧ｹ繝医・菴慕分逶ｮ縺九ｒ迚ｹ螳壹☆繧・
			int currentTexIndex = 0;
			std::string currentPath = targetEmitter->GetTextureFilePath();
			for (int i = 0; i < availableTextures_.size(); ++i) {
				if (availableTextures_[i] == currentPath) {
					currentTexIndex = i;
					break;
				}
			}

			// 繝励Ν繝繧ｦ繝ｳ・医さ繝ｳ繝懊・繝・け繧ｹ・峨ｒ陦ｨ遉ｺ
			if (ImGui::Combo("Particle Texture", &currentTexIndex, textureNames.data(), static_cast<int>(textureNames.size()))) {
				// 繧ゅ＠蛻･縺ｮ逕ｻ蜒上′驕ｸ縺ｰ繧後◆繧峨√お繝溘ャ繧ｿ繝ｼ縺ｫ譁ｰ縺励＞繝・け繧ｹ繝√Ε繧偵そ繝・ヨ縺吶ｋ
				targetEmitter->SetTexture(availableTextures_[currentTexIndex]);
			}

			ImGui::Separator();
			ImGui::Text("Blend Mode");
			// struct.h 縺ｫ縺ゅｋ BlendMode enum 縺ｮ鬆・分縺ｫ蜷医ｏ縺帙※蜷榊燕繝ｪ繧ｹ繝医ｒ菴懊ｋ
			const char* blendModeNames[] = { "None", "Normal", "Add", "Subtract", "Multiply", "Screen" };

			// 迴ｾ蝨ｨ縺ｮ繝悶Ξ繝ｳ繝峨Δ繝ｼ繝峨ｒint蝙九→縺励※蜿門ｾ・
			int currentBlendMode = static_cast<int>(targetEmitter->GetBlendMode());

			// 繝励Ν繝繧ｦ繝ｳ繧定｡ｨ遉ｺ
			if (ImGui::Combo("##BlendMode", &currentBlendMode, blendModeNames, IM_ARRAYSIZE(blendModeNames))) {
				// 繧ゅ＠蛟､縺悟､画峩縺輔ｌ縺溘ｉ縲∵眠縺励＞繝悶Ξ繝ｳ繝峨Δ繝ｼ繝峨ｒ繧ｨ繝溘ャ繧ｿ繝ｼ縺ｫ繧ｻ繝・ヨ
				BlendMode newMode = static_cast<BlendMode>(currentBlendMode);
				targetEmitter->SetBlendMode(newMode);

				// 笘・ｶ・㍾隕・ｼ壼､画峩縺輔ｌ縺溽椪髢薙｀anager蛛ｴ縺ｫ繧ょ叉蠎ｧ縺ｫ蜿肴丐縺輔○繧具ｼ・
				ParticleManager::GetInstance()->SetGroupBlendMode(targetEmitter->GetGroupName(), newMode);
			}
			ImGui::Text("Mesh Type");
			const char* meshTypeNames[] = { "Quad (Square)", "Ring" };

			// 迴ｾ蝨ｨ縺ｮ繝｡繝・す繝･繧ｿ繧､繝励ｒint蝙九→縺励※蜿門ｾ・
			int currentMeshType = static_cast<int>(targetEmitter->GetMeshType());

			if (ImGui::Combo("##MeshType", &currentMeshType, meshTypeNames, IM_ARRAYSIZE(meshTypeNames))) {
				ParticleMeshType newType = static_cast<ParticleMeshType>(currentMeshType);
				targetEmitter->SetMeshType(newType);

				// 縲絶ｻ驥崎ｦ∽ｺ矩・・
				// 繝｡繝・す繝･縺ｮ蠖｢迥ｶ螟画峩縺ｯ縲碁らせ繝舌ャ繝輔ぃ縺ｮ蜀堺ｽ懈・縲阪′蠢・ｦ√↑縺溘ａ縲・
				// 迴ｾ迥ｶ縺ｮParticleManager縺ｮ莉墓ｧ倥〒縺ｯ縲√％縺薙〒螟画峩縺励※繧ゅΜ繧｢繝ｫ繧ｿ繧､繝縺ｫ縺ｯ螟峨ｏ繧翫∪縺帙ｓ縲・
				// 螟画峩蠕後↓縲郡ave All to JSON縲阪ｒ謚ｼ縺励√ご繝ｼ繝繧貞・襍ｷ蜍包ｼ医ン繝ｫ繝峨＠逶ｴ縺暦ｼ峨☆繧九％縺ｨ縺ｧ蜿肴丐縺輔ｌ縺ｾ縺吶・
			}
			ImGui::Separator();
		}

		// 笏笏笏 縲仙虚縺搾ｼ磯溷ｺｦ繝ｻ蜉騾溷ｺｦ・峨・笏笏笏
		if (ImGui::CollapsingHeader("2. Movement & Physics", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::DragFloat3("Base Velocity", &param.baseVelocity.x, 0.01f);
			ImGui::DragFloat3("Random Vel Range", &param.randomVelocityRange.x, 0.01f);

			// 笘・眠讖溯・・壼刈騾溷ｺｦ・・繧偵・繧､繝翫せ縺ｫ縺吶ｌ縺ｰ驥榊鴨縲々縺ｫ蜈･繧後ｌ縺ｰ鬚ｨ縺ｫ縺ｪ繧具ｼ・
			ImGui::DragFloat3("Acceleration", &param.acceleration.x, 0.001f);
		}

		// 笏笏笏 縲仙､ｧ縺阪＆縺ｨ濶ｲ縲・笏笏笏
		if (ImGui::CollapsingHeader("3. Scale & Color", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Text("Scale");
			ImGui::DragFloat3("Start Scale", &param.scale.x, 0.01f);
			ImGui::DragFloat3("End Scale", &param.endScale.x, 0.01f);     // 笘・眠讖溯・
			ImGui::DragFloat3("Random Scale Range", &param.randomScaleRange.x, 0.01f);

			ImGui::Spacing();

			ImGui::Text("Color");
			ImGui::ColorEdit4("Start Color", &param.color.x);
			ImGui::ColorEdit4("End Color", &param.endColor.x);         // 笘・眠讖溯・
		}

		// 笏笏笏 縲仙屓霆｢縲・笏笏笏
		if (ImGui::CollapsingHeader("4. Rotation")) {
			ImGui::DragFloat3("Base Rotate", &param.baseRotate.x, 0.01f);
			ImGui::Checkbox("Enable Random Rotate", &param.isRandomRotate);
			if (param.isRandomRotate) {
				ImGui::DragFloat3("Random Rot Range", &param.randomRotateRange.x, 0.01f);
			}
		}

		// 笏笏笏 邱ｨ髮・＠縺溘ヱ繝ｩ繝｡繝ｼ繧ｿ繧偵そ繝・ヨ縺礼峩縺・笏笏笏
		targetEmitter->SetParam(param);

		ImGui::Separator();

		// 笘・ｾｿ蛻ｩ讖溯・・壹ユ繧ｹ繝域叛蜃ｺ繝懊ち繝ｳ
		if (ImGui::Button("Test Emit", ImVec2(120, 30))) {
			targetEmitter->Emit();
		}
	}

	ImGui::Separator();

	// 竭｣ 菫晏ｭ倥・繧ｿ繝ｳ
	if (ImGui::Button("Save All to JSON", ImVec2(150, 40))) {
		for (auto& emitter : emitters_) {
			emitter->SaveToJson("Resources/Data/emit_status.json");
		}
	}

	ImGui::End();
	ImGui::Begin("System Info");

	// 1. 繝輔Ξ繝ｼ繝繝ｬ繝ｼ繝医→繝輔Ξ繝ｼ繝繧ｿ繧､繝・・繝輔Ξ繝ｼ繝縺ｫ縺九°縺｣縺溘Α繝ｪ遘抵ｼ峨・陦ｨ遉ｺ
	ImGui::Text("Frame Rate : %.1f FPS", ImGui::GetIO().Framerate);
	ImGui::Text("Frame Time : %.3f ms", 1000.0f / ImGui::GetIO().Framerate);

	ImGui::Separator(); // 隕九ｄ縺吶￥縺吶ｋ縺溘ａ縺ｮ蛹ｺ蛻・ｊ邱・

	// 2. 萓ｿ蛻ｩ縺ｪ繝・ヰ繝・げ繝・・繝ｫ・・mGui蜈ｬ蠑上・讖溯・繧ｫ繧ｿ繝ｭ繧ｰ・・
	static bool showDemoWindow = false;
	ImGui::Checkbox("Show ImGui Demo Window", &showDemoWindow);
	if (showDemoWindow) {
		// ImGui縺ｮ蜈ｨ讖溯・縺ｮ菴ｿ縺・婿縺後ｏ縺九ｋ繝・Δ逕ｻ髱｢繧定｡ｨ遉ｺ
		ImGui::ShowDemoWindow(&showDemoWindow);
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


