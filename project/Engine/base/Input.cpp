#include "Input.h"
#include <cassert>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

using namespace Microsoft::WRL;

Input* Input::GetInstance() {
	static Input instance;
	return &instance;
}

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

void Input::Update() {
	memcpy(preKey, key, sizeof(key));
	preMouseState = mouseState;
	mouseWheelDelta = 0;
	mouseMoveX = 0;
	mouseMoveY = 0;

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

bool Input::PushMouseButton(int buttonIndex) const {
	if (buttonIndex < 0 || buttonIndex >= 8) {
		return false;
	}
	return (mouseState.rgbButtons[buttonIndex] & 0x80) != 0;
}

bool Input::TriggerMouseButton(int buttonIndex) const {
	if (buttonIndex < 0 || buttonIndex >= 8) {
		return false;
	}
	return (mouseState.rgbButtons[buttonIndex] & 0x80) != 0 && (preMouseState.rgbButtons[buttonIndex] & 0x80) == 0;
}
