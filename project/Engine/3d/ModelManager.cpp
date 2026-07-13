#include "ModelManager.h"
#include "Model.h"
#include "ModelCommon.h"

ModelManager* ModelManager::instance = nullptr;

/// <summary>
/// 必要なリソースを準備し、オブジェクトを初期化します。
/// </summary>
/// <param name="dxCommon">DirectX 共通処理へアクセスするための参照を指定します。</param>
void ModelManager::Inithialize(DirectXCommon* dxCommon) {
	modelCommon = std::make_unique<ModelCommon>();
	modelCommon->Initialize(dxCommon);
}

/// <summary>
/// 共有インスタンスを取得します。
/// </summary>
/// <returns>処理結果を返します。</returns>
ModelManager* ModelManager::GetInstance() {
	if (instance == nullptr) {
		instance = new ModelManager;
	}
	return instance;
}

/// <summary>
/// 確保したリソースを解放し、終了処理を行います。
/// </summary>
void ModelManager::Finalize() {
	models.clear();
	modelCommon.reset();
	delete instance;
	instance = nullptr;
}

/// <summary>
/// Model を読み込み、内部データへ反映します。
/// </summary>
/// <param name="filePath">読み込みまたは保存に使用するファイルパスを指定します。</param>
/// <param name="isAnimation">isAnimation に使用する値を指定します。</param>
/// <param name="directoryPath">読み込みまたは保存に使用するファイルパスを指定します。</param>
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
/// <param name="filePath">読み込みまたは保存に使用するファイルパスを指定します。</param>
/// <returns>処理結果を返します。</returns>
Model* ModelManager::FindModel(const std::string& filePath) {
	if (models.contains(filePath)) {
		return models.at(filePath).get();
	}
	return nullptr;
}

/// <summary>
/// LoadedModelNames を取得します。
/// </summary>
/// <returns>処理結果を返します。</returns>
std::vector<std::string> ModelManager::GetLoadedModelNames() const {
	std::vector<std::string> modelNames;
	modelNames.reserve(models.size());
	for (const auto& [name, model] : models) {
		modelNames.push_back(name);
	}
	return modelNames;
}
