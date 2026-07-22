#pragma once
#include "struct.h"
#include <map>
#include <memory>
#include <string>
#include <vector>


class Model;
class ModelCommon;
class DirectXCommon;

/// <summary>
/// 読み込み済みモデルをファイルパス単位で所有し、検索と再利用を提供します。
/// モデル共通描画リソースのライフサイクルも管理します。
/// </summary>
class ModelManager {
public:
	/// <summary>
	/// 共有インスタンスを取得します。
	/// </summary>
	static ModelManager* GetInstance();
	/// <summary>
	/// 必要なリソースを準備し、オブジェクトを初期化します。
	/// </summary>
	/// <param name="dxCommon">DirectX 共通処理へアクセスするための参照を指定します。</param>
	void Initialize(DirectXCommon* dxCommon);
	/// <summary>
	/// モデル共通コンテキストが有効であることを保証します。
	/// </summary>
	void EnsureInitialized(DirectXCommon* dxCommon);
	/// <summary>
	/// 確保したリソースを解放し、終了処理を行います。
	/// </summary>
	void Finalize();
	/// <summary>
	/// Model を読み込み、内部データへ反映します。
	/// </summary>
	void LoadModel(const std::string& filePath, bool isAnimation = false, const std::string& directoryPath = "/");
	/// <summary>
	/// Model を検索して取得します。
	/// </summary>
	Model* FindModel(const std::string& filePath);
	std::vector<std::string> GetLoadedModelNames() const;

private:
	std::map<std::string, std::unique_ptr<Model>> models;
	ModelManager() = default;
	~ModelManager() = default;
	ModelManager(const ModelManager&) = delete;
	ModelManager& operator=(const ModelManager&) = delete;
	std::unique_ptr<ModelCommon> modelCommon = nullptr;
};
