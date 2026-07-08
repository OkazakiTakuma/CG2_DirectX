#include "BaseScene.h"
#include "ModelManager.h"
#ifdef USE_IMGUI
#include "../../../imgui/ImGuizmo.h"
#endif
#include <filesystem>
#include <fstream>
#include <json.hpp>
#include <cmath>

namespace {
constexpr float kPi = 3.14159265358979323846f;

nlohmann::json Vector3ToJson(const Vector3& value) {
	return nlohmann::json::array({value.x, value.y, value.z});
}

Vector3 JsonToVector3(const nlohmann::json& value, const Vector3& fallback) {
	if (!value.is_array() || value.size() < 3) {
		return fallback;
	}

	return {
	    value.at(0).get<float>(),
	    value.at(1).get<float>(),
	    value.at(2).get<float>()
	};
}

const char* EditorCreateTypeName(BaseScene::EditorCreateType type) {
	switch (type) {
	case BaseScene::EditorCreateType::Object3dSphere:
		return "Object3dSphere";
	case BaseScene::EditorCreateType::Object3dCylinder:
		return "Object3dCylinder";
	case BaseScene::EditorCreateType::Sprite:
		return "Sprite";
	case BaseScene::EditorCreateType::LoadedModel:
		return "LoadedModel";
	case BaseScene::EditorCreateType::Camera:
		return "Camera";
	case BaseScene::EditorCreateType::Empty:
	default:
		return "Empty";
	}
}

float ToDegrees(float radians) {
	return radians * 180.0f / kPi;
}

float ToRadians(float degrees) {
	return degrees * kPi / 180.0f;
}

Vector3 RotateAroundAxis(const Vector3& value, const Vector3& axis, float radians) {
	const Vector3 normalizedAxis = Normalize(axis);
	const float axisLength = Length(normalizedAxis);
	if (axisLength <= 0.00001f) {
		return value;
	}

	const float cosValue = std::cos(radians);
	const float sinValue = std::sin(radians);
	return
	    cosValue * value +
	    sinValue * Cross(normalizedAxis, value) +
	    Dot(normalizedAxis, value) * (1.0f - cosValue) * normalizedAxis;
}

bool TryGetCameraTransform(Camera* camera, Vector3& translate, Vector3& rotate) {
	if (!camera) {
		return false;
	}

	translate = camera->GetTranslate();
	rotate = camera->GetRotate();
	return true;
}

Vector3 TransformCoord(const Vector3& vector, const Matrix4x4& matrix) {
	return Transformation(vector, matrix);
}

bool IntersectRayToOBB(const Vector3& rayOrigin, const Vector3& rayDirection, const OBBColliderShape& obb, float& distance) {
	constexpr float kEpsilon = 0.00001f;
	float tMin = 0.0f;
	float tMax = 100000.0f;
	const Vector3 centerToRay = rayOrigin - obb.center;

	for (int axisIndex = 0; axisIndex < 3; ++axisIndex) {
		const float origin = Dot(centerToRay, obb.orientation[axisIndex]);
		const float direction = Dot(rayDirection, obb.orientation[axisIndex]);
		const float minValue = -((&obb.halfSize.x)[axisIndex]);
		const float maxValue = (&obb.halfSize.x)[axisIndex];

		if (std::fabs(direction) < kEpsilon) {
			if (origin < minValue || origin > maxValue) {
				return false;
			}
			continue;
		}

		float t1 = (minValue - origin) / direction;
		float t2 = (maxValue - origin) / direction;
		if (t1 > t2) {
			std::swap(t1, t2);
		}
		if (t1 > tMin) {
			tMin = t1;
		}
		if (t2 < tMax) {
			tMax = t2;
		}
		if (tMin > tMax) {
			return false;
		}
	}

	distance = tMin;
	return true;
}

OBBColliderShape MakePickOBB(GameObject* object) {
	if (OBBColliderComponent* collider = object->GetComponent<OBBColliderComponent>()) {
		return collider->GetWorldOBB();
	}

	const EulerTransform& transform = object->GetTransform();
	const Matrix4x4 rotateMatrix = MakeRotateXYZMatrix(transform.rotate);
	const Vector3 axisX{rotateMatrix.m[0][0], rotateMatrix.m[0][1], rotateMatrix.m[0][2]};
	const Vector3 axisY{rotateMatrix.m[1][0], rotateMatrix.m[1][1], rotateMatrix.m[1][2]};
	const Vector3 axisZ{rotateMatrix.m[2][0], rotateMatrix.m[2][1], rotateMatrix.m[2][2]};

	OBBColliderShape obb{};
	obb.center = transform.translate;
	obb.orientation[0] = Normalize(axisX);
	obb.orientation[1] = Normalize(axisY);
	obb.orientation[2] = Normalize(axisZ);
	const float halfX = std::fabs(transform.scale.x) * 0.5f;
	const float halfY = std::fabs(transform.scale.y) * 0.5f;
	const float halfZ = std::fabs(transform.scale.z) * 0.5f;
	obb.halfSize = {
	    halfX < 0.25f ? 0.25f : halfX,
	    halfY < 0.25f ? 0.25f : halfY,
	    halfZ < 0.25f ? 0.25f : halfZ
	};
	return obb;
}
}

