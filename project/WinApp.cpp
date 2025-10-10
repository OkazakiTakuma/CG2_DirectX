#include "WinApp.h"
#include "Resource.h"
#include "extenals/imgui/imgui.h"
#include "extenals/imgui/imgui_impl_dx12.h"
#include "extenals/imgui/imgui_impl_win32.h"

// ウィンドウプロシージャ

// Windowsアプリケーションのエントリポイント
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
		return true; // ImGuiが処理した場合はtrueを返す
	}
	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void WinApp::Initialize() {
	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);

	// ウィンドウプロシージャ
	wc.lpfnWndProc = WindowProc;
	// ウィンドウのクラス名
	wc.lpszClassName = L"CG2WindowClass";
	// インスタンスハンドル
	wc.hInstance = GetModuleHandle(nullptr);
	// カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	// ウィンドウクラスの登録
	RegisterClass(&wc);

	// ウィンドウサイズを表す構造体にクライアント領域のサイズを設定

	RECT wrc = {0, 0, kClientWidth, kClientHeight};

	// ウィンドウのクライアント領域のサイズを取得

	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
	// ウィンドウの作成
	hwnd = CreateWindow(
	    wc.lpszClassName,     // 拡張スタイル
	    L"CG2 Window",        // ウィンドウタイトル
	    WS_OVERLAPPEDWINDOW,  // スタイル
	    CW_USEDEFAULT,        // ウィンドウの位置x座標
	    CW_USEDEFAULT,        // ウィンドウの位置y座標
	    wrc.right - wrc.left, // ウィンドウの幅
	    wrc.bottom - wrc.top, // ウィンドウの高さ
	    nullptr,              // 親ウィンドウハンドル
	    nullptr,              // メニューハンドル
	    wc.hInstance,         // インスタンスハンドル
	    nullptr               // 追加のアプリケーションデータ
	);

	// ウィンドウの表示
	ShowWindow(hwnd, SW_SHOW);
};

void WinApp::Update() {}
void WinApp::Finalize() {
	CloseWindow(hwnd);
	CoUninitialize();
}
bool WinApp::ProcessMessage() {
	MSG msg = {};
	if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (msg.message == WM_QUIT) {
		return true;
	}

	return false;
};
