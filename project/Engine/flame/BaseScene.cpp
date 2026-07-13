#include "BaseScene.h"
#include "BaseSceneHelpers.h"

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
	selectedObjectIndex_ = -1;
	activeCameraObjectName_.clear();
}

/// <summary>
/// シーン内オブジェクトの更新と当たり判定を行います。
/// </summary>
void BaseScene::UpdateSceneObjects() {
	ResolveCameraLinks();
	if (editorSkyBox_) {
		editorSkyBox_->Update();
	}
	for (const auto& object : sceneObjects_) {
		object->Update();
	}
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
	if (editorSkyBox_) {
		editorSkyBox_->Update();
	}
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
#ifdef USE_IMGUI
	DrawEditorHierarchy();
	DrawEditorInspector();
	DrawEditorGizmo();
#endif
}

