#pragma once
#include "DirectXCommon.h"
#include "PerformanceMonitor.h"
#include "SrvManager.h"
#include "WinApp.h"

#ifdef USE_IMGUI
#include "../../../imgui/imgui.h"
#include "../../../imgui/imgui_impl_dx12.h"
#include "../../../imgui/imgui_impl_win32.h"
#endif

class ImGuiManager {
public:
	/// <summary>
	/// 共有インスタンスを取得します。
	/// </summary>
	/// <returns>処理結果を返します。</returns>
	static ImGuiManager* GetInstance();

	~ImGuiManager() = default;

	/// <summary>
	/// 必要なリソースを準備し、オブジェクトを初期化します。
	/// </summary>
	/// <param name="winApp">ウィンドウ管理オブジェクトを指定します。</param>
	/// <param name="dxCommon">DirectX 共通処理へアクセスするための参照を指定します。</param>
	void Initialize(WinApp* winApp, DirectXCommon* dxCommon);
	/// <summary>
	/// 確保したリソースを解放し、終了処理を行います。
	/// </summary>
	void Finalize();

	/// <summary>
	/// Begin の処理を行います。
	/// </summary>
	void Begin();
	/// <summary>
	/// End の処理を行います。
	/// </summary>
	void End();

	/// <summary>
	/// 現在の状態をもとに描画処理を行います。
	/// </summary>
	void Draw();

private:
	ImGuiManager() = default;
	/// <summary>
	/// upExternalEditorDockSpaces を設定します。
	/// </summary>
	void SetupExternalEditorDockSpaces();
	/// <summary>
	/// DrawUtilityWindows の処理を行います。
	/// </summary>
	void DrawUtilityWindows();

	ImGuiManager(const ImGuiManager&) = delete;
	ImGuiManager& operator=(const ImGuiManager&) = delete;

private:
	DirectXCommon* dxcommon = nullptr;
	HWND gameWindowHandle_ = nullptr;
	PerformanceMonitor performanceMonitor_;
};
