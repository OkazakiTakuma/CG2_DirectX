#include "Sprite.h"
#include "SpriteCommon.h"
#include "TextureManager.h"
#include "WinApp.h"

void Sprite::Initialize(std::string textureFilePath) {
	SpriteCommon* common = SpriteCommon::GetInstance();
	if (common == nullptr) {
		assert(false && "SpriteCommon instance does not exist.");
	}

	DirectXCommon* dxCommon = common->GetDxCommon();
	if (dxCommon == nullptr) {
		assert(false && "SpriteCommon is not initialized. dxCommon is null.");
	}
	indexResource = dxCommon->CreateBufferResource(sizeof(uint32_t) * 6);
	assert(indexResource != nullptr);
	indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
	indexBufferView.SizeInBytes = sizeof(uint32_t) * 6;
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	indexResource->Map(0, nullptr, reinterpret_cast<void**>(&indexData));

	vertexResource = dxCommon->CreateBufferResource(sizeof(VertexData) * 4);
	assert(vertexResource != nullptr);
	vertexBufferview.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferview.SizeInBytes = sizeof(VertexData) * 4;
	vertexBufferview.StrideInBytes = sizeof(VertexData);
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	materialResource = dxCommon->CreateBufferResource(sizeof(Material));
	assert(materialResource != nullptr);
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData->enableLighting = false;
	materialData->uvTransform = MakeIdentity4x4();

	transformationMatrixResource = dxCommon->CreateBufferResource(sizeof(TransformationMatrix));
	assert(transformationMatrixResource != nullptr);
	transformationMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));
	transformationMatrixData->WVP = MakeIdentity4x4();
	transformationMatrixData->world = MakeIdentity4x4();
	transformationMatrixData->WorldInverseTranspose = MakeIdentity4x4();

	size = {640.0f, 360.0f};
	filepath = textureFilePath;
	TextureManager::GetInstance()->LoadTexture(filepath);
	textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);
	AdjustTextureSize();
}

void Sprite::Update() {
	indexData[0] = 0;
	indexData[1] = 1;
	indexData[2] = 2;
	indexData[3] = 1;
	indexData[4] = 3;
	indexData[5] = 2;

	float left = 0.0f - anchorPoint.x;
	float right = 1.0f - anchorPoint.x;
	float top = 0.0f - anchorPoint.y;
	float bottom = 1.0f - anchorPoint.y;

	if (isFlipX)
		std::swap(left, right);
	if (isFlipY)
		std::swap(top, bottom);

	const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetTextureMetadata(filepath);
	float uLeft = textureLeftTop.x / static_cast<float>(metadata.width);
	float uRight = (textureLeftTop.x + textureSize.x) / static_cast<float>(metadata.width);
	float vTop = textureLeftTop.y / static_cast<float>(metadata.height);
	float vBottom = (textureLeftTop.y + textureSize.y) / static_cast<float>(metadata.height);

	vertexData[0].position = {left, bottom, 0.0f, 1.0f};
	vertexData[0].texcoord = {uLeft, vBottom};
	vertexData[1].position = {left, top, 0.0f, 1.0f};
	vertexData[1].texcoord = {uLeft, vTop};
	vertexData[2].position = {right, bottom, 0.0f, 1.0f};
	vertexData[2].texcoord = {uRight, vBottom};
	vertexData[3].position = {right, top, 0.0f, 1.0f};
	vertexData[3].texcoord = {uRight, vTop};

	transform.scale = {size.x, size.y, 1.0f};
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	Matrix4x4 viewMatrix = MakeIdentity4x4();
	Matrix4x4 projectionMatrix = MakeOrthographicMatrix(0.0f, 0.0f, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0.0f, 100.0f);
	transformationMatrixData->WVP = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
	transformationMatrixData->world = worldMatrix;
	transformationMatrixData->WorldInverseTranspose = worldMatrix;
	materialData->uvTransform = MakeAffineMatrix(uvTransform.scale, uvTransform.rotate, uvTransform.translate);

}

void Sprite::Draw() {
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = SpriteCommon::GetInstance()->GetDxCommon()->GetCommandList();


	commandList->IASetVertexBuffers(0, 1, &vertexBufferview);
	commandList->IASetIndexBuffer(&indexBufferView);
	commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResource->GetGPUVirtualAddress());
	commandList->SetGraphicsRootDescriptorTable(3, TextureManager::GetInstance()->GetSRVHandleGPU(filepath));
	commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

Sprite::~Sprite() {
	if (indexResource)
		indexResource->Unmap(0, nullptr);
	if (vertexResource)
		vertexResource->Unmap(0, nullptr);
	if (materialResource)
		materialResource->Unmap(0, nullptr);
	if (transformationMatrixResource)
		transformationMatrixResource->Unmap(0, nullptr);

	indexResource.Reset();
	vertexResource.Reset();
	materialResource.Reset();
	transformationMatrixResource.Reset();
}

void Sprite::AdjustTextureSize() {
	const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetTextureMetadata(filepath);
	textureSize = {static_cast<float>(metadata.width), static_cast<float>(metadata.height)};
	size = textureSize;
}
