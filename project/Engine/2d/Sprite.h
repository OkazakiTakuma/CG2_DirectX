#pragma once
#include "Matrix.h"
#include "Screen.h"
#include "Vector.h"
#include "struct.h"
#include <Windows.h>
#include <d3d12.h>
#include <string>
#include <wrl.h>

// 前方宣言は不要になります（GetInstanceを使用するため）
class Sprite {
public:
	// 第1引数の SpriteCommon* を削除
	void Initialize(std::string textureFilePath);
	void Update();
	void Draw();

	// --- ゲッター・セッター群 (変更なし) ---
	const Transform& GetTransform() { return transform; };
	void SetTransform(const Transform& newTransform) { transform = newTransform; }
	const Transform& GetUVTransform() { return uvTransform; }
	void SetUVTransform(const Transform& newUVTransform) { uvTransform = newUVTransform; }
	const Vector4& GetColor() const { return materialData->color; }
	void SetColor(const Vector4& newColor) { materialData->color = newColor; }
	const Vector2 GetSize() const { return size; }
	void SetSize(const Vector2& newSize) { size = newSize; }
	const Vector2& GetAnchorPoint() const { return anchorPoint; }
	void SetAnchorPoint(const Vector2& newAnchorPoint) { anchorPoint = newAnchorPoint; }
	const bool GetIsFlipX() const { return isFlipX; }
	void SetIsFlipX(bool flipX) { isFlipX = flipX; }
	const bool GetIsFlipY() const { return isFlipY; }
	void SetIsFlipY(bool flipY) { isFlipY = flipY; }
	const Vector2& GetTextureLeftTop() const { return textureLeftTop; }
	void SetTextureLeftTop(const Vector2& leftTop) { textureLeftTop = leftTop; }
	const Vector2& GetTextureSize() const { return textureSize; }
	void SetTextureSize(const Vector2& size) { textureSize = size; }

	~Sprite();

private:
	struct Material {
		Vector4 color;
		int32_t enableLighting;
		float padding[3];
		Matrix4x4 uvTransform;
	};

	

	std::string filepath;
	uint32_t* indexData = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;
	D3D12_INDEX_BUFFER_VIEW indexBufferView{};
	VertexData* vertexData = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferview{};
	Material* materialData = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource;

	Transform transform{
	    {1.0f, 1.0f, 1.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}
    };
	Transform uvTransform{
	    {1.0f, 1.0f, 1.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}
    };

	// SpriteCommon* spriteCommon; // シングルトン化により削除

	TransformationMatrix* transformationMatrixData = nullptr;
	Vector2 size = {0, 0};
	uint32_t textureIndex = 0;
	Vector2 anchorPoint = {0.0f, 0.0f};
	bool isFlipX = false;
	bool isFlipY = false;
	Vector2 textureLeftTop = {0.0f, 0.0f};
	Vector2 textureSize = {512.0f, 512.0f};

	void AdjustTextureSize();
};