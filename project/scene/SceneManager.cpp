#include "SceneManager.h"

#include "object/Object3dCommon.h"
#include "particle/ParticleManager.h"
#include "BaseScene.h"
#include "Input.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "TextureManager.h"
#include "sky/SkyBoxCommon.h"
#include "GameTime.h"

#include <assert.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <cmath>

namespace {
const char* kSceneNames[] = {
    "TITLE",
	"PLAYER_SELECT",
	"STAGE_SELECT",
	"GAMEPLAY",
	"RESULT"
};

const char* kTransitionNames[] = {
	"None",
	"Fade",
	"Wipe Left",
	"Wipe Right",
	"Curtains",
	"Horizontal Bars"
};

constexpr const char* kTransitionTextureKey = "__scene_transition_white";

/// <summary>
/// SceneIndex を検索して取得します。
/// </summary>
/// <param name="sceneName">対象となるシーン名を指定します。</param>
int FindSceneIndex(const std::string& sceneName) {
	for (int index = 0; index < static_cast<int>(_countof(kSceneNames)); ++index) {
		if (sceneName == kSceneNames[index]) {
			return index;
		}
	}
	return 0;
}
}

SceneManager::SceneManager() {
	RefreshGameplayStagePatterns();
	TextureManager::GetInstance()->CreateTextureFromRGBA(kTransitionTextureKey, 1, 1, {255, 255, 255, 255});
	for (auto& sprite : transitionSprites_) {
		sprite = std::make_unique<Sprite>();
		sprite->Initialize(kTransitionTextureKey);
		sprite->SetAnchorPoint({0.0f, 0.0f});
	}
}

