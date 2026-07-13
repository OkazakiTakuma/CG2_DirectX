#pragma once
#include "WinApp.h"
#include <Windows.h>
#include <wrl.h>
#include "Vector.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

class Input {
public:
	/// <summary>
	/// 共有インスタンスを取得します。
	/// </summary>
	/// <returns>処理結果を返します。</returns>
	static Input* GetInstance();

	~Input() = default;

	/// <summary>
	/// 必要なリソースを準備し、オブジェクトを初期化します。
	/// </summary>
	/// <param name="winApp">ウィンドウ管理オブジェクトを指定します。</param>
	void Initialize(WinApp* winApp);
	/// <summary>
	/// 毎フレームの状態更新を行います。
	/// </summary>
	void Update();

	/// <summary>
	/// PushKey の処理を行います。
	/// </summary>
	/// <param name="keyNumber">keyNumber に使用する値を指定します。</param>
	/// <returns>処理結果を返します。</returns>
	bool PushKey(BYTE keyNumber);
	/// <summary>
	/// TriggerKey の処理を行います。
	/// </summary>
	/// <param name="keyNumber">keyNumber に使用する値を指定します。</param>
	/// <returns>処理結果を返します。</returns>
	bool TriggerKey(BYTE keyNumber);
	/// <summary>
	/// ReleaseKey の処理を行います。
	/// </summary>
	/// <param name="keyNumber">keyNumber に使用する値を指定します。</param>
	/// <returns>処理結果を返します。</returns>
	bool ReleaseKey(BYTE keyNumber);
	LONG GetMouseWheelDelta() const { return mouseWheelDelta; }
	LONG GetMouseMoveX() const { return mouseMoveX; }
	LONG GetMouseMoveY() const { return mouseMoveY; }
	/// <summary>
	/// PushMouseButton の処理を行います。
	/// </summary>
	/// <param name="buttonIndex">buttonIndex に使用する値を指定します。</param>
	/// <returns>処理結果を返します。</returns>
	bool PushMouseButton(int buttonIndex) const;
	/// <summary>
	/// TriggerMouseButton の処理を行います。
	/// </summary>
	/// <param name="buttonIndex">buttonIndex に使用する値を指定します。</param>
	/// <returns>処理結果を返します。</returns>
	bool TriggerMouseButton(int buttonIndex) const;
	LONG GetMouseClientX() const { return mouseClientPosition.x; }
	LONG GetMouseClientY() const { return mouseClientPosition.y; }
	LONG GetClientWidth() const { return clientWidth; }
	LONG GetClientHeight() const { return clientHeight; }
	bool IsGamepadConnected() const { return isGamepadConnected_; }
	Vector3 GetGamepadLeftStick() const { return gamepadLeftStick_; }

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
	bool isGamepadConnected_ = false;
	Vector3 gamepadLeftStick_{0.0f, 0.0f, 0.0f};

	WinApp* winApp = nullptr;
};
