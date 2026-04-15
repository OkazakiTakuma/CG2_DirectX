#include "Input.h"
#include <cassert>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

using namespace Microsoft::WRL;

// 静的インスタンスの取得
Input* Input::GetInstance() {
	static Input instance;
	return &instance;
}

void Input::Initialize(WinApp* winApp) {
	this->winApp = winApp;
	HRESULT hr;

	// DirectInputの初期化 (引数は5つ必要です)
    hr = DirectInput8Create(
		winApp->GetHInstance(),             // インスタンスハンドル
		DIRECTINPUT_VERSION,                // バージョン
		IID_IDirectInput8,                  // インターフェースID
		reinterpret_cast<void**>(directInput.GetAddressOf()), // ★GetAddressOfを使用
		nullptr                             // 外部オブジェクト(通常はnullptr)
	);
	assert(SUCCEEDED(hr));

	// キーボードデバイスの生成
	hr = directInput->CreateDevice(GUID_SysKeyboard, keyboard.GetAddressOf(), NULL); // ★GetAddressOfを使用
	assert(SUCCEEDED(hr));

	// 入力データ形式のセット
	hr = keyboard->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(hr));

	// 排他制御レベルのセット
	hr = keyboard->SetCooperativeLevel(winApp->GetHwnd(), DISCL_NONEXCLUSIVE | DISCL_NOWINKEY | DISCL_FOREGROUND);
	assert(SUCCEEDED(hr));
}

void Input::Update() {
	// 前回の入力を保存
	memcpy(preKey, key, sizeof(key));

	// キーボード情報の取得開始 (失敗しても次回のAcquireで復帰を試みるため、戻り値チェックは任意)
	HRESULT hr = keyboard->Acquire();

	// キーボードの状態を取得
	hr = keyboard->GetDeviceState(sizeof(key), key);

	// アプリが非アクティブなどで入力を失った(DIERR_INPUTLOST)場合、再取得を試みる
	if (FAILED(hr)) {
		keyboard->Acquire();
		keyboard->GetDeviceState(sizeof(key), key);
	}
}

bool Input::PushKey(BYTE keyNumber) { return key[keyNumber] & 0x80; }

bool Input::TriggerKey(BYTE keyNumber) { return (key[keyNumber] & 0x80) && !(preKey[keyNumber] & 0x80); }

bool Input::ReleaseKey(BYTE keyNumber) { return !(key[keyNumber] & 0x80) && (preKey[keyNumber] & 0x80); }