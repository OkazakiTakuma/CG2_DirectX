#include "Player.h"
#include "GameObject.h"
#include "Input.h"
#include <cmath>
#include <dinput.h>

/// <summary>
/// 毎フレーム WASD 入力を見て、XZ 平面上でプレイヤーを移動します。
/// </summary>
void Player::Update() {
	GameObject* owner = GetOwner();
	if (!owner) {
		return;
	}

	Input* input = Input::GetInstance();
	Vector3 keyboardMove{0.0f, 0.0f, 0.0f};
	if (input->PushKey(DIK_W)) {
		keyboardMove.z += 1.0f;
	}
	if (input->PushKey(DIK_S)) {
		keyboardMove.z -= 1.0f;
	}
	if (input->PushKey(DIK_A)) {
		keyboardMove.x -= 1.0f;
	}
	if (input->PushKey(DIK_D)) {
		keyboardMove.x += 1.0f;
	}
	const float keyboardLength = Length(keyboardMove);
	if (keyboardLength > 0.00001f) {
		keyboardMove = {keyboardMove.x / keyboardLength, 0.0f, keyboardMove.z / keyboardLength};
	}

	const Vector3 leftStick = input->GetGamepadLeftStick();
	Vector3 move{keyboardMove.x + leftStick.x, 0.0f, keyboardMove.z + leftStick.z};

	const float moveLength = Length(move);
	if (moveLength <= 0.00001f) {
		return;
	}

	const Vector3 moveDirection = {move.x / moveLength, 0.0f, move.z / moveLength};
	const float moveAmount = moveLength > 1.0f ? 1.0f : moveLength;
	owner->GetTransform().rotate.y = std::atan2(moveDirection.x, moveDirection.z);
	owner->GetTransform().translate = owner->GetTransform().translate + (moveSpeed_ * moveAmount) * moveDirection;
}

/// <summary>
/// 現在位置をスポーンポイントへ戻します。
/// </summary>
void Player::ResetToSpawnPoint() {
	GameObject* owner = GetOwner();
	if (!owner) {
		return;
	}

	owner->GetTransform().translate = spawnPoint_;
}
