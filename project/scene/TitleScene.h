#pragma once

#include "BaseScene.h"
class TitleScene: public BaseScene {
public:
	void Initialize() override;
	void Update() override;
	void Draw2D() override;
	void Draw3D() override;
	void Finalize() override;
	void SetSceneManager(SceneManager* manager) override { sceneManager = manager; }

private:
	void ImGuiUpdate();
	SceneManager* sceneManager = nullptr;


};
