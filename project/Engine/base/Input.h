#pragma once
#include "WinApp.h"
#include <Windows.h>
#include <wrl.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

class Input {
public:
	static Input* GetInstance();

	~Input() = default;

	void Initialize(WinApp* winApp);
	void Update();

	bool PushKey(BYTE keyNumber);
	bool TriggerKey(BYTE keyNumber);
	bool ReleaseKey(BYTE keyNumber);
	LONG GetMouseWheelDelta() const { return mouseWheelDelta; }
	LONG GetMouseMoveX() const { return mouseMoveX; }
	LONG GetMouseMoveY() const { return mouseMoveY; }
	bool PushMouseButton(int buttonIndex) const;
	bool TriggerMouseButton(int buttonIndex) const;
	LONG GetMouseClientX() const { return mouseClientPosition.x; }
	LONG GetMouseClientY() const { return mouseClientPosition.y; }
	LONG GetClientWidth() const { return clientWidth; }
	LONG GetClientHeight() const { return clientHeight; }

private:
	Input() = default;

	Input(const Input&) = delete;
	Input& operator=(const Input&) = delete;

    template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

private:
	ComPtr<IDirectInputDevice8> keyboard;
	ComPtr<IDirectInputDevice8> mouse;
	ComPtr<IDirectInput8> directInput;
	BYTE key[256] = {};
	BYTE preKey[256] = {};
	DIMOUSESTATE2 mouseState = {};
	DIMOUSESTATE2 preMouseState = {};
	LONG mouseWheelDelta = 0;
	LONG mouseMoveX = 0;
	LONG mouseMoveY = 0;
	POINT mouseClientPosition = {};
	LONG clientWidth = 1;
	LONG clientHeight = 1;

	WinApp* winApp = nullptr;
};
