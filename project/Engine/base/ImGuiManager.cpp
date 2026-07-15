#include "ImGuiManager.h"

#ifdef USE_IMGUI
#include "../../../imgui/imgui_internal.h"
#include <array>
#include <filesystem>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

namespace {
/// <summary>
/// ClampLayoutValue の処理を行います。
/// </summary>
/// <param name="value">計算に使用する値を指定します。</param>
/// <param name="minValue">範囲判定に使用する値を指定します。</param>
/// <param name="maxValue">範囲判定に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
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

#ifdef USE_IMGUI
/// <summary>
/// ImGui に日本語グリフを含むフォントを設定します。
/// </summary>
ImFont* LoadJapaneseFont() {
	ImGuiIO& io = ImGui::GetIO();
	const std::array<const char*, 5> fontPaths = {
	    "C:/Windows/Fonts/meiryo.ttc",
	    "C:/Windows/Fonts/YuGothM.ttc",
	    "C:/Windows/Fonts/YuGothR.ttc",
	    "C:/Windows/Fonts/msgothic.ttc",
	    "C:/Windows/Fonts/meiryob.ttc",
	};

	ImFontConfig fontConfig{};
	fontConfig.MergeMode = false;
	fontConfig.PixelSnapH = true;
	fontConfig.OversampleH = 2;
	fontConfig.OversampleV = 2;

	for (const char* fontPath : fontPaths) {
		if (!std::filesystem::exists(fontPath)) {
			continue;
		}

		ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath, 18.0f, &fontConfig, io.Fonts->GetGlyphRangesJapanese());
		if (font) {
			io.FontDefault = font;
			return font;
		}
	}

	ImFont* defaultFont = io.Fonts->AddFontDefault();
	io.FontDefault = defaultFont;
	return defaultFont;
}
#endif
}

ImGuiManager* ImGuiManager::GetInstance() {
	static ImGuiManager instance;
	return &instance;
}

/// <summary>
/// 必要なリソースを準備し、オブジェクトを初期化します。
/// </summary>
/// <param name="winApp">ウィンドウ管理オブジェクトを指定します。</param>
/// <param name="dxCommon">DirectX 共通処理へアクセスするための参照を指定します。</param>
void ImGuiManager::Initialize([[maybe_unused]] WinApp* winApp, [[maybe_unused]] DirectXCommon* dxCommon) {
#ifdef USE_IMGUI
	dxcommon = dxCommon;
	gameWindowHandle_ = winApp->GetHwnd();

	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	LoadJapaneseFont();
	LoadGameFonts();
#ifdef IMGUI_HAS_DOCK
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
	io.ConfigViewportsNoAutoMerge = false;
	io.ConfigViewportsNoTaskBarIcon = false;
#endif

	ImGui_ImplWin32_Init(winApp->GetHwnd());

	ImGui_ImplDX12_InitInfo initInfo{};
	SrvManager* srvManager = SrvManager::GetInstance();

	initInfo.Device = dxCommon->GetDevice().Get();
	initInfo.NumFramesInFlight = static_cast<int>(dxCommon->GetSwapChainResourceCount());
	initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	initInfo.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	initInfo.SrvDescriptorHeap = srvManager->GetDescriptorHeap().Get();

	/// <summary>
	/// [] の処理を行います。
	/// </summary>
	/// <param name="info">info に使用する値を指定します。</param>
	/// <param name="out_cpu_desc_handle">out_cpu_desc_handle に使用する値を指定します。</param>
	/// <param name="out_gpu_desc_handle">out_gpu_desc_handle に使用する値を指定します。</param>
	/// <returns>処理結果を返します。</returns>
	initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle) {
		SrvManager* srvmanager = SrvManager::GetInstance();
		uint32_t index = srvmanager->Allocate();
		*out_cpu_desc_handle = srvmanager->GetCPUDescriptorHandle(index);
		*out_gpu_desc_handle = srvmanager->GetGPUDescriptorHandle(index);
	};

	initInfo.CommandQueue = dxCommon->GetCommandQueue().Get();
	/// <summary>
	/// [] の処理を行います。
	/// </summary>
	/// <param name="info">info に使用する値を指定します。</param>
	/// <param name="cpu_desc_handle">cpu_desc_handle に使用する値を指定します。</param>
	/// <param name="gpu_desc_handle">gpu_desc_handle に使用する値を指定します。</param>
	/// <returns>処理結果を返します。</returns>
	initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc_handle) {
	};

	ImGui_ImplDX12_Init(&initInfo);
	performanceMonitor_.Initialize();
