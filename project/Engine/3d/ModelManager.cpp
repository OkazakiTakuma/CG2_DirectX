#include "ModelManager.h"
#include "Model.h"
#include "ModelCommon.h"

ModelManager* ModelManager::instance = nullptr;

void ModelManager::Inithialize(DirectXCommon* dxCommon) {
	modelCommon = std::make_unique<ModelCommon>();
	modelCommon->Initialize(dxCommon);
}

ModelManager* ModelManager::GetInstance() {
	if (instance == nullptr) {
		instance = new ModelManager;
	}
	return instance;
}

void ModelManager::Finalize() {
	models.clear();
	modelCommon.reset();
	delete instance;
	instance = nullptr;
}

void ModelManager::LoadModel(const std::string& filePath, bool isAnimation, const std::string& directoryPath) {
	if (models.contains(filePath)) {
		return;
	}
	std::unique_ptr<Model> model = std::make_unique<Model>();
	model->Initialize(modelCommon.get(), "Resources" + directoryPath, filePath, isAnimation);
	models.insert(std::make_pair(filePath, std::move(model)));
}

Model* ModelManager::FindModel(const std::string& filePath) {
	if (models.contains(filePath)) {
		return models.at(filePath).get();
	}
	return nullptr;
}

std::vector<std::string> ModelManager::GetLoadedModelNames() const {
	std::vector<std::string> modelNames;
	modelNames.reserve(models.size());
	for (const auto& [name, model] : models) {
		modelNames.push_back(name);
	}
	return modelNames;
}
