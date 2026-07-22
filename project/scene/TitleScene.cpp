#include "TitleScene.h"
#include"SceneManager.h"


void TitleScene::Initialize() {  }


/// <summary>
/// 毎フレームの状態更新を行います。
/// </summary>
void TitleScene::Update() {
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		sceneManager->ChangeScene("PLAYER_SELECT");

	}
}

void TitleScene::Draw2D() {}

void TitleScene::Draw3D() {}

/// <summary>
/// 確保したリソースを解放し、終了処理を行います。
/// </summary>
void TitleScene::Finalize() {
	BaseScene::Finalize();
}

void TitleScene::ImGuiUpdate() {}

