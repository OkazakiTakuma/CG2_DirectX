#include "SceneManager.h"

#include "Object3dCommon.h"
#include "ParticleManager.h"
#include "BaseScene.h"
#include "Input.h"
#include "SpriteCommon.h"
#include "SkyBoxCommon.h"

#include <assert.h>

namespace {
const char* kSceneNames[] = {
    "TITLE",
    "GAMEPLAY"
};

/// <summary>
/// SceneIndex を検索して取得します。
/// </summary>
/// <param name="sceneName">対象となるシーン名を指定します。</param>
/// <returns>処理結果を返します。</returns>
int FindSceneIndex(const std::string& sceneName) {
	for (int index = 0; index < static_cast<int>(_countof(kSceneNames)); ++index) {
		if (sceneName == kSceneNames[index]) {
			return index;
		}
	}
	return 0;
}
}

/// <summary>
/// 毎フレームの状態更新を行います。
/// </summary>
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
		const bool shouldAdvanceSceneTime = isScenePlaying_ || isFrameStepRequested_;
		if (shouldAdvanceSceneTime) {
			scene_->Update();
			scene_->UpdateSceneObjects();
		} else {
			scene_->UpdateEditorTools();
		}
		isFrameStepRequested_ = false;
		DrawEditorImGui();
		scene_->DrawEditorImGui();
	}
}

/// <summary>
/// スカイボックスの描画処理を行います。
/// </summary>
void SceneManager::DrawSkyBox() {
	if (scene_) {
		scene_->DrawSkyBox();
	}
}

/// <summary>
/// 2D 要素の描画処理を行います。
/// </summary>
void SceneManager::Draw2D() {
	if (scene_) {
		scene_->Draw2D();
		SpriteCommon::GetInstance()->SetDraw(kBlendModeNone);
		scene_->DrawSceneObjects2D();
	}
}

/// <summary>
/// 3D 要素の描画処理を行います。
/// </summary>
void SceneManager::Draw3D() {
	if (scene_) {
		scene_->Draw3D();
		Object3dCommon::GetInstance()->SetDraw();
		scene_->DrawSceneObjects3D();
	}
}

/// <summary>
/// DrawEditorImGui の処理を行います。
/// </summary>
void SceneManager::DrawEditorImGui() {
#ifdef USE_IMGUI
	Input* input = Input::GetInstance();
	if (!ImGui::GetIO().WantCaptureKeyboard) {
		if (input && input->TriggerKey(DIK_P)) {
			isScenePlaying_ = !isScenePlaying_;
		}
		if (input && input->TriggerKey(DIK_O)) {
			isFrameStepRequested_ = true;
			isScenePlaying_ = false;
		}
	}

	ImGui::Begin("Editor Toolbar", nullptr, ImGuiWindowFlags_NoCollapse);

	ImGui::Text("Scene: %s", currentSceneName_.c_str());
	ImGui::SameLine();
	if (nextScene_) {
		ImGui::Text("Next: %s", nextSceneName_.c_str());
		ImGui::SameLine();
	}

	ImGui::SetNextItemWidth(160.0f);
	ImGui::Combo("##Scene", &selectedSceneIndex_, kSceneNames, _countof(kSceneNames));
	ImGui::SameLine();

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
	ImGui::SameLine();
	if (ImGui::Button(isScenePlaying_ ? "Pause" : "Play")) {
		isScenePlaying_ = !isScenePlaying_;
	}
	ImGui::SameLine();
	const bool canStepFrame = !isScenePlaying_;
	if (!canStepFrame) {
		ImGui::BeginDisabled();
	}
	if (ImGui::Button("Step")) {
		isFrameStepRequested_ = true;
	}
	if (!canStepFrame) {
		ImGui::EndDisabled();
	}
	ImGui::SameLine();
	ImGui::Text("%s", isScenePlaying_ ? "Playing" : "Paused");

	ImGui::End();
#endif
}

/// <summary>
/// 破棄時に必要な解放処理を行います。
/// </summary>
SceneManager::~SceneManager() {
	if (scene_) {
		ApplyFallbackCamera();
		scene_->Finalize();
		scene_.reset();
	}
}

/// <summary>
/// FallbackCamera を現在の状態へ反映します。
/// </summary>
void SceneManager::ApplyFallbackCamera() {
	if (!fallbackCamera_) {
		return;
	}

	Object3dCommon::GetInstance()->SetDefaultCamera(fallbackCamera_);
	SkyBoxCommon::GetInstance()->SetDefaultCamera(fallbackCamera_);
	ParticleManager::GetInstance()->SetCamera(fallbackCamera_);
}

/// <summary>
/// ChengeScene の処理を行います。
/// </summary>
/// <param name="sceneName">対象となるシーン名を指定します。</param>
void SceneManager::ChengeScene(const std::string& sceneName) {
	assert(sceneFactory_);
	assert(nextScene_ == nullptr);

	nextScene_ = sceneFactory_->CreateScene(sceneName);
	assert(nextScene_);
	nextSceneName_ = sceneName;
	nextScene_->SetSceneManager(this);
}