#endif
}

void ImGuiManager::LoadGameFonts() {
#ifdef USE_IMGUI
	fontNames_.clear();
	fonts_.clear();

	ImGuiIO& io = ImGui::GetIO();
	auto registerFont = [this](const std::string& name, ImFont* font) {
		if (!font || fonts_.contains(name)) {
			return;
		}
		fonts_[name] = font;
		fontNames_.push_back(name);
	};

	registerFont("Default", io.FontDefault ? io.FontDefault : io.Fonts->AddFontDefault());

	ImFontConfig fontConfig{};
	fontConfig.MergeMode = false;
	fontConfig.PixelSnapH = true;
	fontConfig.OversampleH = 2;
	fontConfig.OversampleV = 2;

	const std::array<std::pair<const char*, const char*>, 5> windowsFonts = {
	    std::pair<const char*, const char*>{"Meiryo", "C:/Windows/Fonts/meiryo.ttc"},
	    std::pair<const char*, const char*>{"Yu Gothic", "C:/Windows/Fonts/YuGothM.ttc"},
	    std::pair<const char*, const char*>{"Yu Gothic Regular", "C:/Windows/Fonts/YuGothR.ttc"},
	    std::pair<const char*, const char*>{"MS Gothic", "C:/Windows/Fonts/msgothic.ttc"},
	    std::pair<const char*, const char*>{"Meiryo Bold", "C:/Windows/Fonts/meiryob.ttc"},
	};
	for (const auto& [name, path] : windowsFonts) {
		if (!std::filesystem::exists(path)) {
			continue;
		}
		registerFont(name, io.Fonts->AddFontFromFileTTF(path, 32.0f, &fontConfig, io.Fonts->GetGlyphRangesJapanese()));
	}

	const std::filesystem::path fontDirectory = "Resources/Fonts";
	if (std::filesystem::exists(fontDirectory)) {
		for (const auto& entry : std::filesystem::recursive_directory_iterator(fontDirectory)) {
			if (!entry.is_regular_file()) {
				continue;
			}
			const std::string extension = entry.path().extension().string();
			if (extension != ".ttf" && extension != ".otf" && extension != ".ttc") {
				continue;
			}
			const std::string name = entry.path().stem().string();
			const std::string path = entry.path().generic_string();
			registerFont(name, io.Fonts->AddFontFromFileTTF(path.c_str(), 32.0f, &fontConfig, io.Fonts->GetGlyphRangesJapanese()));
		}
	}
#endif
}

/// <summary>
/// 確保したリソースを解放し、終了処理を行います。
/// </summary>
void ImGuiManager::Finalize() {
#ifdef USE_IMGUI
	performanceMonitor_.Finalize();
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	dxcommon = nullptr;
	gameWindowHandle_ = nullptr;
#endif
}

/// <summary>
/// Begin の処理を行います。
/// </summary>
void ImGuiManager::Begin() {
#ifdef USE_IMGUI
	performanceMonitor_.Update();
	ImGui_ImplWin32_NewFrame();
	ImGui_ImplDX12_NewFrame();
	ImGui::NewFrame();
	SetupExternalEditorDockSpaces();
	DrawGameViewWindow();
	DrawUtilityWindows();
#endif
}