BaseScene::~BaseScene() {}

void BaseScene::Initialize() {}

void BaseScene::Update() {}

void BaseScene::DrawSkyBox() {}

void BaseScene::Draw2D() {}

void BaseScene::Draw3D() {}

void BaseScene::Finalize() {
	sceneObjects_.clear();
	selectedObjectIndex_ = -1;
	activeCameraObjectName_.clear();
}

void BaseScene::UpdateSceneObjects() {
	ResolveCameraLinks();
	for (const auto& object : sceneObjects_) {
		object->Update();
	}
	UpdateEditorObjectPicking();
	UpdateEditorCameraControl();
	UpdateColliderCollisions();
	ApplyActiveCamera();
}

void BaseScene::DrawSceneObjects2D() {
	for (const auto& object : sceneObjects_) {
		object->Draw2D();
	}
}

void BaseScene::DrawSceneObjects3D() {
	for (const auto& object : sceneObjects_) {
		object->Draw3D();
	}
}

void BaseScene::DrawEditorImGui() {
#ifdef USE_IMGUI
	DrawEditorHierarchy();
	DrawEditorInspector();
	DrawEditorGizmo();
#endif
}

GameObject* BaseScene::CreateEditorObject(EditorCreateType type, const std::string& modelFilePath) {
	auto object = std::make_unique<GameObject>();
	object->SetEditorType(EditorCreateTypeName(type));

	switch (type) {
	case EditorCreateType::Empty:
		object->SetName(MakeUniqueObjectName("Empty"));
		break;
	case EditorCreateType::Object3dSphere: {
		object->SetName(MakeUniqueObjectName("Sphere"));
		ModelManager::GetInstance()->LoadModel("sphere.obj");
		Object3dComponent* object3d = object->AddComponent<Object3dComponent>();
		object3d->SetModel("sphere.obj");
		break;
	}
	case EditorCreateType::Object3dCylinder: {
		object->SetName(MakeUniqueObjectName("Cylinder"));
		Object3dComponent* object3d = object->AddComponent<Object3dComponent>();
		object3d->CreateCylinder(1.0f, 2.0f, 16, true, true);
		object3d->SetTexture("Resources/gradationLine.png");
		break;
	}
	case EditorCreateType::Sprite: {
		object->SetName(MakeUniqueObjectName("Sprite"));
		SpriteComponent* sprite = object->AddComponent<SpriteComponent>();
		sprite->Initialize("Resources/uvChecker.png");
		EulerTransform transform = object->GetTransform();
		transform.scale = {100.0f, 100.0f, 1.0f};
		object->GetTransform() = transform;
		sprite->SetSize({100.0f, 100.0f});
		break;
	}
	case EditorCreateType::LoadedModel: {
		if (modelFilePath.empty() || !ModelManager::GetInstance()->FindModel(modelFilePath)) {
			return nullptr;
		}
		object->SetName(MakeUniqueObjectName(std::filesystem::path(modelFilePath).stem().string()));
		object->SetEditorType("LoadedModel:" + modelFilePath);
		Object3dComponent* object3d = object->AddComponent<Object3dComponent>();
		object3d->SetModel(modelFilePath);
		break;
	}
	case EditorCreateType::Camera: {
		object->SetName(MakeUniqueObjectName("Camera"));
		CameraComponent* camera = object->AddComponent<CameraComponent>();
		camera->SetFovY(0.45f);
		EulerTransform& transform = object->GetTransform();
		transform.translate = {0.0f, 4.0f, -10.0f};
		break;
	}
	}

	object->Update();
	sceneObjects_.push_back(std::move(object));
	selectedObjectIndex_ = static_cast<int>(sceneObjects_.size()) - 1;
	if (type == EditorCreateType::Camera && activeCameraObjectName_.empty()) {
		SetActiveCameraObject(sceneObjects_.back().get());
	}
	++nextObjectId_;
	return sceneObjects_.back().get();
}

