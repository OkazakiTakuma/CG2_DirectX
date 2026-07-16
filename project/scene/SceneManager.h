#pragma once
#include "AbstractSceneFactory.h"

#include <memory>
#include <string>

class BaseScene;
class Camera;

class SceneManager {
public:
	SceneManager() = default;
	/// <summary>
	/// 破棄時に必要な解放処理を行います。
	/// </summary>
	~SceneManager();

	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;

	/// <summary>
	/// ChengeScene の処理を行います。
	/// </summary>
	/// <param name="sceneName">対象となるシーン名を指定します。</param>
	void ChengeScene(const std::string& sceneName);

	/// <summary>
	/// 毎フレームの状態更新を行います。
	/// </summary>
	void Update();
	/// <summary>
	/// スカイボックスの描画処理を行います。
	/// </summary>
	void DrawSkyBox();
	/// <summary>
	/// 2D 要素の描画処理を行います。
	/// </summary>
	void Draw2D();
	/// <summary>
	/// 3D 要素の描画処理を行います。
	/// </summary>
	void Draw3D();
	/// <summary>
	/// DrawEditorImGui の処理を行います。
	/// </summary>
	void DrawEditorImGui();

	void SetSceneFactory(AbstractSceneFactory* factory) { sceneFactory_ = factory; }
	void SetFallbackCamera(Camera* camera) { fallbackCamera_ = camera; }
	void SetScenePlaying(bool isPlaying) { isScenePlaying_ = isPlaying; }
	bool IsScenePlaying() const { return isScenePlaying_; }
	void RequestFrameStep() { isFrameStepRequested_ = true; }
	bool ReloadCurrentSceneJson();
	std::string GetCurrentSceneJsonPath() const;

private:
	/// <summary>
	/// FallbackCamera を現在の状態へ反映します。
	/// </summary>
	void ApplyFallbackCamera();

	std::unique_ptr<BaseScene> scene_ = nullptr;
	std::unique_ptr<BaseScene> nextScene_ = nullptr;
	AbstractSceneFactory* sceneFactory_ = nullptr;
	Camera* fallbackCamera_ = nullptr;
	std::string currentSceneName_ = "None";
	std::string nextSceneName_ = "None";
	int selectedSceneIndex_ = 0;
	bool isScenePlaying_ = true;
	bool isFrameStepRequested_ = false;
};