/// <summary>
/// upExternalEditorDockSpaces を設定します。
/// </summary>
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
	const float topHeight = ClampLayoutValue(layoutSize.y * 0.08f, 44.0f, 72.0f);
	const float leftWidth = ClampLayoutValue(layoutSize.x * 0.18f, 140.0f, layoutSize.x * 0.35f);
	const float rightWidth = ClampLayoutValue(layoutSize.x * 0.24f, 180.0f, layoutSize.x * 0.40f);
	const float bottomHeight = ClampLayoutValue(layoutSize.y * 0.24f, 120.0f, layoutSize.y * 0.40f);
	const float heightAfterTop = layoutSize.y - topHeight;
	const float widthAfterLeft = layoutSize.x - leftWidth;
	const float topRatio = layoutSize.y > 0.0f ? topHeight / layoutSize.y : 0.08f;
	const float leftRatio = layoutSize.x > 0.0f ? leftWidth / layoutSize.x : 0.18f;
	const float rightRatio = widthAfterLeft > 0.0f ? rightWidth / widthAfterLeft : 0.24f;
	const float bottomRatio = heightAfterTop > 0.0f ? bottomHeight / heightAfterTop : 0.24f;

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::DockSpaceOverViewport(dockspaceId, viewport, dockspaceFlags);
	ImGui::PopStyleColor();

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
		ImGuiID topId = 0;
		ImGuiID leftId = 0;
		ImGuiID rightId = 0;
		ImGuiID bottomId = 0;

		ImGui::DockBuilderSplitNode(mainId, ImGuiDir_Up, topRatio, &topId, &mainId);
		ImGui::DockBuilderSplitNode(mainId, ImGuiDir_Left, leftRatio, &leftId, &mainId);
		ImGui::DockBuilderSplitNode(mainId, ImGuiDir_Right, rightRatio, &rightId, &mainId);
		ImGui::DockBuilderSplitNode(mainId, ImGuiDir_Down, bottomRatio, &bottomId, &mainId);

		ImGui::DockBuilderDockWindow("GameView", mainId);
		ImGui::DockBuilderDockWindow("Editor Toolbar", topId);
		ImGui::DockBuilderDockWindow("Component Manager", leftId);
		ImGui::DockBuilderDockWindow("Hierarchy", leftId);
		ImGui::DockBuilderDockWindow("Scene Objects", leftId);
		ImGui::DockBuilderDockWindow("Component Inspector", rightId);
		ImGui::DockBuilderDockWindow("Player Inspector", rightId);
		ImGui::DockBuilderDockWindow("Player Attack Inspector", rightId);
		ImGui::DockBuilderDockWindow("Inspector", rightId);
		ImGui::DockBuilderDockWindow("Object Inspector", rightId);
		ImGui::DockBuilderDockWindow("Scene Manager", rightId);
		ImGui::DockBuilderDockWindow("PostEffect Settings", bottomId);
		ImGui::DockBuilderDockWindow("Post Effects", bottomId);
		ImGui::DockBuilderDockWindow("Particle Editor", bottomId);
		ImGui::DockBuilderDockWindow("Project", bottomId);
		ImGui::DockBuilderDockWindow("Console", bottomId);

		ImGui::DockBuilderFinish(dockspaceId);
		isLayoutBuilt = true;
	}
#endif
}

/// <summary>
/// DrawUtilityWindows の処理を行います。
/// </summary>
void ImGuiManager::DrawUtilityWindows() {
#ifdef USE_IMGUI
	if (ImGui::Begin("Console")) {
		ImGui::Text("Ready %.1f FPS", ImGui::GetIO().Framerate);
		const float cpuUsage = performanceMonitor_.GetCpuUsagePercent();
		ImGui::Text("CPU Usage : %.1f%%", cpuUsage);
		ImGui::ProgressBar(cpuUsage / 100.0f, ImVec2(-1.0f, 0.0f));
		if (performanceMonitor_.IsGpuUsageAvailable()) {
			const float gpuUsage = performanceMonitor_.GetGpuUsagePercent();
			const float gpu3DUsage = performanceMonitor_.GetGpu3DUsagePercent();
			ImGui::Text("GPU Usage : %.1f%%", gpuUsage);
			ImGui::ProgressBar(gpuUsage / 100.0f, ImVec2(-1.0f, 0.0f));
			ImGui::Text("GPU 3D    : %.1f%%", gpu3DUsage);
			ImGui::ProgressBar(gpu3DUsage / 100.0f, ImVec2(-1.0f, 0.0f));
		} else {
			ImGui::Text("GPU Usage : N/A");
		}
		if (gameWindowHandle_) {
			const LONG_PTR style = GetWindowLongPtr(gameWindowHandle_, GWL_STYLE);
			ImGui::Text("F11: %s", (style & WS_OVERLAPPEDWINDOW) ? "Windowed" : "Fullscreen");
		}
	}
	ImGui::End();
#endif
}

