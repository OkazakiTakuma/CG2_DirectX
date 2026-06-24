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
class Model {
public:
	// 初期化
	void Initialize(ModelCommon* modelCommon, const std::string& directoryPath, const std::string& filename);
	// 終了
	void Finalize();
	// 描画前処理
	void Draw();
	~Model();
	const Node& GetRootNode() const { return modelData.rootNode; }
	uint32_t GetVertexCount() const {
		return static_cast<uint32_t>(modelData.vertices.size());
	}
private:
	ModelCommon* modelCommon_ = nullptr;
	ModelData modelData;
	static ModelData LoadModelFile(const std::string& directoryPath, const std::string& filename);
	static Node ReadNode(aiNode* aiNode);
	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
	Material* materialData = nullptr;
	void CreateMaterialData();
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	VertexData* vertexData = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	void CreateVertexdata();
};