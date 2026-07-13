#include "Input.h"
#include <cassert>
#include <cmath>
#include <Xinput.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "xinput.lib")

using namespace Microsoft::WRL;

namespace {
float NormalizeRawGamepadAxis(SHORT value) {
	const float maxValue = value < 0 ? 32768.0f : 32767.0f;
	float normalized = static_cast<float>(value) / maxValue;
	if (normalized < -1.0f) {
		normalized = -1.0f;
	}
	if (normalized > 1.0f) {
		normalized = 1.0f;
	}
	return normalized;
}

Vector3 NormalizeGamepadLeftStick(SHORT rawX, SHORT rawY) {
	const float x = NormalizeRawGamepadAxis(rawX);
	const float z = NormalizeRawGamepadAxis(rawY);
	const float length = std::sqrt((x * x) + (z * z));
	const float deadZone = static_cast<float>(XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) / 32767.0f;
	if (length <= deadZone) {
		return {0.0f, 0.0f, 0.0f};
	}

    const float clampedLength = length > 1.0f ? 1.0f : length;
    const float adjustedLength = (clampedLength - deadZone) / (1.0f - deadZone);
	const float directionX = x / length;
	const float directionZ = z / length;
	return {directionX * adjustedLength, 0.0f, directionZ * adjustedLength};
}
}

/// <summary>
/// 共有インスタンスを取得します。
/// </summary>
/// <returns>処理結果を返します。</returns>
Input* Input::GetInstance() {
	static Input instance;
	return &instance;
}

/// <summary>
/// 必要なリソースを準備し、オブジェクトを初期化します。
/// </summary>
/// <param name="winApp">ウィンドウ管理オブジェクトを指定します。</param>
void Input::Initialize(WinApp* winApp) {
	this->winApp = winApp;
	HRESULT hr;

    hr = DirectInput8Create(
		winApp->GetHInstance(),
		DIRECTINPUT_VERSION,
		IID_IDirectInput8,
		reinterpret_cast<void**>(directInput.GetAddressOf()),
		nullptr
	);
	assert(SUCCEEDED(hr));

	hr = directInput->CreateDevice(GUID_SysKeyboard, keyboard.GetAddressOf(), NULL);
	assert(SUCCEEDED(hr));

	hr = keyboard->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(hr));

	hr = keyboard->SetCooperativeLevel(winApp->GetHwnd(), DISCL_NONEXCLUSIVE | DISCL_NOWINKEY | DISCL_FOREGROUND);
	assert(SUCCEEDED(hr));

	hr = directInput->CreateDevice(GUID_SysMouse, mouse.GetAddressOf(), NULL);
	assert(SUCCEEDED(hr));

	hr = mouse->SetDataFormat(&c_dfDIMouse2);
	assert(SUCCEEDED(hr));

	hr = mouse->SetCooperativeLevel(winApp->GetHwnd(), DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
	assert(SUCCEEDED(hr));
}

/// <summary>
/// 毎フレームの状態更新を行います。
/// </summary>
void Input::Update() {
	memcpy(preKey, key, sizeof(key));
	preMouseState = mouseState;
	mouseWheelDelta = 0;
	mouseMoveX = 0;
	mouseMoveY = 0;
	isGamepadConnected_ = false;
	gamepadLeftStick_ = {0.0f, 0.0f, 0.0f};

	HRESULT hr = keyboard->Acquire();

	hr = keyboard->GetDeviceState(sizeof(key), key);

	if (FAILED(hr)) {
		keyboard->Acquire();
		keyboard->GetDeviceState(sizeof(key), key);
	}

	if (mouse) {
		hr = mouse->Acquire();
		hr = mouse->GetDeviceState(sizeof(mouseState), &mouseState);

		if (FAILED(hr)) {
			mouse->Acquire();
			hr = mouse->GetDeviceState(sizeof(mouseState), &mouseState);
		}

		if (SUCCEEDED(hr)) {
			mouseMoveX = mouseState.lX;
			mouseMoveY = mouseState.lY;
			mouseWheelDelta = mouseState.lZ;
		}
	}

	XINPUT_STATE gamepadState{};
	if (XInputGetState(0, &gamepadState) == ERROR_SUCCESS) {
		isGamepadConnected_ = true;
		const XINPUT_GAMEPAD& gamepad = gamepadState.Gamepad;
		gamepadLeftStick_ = NormalizeGamepadLeftStick(gamepad.sThumbLX, gamepad.sThumbLY);
	}

	if (winApp && winApp->GetHwnd()) {
		POINT cursorPosition{};
		if (GetCursorPos(&cursorPosition)) {
			ScreenToClient(winApp->GetHwnd(), &cursorPosition);
			mouseClientPosition = cursorPosition;
		}

		RECT clientRect{};
		if (GetClientRect(winApp->GetHwnd(), &clientRect)) {
			clientWidth = clientRect.right - clientRect.left;
			clientHeight = clientRect.bottom - clientRect.top;
			if (clientWidth <= 0) {
				clientWidth = 1;
			}
			if (clientHeight <= 0) {
				clientHeight = 1;
			}
		}
	}
}

bool Input::PushKey(BYTE keyNumber) { return key[keyNumber] & 0x80; }

bool Input::TriggerKey(BYTE keyNumber) { return (key[keyNumber] & 0x80) && !(preKey[keyNumber] & 0x80); }

bool Input::ReleaseKey(BYTE keyNumber) { return !(key[keyNumber] & 0x80) && (preKey[keyNumber] & 0x80); }

/// <summary>
/// PushMouseButton の処理を行います。
/// </summary>
/// <param name="buttonIndex">buttonIndex に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
bool Input::PushMouseButton(int buttonIndex) const {
	if (buttonIndex < 0 || buttonIndex >= 8) {
		return false;
	}
	return (mouseState.rgbButtons[buttonIndex] & 0x80) != 0;
}

/// <summary>
/// TriggerMouseButton の処理を行います。
/// </summary>
/// <param name="buttonIndex">buttonIndex に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
bool Input::TriggerMouseButton(int buttonIndex) const {
	if (buttonIndex < 0 || buttonIndex >= 8) {
		return false;
	}
	return (mouseState.rgbButtons[buttonIndex] & 0x80) != 0 && (preMouseState.rgbButtons[buttonIndex] & 0x80) == 0;
}