void BaseScene::DeleteSelectedEditorObject() {
	if (selectedObjectIndex_ < 0 || selectedObjectIndex_ >= static_cast<int>(sceneObjects_.size())) {
		return;
	}

	const bool deletesActiveCamera =
	    !activeCameraObjectName_.empty() &&
	    sceneObjects_[selectedObjectIndex_]->GetName() == activeCameraObjectName_;
	sceneObjects_.erase(sceneObjects_.begin() + selectedObjectIndex_);
	if (sceneObjects_.empty()) {
		selectedObjectIndex_ = -1;
	} else if (selectedObjectIndex_ >= static_cast<int>(sceneObjects_.size())) {
		selectedObjectIndex_ = static_cast<int>(sceneObjects_.size()) - 1;
	}

	if (deletesActiveCamera) {
		GameObject* nextCamera = FindFirstCameraObject();
		if (nextCamera) {
			SetActiveCameraObject(nextCamera);
		} else {
			activeCameraObjectName_.clear();
			ApplyCamera(fallbackCamera_);
		}
	}
}

void BaseScene::DrawEditorHierarchy() {
#ifdef USE_IMGUI
	ImGui::Begin("Scene Objects");

	const char* createLabels[] = {"Empty", "3D Sphere", "3D Cylinder", "Sprite", "Loaded Model", "Camera"};
	int createTypeIndex = static_cast<int>(createType_);
	if (ImGui::Combo("Type", &createTypeIndex, createLabels, _countof(createLabels))) {
		createType_ = static_cast<EditorCreateType>(createTypeIndex);
	}

	std::string selectedModelFilePath;
	if (createType_ == EditorCreateType::LoadedModel) {
		const std::vector<std::string> loadedModels = ModelManager::GetInstance()->GetLoadedModelNames();
		if (loadedModels.empty()) {
			ImGui::Text("No loaded models");
		} else {
			if (selectedLoadedModelIndex_ >= static_cast<int>(loadedModels.size())) {
				selectedLoadedModelIndex_ = 0;
			}

			std::vector<const char*> loadedModelLabels;
			loadedModelLabels.reserve(loadedModels.size());
			for (const std::string& modelName : loadedModels) {
				loadedModelLabels.push_back(modelName.c_str());
			}
			ImGui::Combo("Model", &selectedLoadedModelIndex_, loadedModelLabels.data(), static_cast<int>(loadedModelLabels.size()));
			selectedModelFilePath = loadedModels[selectedLoadedModelIndex_];
		}
	}

	if (ImGui::Button("Create")) {
		CreateEditorObject(createType_, selectedModelFilePath);
	}
	ImGui::SameLine();
	const bool canDelete = selectedObjectIndex_ >= 0 && selectedObjectIndex_ < static_cast<int>(sceneObjects_.size());
	if (!canDelete) {
		ImGui::BeginDisabled();
	}
	if (ImGui::Button("Delete")) {
		DeleteSelectedEditorObject();
	}
	if (!canDelete) {
		ImGui::EndDisabled();
	}

	if (ImGui::Button("Save Layout")) {
		SaveEditorObjects();
	}
	ImGui::SameLine();
	if (ImGui::Button("Load Layout")) {
		LoadEditorObjects();
	}

	ImGui::Separator();
	for (int index = 0; index < static_cast<int>(sceneObjects_.size()); ++index) {
		const bool isSelected = selectedObjectIndex_ == index;
		std::string label = sceneObjects_[index]->GetName();
		if (!activeCameraObjectName_.empty() && sceneObjects_[index]->GetName() == activeCameraObjectName_) {
			label += " [Active Camera]";
		}
		if (ImGui::Selectable(label.c_str(), isSelected)) {
			selectedObjectIndex_ = index;
		}
	}

	ImGui::End();
#endif
}

