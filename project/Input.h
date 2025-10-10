#pragma once
#include <dinput.h>
#include <Windows.h>

class Input {

public:
	void Initialize(HINSTANCE hinstance, HWND hwnd);
	void Update();
};
