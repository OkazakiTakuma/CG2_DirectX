#include "ImGuiManager.h"

#ifdef USE_IMGUI
#include "../../../imgui/imgui_internal.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

namespace {
float ClampLayoutValue(float value, float minValue, float maxValue) {
	if (maxValue < minValue) {
		return maxValue;
	}
	if (value < minValue) {
		return minValue;
	}
	if (value > maxValue) {
		return maxValue;
	}
	return value;
}
}

ImGuiManager* ImGuiManager::GetInstance() {
	static ImGuiManager instance;
	return &instance;
}

void ImGuiManager::Initialize([[maybe_unused]] WinApp* winApp, [[maybe_unused]] DirectXCommon* dxCommon) {
#ifdef USE_IMGUI
	dxcommon = dxCommon;
	gameWindowHandle_ = winApp->GetHwnd();

	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#ifdef IMGUI_HAS_DOCK
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	io.ConfigViewportsNoAutoMerge = true;
	io.ConfigViewportsNoTaskBarIcon = true;
#endif

	ImGui_ImplWin32_Init(winApp->GetHwnd());

	ImGui_ImplDX12_InitInfo initInfo{};
	SrvManager* srvManager = SrvManager::GetInstance();

	initInfo.Device = dxCommon->GetDevice().Get();
	initInfo.NumFramesInFlight = static_cast<int>(dxCommon->GetSwapChainResourceCount());
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
	initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc_handle) {
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
	gameWindowHandle_ = nullptr;
#endif
}

void ImGuiManager::Begin() {
#ifdef USE_IMGUI
	ImGui_ImplWin32_NewFrame();
	ImGui_ImplDX12_NewFrame();
	ImGui::NewFrame();
	SetupExternalEditorDockSpaces();
	DrawUtilityWindows();
#endif
}

void ImGuiManager::SetupExternalEditorDockSpaces() {
#if defined(USE_IMGUI) && defined(IMGUI_HAS_DOCK)
	static bool isLayoutBuilt = false;
	static bool wasFullscreen = false;
	static ImVec2 previousClientSize = {};
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	const ImGuiID dockspaceId = ImGui::GetID("UnityStyleEditorDockSpace");
	const ImGuiDockNodeFlags dockspaceFlags =
	    ImGuiDockNodeFlags_PassthruCentralNode |
	    ImGuiDockNodeFlags_NoDockingOverCentralNode;
	const bool isFullscreen = gameWindowHandle_ ? (GetWindowLongPtr(gameWindowHandle_, GWL_STYLE) & WS_OVERLAPPEDWINDOW) == 0 : false;
	RECT clientRect{};
	if (gameWindowHandle_) {
		GetClientRect(gameWindowHandle_, &clientRect);
	}
	const ImVec2 clientSize = ImVec2(
	    static_cast<float>(clientRect.right - clientRect.left),
	    static_cast<float>(clientRect.bottom - clientRect.top)
	);
	const ImVec2 layoutSize = clientSize.x > 0.0f && clientSize.y > 0.0f ? clientSize : viewport->Size;

	ImGui::DockSpaceOverViewport(dockspaceId, viewport, dockspaceFlags);

	if (isFullscreen != wasFullscreen || layoutSize.x != previousClientSize.x || layoutSize.y != previousClientSize.y) {
		isLayoutBuilt = false;
		wasFullscreen = isFullscreen;
		previousClientSize = layoutSize;
	}

	if (!isLayoutBuilt) {
		ImGui::DockBuilderRemoveNode(dockspaceId);
		ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace | dockspaceFlags);
		ImGui::DockBuilderSetNodePos(dockspaceId, viewport->Pos);
		ImGui::DockBuilderSetNodeSize(dockspaceId, layoutSize);

		ImGuiID mainId = dockspaceId;
		ImGuiID leftId = 0;
		ImGuiID rightId = 0;
		ImGuiID bottomId = 0;
		const float leftWidth = ClampLayoutValue(layoutSize.x * 0.18f, 140.0f, layoutSize.x * 0.35f);
		const float rightWidth = ClampLayoutValue(layoutSize.x * 0.24f, 180.0f, layoutSize.x * 0.40f);
		const float bottomHeight = ClampLayoutValue(layoutSize.y * 0.24f, 120.0f, layoutSize.y * 0.40f);
		const float widthAfterLeft = layoutSize.x - leftWidth;
		const float leftRatio = layoutSize.x > 0.0f ? leftWidth / layoutSize.x : 0.18f;
		const float rightRatio = widthAfterLeft > 0.0f ? rightWidth / widthAfterLeft : 0.24f;
		const float bottomRatio = layoutSize.y > 0.0f ? bottomHeight / layoutSize.y : 0.24f;

		ImGui::DockBuilderSplitNode(mainId, ImGuiDir_Left, leftRatio, &leftId, &mainId);
		ImGui::DockBuilderSplitNode(mainId, ImGuiDir_Right, rightRatio, &rightId, &mainId);
		ImGui::DockBuilderSplitNode(mainId, ImGuiDir_Down, bottomRatio, &bottomId, &mainId);

		ImGui::DockBuilderDockWindow("Hierarchy", leftId);
		ImGui::DockBuilderDockWindow("Scene Objects", leftId);
		ImGui::DockBuilderDockWindow("Inspector", rightId);
		ImGui::DockBuilderDockWindow("Object Inspector", rightId);
		ImGui::DockBuilderDockWindow("Scene Manager", rightId);
		ImGui::DockBuilderDockWindow("PostEffect Settings", rightId);
		ImGui::DockBuilderDockWindow("Particle Editor", bottomId);
		ImGui::DockBuilderDockWindow("Project", bottomId);
		ImGui::DockBuilderDockWindow("Console", bottomId);

		ImGui::DockBuilderFinish(dockspaceId);
		isLayoutBuilt = true;
	}
#endif
}

void ImGuiManager::DrawUtilityWindows() {
#ifdef USE_IMGUI
	if (ImGui::Begin("Project")) {
		ImGui::Text("Assets");
		ImGui::Separator();
		ImGui::Text("Resources");
	}
	ImGui::End();

	if (ImGui::Begin("Console")) {
		ImGui::Text("Ready %.1f FPS", ImGui::GetIO().Framerate);
		if (gameWindowHandle_) {
			const LONG_PTR style = GetWindowLongPtr(gameWindowHandle_, GWL_STYLE);
			ImGui::Text("F11: %s", (style & WS_OVERLAPPEDWINDOW) ? "Windowed" : "Fullscreen");
		}
	}
	ImGui::End();
#endif
}

void ImGuiManager::End() {
#ifdef USE_IMGUI
	ImGui::Render();
#endif
}

void ImGuiManager::Draw() {
#ifdef USE_IMGUI
	auto commandList = dxcommon->GetCommandList();

	ID3D12DescriptorHeap* ppHeaps[] = { SrvManager::GetInstance()->GetDescriptorHeap().Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	// ============================

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());

#ifdef IMGUI_HAS_DOCK
	ImGui::UpdatePlatformWindows();
	ImGui::RenderPlatformWindowsDefault();
#endif
#endif
}
