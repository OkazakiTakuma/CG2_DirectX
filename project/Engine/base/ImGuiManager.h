#pragma once
#include "DirectXCommon.h"
#include "PerformanceMonitor.h"
#include "SrvManager.h"
#include "WinApp.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <functional>
#include <future>

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

	void Begin();
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
	float GetGameViewAspectRatio() const;
	bool ApplyGameViewRenderArea();
	void RestoreFullRenderArea();
	bool UpdateHotReload(const std::string& sceneJsonPath, const std::function<bool()>& reloadScene);

private:
	ImGuiManager() = default;
	void SetupExternalEditorDockSpaces();
	void DrawUtilityWindows();
	void DrawGameViewWindow();
	void DrawEditorBackgroundMask(const ImVec2& gameViewPosition, const ImVec2& gameViewSize);
	bool CalculateGameViewRenderRect(float& left, float& top, float& width, float& height) const;
	void LoadGameFonts();
	bool ReloadShaders();
	bool ReloadTextures();
	void StartCppBuild();
	bool PollCppBuild();
	bool LaunchRebuiltExecutable();
	bool DetectFileChanges(const std::filesystem::path& root, const std::vector<std::string>& extensions, std::unordered_map<std::string, std::filesystem::file_time_type>& timestamps, bool recursive = true);

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
	bool isGameViewVisible_ = false;
	bool autoReloadShaders_ = true;
	bool autoReloadScene_ = true;
	bool autoReloadTextures_ = true;
	bool autoReloadCpp_ = false;
	bool cppBuildRunning_ = false;
	bool restartRequested_ = false;
	std::future<int> cppBuildFuture_;
	std::filesystem::path rebuiltExecutablePath_;
	std::string hotReloadStatus_ = "Ready";
	std::string hotReloadError_;
	std::unordered_map<std::string, std::filesystem::file_time_type> shaderTimestamps_;
	std::unordered_map<std::string, std::filesystem::file_time_type> sceneTimestamps_;
	std::unordered_map<std::string, std::filesystem::file_time_type> textureTimestamps_;
	std::unordered_map<std::string, std::filesystem::file_time_type> cppTimestamps_;
};
