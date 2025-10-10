#pragma once
#include <Windows.h>
#include <cstdint>
class WinApp {
public:
	void Initialize();
	void Update();
	void Finalize();
	HWND GetHwnd() const { return hwnd; }
	HINSTANCE GetHInstance() const { return wc.hInstance; }

	bool ProcessMessage();

	// クライアント領域のサイズを指定

	static const int32_t kClientWidth = 1280; // クライアント領域の幅

	static const int32_t kClientHeight = 720; // クライアント領域の高さ

private:
	HWND hwnd = nullptr;
	// ウィンドウクラスの設定
	WNDCLASS wc = {};
};
