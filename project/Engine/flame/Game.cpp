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
#include "TextComponent.h"
#include <DbgHelp.h>
#include <strsafe.h>
#include <Xinput.h>

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

	recordingIndicator_ = std::make_unique<GameObject>();
	recordingIndicator_->SetName("ScreenRecordingIndicator");
	TextComponent* recordingText = recordingIndicator_->AddComponent<TextComponent>();
	recordingText->SetText("● REC");
	recordingText->SetFontSize(28.0f);
	recordingText->SetColor({1.0f, 0.08f, 0.08f, 1.0f});
	recordingText->SetAnchor(TextComponent::Anchor::TopRight);
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

	// キーボードがなくても Pad の BACK（View）ボタンで表示モードを切り替えられる。
	if (Input::GetInstance()->TriggerKey(DIK_F11) ||
		Input::GetInstance()->TriggerGamepadButton(XINPUT_GAMEPAD_BACK)) {
		ToggleFullscreen();
	}
	if (Input::GetInstance()->TriggerKey(DIK_F10)) {
		SpriteCommon::GetInstance()->GetDxCommon()->RequestScreenshot();
	}
	if (Input::GetInstance()->TriggerKey(DIK_F9)) {
		SpriteCommon::GetInstance()->GetDxCommon()->ToggleScreenRecording();
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

	DirectXCommon* dxCommon = SpriteCommon::GetInstance()->GetDxCommon();
	if (dxCommon) {
		dxCommon->CaptureFrameBeforeOverlay();
	}
	if (recordingIndicator_ && dxCommon && dxCommon->IsScreenRecording()) {
		recordingIndicator_->GetTransform().translate = {
			static_cast<float>(dxCommon->GetRenderWidth()) - 20.0f,
			16.0f,
			0.0f};
		recordingIndicator_->Draw2D();
	}

	SpriteCommon::GetInstance()->GetDxCommon()->PostDraw();

#pragma endregion
}

/// <summary>
/// 確保したリソースを解放し、終了処理を行います。
/// </summary>
void Game::Finalize() {
	recordingIndicator_.reset();
	sceneManager.reset();
	sceneFactory.reset();
	camera.reset();
	FlameWork::Finalize();
}