void BaseScene::DrawEditorInspector() {
#ifdef USE_IMGUI
	ImGui::Begin("Object Inspector");

	if (selectedObjectIndex_ < 0 || selectedObjectIndex_ >= static_cast<int>(sceneObjects_.size())) {
		ImGui::Text("No object selected");
		ImGui::End();
		return;
	}

	GameObject* selectedObject = sceneObjects_[selectedObjectIndex_].get();
	ImGui::Text("%s", selectedObject->GetName().c_str());
	ImGui::Separator();

	EulerTransform& transform = selectedObject->GetTransform();
	ImGui::DragFloat3("Position", &transform.translate.x, 0.1f);
	ImGui::SliderAngle("Rotate X", &transform.rotate.x);
	ImGui::SliderAngle("Rotate Y", &transform.rotate.y);
	ImGui::SliderAngle("Rotate Z", &transform.rotate.z);
	ImGui::DragFloat3("Scale", &transform.scale.x, 0.1f, 0.01f, 1000.0f);
	ImGui::Checkbox("Enable Gizmo", &isGizmoEnabled_);
	const char* gizmoOperationLabels[] = {"Move", "Rotate", "Scale"};
	ImGui::Combo("Gizmo Operation", &gizmoOperationIndex_, gizmoOperationLabels, _countof(gizmoOperationLabels));

	if (Object3dComponent* object3dComponent = selectedObject->GetComponent<Object3dComponent>()) {
		ImGui::Separator();
		ImGui::Text("Object3dComponent");
		bool isDrawSkeleton = object3dComponent->GetDrawSkeleton();
		if (ImGui::Checkbox("Draw Skeleton", &isDrawSkeleton)) {
			object3dComponent->SetDrawSkeleton(isDrawSkeleton);
		}
	}
	if (selectedObject->GetComponent<SpriteComponent>()) {
		ImGui::Separator();
		ImGui::Text("SpriteComponent");
	}
	if (selectedObject->GetComponent<CameraComponent>()) {
		DrawCameraInspector(selectedObject);
	}
	DrawOBBColliderInspector(selectedObject);

	ImGui::End();
#endif
}

void BaseScene::DrawOBBColliderInspector(GameObject* selectedObject) {
#ifdef USE_IMGUI
	if (!selectedObject) {
		return;
	}

	OBBColliderComponent* collider = selectedObject->GetComponent<OBBColliderComponent>();
	ImGui::Separator();
	if (!collider) {
		if (ImGui::Button("Add OBB Collider")) {
			selectedObject->AddComponent<OBBColliderComponent>();
		}
		return;
	}

	ImGui::Text("OBBColliderComponent");
	Vector3 centerOffset = collider->GetCenterOffset();
	if (ImGui::DragFloat3("Collider Center", &centerOffset.x, 0.05f)) {
		collider->SetCenterOffset(centerOffset);
	}

	Vector3 halfSize = collider->GetHalfSize();
	if (ImGui::DragFloat3("Collider Half Size", &halfSize.x, 0.05f, 0.0f, 1000.0f)) {
		collider->SetHalfSize(halfSize);
	}

	bool isDrawDebug = collider->GetDrawDebug();
	if (ImGui::Checkbox("Draw Collider", &isDrawDebug)) {
		collider->SetDrawDebug(isDrawDebug);
	}
	ImGui::Text("Collision: %s", collider->IsColliding() ? "Hit" : "None");
#else
	(void)selectedObject;
#endif
}

void BaseScene::DrawCameraInspector(GameObject* selectedObject) {
#ifdef USE_IMGUI
	CameraComponent* cameraComponent = selectedObject->GetComponent<CameraComponent>();
	if (!cameraComponent) {
		return;
	}

	ImGui::Separator();
	ImGui::Text("CameraComponent");

	const bool isActive = selectedObject->GetName() == activeCameraObjectName_;
	if (isActive) {
		ImGui::Text("Active Camera");
	} else if (ImGui::Button("Set Active Camera")) {
		SetActiveCameraObject(selectedObject);
	}

	float fovY = cameraComponent->GetFovY();
	if (ImGui::SliderAngle("FOV", &fovY, 1.0f, 120.0f)) {
		cameraComponent->SetFovY(fovY);
	}
	float nearClip = cameraComponent->GetNearClip();
	if (ImGui::DragFloat("Near Clip", &nearClip, 0.01f, 0.001f, 100.0f)) {
		cameraComponent->SetNearClip(nearClip);
	}
	float farClip = cameraComponent->GetFarClip();
	if (ImGui::DragFloat("Far Clip", &farClip, 1.0f, 1.0f, 10000.0f)) {
		cameraComponent->SetFarClip(farClip);
	}

	std::vector<std::string> linkLabels;
	linkLabels.push_back("None");
	int currentLinkIndex = 0;
	for (const auto& object : sceneObjects_) {
		if (object.get() == selectedObject) {
			continue;
		}
		linkLabels.push_back(object->GetName());
		if (cameraComponent->GetFollowTarget() == object.get() || cameraComponent->GetFollowTargetName() == object->GetName()) {
			currentLinkIndex = static_cast<int>(linkLabels.size()) - 1;
		}
	}

	std::vector<const char*> linkLabelPtrs;
	linkLabelPtrs.reserve(linkLabels.size());
	for (const std::string& label : linkLabels) {
		linkLabelPtrs.push_back(label.c_str());
	}

	if (ImGui::Combo("Follow Object", &currentLinkIndex, linkLabelPtrs.data(), static_cast<int>(linkLabelPtrs.size()))) {
		if (currentLinkIndex == 0) {
			cameraComponent->SetFollowTarget(nullptr);
		} else {
			cameraComponent->SetFollowTarget(FindObjectByName(linkLabels[currentLinkIndex]));
		}
	}
#endif
}

