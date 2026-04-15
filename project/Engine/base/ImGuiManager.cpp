#include "ImGuiManager.h"

#ifdef USE_IMGUI
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

// 静적インスタンスの取得
ImGuiManager* ImGuiManager::GetInstance() {
	static ImGuiManager instance;
	return &instance;
}

void ImGuiManager::Initialize([[maybe_unused]] WinApp* winApp, [[maybe_unused]] DirectXCommon* dxCommon) {
#ifdef USE_IMGUI
	dxcommon = dxCommon;

	// コンテキスト作成
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	// プラットフォームとレンダラーの初期化
	ImGui_ImplWin32_Init(winApp->GetHwnd());

	ImGui_ImplDX12_InitInfo initInfo{};
	SrvManager* srvManager = SrvManager::GetInstance();

	initInfo.Device = dxCommon->GetDevice().Get();
	initInfo.NumFramesInFlight = dxCommon->GetSwapChainResourceCount();
	initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	initInfo.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	initInfo.SrvDescriptorHeap = srvManager->GetDescriptorHeap().Get();

	// デスクリプタ割り当て関数 (SrvManagerを利用)
	initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle) {
		SrvManager* srvmanager = SrvManager::GetInstance();
		uint32_t index = srvmanager->Allocate();
		*out_cpu_desc_handle = srvmanager->GetCPUDescriptorHandle(index);
		*out_gpu_desc_handle = srvmanager->GetGPUDescriptorHandle(index);
	};

	initInfo.CommandQueue = dxCommon->GetCommandQueue().Get();
	initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc_handle) {
		// 解放が必要な場合はここに記述
	};

	ImGui_ImplDX12_Init(&initInfo);
#endif
}

void ImGuiManager::Finalize() {
#ifdef USE_IMGUI
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	dxcommon = nullptr;
#endif
}

void ImGuiManager::Begin() {
#ifdef USE_IMGUI
	ImGui_ImplWin32_NewFrame();
	ImGui_ImplDX12_NewFrame();
	ImGui::NewFrame();
#endif
}

void ImGuiManager::End() {
#ifdef USE_IMGUI
	ImGui::Render();
#endif
}

void ImGuiManager::Draw() {
#ifdef USE_IMGUI
	// デスクリプタヒープをセット
	SrvManager::GetInstance()->preDraw();

	ID3D12GraphicsCommandList* commandList = dxcommon->GetCommandList().Get();

	// 描画実行
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
#endif
}