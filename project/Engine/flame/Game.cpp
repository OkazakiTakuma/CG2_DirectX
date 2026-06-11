#include "Game.h"
#include "SceneFactory.h"
#include "struct.h"
#include <DbgHelp.h> // MiniDumpWriteDump の宣言用
#include <Windows.h>
#include <strsafe.h> // StringCchPrintfW を使っているなら
#include <utility>
#include"PostEffect.h"
using namespace Logger;
using namespace StringUtility;
using namespace Microsoft::WRL;

void Game::Initialize() {

	FlameWork::Initialize();
	camera = std::make_unique<Camera>();
	camera->SetTranslate({0.0f, 0.0f, -20.0f});
	Object3dCommon::GetInstance()->SetDefaultCamera(camera.get());
	// 1. 初期化
	// 1. パーティクルグループを作成（テクスチャのロードなどもここで行われます）
	// 引数：グループ名, テクスチャパス
	// 2. グループ作成（テクスチャロード）
	ParticleManager::GetInstance()->CreateParticleGroup("Smoke", "Resources/uvChecker.png");
	ParticleManager::GetInstance()->CreateParticleGroup("Fire", "Resources/uvChecker.png");
	ParticleManager::GetInstance()->CreateParticleGroup("Slash", "Resources/uvChecker.png");
	ParticleManager::GetInstance()->SetGroupBlendMode("Slash", kBlendModeAdd);
	ParticleManager::GetInstance()->SetCamera(camera.get());
	sceneFactory = new SceneFactory();
	SceneManager::GetInstance()->SetSceneFactory(sceneFactory);
	SceneManager::GetInstance()->ChengeScene("TITLE");
	SkyBoxCommon::GetInstance()->SetDefaultCamera(camera.get());
}

void Game::Update() {
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
	// スカイボックスの描画
	SkyBoxCommon::GetInstance()->SetDraw();
	SceneManager::GetInstance()->DrawSkyBox();

	// モデルの描画
	Object3dCommon::GetInstance()->SetDraw();
	SceneManager::GetInstance()->Draw3D();
	SpriteCommon::GetInstance()->SetDraw(BlendMode::kBlendModeAdd);
	SceneManager::GetInstance()->Draw2D();


	PostEffect::GetInstance()->PostDrawScene();

	
	SkyBoxCommon::GetInstance()->GetDxCommon()->PreDraw();

	PostEffect::GetInstance()->Draw();

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
