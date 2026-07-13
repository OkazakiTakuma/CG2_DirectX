#pragma once
#include <Windows.h>
#include <cstdint>
class WinApp {
public:
	/// <summary>
	/// 必要なリソースを準備し、オブジェクトを初期化します。
	/// </summary>
	void Initialize();
	/// <summary>
	/// 毎フレームの状態更新を行います。
	/// </summary>
	void Update();
	/// <summary>
	/// 確保したリソースを解放し、終了処理を行います。
	/// </summary>
	void Finalize();
	HWND GetHwnd() const { return hwnd; }
	HINSTANCE GetHInstance() const { return wc.hInstance; }
	bool IsFullscreen() const { return isFullscreen_; }
	int32_t GetClientWidth() const { return clientWidth_; }
	int32_t GetClientHeight() const { return clientHeight_; }
	void UpdateClientSize();
	/// <summary>
	/// Fullscreen の状態を切り替えます。
	/// </summary>
	void ToggleFullscreen();

	/// <summary>
	/// Windows メッセージを処理し、終了要求の有無を返します。
	/// </summary>
	/// <returns>処理結果を返します。</returns>
	bool ProcessMessage();


	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;

private:
	HWND hwnd = nullptr;
	WNDCLASS wc = {};
	WINDOWPLACEMENT windowPlacement_ = {sizeof(WINDOWPLACEMENT)};
	LONG_PTR windowStyle_ = 0;
	int32_t clientWidth_ = kClientWidth;
	int32_t clientHeight_ = kClientHeight;
	bool isFullscreen_ = false;
};