/// <summary>
/// 毎フレームの状態更新を行います。
/// </summary>
void SceneManager::Update() {
	// 起動直後の最初のシーンだけは演出を待たずに読み込みます。
	if (nextScene_ && transitionPhase_ == TransitionPhase::Idle) {
		SwitchToNextScene();
	}
	UpdateTransition();

	if (scene_) {
		const bool shouldAdvanceSceneTime = transitionPhase_ == TransitionPhase::Idle &&
		    (isScenePlaying_ || isFrameStepRequested_) && !GameTime::IsPaused();
		if (shouldAdvanceSceneTime) {
			scene_->Update();
			scene_->UpdateSceneObjects();
			ParticleManager::GetInstance()->Update();
		} else {
			scene_->UpdateEditorTools();
		}
		isFrameStepRequested_ = false;
		scene_->DrawEditorImGui();
		// 共通ツールバーとステージパターン操作を最後に描き、シーン固有パネルより前面に表示します。
		DrawEditorImGui();
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
	DrawTransition();
}

/// <summary>
/// 3D 要素の描画処理を行います。
/// </summary>
void SceneManager::Draw3D() {
	if (scene_) {
		scene_->Draw3D();
		Object3dCommon::GetInstance()->SetDraw();
		scene_->DrawSceneObjects3D();
		Camera* camera = Object3dCommon::GetInstance()->GetDefaultCamera();
		if (camera && scene_->IsParticleRenderingEnabled()) {
			ParticleManager::GetInstance()->Draw(camera);
		}
	}
}

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
		ChangeScene(kSceneNames[selectedSceneIndex_]);
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

	int transitionType = static_cast<int>(transitionSettings_.type);
	ImGui::SetNextItemWidth(150.0f);
	if (ImGui::Combo("Transition", &transitionType, kTransitionNames, _countof(kTransitionNames))) {
		transitionSettings_.type = static_cast<SceneTransitionType>(transitionType);
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(90.0f);
	ImGui::DragFloat("Out sec", &transitionSettings_.outDuration, 0.01f, 0.0f, 5.0f, "%.2f");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(90.0f);
	ImGui::DragFloat("In sec", &transitionSettings_.inDuration, 0.01f, 0.0f, 5.0f, "%.2f");
	ImGui::SameLine();
	ImGui::ColorEdit4("Color", &transitionSettings_.color.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
	SetTransitionSettings(transitionSettings_);

	ImGui::End();
	if (currentSceneName_ != "GAMEPLAY") {
		return;
	}

	// ゲームプレイシーンの左右パネルに隠れないよう、中央上部へ独立表示します。
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	const float displayWidth = displaySize.x > 0.0f ? displaySize.x : 1280.0f;
	const float leftPanelWidth = displayWidth * 0.18f;
	const float rightPanelWidth = displayWidth * 0.24f;
	const float availableStagePanelWidth = displayWidth - leftPanelWidth - rightPanelWidth;
	const float stagePanelWidth = availableStagePanelWidth > 420.0f ? availableStagePanelWidth : 420.0f;
	const float stagePanelHeight = currentSceneName_ == "GAMEPLAY" ? 132.0f : 82.0f;
	ImGui::SetNextWindowPos(ImVec2(leftPanelWidth, 0.0f), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(stagePanelWidth, stagePanelHeight), ImGuiCond_Always);
	const ImGuiWindowFlags stagePanelFlags =
	    ImGuiWindowFlags_NoMove |
	    ImGuiWindowFlags_NoCollapse |
	    ImGuiWindowFlags_NoSavedSettings;
	ImGui::Begin("Gameplay Stage Patterns", nullptr, stagePanelFlags);

	if (gameplayStageIds_.empty()) {
		RefreshGameplayStagePatterns();
	}
	std::vector<const char*> stageNames;
	stageNames.reserve(gameplayStageIds_.size());
	for (const std::string& stageId : gameplayStageIds_) {
		stageNames.push_back(stageId.c_str());
	}
	ImGui::Text("Gameplay Stage Pattern");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(180.0f);
	if (ImGui::Combo("##GameplayStagePattern", &selectedGameplayStageIndex_, stageNames.data(), static_cast<int>(stageNames.size()))) {
		selectedGameplayStageId_ = gameplayStageIds_[selectedGameplayStageIndex_];
	}
	ImGui::SameLine();
	if (ImGui::Button("Rescan Patterns")) {
		RefreshGameplayStagePatterns();
	}
	if (currentSceneName_ == "GAMEPLAY" && scene_) {
		ImGui::SameLine();
		const bool canLoadPattern = activeGameplayStageId_ != selectedGameplayStageId_;
		if (!canLoadPattern) {
			ImGui::BeginDisabled();
		}
		if (ImGui::Button("Load Selected Pattern")) {
			activeGameplayStageId_ = selectedGameplayStageId_;
			ApplyGameplayStagePath(scene_.get(), activeGameplayStageId_);
			scene_->LoadEditorObjects();
			gameplayStageMessage_ = "Loaded: " + activeGameplayStageId_;
		}
		if (!canLoadPattern) {
			ImGui::EndDisabled();
		}

		ImGui::SetNextItemWidth(180.0f);
		ImGui::InputTextWithHint("##NewGameplayStageId", "new pattern id", newGameplayStageIdBuffer_.data(), newGameplayStageIdBuffer_.size());
		ImGui::SameLine();
		if (ImGui::Button("Duplicate Current Pattern")) {
			const std::string newStageId = newGameplayStageIdBuffer_.data();
			if (!IsValidGameplayStageId(newStageId) || newStageId == "default") {
				gameplayStageMessage_ = "Use letters, numbers, '-' or '_' (not 'default').";
			} else if (std::filesystem::exists(GetGameplayStageJsonPath(newStageId))) {
				gameplayStageMessage_ = "Pattern already exists: " + newStageId;
			} else {
				selectedGameplayStageId_ = newStageId;
				activeGameplayStageId_ = newStageId;
				ApplyGameplayStagePath(scene_.get(), activeGameplayStageId_);
				scene_->SaveEditorObjects();
				RefreshGameplayStagePatterns();
				newGameplayStageIdBuffer_.fill('\0');
				gameplayStageMessage_ = "Created: " + newStageId;
			}
		}
		ImGui::SameLine();
		ImGui::Text("Active: %s", activeGameplayStageId_.c_str());
		ImGui::SameLine();
		if (ImGui::Button("Save Current Pattern")) {
			scene_->SaveEditorObjects();
			gameplayStageMessage_ = "Saved: " + activeGameplayStageId_;
		}
	}
	if (!gameplayStageMessage_.empty()) {
		ImGui::TextUnformatted(gameplayStageMessage_.c_str());
	}

	ImGui::End();
#endif
}

bool SceneManager::ReloadCurrentSceneJson() {
	if (!scene_ || nextScene_) {
		return false;
	}
	scene_->LoadEditorObjects();
	return true;
}

std::string SceneManager::GetCurrentSceneJsonPath() const {
	if (!scene_ || currentSceneName_ == "None") {
		return {};
	}
	return scene_->GetSceneObjectFilePath();
}

void SceneManager::RefreshGameplayStagePatterns() {
	const std::string keepSelectedId = selectedGameplayStageId_;
	gameplayStageIds_.clear();
	gameplayStageIds_.push_back("default");

	const std::filesystem::path sceneDirectory = "Resources/Data/Scenes";
	std::error_code error;
	for (std::filesystem::directory_iterator iterator(sceneDirectory, error), end; !error && iterator != end; iterator.increment(error)) {
		if (!iterator->is_regular_file() || iterator->path().extension() != ".json") {
			continue;
		}
		const std::string fileName = iterator->path().filename().string();
		constexpr const char* prefix = "GAMEPLAY_";
		constexpr const char* suffix = "_objects.json";
		if (fileName == "GAMEPLAY_objects.json" || fileName.size() <= std::strlen(prefix) + std::strlen(suffix)) {
			continue;
		}
		if (fileName.compare(0, std::strlen(prefix), prefix) != 0 ||
		    fileName.compare(fileName.size() - std::strlen(suffix), std::strlen(suffix), suffix) != 0) {
			continue;
		}
		const std::string stageId = fileName.substr(
		    std::strlen(prefix),
		    fileName.size() - std::strlen(prefix) - std::strlen(suffix));
		if (IsValidGameplayStageId(stageId)) {
			gameplayStageIds_.push_back(stageId);
		}
	}
	std::sort(gameplayStageIds_.begin() + 1, gameplayStageIds_.end());
	gameplayStageIds_.erase(std::unique(gameplayStageIds_.begin(), gameplayStageIds_.end()), gameplayStageIds_.end());
	selectedGameplayStageIndex_ = FindGameplayStageIndex(keepSelectedId);
	selectedGameplayStageId_ = gameplayStageIds_[selectedGameplayStageIndex_];
}

void SceneManager::SetSelectedGameplayStageId(const std::string& stageId) {
	if (!IsValidGameplayStageId(stageId)) {
		return;
	}
	RefreshGameplayStagePatterns();
	const int index = FindGameplayStageIndex(stageId);
	if (gameplayStageIds_[index] == stageId) {
		selectedGameplayStageId_ = stageId;
		selectedGameplayStageIndex_ = index;
	}
}

std::string SceneManager::GetGameplayStageJsonPath(const std::string& stageId) const {
	if (stageId.empty() || stageId == "default") {
		return "Resources/Data/Scenes/GAMEPLAY_objects.json";
	}
	return "Resources/Data/Scenes/GAMEPLAY_" + stageId + "_objects.json";
}

void SceneManager::ApplyGameplayStagePath(BaseScene* scene, const std::string& stageId) const {
	if (scene) {
		scene->SetSceneObjectFilePathOverride(GetGameplayStageJsonPath(stageId));
	}
}

bool SceneManager::IsValidGameplayStageId(const std::string& stageId) const {
	if (stageId.empty()) {
		return false;
	}
	return std::all_of(stageId.begin(), stageId.end(), [](unsigned char character) {
		return std::isalnum(character) || character == '-' || character == '_';
	});
}

int SceneManager::FindGameplayStageIndex(const std::string& stageId) const {
	const auto found = std::find(gameplayStageIds_.begin(), gameplayStageIds_.end(), stageId);
	return found == gameplayStageIds_.end() ? 0 : static_cast<int>(std::distance(gameplayStageIds_.begin(), found));
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
/// 次のシーンへの切り替えを予約します。
/// </summary>
/// <param name="sceneName">対象となるシーン名を指定します。</param>
void SceneManager::ChangeScene(const std::string& sceneName) {
	ChangeScene(sceneName, transitionSettings_.type);
}

void SceneManager::ChangeScene(const std::string& sceneName, SceneTransitionType transitionType) {
	assert(sceneFactory_);
	assert(nextScene_ == nullptr);
	QueueNextScene(sceneName);

	activeTransitionType_ = transitionType;
	transitionElapsed_ = 0.0f;
	transitionCoverage_ = 0.0f;
	if (!scene_ || transitionType == SceneTransitionType::None || transitionSettings_.outDuration <= 0.0f) {
		transitionPhase_ = TransitionPhase::Idle;
		return;
	}
	transitionPhase_ = TransitionPhase::Out;
}

void SceneManager::SetTransitionSettings(const SceneTransitionSettings& settings) {
	transitionSettings_ = settings;
	transitionSettings_.outDuration = (std::max)(0.0f, transitionSettings_.outDuration);
	transitionSettings_.inDuration = (std::max)(0.0f, transitionSettings_.inDuration);
	transitionSettings_.color.x = std::clamp(transitionSettings_.color.x, 0.0f, 1.0f);
	transitionSettings_.color.y = std::clamp(transitionSettings_.color.y, 0.0f, 1.0f);
	transitionSettings_.color.z = std::clamp(transitionSettings_.color.z, 0.0f, 1.0f);
	transitionSettings_.color.w = std::clamp(transitionSettings_.color.w, 0.0f, 1.0f);
}

void SceneManager::QueueNextScene(const std::string& sceneName) {

	nextScene_ = sceneFactory_->CreateScene(sceneName);
	assert(nextScene_);
	nextSceneName_ = sceneName;
	nextScene_->SetSceneManager(this);
	if (sceneName == "GAMEPLAY") {
		RefreshGameplayStagePatterns();
		ApplyGameplayStagePath(nextScene_.get(), selectedGameplayStageId_);
		nextScene_->SetPlayerTypeOverride(selectedPlayerTypeName_);
	}
}

void SceneManager::SwitchToNextScene() {
	if (!nextScene_) {
		return;
	}
	if (scene_) {
		ApplyFallbackCamera();
		scene_->Finalize();
		scene_.reset();
	}

	scene_ = std::move(nextScene_);
	currentSceneName_ = nextSceneName_;
	selectedSceneIndex_ = FindSceneIndex(currentSceneName_);
	scene_->SetSceneName(currentSceneName_);
	if (currentSceneName_ == "GAMEPLAY") {
		activeGameplayStageId_ = selectedGameplayStageId_;
		ApplyGameplayStagePath(scene_.get(), activeGameplayStageId_);
	}
	scene_->SetFallbackCamera(fallbackCamera_);
	scene_->Initialize();
	scene_->LoadEditorObjects();
}

void SceneManager::UpdateTransition() {
	if (transitionPhase_ == TransitionPhase::Idle) {
		return;
	}
	transitionElapsed_ += GameTime::GetDeltaTime();
	if (transitionPhase_ == TransitionPhase::Out) {
		const float duration = (std::max)(transitionSettings_.outDuration, 0.0001f);
		transitionCoverage_ = std::clamp(transitionElapsed_ / duration, 0.0f, 1.0f);
		if (transitionCoverage_ >= 1.0f) {
			SwitchToNextScene();
			transitionElapsed_ = 0.0f;
			if (transitionSettings_.inDuration <= 0.0f) {
				transitionCoverage_ = 0.0f;
				transitionPhase_ = TransitionPhase::Idle;
			} else {
				transitionPhase_ = TransitionPhase::In;
			}
		}
	} else {
		const float duration = (std::max)(transitionSettings_.inDuration, 0.0001f);
		transitionCoverage_ = 1.0f - std::clamp(transitionElapsed_ / duration, 0.0f, 1.0f);
		if (transitionCoverage_ <= 0.0f) {
			transitionCoverage_ = 0.0f;
			transitionPhase_ = TransitionPhase::Idle;
		}
	}
}

void SceneManager::SetTransitionSpriteRect(size_t index, float x, float y, float width, float height, float alpha) {
	if (index >= transitionSprites_.size() || !transitionSprites_[index] || width <= 0.0f || height <= 0.0f) {
		return;
	}
	Sprite* sprite = transitionSprites_[index].get();
	EulerTransform transform = sprite->GetTransform();
	transform.translate = {x, y, 0.0f};
	sprite->SetTransform(transform);
	sprite->SetSize({width, height});
	sprite->SetColor({transitionSettings_.color.x, transitionSettings_.color.y, transitionSettings_.color.z,
	    std::clamp(alpha * transitionSettings_.color.w, 0.0f, 1.0f)});
	sprite->Update();
	sprite->Draw();
}

void SceneManager::DrawTransition() {
	if (transitionPhase_ == TransitionPhase::Idle || activeTransitionType_ == SceneTransitionType::None || transitionCoverage_ <= 0.0f) {
		return;
	}
	DirectXCommon* dxCommon = SpriteCommon::GetInstance()->GetDxCommon();
	if (!dxCommon) {
		return;
	}
	const float width = static_cast<float>(dxCommon->GetRenderWidth());
	const float height = static_cast<float>(dxCommon->GetRenderHeight());
	const float coverage = std::clamp(transitionCoverage_, 0.0f, 1.0f);
	SpriteCommon::GetInstance()->SetDraw(kBlendModeNormal);

	switch (activeTransitionType_) {
	case SceneTransitionType::Fade:
		SetTransitionSpriteRect(0, 0.0f, 0.0f, width, height, coverage);
		break;
	case SceneTransitionType::WipeLeft:
		SetTransitionSpriteRect(0, 0.0f, 0.0f, width * coverage, height, 1.0f);
		break;
	case SceneTransitionType::WipeRight: {
		const float coveredWidth = width * coverage;
		SetTransitionSpriteRect(0, width - coveredWidth, 0.0f, coveredWidth, height, 1.0f);
		break;
	}
	case SceneTransitionType::Curtains: {
		const float panelWidth = width * 0.5f * coverage;
		SetTransitionSpriteRect(0, 0.0f, 0.0f, panelWidth, height, 1.0f);
		SetTransitionSpriteRect(1, width - panelWidth, 0.0f, panelWidth, height, 1.0f);
		break;
	}
	case SceneTransitionType::HorizontalBars: {
		const float barHeight = height / static_cast<float>(kTransitionSpriteCount);
		const float coveredWidth = width * coverage;
		for (size_t index = 0; index < kTransitionSpriteCount; ++index) {
			const float x = (index % 2 == 0) ? 0.0f : width - coveredWidth;
			const float y = barHeight * static_cast<float>(index);
			const float h = index + 1 == kTransitionSpriteCount ? height - y : std::ceil(barHeight);
			SetTransitionSpriteRect(index, x, y, coveredWidth, h, 1.0f);
		}
		break;
	}
	case SceneTransitionType::None:
		break;
	}
}
