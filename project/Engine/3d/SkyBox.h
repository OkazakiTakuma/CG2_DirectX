#pragma once
#include "Camera.h"
#include "SkyBoxCommon.h"
#include "struct.h"
#include <string>
#include <wrl.h>

class SkyBox {
public:
	void Initialize(const std::string& filePath);
	void Update();
	void Draw();

private:
	void CreateVertexData();
	void CreateConstantBuffers();

	SkyBoxCommon* common_ = nullptr;
	std::string textureFilePath;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};

	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;
	D3D12_INDEX_BUFFER_VIEW indexBufferView{};

	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
	Material* materialData = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> transformResource;
	TransformationMatrix* transformData = nullptr;
	Camera* camera = nullptr;
};
