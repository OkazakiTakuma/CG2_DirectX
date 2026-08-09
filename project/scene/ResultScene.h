#pragma once

#include "BaseScene.h"

/// <summary>最終ボス撃破後にステージクリアを表示します。</summary>
class ResultScene : public BaseScene {
public:
	void Initialize() override;
	void Update() override;
	void Draw2D() override;
	void Draw3D() override {}
	void Finalize() override;
	void SetSceneManager(SceneManager* manager) override { sceneManager_ = manager; }

private:
	SceneManager* sceneManager_ = nullptr;
	bool isStageClear_ = false;
	std::unique_ptr<Sprite> backgroundSprite_;
	std::unique_ptr<Sprite> panelSprite_;
	std::unique_ptr<GameObject> clearTextObject_;
	std::unique_ptr<GameObject> resultTextObject_;
	std::unique_ptr<GameObject> attackEquipmentTextObject_;
	std::unique_ptr<GameObject> statusEquipmentTextObject_;
	std::unique_ptr<GameObject> instructionTextObject_;
};
