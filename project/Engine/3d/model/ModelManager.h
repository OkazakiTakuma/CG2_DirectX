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
	/// <returns>処理結果を返します。</returns>
	static ModelManager* GetInstance();
	/// <summary>
	/// 必要なリソースを準備し、オブジェクトを初期化します。
	/// </summary>
	/// <param name="dxCommon">DirectX 共通処理へアクセスするための参照を指定します。</param>
	void Initialize(DirectXCommon* dxCommon);
	/// <summary>
	/// 確保したリソースを解放し、終了処理を行います。
	/// </summary>
	void Finalize();
	/// <summary>
	/// Model を読み込み、内部データへ反映します。
	/// </summary>
	/// <param name="filePath">読み込みまたは保存に使用するファイルパスを指定します。</param>
	/// <param name="isAnimation">isAnimation に使用する値を指定します。</param>
	/// <param name="directoryPath">読み込みまたは保存に使用するファイルパスを指定します。</param>
	void LoadModel(const std::string& filePath, bool isAnimation = false, const std::string& directoryPath = "/");
	/// <summary>
	/// Model を検索して取得します。
	/// </summary>
	/// <param name="filePath">読み込みまたは保存に使用するファイルパスを指定します。</param>
	/// <returns>処理結果を返します。</returns>
	Model* FindModel(const std::string& filePath);
	/// <summary>
	/// LoadedModelNames を取得します。
	/// </summary>
	/// <returns>処理結果を返します。</returns>
	std::vector<std::string> GetLoadedModelNames() const;

private:
	std::map<std::string, std::unique_ptr<Model>> models;
	ModelManager() = default;
	~ModelManager() = default;
	ModelManager(const ModelManager&) = delete;
	ModelManager& operator=(const ModelManager&) = delete;
	std::unique_ptr<ModelCommon> modelCommon = nullptr;
};
