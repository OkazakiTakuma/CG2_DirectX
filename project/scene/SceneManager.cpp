#include "SceneManager.h"

#include "BaseScene.h"

#include <assert.h>

void SceneManager::Update() {
	if (nextScene_) {
		if (scene_) {
			scene_->Finalize();
			scene_.reset();
		}

		scene_ = std::move(nextScene_);
		nextScene_ = nullptr;

		scene_->Initialize();
	}

	if (scene_) {
		scene_->Update();
	}
}

void SceneManager::DrawSkyBox() {
	if (scene_) {
		scene_->DrawSkyBox();
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
	nextScene_->SetSceneManager(this);
}
