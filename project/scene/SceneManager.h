#pragma once
#include "AbstractSceneFactory.h"

#include <memory>
#include <string>

class BaseScene;
class Camera;

class SceneManager {
public:
	SceneManager() = default;
	~SceneManager();

	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;

	void ChengeScene(const std::string& sceneName);

	void Update();
	void DrawSkyBox();
	void Draw2D();
	void Draw3D();
	void DrawEditorImGui();

	void SetSceneFactory(AbstractSceneFactory* factory) { sceneFactory_ = factory; }
	void SetFallbackCamera(Camera* camera) { fallbackCamera_ = camera; }

private:
	void ApplyFallbackCamera();

	std::unique_ptr<BaseScene> scene_ = nullptr;
	std::unique_ptr<BaseScene> nextScene_ = nullptr;
	AbstractSceneFactory* sceneFactory_ = nullptr;
	Camera* fallbackCamera_ = nullptr;
	std::string currentSceneName_ = "None";
	std::string nextSceneName_ = "None";
	int selectedSceneIndex_ = 0;
};
