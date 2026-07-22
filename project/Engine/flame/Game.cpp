#include "Game.h"
#include "ImGuiManager.h"
#include "Input.h"
#include "LineDrawer.h"
#include "particle/TrailRenderer.h"
#include "object/Object3dCommon.h"
#include "particle/ParticleManager.h"
#include"LineCommon.h"
#include "PostEffect.h"
#include "SceneFactory.h"
#include "SceneManager.h"
#include "sky/SkyBoxCommon.h"
#include <DbgHelp.h>
#include <strsafe.h>

/// <summary>
/// 必要なリソースを準備し、オブジェクトを初期化します。
/// </summary>
void Game::Initialize() {

	FlameWork::Initialize();
	camera = std::make_unique<Camera>();
	camera->SetTranslate({ 0.0f, 0.0f, -20.0f });
	Object3dCommon::GetInstance()->SetDefaultCamera(camera.get());

	ParticleManager::GetInstance()->SetCamera(camera.get());
	sceneFactory = std::make_unique<SceneFactory>();
	sceneManager = std::make_unique<SceneManager>();
	sceneManager->SetFallbackCamera(camera.get());
	sceneManager->SetSceneFactory(sceneFactory.get());
	sceneManager->ChangeScene("TITLE");
	SkyBoxCommon::GetInstance()->SetDefaultCamera(camera.get());
}

/// <summary>
/// 毎フレームの状態更新を行います。
/// </summary>
void Game::Update() {
	ImGuiManager::GetInstance()->Begin();

	FlameWork::Update();
	if (FlameWork::IsEndRequest()) {
		return;
	}
	if (ImGuiManager::GetInstance()->UpdateHotReload(
		sceneManager ? sceneManager->GetCurrentSceneJsonPath() : std::string(),
		[this]() { return sceneManager && sceneManager->ReloadCurrentSceneJson(); })) {
		endRequest = true;
		return;
	}
	Input::GetInstance()->Update();

	if (Input::GetInstance()->TriggerKey(DIK_F11)) {
		ToggleFullscreen();
	}
	if (camera) {
		float aspectRatio = GetRenderAspectRatio();
#ifdef USE_IMGUI
		const float gameViewAspectRatio = ImGuiManager::GetInstance()->GetGameViewAspectRatio();
		if (gameViewAspectRatio > 0.0f) {
			aspectRatio = gameViewAspectRatio;
		}
#endif
		camera->SetAspectRatio(aspectRatio);
		camera->Update();
	}
	PostEffect::GetInstance()->UpdateHotkeys();

	sceneManager->Update();

	if (Input::GetInstance()->TriggerKey(DIK_ESCAPE)) {
		endRequest = true;
	}
}

/// <summary>
/// 現在の状態をもとに描画処理を行います。
/// </summary>
void Game::Draw() {
#pragma region Setup

	if (PostEffect::GetInstance()->IsActive()) {
		PostEffect::GetInstance()->PreDrawScene();

		SkyBoxCommon::GetInstance()->SetDraw();
		sceneManager->DrawSkyBox();

		Object3dCommon::GetInstance()->SetDraw();
		sceneManager->Draw3D();
		TrailRenderer::GetInstance()->Draw(Object3dCommon::GetInstance()->GetDefaultCamera());
		LineDrawer::GetInstance()->Draw(Object3dCommon::GetInstance()->GetDefaultCamera());

		SpriteCommon::GetInstance()->SetDraw(BlendMode::kBlendModeNone);
		sceneManager->Draw2D();

		PostEffect::GetInstance()->PostDrawScene();

		SkyBoxCommon::GetInstance()->GetDxCommon()->PreDraw();
		ImGuiManager::GetInstance()->ApplyGameViewRenderArea();

		PostEffect::GetInstance()->Draw();
	} else {
		SkyBoxCommon::GetInstance()->GetDxCommon()->PreDraw();
		ImGuiManager::GetInstance()->ApplyGameViewRenderArea();

		SkyBoxCommon::GetInstance()->SetDraw();
		sceneManager->DrawSkyBox();

		Object3dCommon::GetInstance()->SetDraw();
		sceneManager->Draw3D();
		TrailRenderer::GetInstance()->Draw(Object3dCommon::GetInstance()->GetDefaultCamera());
		LineDrawer::GetInstance()->Draw(Object3dCommon::GetInstance()->GetDefaultCamera());

		SpriteCommon::GetInstance()->SetDraw(BlendMode::kBlendModeNone);
		sceneManager->Draw2D();
	}
	ImGuiManager::GetInstance()->RestoreFullRenderArea();
	PostEffect::GetInstance()->DrawImGui();
	ImGuiManager::GetInstance()->End();
	ImGuiManager::GetInstance()->Draw();

	SpriteCommon::GetInstance()->GetDxCommon()->PostDraw();

#pragma endregion
}

/// <summary>
/// 確保したリソースを解放し、終了処理を行います。
/// </summary>
void Game::Finalize() {
	sceneManager.reset();
	sceneFactory.reset();
	camera.reset();
	FlameWork::Finalize();
}
