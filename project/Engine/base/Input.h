#pragma once
#include "WinApp.h"
#include <Windows.h>
#include <wrl.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

class Input {
public:
	// シングルトンインスタンスの取得
	static Input* GetInstance();

	// デストラクタはpublic（プログラム終了時に破棄できるようにするため）
	~Input() = default;

	// 初期化・更新
	void Initialize(WinApp* winApp);
	void Update();

	// キー操作判定
	bool PushKey(BYTE keyNumber);
	bool TriggerKey(BYTE keyNumber);
	bool ReleaseKey(BYTE keyNumber);

private:
	// シングルトンのためコンストラクタはprivate
	Input() = default;

	// コピー禁止
	Input(const Input&) = delete;
	Input& operator=(const Input&) = delete;

    template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

private:
	ComPtr<IDirectInputDevice8> keyboard;
	ComPtr<IDirectInput8> directInput;
	BYTE key[256] = {};
	BYTE preKey[256] = {};

	WinApp* winApp = nullptr;
};