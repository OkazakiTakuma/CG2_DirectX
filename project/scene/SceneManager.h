#pragma once
#include "AbstractSceneFactory.h"

#include <memory>

class BaseScene;

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

	void SetSceneFactory(AbstractSceneFactory* factory) { sceneFactory_ = factory; }

private:
	std::unique_ptr<BaseScene> scene_ = nullptr;
	std::unique_ptr<BaseScene> nextScene_ = nullptr;
	AbstractSceneFactory* sceneFactory_ = nullptr;
};
