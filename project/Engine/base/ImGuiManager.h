#pragma once
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "WinApp.h"
#ifdef USE_IMGUI
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx12.h"
#include "../imgui/imgui_impl_win32.h"
#endif 

class ImGuiManager {
public:
	void Initialize([[maybe_unused]] WinApp* winApp, [[maybe_unused]] DirectXCommon* dxCommon);
	void Finalize();
	void Begin();
	void End();
	void Draw();

private:
	DirectXCommon* dxcommon = nullptr;
};
