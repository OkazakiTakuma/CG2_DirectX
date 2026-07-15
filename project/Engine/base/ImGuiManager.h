#pragma once
#include "DirectXCommon.h"
#include "PerformanceMonitor.h"
#include "SrvManager.h"
#include "WinApp.h"
#include <string>
#include <unordered_map>
#include <vector>

#ifdef USE_IMGUI
#include "../../../imgui/imgui.h"
#include "../../../imgui/imgui_impl_dx12.h"
#include "../../../imgui/imgui_impl_win32.h"
#else
struct ImFont;
struct ImVec2 {
	float x = 0.0f;
	float y = 0.0f;
	ImVec2() = default;
	ImVec2(float xValue, float yValue) : x(xValue), y(yValue) {}
};
#endif

class ImGuiManager {
public:
	struct DroppedAssetPayload {
		enum class Type {
			None,
			Model,
			AnimatedModel,
			SpriteTexture
		};

		Type type = Type::None;
		std::string path;
	};

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
	bool ConsumeDroppedAsset(DroppedAssetPayload& outPayload);
	std::vector<std::string> GetAvailableFontNames() const;
	ImFont* GetFont(const std::string& fontName) const;
	ImVec2 GetGameViewContentPosition() const { return gameViewContentPosition_; }
	ImVec2 GetGameViewContentSize() const { return gameViewContentSize_; }

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
	void DrawGameViewWindow();
	void DrawEditorBackgroundMask(const ImVec2& gameViewPosition, const ImVec2& gameViewSize);
	void LoadGameFonts();

	ImGuiManager(const ImGuiManager&) = delete;
	ImGuiManager& operator=(const ImGuiManager&) = delete;

private:
	DirectXCommon* dxcommon = nullptr;
	HWND gameWindowHandle_ = nullptr;
	PerformanceMonitor performanceMonitor_;
	DroppedAssetPayload droppedAssetPayload_;
	bool hasDroppedAssetPayload_ = false;
	std::vector<std::string> fontNames_;
	std::unordered_map<std::string, ImFont*> fonts_;
	ImVec2 gameViewContentPosition_ = {};
	ImVec2 gameViewContentSize_ = {};
};
