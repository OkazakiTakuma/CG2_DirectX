#pragma once
#include "AbstractSceneFactory.h"
#include "Camera.h"
#include "FlameWork.h"
#include "SceneManager.h"
#include <memory>

class Game : public FlameWork {
public:
	void Initialize() override;
	void Update() override;
	void Draw() override;
	void Finalize() override;
	bool IsEndRequest() const override { return endRequest; }	

private:
	std::unique_ptr<Camera> camera = nullptr;
	bool endRequest = false;
	std::unique_ptr<AbstractSceneFactory> sceneFactory = nullptr;
	std::unique_ptr<SceneManager> sceneManager = nullptr;

};
