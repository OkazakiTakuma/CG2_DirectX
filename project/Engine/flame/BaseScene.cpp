#include "BaseScene.h"

BaseScene::~BaseScene() {}

void BaseScene::Initialize() {}

void BaseScene::Update() {}

void BaseScene::DrawSkyBox() {
	if (isEditorSkyBoxEnabled_ && editorSkyBox_) {
		editorSkyBox_->Draw();
	}
}

void BaseScene::Draw2D() {}

void BaseScene::Draw3D() {}

/// <summary>
/// シーンが保持しているリソースを解放します。
/// </summary>
void BaseScene::Finalize() {
	sceneObjects_.clear();
	editorSkyBox_.reset();
	playerHealthBarBackground_.reset();
	playerHealthBarFill_.reset();
	isPlayerHealthHudVisible_ = false;
	playerExperienceBarBackground_.reset();
	playerExperienceBarFill_.reset();
	playerExperienceTextObject_.reset();
	playerExperienceRate_ = 0.0f;
	isPlayerExperienceHudVisible_ = false;
	for (auto& sprite : playerAttackSlotBackgroundSprites_) sprite.reset();
	for (auto& sprite : playerAttackSlotIconSprites_) sprite.reset();
	for (auto& sprite : playerStatusSlotBackgroundSprites_) sprite.reset();
	for (auto& sprite : playerStatusSlotIconSprites_) sprite.reset();
	playerAttackSlotIconVisible_.fill(false);
	playerStatusSlotIconVisible_.fill(false);
	playerAttackSlotTextureKeys_.fill({});
	playerAttackSlotTexturePaths_.fill({});
	playerStatusSlotTextureKeys_.fill({});
	playerStatusSlotTexturePaths_.fill({});
	playerAttackSlotLabelObject_.reset();
	playerStatusSlotLabelObject_.reset();
	isPlayerSlotHudVisible_ = false;
	isLevelUpSelectionActive_ = false;
	levelUpPlayer_ = nullptr;
	levelUpChoices_.clear();
	levelUpOverlaySprite_.reset();
	levelUpPanelSprite_.reset();
	for (auto& sprite : levelUpChoiceBorderSprites_) sprite.reset();
	for (auto& sprite : levelUpChoiceSprites_) sprite.reset();
	for (auto& sprite : levelUpChoiceIconSprites_) sprite.reset();
	levelUpTitleTextObject_.reset();
	levelUpInstructionTextObject_.reset();
	for (auto& object : levelUpChoiceTextObjects_) object.reset();
	GameTime::SetPaused(false);
	selectedObjectIndex_ = -1;
	activeCameraObjectName_.clear();
	enemyInspectorObjectName_.clear();
}

/// <summary>
/// シーン内オブジェクトの更新と当たり判定を行います。
/// </summary>
void BaseScene::UpdateSceneObjects() {
	ResolveCameraLinks();
	ResolveEnemySpawnPointLinks();
	ResolveEnemyLinks();
	if (editorSkyBox_) {
		editorSkyBox_->Update();
	}
	for (const auto& object : sceneObjects_) {
		object->Update();
	}
	UpdateEnemyAttacks();
	UpdateEnemyProjectileHits();
	UpdatePlayerAttacks();
	UpdatePlayerProjectileHits();
	UpdateExperienceCompression();
	UpdateEnemySpawning();
	CleanupExpiredPlayerProjectiles();
	UpdatePlayerHealthHud();
	UpdatePlayerExperienceHud();
	UpdatePlayerSlotHud();
	UpdateEditorObjectPicking();
	UpdateEditorCameraControl();
	UpdateColliderCollisions();
	ApplyActiveCamera();
}

/// <summary>
/// エディタ用のカメラ操作とオブジェクト選択を更新します。
/// </summary>
void BaseScene::UpdateEditorTools() {
	ResolveCameraLinks();
	ResolveEnemySpawnPointLinks();
	ResolveEnemyLinks();
	if (editorSkyBox_) {
		editorSkyBox_->Update();
	}
	UpdatePlayerHealthHud();
	UpdatePlayerExperienceHud();
	UpdatePlayerSlotHud();
	UpdateEditorObjectPicking();
	UpdateEditorCameraControl();
	ApplyActiveCamera();
}

/// <summary>
/// シーン内オブジェクトの2D描画を行います。
/// </summary>
void BaseScene::DrawSceneObjects2D() {
	for (const auto& object : sceneObjects_) {
		object->Draw2D();
	}
	if (isPlayerHealthHudVisible_ && playerHealthBarBackground_ && playerHealthBarFill_) {
		playerHealthBarBackground_->Draw();
		playerHealthBarFill_->Draw();
	}
	DrawPlayerExperienceHud();
	DrawPlayerSlotHud();
	DrawLevelUpSelection2D();
}

/// <summary>
/// シーン内オブジェクトの3D描画を行います。
/// </summary>
void BaseScene::DrawSceneObjects3D() {
	for (const auto& object : sceneObjects_) {
		object->Draw3D();
	}
}

/// <summary>
/// エディタ用ImGuiウィンドウを描画します。
/// </summary>
void BaseScene::DrawEditorImGui() {
	UpdateLevelUpSelection();
#ifdef USE_IMGUI
	DrawEditorHierarchy();
	DrawEditorProjectAssets();
	DrawEditorInspector();
	DrawPlayerInspector();
	DrawEnemyInspector();
	DrawPlayerAttackInspector();
	DrawEditorGizmo();
	HandleGameViewAssetDrop();
#endif
}