void BaseScene::DrawEditorGizmo() {
#ifdef USE_IMGUI
	if (!isGizmoEnabled_ || selectedObjectIndex_ < 0 || selectedObjectIndex_ >= static_cast<int>(sceneObjects_.size())) {
		return;
	}

	Camera* camera = Object3dCommon::GetInstance()->GetDefaultCamera();
	if (!camera) {
		return;
	}

	GameObject* selectedObject = sceneObjects_[selectedObjectIndex_].get();
	EulerTransform& transform = selectedObject->GetTransform();

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	const ImVec2 viewportPos = viewport ? viewport->Pos : ImVec2(0.0f, 0.0f);
	const ImVec2 viewportSize = viewport ? viewport->Size : ImGui::GetIO().DisplaySize;

	Matrix4x4 objectMatrix{};
	float translation[3] = {transform.translate.x, transform.translate.y, transform.translate.z};
	float rotation[3] = {ToDegrees(transform.rotate.x), ToDegrees(transform.rotate.y), ToDegrees(transform.rotate.z)};
	float scale[3] = {transform.scale.x, transform.scale.y, transform.scale.z};
	ImGuizmo::RecomposeMatrixFromComponents(translation, rotation, scale, &objectMatrix.m[0][0]);

	ImGuizmo::BeginFrame();
	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList(viewport));
	ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);

	const Matrix4x4& viewMatrix = camera->GetViewMatrix();
	const Matrix4x4& projectionMatrix = camera->GetProjectionMatrix();
	const ImGuizmo::OPERATION gizmoOperations[] = {
	    ImGuizmo::TRANSLATE,
	    ImGuizmo::ROTATE,
	    ImGuizmo::SCALE
	};
	const int operationIndex = gizmoOperationIndex_ >= 0 && gizmoOperationIndex_ < _countof(gizmoOperations) ? gizmoOperationIndex_ : 0;
	const ImGuizmo::MODE gizmoMode = gizmoOperations[operationIndex] == ImGuizmo::SCALE ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

	if (ImGuizmo::Manipulate(&viewMatrix.m[0][0], &projectionMatrix.m[0][0], gizmoOperations[operationIndex], gizmoMode, &objectMatrix.m[0][0])) {
		ImGuizmo::DecomposeMatrixToComponents(&objectMatrix.m[0][0], translation, rotation, scale);
		transform.translate = {translation[0], translation[1], translation[2]};
		transform.rotate = {ToRadians(rotation[0]), ToRadians(rotation[1]), ToRadians(rotation[2])};
		transform.scale = {scale[0], scale[1], scale[2]};
	}
#endif
}

void BaseScene::ApplyCamera(Camera* camera) {
	if (!camera) {
		return;
	}

	camera->Update();
	Object3dCommon::GetInstance()->SetDefaultCamera(camera);
	SkyBoxCommon::GetInstance()->SetDefaultCamera(camera);
	ParticleManager::GetInstance()->SetCamera(camera);
	for (const auto& object : sceneObjects_) {
		if (Object3dComponent* object3dComponent = object->GetComponent<Object3dComponent>()) {
			object3dComponent->SetCamera(camera);
			if (object3dComponent->GetObject3d()) {
				object3dComponent->GetObject3d()->Update();
			}
		}
	}
}

void BaseScene::ApplyActiveCamera() {
	if (activeCameraObjectName_.empty()) {
		ApplyCamera(fallbackCamera_);
		return;
	}

	GameObject* cameraObject = FindObjectByName(activeCameraObjectName_);
	if (!cameraObject) {
		activeCameraObjectName_.clear();
		ApplyCamera(fallbackCamera_);
		return;
	}

	CameraComponent* cameraComponent = cameraObject->GetComponent<CameraComponent>();
	if (!cameraComponent || !cameraComponent->GetCamera()) {
		activeCameraObjectName_.clear();
		ApplyCamera(fallbackCamera_);
		return;
	}

	cameraComponent->Update();
	ApplyCamera(cameraComponent->GetCamera());
}

void BaseScene::ResolveCameraLinks() {
	for (const auto& object : sceneObjects_) {
		CameraComponent* cameraComponent = object->GetComponent<CameraComponent>();
		if (!cameraComponent || cameraComponent->GetFollowTargetName().empty()) {
			continue;
		}

		GameObject* target = FindObjectByName(cameraComponent->GetFollowTargetName());
		if (target && target != object.get() && target != cameraComponent->GetFollowTarget()) {
			cameraComponent->SetFollowTarget(target);
		}
	}
}

