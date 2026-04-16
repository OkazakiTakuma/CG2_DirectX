#include "TitleScene.h"
#include"SceneManager.h"


void TitleScene::Initialize() {}


void TitleScene::Update() {
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		SceneManager::GetInstance()->ChengeScene("GAMEPLAY");

	}
}

void TitleScene::Draw2D() {}

void TitleScene::Draw3D() {}

void TitleScene::Finalize() {
}

void TitleScene::ImGuiUpdate() {}

