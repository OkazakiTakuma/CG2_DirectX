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
	static ImGuiManager* GetInstance();

	~ImGuiManager() = default;

	void Initialize(WinApp* winApp, DirectXCommon* dxCommon);
	void Finalize();

	void Begin();
	void End();

	void Draw();

private:
	ImGuiManager() = default;

	ImGuiManager(const ImGuiManager&) = delete;
	ImGuiManager& operator=(const ImGuiManager&) = delete;

private:
	DirectXCommon* dxcommon = nullptr;
};
