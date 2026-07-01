#pragma once
#include "Matrix.h"
#include "struct.h"
#include <Windows.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <d3d12.h>
#include <wrl.h>

class ModelCommon;

// アニメーション（スキンメッシュ）専用のモデルクラス
class SkinnedModel {
public:
	void Initialize(ModelCommon* modelCommon, const std::string& directoryPath, const std::string& filename);
	void Finalize();
	void Draw();
	~SkinnedModel();

	const Node& GetRootNode() const { return modelData.rootNode; }
	uint32_t GetVertexCount() const { return static_cast<uint32_t>(modelData.vertices.size()); }

	// アニメーションの読み込みと更新
	static Animation LoadAnimationFile(const std::string& directoryPath, const std::string& filename);
	void UpdateAnimation(const Animation& animation, float time);
	void UpdateBoneMatrix();

	// GPUにボーンデータを渡すためのアドレス取得
	D3D12_GPU_VIRTUAL_ADDRESS GetBoneBufferVirtualAddress() const { return boneResource->GetGPUVirtualAddress(); }

private:
	ModelCommon* modelCommon_ = nullptr;
	SkinnedModelData modelData;

	static SkinnedModelData LoadModelFile(const std::string& directoryPath, const std::string& filename);
	static Node ReadNode(aiNode* aiNode);

	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
	Material* materialData = nullptr;
	void CreateMaterialData();

	// VertexDataSkinned を使うように変更
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	VertexDataSkinned* vertexData = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	void CreateVertexdata();

	// GPUスキニング用: 元頂点のSRV（構造化バッファ）と出力UAV
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexStructuredResource; // Default heap with original structured vertices (t0)
	Microsoft::WRL::ComPtr<ID3D12Resource> skinnedOutputResource; // Default heap output (u0)
	void CreateGpuSkinningBuffers();

	// Descriptor indices in global SRV/UAV descriptor heap (allocated from SrvManager)
	uint32_t vertexStructuredSrvIndex = 0xFFFFFFFFu;
	uint32_t skinnedOutputUavIndex = 0xFFFFFFFFu;
	// インデックスバッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;
	D3D12_INDEX_BUFFER_VIEW indexBufferView{};

	// ボーンデータ用のリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> boneResource;
	SkinCluster* mappedSkinCluster = nullptr;
	void CreateBoneData();

	// アニメーション計算用の内部関数
	Vector3 CalculateTranslateValue(const std::vector<KeyframeVector3>& keyframes, float time);
	Quaternion CalculateRotationValue(const std::vector<KeyframeQuaternion>& keyframes, float time);
	Vector3 CalculateScaleValue(const std::vector<KeyframeVector3>& keyframes, float time);
	void UpdateNodeAnimation(Node* node, const Animation& animation, float time, const Matrix4x4& parentGlobalMatrix);
};