#include "ImGuiManager.h"
#ifdef USE_IMGUI
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif // USE_IMGUI

void ImGuiManager::Initialize([[maybe_unused]] WinApp* winApp, [[maybe_unused]] DirectXCommon* dxCommon) {
#ifdef USE_IMGUI
	dxcommon = dxCommon;
	ImGui::CreateContext();

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(winApp->GetHwnd());

	ImGui_ImplDX12_InitInfo initInfo{};
	SrvManager* srvManager = SrvManager::GetInstance();
	initInfo.Device = dxCommon->GetDevice().Get();
	initInfo.NumFramesInFlight = dxCommon->GetSwapChainResourceCount();
	initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	initInfo.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	initInfo.SrvDescriptorHeap = srvManager->GetDescriptorHeap().Get();
	initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle) {
		SrvManager* srvmanager = SrvManager::GetInstance();
		uint32_t index = srvmanager->Allocate();
		*out_cpu_desc_handle = srvmanager->GetCPUDescriptorHandle(index);
		*out_gpu_desc_handle = srvmanager->GetGPUDescriptorHandle(index);
	};
	initInfo.CommandQueue = dxCommon->GetCommandQueue().Get();
	SrvManager* srvmanager = SrvManager::GetInstance();
	initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc_handle) {};

	ImGui_ImplDX12_Init(&initInfo);
#endif // !USE_IMGUI

	
}

void ImGuiManager::Finalize() {
#ifdef USE_IMGUI
	// 1. ImGui内部のDX12リソース（フォントテクスチャやパイプライン）を解放
	ImGui_ImplDX12_Shutdown();

	// 2. プラットフォーム固有の終了処理
	ImGui_ImplWin32_Shutdown();

	// 3. コンテキストの破棄
	ImGui::DestroyContext();

	// 4. 保持しているポインタをクリア（安全のため）
	dxcommon = nullptr;
#endif // USE_IMGUI
}
void ImGuiManager::Begin() {
#ifdef USE_IMGUI

	ImGui_ImplWin32_NewFrame();
	ImGui_ImplDX12_NewFrame();
	ImGui::NewFrame();
#endif // !USE_IMGUI
}

void ImGuiManager::End() {
#ifdef USE_IMGUI

	ImGui::Render();
#endif // !USE_IMGUI

}

void ImGuiManager::Draw() {
#ifdef USE_IMGUI

	// SrvManager経由でデスクリプタヒープをコマンドリストにセットする
	SrvManager::GetInstance()->preDraw();
	ID3D12GraphicsCommandList* commandList = dxcommon->GetCommandList().Get();

	// ImGuiの描画データをコマンドリストに積む
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
#endif // !USE_IMGUI

	
}