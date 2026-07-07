#include "BaseScene.h"
#include "ModelManager.h"
#ifdef USE_IMGUI
#include "../../../imgui/ImGuizmo.h"
#endif
#include <filesystem>
#include <fstream>
#include <json.hpp>

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
}

void BaseScene::UpdateSceneObjects() {
	for (const auto& object : sceneObjects_) {
		object->Update();
	}
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
	}

	sceneObjects_.push_back(std::move(object));
	selectedObjectIndex_ = static_cast<int>(sceneObjects_.size()) - 1;
	++nextObjectId_;
	return sceneObjects_.back().get();
}

void BaseScene::DeleteSelectedEditorObject() {
	if (selectedObjectIndex_ < 0 || selectedObjectIndex_ >= static_cast<int>(sceneObjects_.size())) {
		return;
	}

	sceneObjects_.erase(sceneObjects_.begin() + selectedObjectIndex_);
	if (sceneObjects_.empty()) {
		selectedObjectIndex_ = -1;
	} else if (selectedObjectIndex_ >= static_cast<int>(sceneObjects_.size())) {
		selectedObjectIndex_ = static_cast<int>(sceneObjects_.size()) - 1;
	}
}

void BaseScene::DrawEditorHierarchy() {
#ifdef USE_IMGUI
	ImGui::Begin("Scene Objects");

	const char* createLabels[] = {"Empty", "3D Sphere", "3D Cylinder", "Sprite", "Loaded Model"};
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
		if (ImGui::Selectable(sceneObjects_[index]->GetName().c_str(), isSelected)) {
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

	ImGui::End();
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

std::string BaseScene::MakeUniqueObjectName(const std::string& baseName) const {
	return baseName + "_" + std::to_string(nextObjectId_);
}

void BaseScene::SaveEditorObjects() {
	nlohmann::json root;
	root["scene"] = sceneName_;
	root["nextObjectId"] = nextObjectId_;
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
	}

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
	return EditorCreateType::Empty;
}
