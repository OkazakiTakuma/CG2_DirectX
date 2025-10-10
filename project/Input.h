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
	/// <summary>
	/// キーを押しているか
	/// </summary>
	/// <param name="key">キー番号</param>
	/// <returns>押されているか</returns>
	bool PushKey(BYTE keyNumber);

	/// <summary>
	/// キーを押したか
	/// </summary>
	/// <param name="keyNumber">キー番号</param>
	/// <returns>押したか</returns>
	bool TriggerKey(BYTE keyNumber);

	/// <summary>
	/// キーを離したか
	/// </summary>
	/// <param name="keyNumber">キー番号</param>
	/// <returns>離したか</returns>
	bool ReleaseKey(BYTE keyNumber);

private:
	ComPtr<IDirectInputDevice8> keyboard;
	ComPtr<IDirectInput8> directInput = nullptr;
	BYTE key[256] = {};
	BYTE preKey[256] = {};
};
