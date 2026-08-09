#pragma once
#include "AbstractSceneFactory.h"
#include "StageResultData.h"
#include "Vector.h"

#include <memory>
#include <array>
#include <string>
#include <vector>

class BaseScene;
class Camera;
class Sprite;

/// <summary>シーン切り替え時に使用する画面演出です。</summary>
enum class SceneTransitionType {
	None,
	Fade,
	WipeLeft,
	WipeRight,
	Curtains,
	HorizontalBars
};

/// <summary>シーン遷移の既定値です。ChangeSceneごとに種類だけ上書きすることもできます。</summary>
struct SceneTransitionSettings {
	SceneTransitionType type = SceneTransitionType::Fade;
	float outDuration = 0.35f;
	float inDuration = 0.35f;
	Vector4 color = {0.0f, 0.0f, 0.0f, 1.0f};
};

/// <summary>
/// 現在のシーンと遷移先シーンを所有し、ライフサイクルと描画呼び出しを管理します。
/// シーン生成そのものはAbstractSceneFactoryへ委譲します。
/// </summary>
class SceneManager {
private:
	enum class TransitionPhase { Idle, Out, In };

public:
	SceneManager();
	/// <summary>
	/// 破棄時に必要な解放処理を行います。
	/// </summary>
	~SceneManager();

	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;

	/// <summary>
	/// 次のシーンへの切り替えを予約します。
	/// </summary>
	/// <param name="sceneName">対象となるシーン名を指定します。</param>
	void ChangeScene(const std::string& sceneName);
	/// <summary>この切り替えに限り、指定した遷移パターンを使用します。</summary>
	void ChangeScene(const std::string& sceneName, SceneTransitionType transitionType);
	void SetTransitionSettings(const SceneTransitionSettings& settings);
	const SceneTransitionSettings& GetTransitionSettings() const { return transitionSettings_; }
	bool IsTransitioning() const { return transitionPhase_ != TransitionPhase::Idle; }
	void SetSelectedPlayerTypeName(const std::string& playerTypeName) { selectedPlayerTypeName_ = playerTypeName; }
	const std::string& GetSelectedPlayerTypeName() const { return selectedPlayerTypeName_; }

	/// <summary>
	/// 毎フレームの状態更新を行います。
	/// </summary>
	void Update();
	/// <summary>
	/// スカイボックスの描画処理を行います。
	/// </summary>
	void DrawSkyBox();
	/// <summary>
	/// 2D 要素の描画処理を行います。
	/// </summary>
	void Draw2D();
	/// <summary>
	/// 3D 要素の描画処理を行います。
	/// </summary>
	void Draw3D();
	void DrawEditorImGui();

	void SetSceneFactory(AbstractSceneFactory* factory) { sceneFactory_ = factory; }
	void SetFallbackCamera(Camera* camera) { fallbackCamera_ = camera; }
	void SetScenePlaying(bool isPlaying) { isScenePlaying_ = isPlaying; }
	bool IsScenePlaying() const { return isScenePlaying_; }
	void RequestFrameStep() { isFrameStepRequested_ = true; }
	bool ReloadCurrentSceneJson();
	std::string GetCurrentSceneJsonPath() const;
	void RefreshGameplayStagePatterns();
	void SetSelectedGameplayStageId(const std::string& stageId);
	const std::string& GetSelectedGameplayStageId() const { return selectedGameplayStageId_; }
	const std::vector<std::string>& GetGameplayStageIds() const { return gameplayStageIds_; }
	void SetStageResultData(const StageResultData& resultData) { stageResultData_ = resultData; }
	const StageResultData& GetStageResultData() const { return stageResultData_; }

private:
	/// <summary>
	/// FallbackCamera を現在の状態へ反映します。
	/// </summary>
	void ApplyFallbackCamera();
	void QueueNextScene(const std::string& sceneName);
	void SwitchToNextScene();
	void UpdateTransition();
	void DrawTransition();
	void SetTransitionSpriteRect(size_t index, float x, float y, float width, float height, float alpha);
	std::string GetGameplayStageJsonPath(const std::string& stageId) const;
	void ApplyGameplayStagePath(BaseScene* scene, const std::string& stageId) const;
	bool IsValidGameplayStageId(const std::string& stageId) const;
	int FindGameplayStageIndex(const std::string& stageId) const;

	std::unique_ptr<BaseScene> scene_ = nullptr;
	std::unique_ptr<BaseScene> nextScene_ = nullptr;
	static constexpr size_t kTransitionSpriteCount = 8;
	std::array<std::unique_ptr<Sprite>, kTransitionSpriteCount> transitionSprites_{};
	TransitionPhase transitionPhase_ = TransitionPhase::Idle;
	SceneTransitionSettings transitionSettings_{};
	SceneTransitionType activeTransitionType_ = SceneTransitionType::Fade;
	float transitionElapsed_ = 0.0f;
	float transitionCoverage_ = 0.0f;
	AbstractSceneFactory* sceneFactory_ = nullptr;
	Camera* fallbackCamera_ = nullptr;
	std::string currentSceneName_ = "None";
	std::string nextSceneName_ = "None";
	int selectedSceneIndex_ = 0;
	bool isScenePlaying_ = true;
	bool isFrameStepRequested_ = false;
	std::string selectedPlayerTypeName_ = "Default";
	std::vector<std::string> gameplayStageIds_ = {"default"};
	std::string selectedGameplayStageId_ = "default";
	std::string activeGameplayStageId_ = "default";
	int selectedGameplayStageIndex_ = 0;
	std::array<char, 64> newGameplayStageIdBuffer_{};
	std::string gameplayStageMessage_;
	StageResultData stageResultData_;
};
