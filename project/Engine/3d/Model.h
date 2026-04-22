#pragma once
#include "struct.h"
#include "Matrix.h"
#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>

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

private:
	ModelCommon* modelCommon_ = nullptr;
	ModelData modelData;
	static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
	Material* materialData = nullptr;
	void CreateMaterialData();
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	VertexData* vertexData = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	void CreateVertexdata();
};
