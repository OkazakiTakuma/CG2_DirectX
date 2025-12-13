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

private:
	ModelCommon* modelCommon_ = nullptr;
	ModelData modelData;
	static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
	struct Material {
		Vector4 color;          // 色
		int32_t enableLighting; // ライティングの有効化フラグ
		float padding[3];       // パディング
		Matrix4x4 uvTransform;  // UV変換行列
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
	Material* materialData = nullptr;
	void CreateMaterialData();
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	VertexData* vertexData = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	void CreateVertexdata();
};
