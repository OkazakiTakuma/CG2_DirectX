#include "ModelManager.h"
#include "Model.h"
#include "ModelCommon.h"
#include <cassert>

/// <summary>
/// 必要なリソースを準備し、オブジェクトを初期化します。
/// </summary>
/// <param name="dxCommon">DirectX 共通処理へアクセスするための参照を指定します。</param>
void ModelManager::Initialize(DirectXCommon* dxCommon) {
	assert(dxCommon);
	if (!modelCommon) {
		modelCommon = std::make_unique<ModelCommon>();
	}
	modelCommon->Initialize(dxCommon);
}

void ModelManager::EnsureInitialized(DirectXCommon* dxCommon) {
	assert(dxCommon);
	if (!modelCommon || modelCommon->GetDxCommon() != dxCommon) {
		Initialize(dxCommon);
	}
}

/// <summary>
/// 共有インスタンスを取得します。
/// </summary>
ModelManager* ModelManager::GetInstance() {
	static ModelManager instance;
	return &instance;
}

/// <summary>
/// 確保したリソースを解放し、終了処理を行います。
/// </summary>
void ModelManager::Finalize() {
	models.clear();
	modelCommon.reset();
}

/// <summary>
/// Model を読み込み、内部データへ反映します。
/// </summary>
void ModelManager::LoadModel(const std::string& filePath, bool isAnimation, const std::string& directoryPath) {
	if (models.contains(filePath)) {
		return;
	}
	std::unique_ptr<Model> model = std::make_unique<Model>();
	model->Initialize(modelCommon.get(), "Resources" + directoryPath, filePath, isAnimation);
	models.insert(std::make_pair(filePath, std::move(model)));
}

/// <summary>
/// Model を検索して取得します。
/// </summary>
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