void BaseScene::UpdateColliderCollisions() {
	std::vector<OBBColliderComponent*> colliders;
	colliders.reserve(sceneObjects_.size());

	for (const auto& object : sceneObjects_) {
		OBBColliderComponent* collider = object->GetComponent<OBBColliderComponent>();
		if (collider && collider->IsEnabled()) {
			collider->SetColliding(false);
			colliders.push_back(collider);
		}
	}

	for (size_t i = 0; i < colliders.size(); ++i) {
		for (size_t j = i + 1; j < colliders.size(); ++j) {
			if (IsCollisionOBBToOBB(colliders[i]->GetWorldOBB(), colliders[j]->GetWorldOBB())) {
				colliders[i]->SetColliding(true);
				colliders[j]->SetColliding(true);
			}
		}
	}
}

void BaseScene::UpdateEditorObjectPicking() {
#ifdef USE_IMGUI
	constexpr int kLeftMouseButton = 0;
	Input* input = Input::GetInstance();
	if (!input->TriggerMouseButton(kLeftMouseButton)) {
		return;
	}
	if (ImGui::GetIO().WantCaptureMouse || ImGuizmo::IsOver() || ImGuizmo::IsUsing()) {
		return;
	}

	Camera* camera = Object3dCommon::GetInstance()->GetDefaultCamera();
	if (!camera) {
		return;
	}

	const float width = static_cast<float>(input->GetClientWidth());
	const float height = static_cast<float>(input->GetClientHeight());
	const float mouseX = static_cast<float>(input->GetMouseClientX());
	const float mouseY = static_cast<float>(input->GetMouseClientY());
	if (mouseX < 0.0f || mouseX > width || mouseY < 0.0f || mouseY > height) {
		return;
	}

	const float ndcX = (mouseX / width) * 2.0f - 1.0f;
	const float ndcY = 1.0f - (mouseY / height) * 2.0f;
	const Matrix4x4 inverseViewProjection = Inverse(camera->GetViewProjectionMatrix());
	const Vector3 nearPoint = TransformCoord({ndcX, ndcY, 0.0f}, inverseViewProjection);
	const Vector3 farPoint = TransformCoord({ndcX, ndcY, 1.0f}, inverseViewProjection);
	const Vector3 rayDirection = Normalize(farPoint - nearPoint);

	int hitIndex = -1;
	float nearestDistance = 100000.0f;
	for (int index = 0; index < static_cast<int>(sceneObjects_.size()); ++index) {
		float distance = 0.0f;
		if (IntersectRayToOBB(nearPoint, rayDirection, MakePickOBB(sceneObjects_[index].get()), distance) && distance < nearestDistance) {
			nearestDistance = distance;
			hitIndex = index;
		}
	}

	if (hitIndex >= 0) {
		selectedObjectIndex_ = hitIndex;
	}
#endif
}

