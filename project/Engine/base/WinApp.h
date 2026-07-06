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


	static const int32_t kClientWidth = 1280;

	static const int32_t kClientHeight = 720;

private:
	HWND hwnd = nullptr;
	WNDCLASS wc = {};
};
