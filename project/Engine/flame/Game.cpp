#include "Game.h"
#include "struct.h"
#include <DbgHelp.h> // MiniDumpWriteDump の宣言用
#include <Windows.h>
#include <strsafe.h> // StringCchPrintfW を使っているなら
#include <utility>
using namespace Logger;
using namespace StringUtility;
using namespace Microsoft::WRL;

void Game::Initialize() {

	FlameWork::Initialize();
	camera = std::make_unique<Camera>();
	camera->SetTranslate({0.0f, 0.0f, -20.0f});
	Object3dCommon::GetInstance()->SetDefaultCamera(camera.get());
	TextureManager::GetInstance()->LoadTexture("Resources/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("Resources/monsterball.png");

	sprite = std::make_unique<Sprite>();
	sprite->Initialize("Resources/uvChecker.png");
	Audio::GetInstance().Initialize();

#pragma endregion
#pragma region それぞれのリソースの生成

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
	// 1. 初期化
	// 1. パーティクルグループを作成（テクスチャのロードなどもここで行われます）
	// 引数：グループ名, テクスチャパス
	// 2. グループ作成（テクスチャロード）
	ParticleManager::GetInstance()->CreateParticleGroup("Smoke", "Resources/uvChecker.png");
	ParticleManager::GetInstance()->CreateParticleGroup("Fire", "Resources/uvChecker.png");
	ParticleManager::GetInstance()->SetCamera(camera.get());

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
	smokeEmitter = std::make_unique<ParticleEmitter>("Smoke", emitterTransform, 5, 60.0f);
	// ※ここでは「1秒間に10回、1回につき5個発生」という設定にしています
	const float clearColor[4] = {0.1f, 0.25f, 0.5f, 1.0f}; // 青色
	                                                       // メッセージループ
#pragma endregion

	bool useTexture = true;
	SoundData fanfare = {};
	Audio::GetInstance().LoadWave(L"Resources/fanfare.wav", fanfare);
	// Audio::GetInstance().Play(fanfare, 1);
}

void Game::Update() {
	// メッセージを取得
	FlameWork::Update();
	Input::GetInstance()->Update();

	if (Input::GetInstance()->TriggerKey(DIK_0)) {
		OutputDebugStringA("Hit 0\n");
	}
	if (Input::GetInstance()->TriggerKey(DIK_ESCAPE)) {
		endRequest = true;
	}

	// 更新
	smokeEmitter->Update(1.0f / 60.0f);
	ParticleManager::GetInstance()->Update();
	camera->Update();
	camera->Update();
	object3d->Update();
	for (auto& axisObj : axisObjects) {
		axisObj->Update();
	}

	sprite->Update();
	for (auto& s : sprites) {
		s->Update();
		s->SetSize({100.0f, 100.0f});
	}
#ifdef USE_IMGUI
	ImGuiUpdate();
#endif
}

void Game::Draw() {
#pragma region コマンドリストのリセット

	SpriteCommon::GetInstance()->GetDxCommon()->PreDraw();
	// モデルの描画
	Object3dCommon::GetInstance()->SetDraw();
	// object3d->Draw();
	//   複数axis.obj描画
	for (auto& axisObj : axisObjects) {
		axisObj->Draw();
	}
	ParticleManager::GetInstance()->Draw(camera.get());

	// スプライトの描画
	SpriteCommon::GetInstance()->SetDraw();
	// sprite->Draw();
	for (auto& s : sprites) {
		// s->Draw();
	}
	ImGuiManager::GetInstance()->Draw();

	SpriteCommon::GetInstance()->GetDxCommon()->PostDraw();
#pragma endregion
}

void Game::Finalize() {
	// ImGuiの終了処理
	// --- 終了処理 ---
	Audio::GetInstance().Finalize();
	FlameWork::Finalize();

}

void Game::ImGuiUpdate() {
	ImGuiManager::GetInstance()->Begin();
	cameraPosition = camera->GetTranslate();
	cameraRotate = camera->GetRotate();

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
	ImGui::DragFloat3("camera pos", &cameraPosition.x, 0.1f);
	ImGui::SliderAngle("camera rotate x", &cameraRotate.x);
	ImGui::SliderAngle("camera rotate y", &cameraRotate.y);
	ImGui::SliderAngle("camera rotate z", &cameraRotate.z);
	ImGui::DragFloat3("model pos", &modelPosition.x, 0.1f);
	ImGui::SliderAngle("model rotate x", &modelRotate.x);
	ImGui::SliderAngle("model rotate y", &modelRotate.y);
	ImGui::SliderAngle("model rotate z", &modelRotate.z);
	ImGui::DragFloat3("model scale", &modelScale.x, 0.1f);
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
	//  ImGuiのウィンドウを作成
	ImGuiManager::GetInstance()->End();
	camera->SetTranslate(cameraPosition);
	camera->SetRotate(cameraRotate);
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
}
