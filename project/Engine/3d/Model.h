#pragma once
#include "Matrix.h"
#include "struct.h"
#include <Windows.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <d3d12.h>
#include <wrl.h>

// Forward declaration for Assimp node type to avoid including Assimp headers in this header

class ModelCommon;
class Model {
public:
	friend class InstancingModel;

	/// <summary>
	/// 必要なリソースを準備し、オブジェクトを初期化します。
	/// </summary>
	/// <param name="modelCommon">modelCommon に使用する値を指定します。</param>
	/// <param name="directoryPath">読み込みまたは保存に使用するファイルパスを指定します。</param>
	/// <param name="filename">読み込みまたは保存に使用するファイルパスを指定します。</param>
	/// <param name="isAnimation">isAnimation に使用する値を指定します。</param>
	void Initialize(ModelCommon* modelCommon, const std::string& directoryPath, const std::string& filename,const bool isAnimation);
	/// <summary>
	/// 確保したリソースを解放し、終了処理を行います。
	/// </summary>
	void Finalize();
	/// <summary>
	/// 現在の状態をもとに描画処理を行います。
	/// </summary>
	void Draw(ID3D12Resource* overrideMaterialResource = nullptr);
	/// <summary>
	/// 破棄時に必要な解放処理を行います。
	/// </summary>
	~Model();
	const Node& GetRootNode() const { return modelData.rootNode; }
	const ModelData& GetModelData() const { return modelData; }
	/// <summary>
	/// VertexCount を取得します。
	/// </summary>
	/// <returns>処理結果を返します。</returns>
	uint32_t GetVertexCount() const {
		return static_cast<uint32_t>(modelData.vertices.size());
	}
	/// <summary>
	/// Animation を読み込み、内部データへ反映します。
	/// </summary>
	/// <param name="directoryPath">読み込みまたは保存に使用するファイルパスを指定します。</param>
	/// <param name="filename">読み込みまたは保存に使用するファイルパスを指定します。</param>
	/// <returns>処理結果を返します。</returns>
	Animation LoadAnimation(const std::string& directoryPath, const std::string& filename);
	Animation GetAnimation() { return animation; };
	const bool GetIsAnimation() { return isAnimation_; };
	bool HasSkinCluster() const { return !modelData.skincluserData.empty(); }
	void SetTextureFilePath(const std::string& textureFilePath);
	const std::string& GetTextureFilePath() const { return modelData.material.textureFilePath; }
	/// <summary>
	/// SkinningPaletteSize を取得します。
	/// </summary>
	/// <returns>処理結果を返します。</returns>
	uint32_t GetSkinningPaletteSize() const;
	/// <summary>
	/// SkinningPalette を構築します。
	/// </summary>
	/// <param name="skeleton">skeleton に使用する値を指定します。</param>
	/// <param name="palette">palette に使用する値を指定します。</param>
	void BuildSkinningPalette(const Skeleton& skeleton, std::vector<Matrix4x4>& palette) const;
	/// <summary>
	/// Skinning を現在の状態へ反映します。
	/// </summary>
	/// <param name="skeleton">skeleton に使用する値を指定します。</param>
	void ApplySkinning(const Skeleton& skeleton);

private:
	ModelCommon* modelCommon_ = nullptr;
	ModelData modelData;
	std::vector<VertexData> originalVertices_;
	std::vector<VertexData> skinnedVertices_;
	std::vector<float> skinWeights_;
	Animation animation;
	bool isAnimation_;
	/// <summary>
	/// ModelFile を読み込み、内部データへ反映します。
	/// </summary>
	/// <param name="directoryPath">読み込みまたは保存に使用するファイルパスを指定します。</param>
	/// <param name="filename">読み込みまたは保存に使用するファイルパスを指定します。</param>
	/// <returns>処理結果を返します。</returns>
	static ModelData LoadModelFile(const std::string& directoryPath, const std::string& filename);
	/// <summary>
	/// ReadNode の処理を行います。
	/// </summary>
	/// <param name="aiNode">aiNode に使用する値を指定します。</param>
	/// <returns>処理結果を返します。</returns>
	static Node ReadNode(aiNode* aiNode);
	/// <summary>
	/// MaterialTemplateFile を読み込み、内部データへ反映します。
	/// </summary>
	/// <param name="directoryPath">読み込みまたは保存に使用するファイルパスを指定します。</param>
	/// <param name="filename">読み込みまたは保存に使用するファイルパスを指定します。</param>
	/// <returns>処理結果を返します。</returns>
	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
	Material* materialData = nullptr;
	/// <summary>
	/// MaterialData を作成し、利用できる状態にします。
	/// </summary>
	void CreateMaterialData();
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	VertexData* vertexData = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	/// <summary>
	/// Vertexdata を作成し、利用できる状態にします。
	/// </summary>
	void CreateVertexdata();
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;
	/// <summary>
	/// IndexData を作成し、利用できる状態にします。
	/// </summary>
	void CreateIndexData();

};
