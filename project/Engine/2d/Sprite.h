#pragma once
#include "../3d/Vector.h"
#include "../3d/Matrix.h"
#include "SpriteCommon.h"


struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};


struct Material {
	Vector4 color;          // 色
	int32_t enableLighting; // ライティングの有効化フラグ
	float padding[3];       // パディング
	Matrix4x4 uvTransform;  // UV変換行列
};

struct TransformationMatrix {
	Matrix4x4 WVP;
	Matrix4x4 world;
};


class Sprite {
public:
	void Initialize(SpriteCommon* spriteCommon);
	void Update();
	void Draw(D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU);
	const Transforms& GetTransform() { return transform; };
	void SetTransform(const Transforms& newTransform) { transform = newTransform; }
	const Transforms& GetUVTransform() { return uvTransform; }
	void SetUVTransform(const Transforms& newUVTransform) { uvTransform = newUVTransform; }
	const Vector4& GetColor() const { return materialData->color; }
	void SetColor(const Vector4& newColor) { materialData->color = newColor; }
	const Vector2 GetSize() const { return size; }
	void SetSize(const Vector2& newSize) { size = newSize; }
	

private:
	uint32_t* indexData = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;
	D3D12_INDEX_BUFFER_VIEW indexBufferView{};
	VertexData* vertexData = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferview{};
	Material* materialData = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource;
	Transforms transform{
	    {1.0f, 1.0f, 1.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}
    };
	Transforms uvTransform{
	    {1.0f, 1.0f, 1.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}
    };
	SpriteCommon* spriteCommon = nullptr;
	TransformationMatrix* transformationMatrixData;
	Vector2 size;
};