void BaseScene::UpdateEditorCameraControl() {
#ifdef USE_IMGUI
	constexpr int kLeftMouseButton = 0;
	constexpr int kMiddleMouseButton = 2;
	constexpr float kRotateSpeed = 0.004f;
	constexpr float kPanSpeed = 0.0015f;

	Input* input = Input::GetInstance();
	const bool isLeftDragging = input->PushMouseButton(kLeftMouseButton);
	const bool isMiddleDragging = input->PushMouseButton(kMiddleMouseButton);
	if (ImGui::GetIO().WantCaptureMouse || (!isLeftDragging && !isMiddleDragging)) {
		return;
	}
	if (isLeftDragging && (ImGuizmo::IsOver() || ImGuizmo::IsUsing())) {
		return;
	}

	const float moveX = static_cast<float>(input->GetMouseMoveX());
	const float moveY = static_cast<float>(input->GetMouseMoveY());
	if (moveX == 0.0f && moveY == 0.0f) {
		return;
	}

	GameObject* selectedObject =
	    selectedObjectIndex_ >= 0 && selectedObjectIndex_ < static_cast<int>(sceneObjects_.size())
	        ? sceneObjects_[selectedObjectIndex_].get()
	        : nullptr;

	GameObject* activeCameraObject = FindObjectByName(activeCameraObjectName_);
	EulerTransform* cameraTransform = nullptr;
	Camera* fallbackCamera = nullptr;
	Vector3 cameraPosition{};
	Vector3 cameraRotate{};

	if (activeCameraObject && activeCameraObject->GetComponent<CameraComponent>()) {
		cameraTransform = &activeCameraObject->GetTransform();
		cameraPosition = cameraTransform->translate;
		cameraRotate = cameraTransform->rotate;
	} else if (TryGetCameraTransform(fallbackCamera_, cameraPosition, cameraRotate)) {
		fallbackCamera = fallbackCamera_;
	} else {
		return;
	}

	if (isMiddleDragging) {
		const float pitchDelta = moveY * kRotateSpeed;
		const float yawDelta = moveX * kRotateSpeed;
		if (selectedObject) {
			const Vector3 target = selectedObject->GetTransform().translate;
			Vector3 offset = cameraPosition - target;
			if (Length(offset) > 0.0001f) {
				const Vector3 worldUp{0.0f, 1.0f, 0.0f};
				offset = RotateAroundAxis(offset, worldUp, yawDelta);

				Vector3 right = Cross(worldUp, Normalize(offset));
				if (Length(right) > 0.0001f) {
					offset = RotateAroundAxis(offset, right, pitchDelta);
				}
				cameraPosition = target + offset;
			}
		}

		cameraRotate.x += pitchDelta;
		cameraRotate.y += yawDelta;
	} else if (isLeftDragging) {
		const Matrix4x4 rotateMatrix = MakeRotateXYZMatrix(cameraRotate);
		const Vector3 rightAxis{rotateMatrix.m[0][0], rotateMatrix.m[0][1], rotateMatrix.m[0][2]};
		const Vector3 upAxis{rotateMatrix.m[1][0], rotateMatrix.m[1][1], rotateMatrix.m[1][2]};
		const Vector3 right = Normalize(rightAxis);
		const Vector3 up = Normalize(upAxis);
		float distanceScale = 10.0f;
		if (selectedObject) {
			distanceScale = Length(cameraPosition - selectedObject->GetTransform().translate);
			if (distanceScale < 1.0f) {
				distanceScale = 1.0f;
			}
		}
		cameraPosition = cameraPosition + (moveX * distanceScale * kPanSpeed) * right - (moveY * distanceScale * kPanSpeed) * up;
	}

	if (cameraTransform) {
		cameraTransform->translate = cameraPosition;
		cameraTransform->rotate = cameraRotate;
	} else if (fallbackCamera) {
		fallbackCamera->SetTranslate(cameraPosition);
		fallbackCamera->SetRotate(cameraRotate);
	}
#endif
}

void BaseScene::SetActiveCameraObject(GameObject* object) {
	if (!object || !object->GetComponent<CameraComponent>()) {
		return;
	}

	activeCameraObjectName_ = object->GetName();
	ApplyActiveCamera();
}

GameObject* BaseScene::FindFirstCameraObject() {
	for (const auto& object : sceneObjects_) {
		if (object->GetComponent<CameraComponent>()) {
			return object.get();
		}
	}
	return nullptr;
}

GameObject* BaseScene::FindObjectByName(const std::string& name) const {
	if (name.empty()) {
		return nullptr;
	}

	for (const auto& object : sceneObjects_) {
		if (object->GetName() == name) {
			return object.get();
		}
	}
	return nullptr;
}

std::string BaseScene::MakeUniqueObjectName(const std::string& baseName) const {
	return baseName + "_" + std::to_string(nextObjectId_);
}

void BaseScene::SaveEditorObjects() {
	nlohmann::json root;
	root["scene"] = sceneName_;
	root["nextObjectId"] = nextObjectId_;
	root["activeCamera"] = activeCameraObjectName_;
	root["objects"] = nlohmann::json::array();

	for (const auto& object : sceneObjects_) {
		const EulerTransform& transform = object->GetTransform();

		nlohmann::json objectJson;
		objectJson["name"] = object->GetName();
		objectJson["type"] = object->GetEditorType();
		if (object->GetEditorType().starts_with("LoadedModel:")) {
			objectJson["type"] = "LoadedModel";
			objectJson["model"] = object->GetEditorType().substr(std::string("LoadedModel:").size());
		}
		if (CameraComponent* cameraComponent = object->GetComponent<CameraComponent>()) {
			objectJson["camera"]["fovY"] = cameraComponent->GetFovY();
			objectJson["camera"]["nearClip"] = cameraComponent->GetNearClip();
			objectJson["camera"]["farClip"] = cameraComponent->GetFarClip();
			objectJson["camera"]["followTarget"] = cameraComponent->GetFollowTargetName();
		}
		if (OBBColliderComponent* collider = object->GetComponent<OBBColliderComponent>()) {
			objectJson["obbCollider"]["centerOffset"] = Vector3ToJson(collider->GetCenterOffset());
			objectJson["obbCollider"]["halfSize"] = Vector3ToJson(collider->GetHalfSize());
			objectJson["obbCollider"]["drawDebug"] = collider->GetDrawDebug();
		}
		objectJson["transform"]["scale"] = Vector3ToJson(transform.scale);
		objectJson["transform"]["rotate"] = Vector3ToJson(transform.rotate);
		objectJson["transform"]["translate"] = Vector3ToJson(transform.translate);

		root["objects"].push_back(objectJson);
	}

	const std::string filePath = GetSceneObjectFilePath();
	std::filesystem::create_directories(std::filesystem::path(filePath).parent_path());

	std::ofstream ofs(filePath);
	if (!ofs) {
		return;
	}
	ofs << root.dump(4);
}

