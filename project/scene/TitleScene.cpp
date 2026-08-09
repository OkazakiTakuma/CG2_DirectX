#include "TitleScene.h"
#include"SceneManager.h"
#include <Xinput.h>


void TitleScene::Initialize() {  }


/// <summary>
/// 毎フレームの状態更新を行います。
/// </summary>
void TitleScene::Update() {
	Input* input = Input::GetInstance();
	if (input->TriggerKey(DIK_SPACE) || input->TriggerKey(DIK_RETURN) ||
		input->TriggerGamepadButton(XINPUT_GAMEPAD_A) ||
		input->TriggerGamepadButton(XINPUT_GAMEPAD_START)) {
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

