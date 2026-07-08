#include "SceneManager.h"

#include "Object3dCommon.h"
#include "ParticleManager.h"
#include "BaseScene.h"
#include "SpriteCommon.h"
#include "SkyBoxCommon.h"

#include <assert.h>

namespace {
const char* kSceneNames[] = {
    "TITLE",
    "GAMEPLAY"
};

int FindSceneIndex(const std::string& sceneName) {
	for (int index = 0; index < static_cast<int>(_countof(kSceneNames)); ++index) {
		if (sceneName == kSceneNames[index]) {
			return index;
		}
	}
	return 0;
}
}

void SceneManager::Update() {
	if (nextScene_) {
		if (scene_) {
			ApplyFallbackCamera();
			scene_->Finalize();
			scene_.reset();
		}

		scene_ = std::move(nextScene_);
		nextScene_ = nullptr;
		currentSceneName_ = nextSceneName_;
		selectedSceneIndex_ = FindSceneIndex(currentSceneName_);

		scene_->SetSceneName(currentSceneName_);
		scene_->SetFallbackCamera(fallbackCamera_);
		scene_->Initialize();
		scene_->LoadEditorObjects();
	}

	if (scene_) {
		scene_->Update();
		scene_->UpdateSceneObjects();
		DrawEditorImGui();
		scene_->DrawEditorImGui();
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
		SpriteCommon::GetInstance()->SetDraw(kBlendModeNone);
		scene_->DrawSceneObjects2D();
	}
}

void SceneManager::Draw3D() {
	if (scene_) {
		scene_->Draw3D();
		Object3dCommon::GetInstance()->SetDraw();
		scene_->DrawSceneObjects3D();
	}
}

void SceneManager::DrawEditorImGui() {
#ifdef USE_IMGUI
	ImGui::Begin("Scene Manager");

	ImGui::Text("Current: %s", currentSceneName_.c_str());
	if (nextScene_) {
		ImGui::Text("Next: %s", nextSceneName_.c_str());
	}

	ImGui::Separator();
	ImGui::Combo("Scene", &selectedSceneIndex_, kSceneNames, _countof(kSceneNames));

	const bool canChange = !nextScene_ && currentSceneName_ != kSceneNames[selectedSceneIndex_];
	if (!canChange) {
		ImGui::BeginDisabled();
	}
	if (ImGui::Button("Change Scene")) {
		ChengeScene(kSceneNames[selectedSceneIndex_]);
	}
	if (!canChange) {
		ImGui::EndDisabled();
	}

	ImGui::End();
#endif
}

SceneManager::~SceneManager() {
	if (scene_) {
		ApplyFallbackCamera();
		scene_->Finalize();
		scene_.reset();
	}
}

void SceneManager::ApplyFallbackCamera() {
	if (!fallbackCamera_) {
		return;
	}

	Object3dCommon::GetInstance()->SetDefaultCamera(fallbackCamera_);
	SkyBoxCommon::GetInstance()->SetDefaultCamera(fallbackCamera_);
	ParticleManager::GetInstance()->SetCamera(fallbackCamera_);
}

void SceneManager::ChengeScene(const std::string& sceneName) {
	assert(sceneFactory_);
	assert(nextScene_ == nullptr);

	nextScene_ = sceneFactory_->CreateScene(sceneName);
	assert(nextScene_);
	nextSceneName_ = sceneName;
	nextScene_->SetSceneManager(this);
}
