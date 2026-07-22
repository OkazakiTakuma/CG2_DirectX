#include "WinApp.h"
#include "ImGuiManager.h"
#include "Resource.h"
#ifdef USE_IMGUI
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif // USE_IMGUI

LRESULT CALLBACK WinApp::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
#ifdef USE_IMGUI

	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
		return true;
	}
#endif
	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		break;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

/// <summary>
/// 必要なリソースを準備し、オブジェクトを初期化します。
/// </summary>
void WinApp::Initialize() {
	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);

	wc.lpfnWndProc = &WinApp::WindowProc;
	wc.lpszClassName = L"CG2WindowClass";
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	RegisterClass(&wc);


	RECT wrc = {0, 0, kClientWidth, kClientHeight};


	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
#ifdef USE_IMGUI
	RECT workArea{};
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
	const int32_t windowWidth = wrc.right - wrc.left;
	const int32_t windowHeight = wrc.bottom - wrc.top;
	const int32_t x = workArea.left + 8;
	const int32_t y = workArea.top + 8;
#else
	const int32_t windowWidth = wrc.right - wrc.left;
	const int32_t windowHeight = wrc.bottom - wrc.top;
	const int32_t x = CW_USEDEFAULT;
	const int32_t y = CW_USEDEFAULT;
#endif
	hwnd = CreateWindow(
	    wc.lpszClassName,
	    L"CG2 Window",
	    WS_OVERLAPPEDWINDOW,
	    x,
	    y,
	    windowWidth,
	    windowHeight,
	    nullptr,
	    nullptr,
	    wc.hInstance,
	    nullptr
	);

	ShowWindow(hwnd, SW_SHOW);
	windowStyle_ = GetWindowLongPtr(hwnd, GWL_STYLE);
	GetWindowPlacement(hwnd, &windowPlacement_);
	UpdateClientSize();
};

void WinApp::Update() {
	UpdateClientSize();
}

void WinApp::UpdateClientSize() {
	if (!hwnd) {
		clientWidth_ = kClientWidth;
		clientHeight_ = kClientHeight;
		return;
	}

	RECT clientRect{};
	if (GetClientRect(hwnd, &clientRect)) {
		clientWidth_ = clientRect.right - clientRect.left;
		clientHeight_ = clientRect.bottom - clientRect.top;
		if (clientWidth_ <= 0) {
			clientWidth_ = 1;
		}
		if (clientHeight_ <= 0) {
			clientHeight_ = 1;
		}
	}
}

/// <summary>
/// Fullscreen の状態を切り替えます。
/// </summary>
void WinApp::ToggleFullscreen() {
	if (!hwnd) {
		return;
	}

	if (!isFullscreen_) {
		windowStyle_ = GetWindowLongPtr(hwnd, GWL_STYLE);
		GetWindowPlacement(hwnd, &windowPlacement_);

		HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO monitorInfo = {sizeof(MONITORINFO)};
		GetMonitorInfo(monitor, &monitorInfo);

		SetWindowLongPtr(hwnd, GWL_STYLE, windowStyle_ & ~WS_OVERLAPPEDWINDOW);
		SetWindowPos(
		    hwnd,
		    HWND_TOP,
		    monitorInfo.rcMonitor.left,
		    monitorInfo.rcMonitor.top,
		    monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
		    monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
		    SWP_NOOWNERZORDER | SWP_FRAMECHANGED
		);
		isFullscreen_ = true;
		UpdateClientSize();
		return;
	}

	SetWindowLongPtr(hwnd, GWL_STYLE, windowStyle_);
	SetWindowPlacement(hwnd, &windowPlacement_);
	SetWindowPos(
	    hwnd,
	    nullptr,
	    0,
	    0,
	    0,
	    0,
	    SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED
	);
	isFullscreen_ = false;
	UpdateClientSize();
}

/// <summary>
/// 確保したリソースを解放し、終了処理を行います。
/// </summary>
void WinApp::Finalize() {
	CloseWindow(hwnd);
	CoUninitialize();
}
/// <summary>
/// Windows メッセージを処理し、終了要求の有無を返します。
/// </summary>
bool WinApp::ProcessMessage() {
	MSG msg = {};

	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		if (msg.message == WM_QUIT) {
			return true;
		}
	}

	UpdateClientSize();
	return false;
}