void BaseScene::LoadEditorObjects() {
	const std::string filePath = GetSceneObjectFilePath();
	std::ifstream ifs(filePath);
	if (!ifs) {
		return;
	}

	nlohmann::json root;
	ifs >> root;

	sceneObjects_.clear();
	selectedObjectIndex_ = -1;
	nextObjectId_ = root.value("nextObjectId", 1);
	activeCameraObjectName_ = root.value("activeCamera", "");

	const nlohmann::json objects = root.value("objects", nlohmann::json::array());
	for (const auto& objectJson : objects) {
		const std::string typeName = objectJson.value("type", "Empty");
		const std::string modelFilePath = objectJson.value("model", "");
		GameObject* object = CreateEditorObject(EditorCreateTypeFromName(typeName), modelFilePath);
		if (!object) {
			continue;
		}

		object->SetName(objectJson.value("name", object->GetName()));
		if (typeName == "LoadedModel" && !modelFilePath.empty()) {
			object->SetEditorType("LoadedModel:" + modelFilePath);
		} else {
			object->SetEditorType(typeName);
		}

		const nlohmann::json transformJson = objectJson.value("transform", nlohmann::json::object());
		EulerTransform& transform = object->GetTransform();
		transform.scale = JsonToVector3(transformJson.value("scale", nlohmann::json::array()), transform.scale);
		transform.rotate = JsonToVector3(transformJson.value("rotate", nlohmann::json::array()), transform.rotate);
		transform.translate = JsonToVector3(transformJson.value("translate", nlohmann::json::array()), transform.translate);

		if (CameraComponent* cameraComponent = object->GetComponent<CameraComponent>()) {
			const nlohmann::json cameraJson = objectJson.value("camera", nlohmann::json::object());
			cameraComponent->SetFovY(cameraJson.value("fovY", cameraComponent->GetFovY()));
			cameraComponent->SetNearClip(cameraJson.value("nearClip", cameraComponent->GetNearClip()));
			cameraComponent->SetFarClip(cameraJson.value("farClip", cameraComponent->GetFarClip()));
			cameraComponent->SetFollowTargetName(cameraJson.value("followTarget", ""));
		}
		if (objectJson.contains("obbCollider")) {
			const nlohmann::json colliderJson = objectJson.value("obbCollider", nlohmann::json::object());
			OBBColliderComponent* collider = object->GetComponent<OBBColliderComponent>();
			if (!collider) {
				collider = object->AddComponent<OBBColliderComponent>();
			}
			collider->SetCenterOffset(JsonToVector3(colliderJson.value("centerOffset", nlohmann::json::array()), collider->GetCenterOffset()));
			collider->SetHalfSize(JsonToVector3(colliderJson.value("halfSize", nlohmann::json::array()), collider->GetHalfSize()));
			collider->SetDrawDebug(colliderJson.value("drawDebug", collider->GetDrawDebug()));
		}
	}

	ResolveCameraLinks();
	ApplyActiveCamera();

	if (!sceneObjects_.empty()) {
		selectedObjectIndex_ = 0;
	}
	nextObjectId_ = root.value("nextObjectId", nextObjectId_);
}

std::string BaseScene::GetSceneObjectFilePath() const {
	return "Resources/Data/Scenes/" + sceneName_ + "_objects.json";
}

BaseScene::EditorCreateType BaseScene::EditorCreateTypeFromName(const std::string& typeName) const {
	if (typeName == "Object3dSphere") {
		return EditorCreateType::Object3dSphere;
	}
	if (typeName == "Object3dCylinder") {
		return EditorCreateType::Object3dCylinder;
	}
	if (typeName == "Sprite") {
		return EditorCreateType::Sprite;
	}
	if (typeName == "LoadedModel" || typeName.starts_with("LoadedModel:")) {
		return EditorCreateType::LoadedModel;
	}
	if (typeName == "Camera") {
		return EditorCreateType::Camera;
	}
	return EditorCreateType::Empty;
}
