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
	bool IsFullscreen() const { return isFullscreen_; }
	void ToggleFullscreen();

	bool ProcessMessage();


#ifdef USE_IMGUI
	static const int32_t kClientWidth = 800;
	static const int32_t kClientHeight = 450;
#else
	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;
#endif

private:
	HWND hwnd = nullptr;
	WNDCLASS wc = {};
	WINDOWPLACEMENT windowPlacement_ = {sizeof(WINDOWPLACEMENT)};
	LONG_PTR windowStyle_ = 0;
	bool isFullscreen_ = false;
};
