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
	// シングルトンインスタンスの取得
	static ImGuiManager* GetInstance();

	// デストラクタはpublic
	~ImGuiManager() = default;

	// 初期化・終了処理
	void Initialize(WinApp* winApp, DirectXCommon* dxCommon);
	void Finalize();

	// フレーム開始・終了
	void Begin();
	void End();

	// 描画
	void Draw();

private:
	// シングルトンのためコンストラクタはprivate
	ImGuiManager() = default;

	// コピー禁止
	ImGuiManager(const ImGuiManager&) = delete;
	ImGuiManager& operator=(const ImGuiManager&) = delete;

private:
	DirectXCommon* dxcommon = nullptr;
};