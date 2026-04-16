#include "SceneManager.h"
#include "BaseScene.h"
#include <assert.h>

// 静的インスタンスの取得
SceneManager* SceneManager::GetInstance() {
	static SceneManager instance;
	return &instance;
}

void SceneManager::Update() {
	// シーン切り替えがある場合
	if (nextScene_) {
		if (scene_) {
			scene_->Finalize();
			scene_.reset();
		}

		scene_ = std::move(nextScene_);
		nextScene_ = nullptr; // 明示的にクリア

		scene_->Initialize();
	}

	// 現在のシーンを更新（初期化直後も含めて実行）
	if (scene_) {
		scene_->Update();
	}
}

void SceneManager::Draw2D() {
	if (scene_) {
		scene_->Draw2D();
	}
}

void SceneManager::Draw3D() {
	if (scene_) {
		scene_->Draw3D();
	}
}

SceneManager::~SceneManager() {
	if (scene_) {
		scene_->Finalize();
		scene_.reset();
	}
}

void SceneManager::ChengeScene(const std::string& sceneName) {
	assert(sceneFactory_);
	assert(nextScene_ == nullptr);
	nextScene_ = sceneFactory_->CreateScene(sceneName);


}
