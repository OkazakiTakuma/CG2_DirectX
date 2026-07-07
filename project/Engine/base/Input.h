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
	LONG mouseWheelDelta = 0;

	WinApp* winApp = nullptr;
};
