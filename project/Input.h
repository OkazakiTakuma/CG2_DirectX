#pragma once
#include <Windows.h>
#include <dinput.h>
#include <wrl.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

class Input {

public:
	void Initialize(HINSTANCE hinstance, HWND hwnd);
	void Update();
	template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

private:
	ComPtr<IDirectInputDevice8> keyboard;
	ComPtr<IDirectInput8> directInput = nullptr;
};
