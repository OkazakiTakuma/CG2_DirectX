#pragma once
#include "AbstractSceneFactory.h"

#include <memory>

class BaseScene;

class SceneManager {
public:
	// シングルトンインスタンスの取得
	static SceneManager* GetInstance();

	// デストラクタは破棄のためにpublic
	~SceneManager();

	// シーン切り替え予約
	void ChengeScene(const std::string& sceneName);

	// 更新・描画
	void Update();
	void DrawSkyBox();
	void Draw2D();
	void Draw3D();

	// シーンファクトリーのセット
	void SetSceneFactory(AbstractSceneFactory* factory) { sceneFactory_ = factory; }

private:
	// シングルトンのためコンストラクタはprivate
	SceneManager() = default;

	// コピー禁止
	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;

private:
	std::unique_ptr<BaseScene> scene_ = nullptr;
	std::unique_ptr<BaseScene> nextScene_ = nullptr;
	AbstractSceneFactory* sceneFactory_ = nullptr;
};