void ImGuiManager::DrawGameViewWindow() {
#ifdef USE_IMGUI
	const ImGuiWindowFlags windowFlags =
	    ImGuiWindowFlags_NoBackground |
	    ImGuiWindowFlags_NoScrollbar |
	    ImGuiWindowFlags_NoScrollWithMouse;

	ImGui::SetNextWindowBgAlpha(0.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
#ifdef IMGUI_HAS_DOCK
	ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
#endif
	if (ImGui::Begin("GameView", nullptr, windowFlags)) {
		gameViewContentPosition_ = ImGui::GetCursorScreenPos();
		gameViewContentSize_ = ImGui::GetContentRegionAvail();
		DrawEditorBackgroundMask(ImGui::GetWindowPos(), ImGui::GetWindowSize());
		if (ImGui::BeginDragDropTarget()) {
			auto acceptAssetPayload = [this](const char* payloadType, DroppedAssetPayload::Type assetType) {
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadType);
				if (!payload || !payload->Data || payload->DataSize <= 0) {
					return;
				}
				droppedAssetPayload_.type = assetType;
				droppedAssetPayload_.path.assign(static_cast<const char*>(payload->Data), static_cast<size_t>(payload->DataSize));
				if (!droppedAssetPayload_.path.empty() && droppedAssetPayload_.path.back() == '\0') {
					droppedAssetPayload_.path.pop_back();
				}
				hasDroppedAssetPayload_ = true;
			};
			acceptAssetPayload("CG2_ASSET_MODEL", DroppedAssetPayload::Type::Model);
			acceptAssetPayload("CG2_ASSET_ANIM_MODEL", DroppedAssetPayload::Type::AnimatedModel);
			acceptAssetPayload("CG2_ASSET_SPRITE", DroppedAssetPayload::Type::SpriteTexture);
			ImGui::EndDragDropTarget();
		}
	}
	ImGui::End();
#ifdef IMGUI_HAS_DOCK
	ImGui::PopStyleColor();
#endif
	ImGui::PopStyleColor(2);
#endif
}

bool ImGuiManager::ConsumeDroppedAsset(DroppedAssetPayload& outPayload) {
	if (!hasDroppedAssetPayload_) {
		return false;
	}
	outPayload = droppedAssetPayload_;
	droppedAssetPayload_ = {};
	hasDroppedAssetPayload_ = false;
	return true;
}

std::vector<std::string> ImGuiManager::GetAvailableFontNames() const {
	return fontNames_;
}

ImFont* ImGuiManager::GetFont(const std::string& fontName) const {
#ifdef USE_IMGUI
	auto it = fonts_.find(fontName);
	if (it != fonts_.end() && it->second) {
		return it->second;
	}
	auto defaultIt = fonts_.find("Default");
	if (defaultIt != fonts_.end() && defaultIt->second) {
		return defaultIt->second;
	}
	return ImGui::GetFont();
#else
	(void)fontName;
	return nullptr;
#endif
}

void ImGuiManager::DrawEditorBackgroundMask(const ImVec2& gameViewPosition, const ImVec2& gameViewSize) {
#ifdef USE_IMGUI
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	const ImVec2 viewportMin = viewport->Pos;
	const ImVec2 viewportMax = ImVec2(viewport->Pos.x + viewport->Size.x, viewport->Pos.y + viewport->Size.y);
	const ImVec2 gameMin = ImVec2(
	    ClampLayoutValue(gameViewPosition.x, viewportMin.x, viewportMax.x),
	    ClampLayoutValue(gameViewPosition.y, viewportMin.y, viewportMax.y)
	);
	const ImVec2 gameMax = ImVec2(
	    ClampLayoutValue(gameViewPosition.x + gameViewSize.x, viewportMin.x, viewportMax.x),
	    ClampLayoutValue(gameViewPosition.y + gameViewSize.y, viewportMin.y, viewportMax.y)
	);

	ImVec4 maskColor = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
	maskColor.w = 1.0f;
	ImDrawList* drawList = ImGui::GetBackgroundDrawList(viewport);
	const ImU32 color = ImGui::ColorConvertFloat4ToU32(maskColor);

	drawList->AddRectFilled(viewportMin, ImVec2(viewportMax.x, gameMin.y), color);
	drawList->AddRectFilled(ImVec2(viewportMin.x, gameMax.y), viewportMax, color);
	drawList->AddRectFilled(ImVec2(viewportMin.x, gameMin.y), ImVec2(gameMin.x, gameMax.y), color);
	drawList->AddRectFilled(ImVec2(gameMax.x, gameMin.y), ImVec2(viewportMax.x, gameMax.y), color);
#endif
}

/// <summary>
/// End の処理を行います。
/// </summary>
void ImGuiManager::End() {
#ifdef USE_IMGUI
	ImGui::Render();
#endif
}

/// <summary>
/// 現在の状態をもとに描画処理を行います。
/// </summary>
void ImGuiManager::Draw() {
#ifdef USE_IMGUI
	auto commandList = dxcommon->GetCommandList();

	ID3D12DescriptorHeap* ppHeaps[] = { SrvManager::GetInstance()->GetDescriptorHeap().Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	// ============================

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());

#endif
}
