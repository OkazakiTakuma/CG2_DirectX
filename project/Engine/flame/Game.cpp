#include "Game.h"
#include "ImGuiManager.h"
#include "Input.h"
#include "LineDrawer.h"
#include "Object3dCommon.h"
#include "ParticleManager.h"
#include"LineCommon.h"
#include "PostEffect.h"
#include "SceneFactory.h"
#include "SceneManager.h"
#include "SkyBoxCommon.h"
#include <DbgHelp.h>
#include <strsafe.h>

void Game::Initialize() {

	FlameWork::Initialize();
	camera = std::make_unique<Camera>();
	camera->SetTranslate({ 0.0f, 0.0f, -20.0f });
	Object3dCommon::GetInstance()->SetDefaultCamera(camera.get());

	ParticleManager::GetInstance()->SetCamera(camera.get());
	sceneFactory = std::make_unique<SceneFactory>();
	sceneManager = std::make_unique<SceneManager>();
	sceneManager->SetSceneFactory(sceneFactory.get());
	sceneManager->ChengeScene("TITLE");
	SkyBoxCommon::GetInstance()->SetDefaultCamera(camera.get());
}

void Game::Update() {
	ImGuiManager::GetInstance()->Begin();
	// ============================

	FlameWork::Update();
	if (FlameWork::IsEndRequest()) {
		return;
	}
	Input::GetInstance()->Update();

	if (Input::GetInstance()->TriggerKey(DIK_F11)) {
		ToggleFullscreen();
	}

	sceneManager->Update();

	if (Input::GetInstance()->TriggerKey(DIK_ESCAPE)) {
		endRequest = true;
	}
}

void Game::Draw() {
#pragma region Setup

	PostEffect::GetInstance()->PreDrawScene();

	Object3dCommon::GetInstance()->SetDraw();
	sceneManager->Draw3D();
	LineDrawer::GetInstance()->Draw(Object3dCommon::GetInstance()->GetDefaultCamera());
	SkyBoxCommon::GetInstance()->SetDraw();
	sceneManager->DrawSkyBox();

	SpriteCommon::GetInstance()->SetDraw(BlendMode::kBlendModeNone);
	sceneManager->Draw2D();

	PostEffect::GetInstance()->PostDrawScene();


	SkyBoxCommon::GetInstance()->GetDxCommon()->PreDraw();

	PostEffect::GetInstance()->Draw();
	PostEffect::GetInstance()->DrawImGui();
	ImGuiManager::GetInstance()->End();
	ImGuiManager::GetInstance()->Draw();

	SpriteCommon::GetInstance()->GetDxCommon()->PostDraw();

#pragma endregion
}

void Game::Finalize() {
	sceneManager.reset();
	sceneFactory.reset();
	camera.reset();
	FlameWork::Finalize();
}
