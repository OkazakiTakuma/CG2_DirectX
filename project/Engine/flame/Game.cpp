#include "Game.h"
#include "Audio.h"
#include "ImGuiManager.h"
#include "Input.h"
#include "Object3dCommon.h"
#include "ParticleManager.h"
#include "PostEffect.h"
#include "SceneFactory.h"
#include "SceneManager.h"
#include "SkyBoxCommon.h"
#include <DbgHelp.h> // MiniDumpWriteDump の宣言用
#include <strsafe.h> // StringCchPrintfW を使っているなら

void Game::Initialize() {

	FlameWork::Initialize();
	camera = std::make_unique<Camera>();
	camera->SetTranslate({0.0f, 0.0f, -20.0f});
	Object3dCommon::GetInstance()->SetDefaultCamera(camera.get());
	// 1. 初期化
	// 1. パーティクルグループを作成（テクスチャのロードなどもここで行われます）
	// 引数：グループ名, テクスチャパス
	// 2. グループ作成（テクスチャロード）
	ParticleManager::GetInstance()->SetCamera(camera.get());
	sceneFactory = std::make_unique<SceneFactory>();
	SceneManager::GetInstance()->SetSceneFactory(sceneFactory.get());
	SceneManager::GetInstance()->ChengeScene("TITLE");
	SkyBoxCommon::GetInstance()->SetDefaultCamera(camera.get());
}

void Game::Update() {
	// ====== 【ここを追加】 ======
	// どのシーンでも必ず毎フレーム最初にImGuiの準備を開始する
	ImGuiManager::GetInstance()->Begin();
	// ============================

	// メッセージを取得
	FlameWork::Update();
	if (FlameWork::IsEndRequest()) {
		return;
	}
	Input::GetInstance()->Update();

	// シーンの更新
	SceneManager::GetInstance()->Update();

	if (Input::GetInstance()->TriggerKey(DIK_ESCAPE)) {
		endRequest = true;
	}
}

void Game::Draw() {
#pragma region コマンドリストのリセット

	PostEffect::GetInstance()->PreDrawScene();

	// モデルの描画
	Object3dCommon::GetInstance()->SetDraw();
	SceneManager::GetInstance()->Draw3D();
	// スカイボックスの描画
	SkyBoxCommon::GetInstance()->SetDraw();
	SceneManager::GetInstance()->DrawSkyBox();

	SpriteCommon::GetInstance()->SetDraw(BlendMode::kBlendModeNone);
	SceneManager::GetInstance()->Draw2D();

	PostEffect::GetInstance()->PostDrawScene();

	
	SkyBoxCommon::GetInstance()->GetDxCommon()->PreDraw();

	PostEffect::GetInstance()->Draw();
	ImGuiManager::GetInstance()->End();
	ImGuiManager::GetInstance()->Draw();
	// スプライトの描画

	SpriteCommon::GetInstance()->GetDxCommon()->PostDraw();

#pragma endregion
}

void Game::Finalize() {
	// ImGuiの終了処理
	// --- 終了処理 ---
	Audio::GetInstance().Finalize();
	FlameWork::Finalize();
}
