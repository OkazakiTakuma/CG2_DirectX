#include "Sprite.h"
#include "TextureManager.h"
#include "SpriteCommon.h"

void Sprite::Initialize(SpriteCommon* spriteCommon, std::string textureFilePath) {
	this->spriteCommon = spriteCommon;

#pragma region スプライトの描画に必要なデータの作成
#pragma region インデックスを使った描画
	indexResource = spriteCommon->GetDxCommon()->CreateBufferResource(sizeof(uint32_t) * 6);
	assert(indexResource != nullptr);                                       // リソース生成が成功したか確認	// リソースの先頭のアドレスから使う
	indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress(); // GPU仮想アドレス
	// 使用するリソースのサイズはインデックスのサイズ * インデックス数
	indexBufferView.SizeInBytes = sizeof(uint32_t) * 6; // インデックスバッファのサイズ
	// インデックスはuint32_t型
	indexBufferView.Format = DXGI_FORMAT_R32_UINT; // 1インデックスのサイズ

	// 書き込むためのアドレスを取得
	indexResource->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
#pragma endregion
	vertexResource = spriteCommon->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * 4);
	assert(vertexResource != nullptr);

	// 頂点バッファビューを作成する

	// リソースの先頭のアドレスから使う
	vertexBufferview.BufferLocation = vertexResource->GetGPUVirtualAddress();
	// リソースの頂点のサイズは頂点4つ分
	vertexBufferview.SizeInBytes = sizeof(VertexData) * 4;
	// 1頂点あたりのサイズ
	vertexBufferview.StrideInBytes = sizeof(VertexData);

	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	// スプライト用のマテリアルリソースを作成
	materialResource = spriteCommon->GetDxCommon()->CreateBufferResource(sizeof(Material));
	assert(materialResource != nullptr);

	// スプライト用のマテリアルリソースにデータを書き込む
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	// スプライトの色を設定
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f); // 白色
	materialData->enableLighting = false;                  // ライティングを無効化
	materialData->uvTransform = MakeIdentity4x4();

	// Sprite用のTransformationMatrix用リソースを作る
	transformationMatrixResource = spriteCommon->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));
	assert(transformationMatrixResource != nullptr);
	// データを書き込む
	transformationMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));
	// 単位行列を入れておく
	transformationMatrixData->WVP = MakeIdentity4x4();
	transformationMatrixData->world = MakeIdentity4x4();
	size = {640.0f, 360.0f};
	// テクスチャの読み込み

	textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);
	filepath = textureFilePath;
	AdjustTextureSize();

#pragma endregion
}
void Sprite::Update() {
	// インデックスデータを設定

	// 三角形1枚目のインデックスを設定
	indexData[0] = 0; // 三角形1枚目の1頂点目
	indexData[1] = 1; // 三角形1枚目の2頂点目
	indexData[2] = 2; // 三角形1枚目の3頂点目

	// 三角形2枚目のインデックスを設定
	indexData[3] = 1; // 三角形2枚目の1頂点目
	indexData[4] = 3; // 三角形2枚目の2頂点目
	indexData[5] = 2; // 三角形2枚目の3頂点目

	// 頂点データを設定

	// anchorPointを反映
	float left = 0.0f - anchorPoint.x;
	float right = 1.0f - anchorPoint.x;
	float top = 0.0f - anchorPoint.y;
	float bottom = 1.0f - anchorPoint.y;

	if (isFlipX) {
		std::swap(left, right);
	}
	if (isFlipY) {
		std::swap(top, bottom);
	}

	const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetTextureMetadata(filepath);
	float uLeft = textureLeftTop.x / static_cast<float>(metadata.width);
	float uRight = (textureLeftTop.x + textureSize.x) / static_cast<float>(metadata.width);
	float vTop = textureLeftTop.y / static_cast<float>(metadata.height);
	float vBottom = (textureLeftTop.y + textureSize.y) / static_cast<float>(metadata.height);


	vertexData[0].position = {left, bottom, 0.0f, 1.0f};
	vertexData[0].texcoord = {uLeft, vBottom};
	vertexData[0].normal = {0.0f, 0.0f, -1.0f};
	vertexData[1].position = {left, top, 0.0f, 1.0f};
	vertexData[1].texcoord = {uLeft, vTop};
	vertexData[1].normal = {0.0f, 0.0f, -1.0f};
	vertexData[2].position = {right, bottom, 0.0f, 1.0f};
	vertexData[2].texcoord = {uRight, vBottom};
	vertexData[2].normal = {0.0f, 0.0f, -1.0f};
	vertexData[3].position = {right, top, 0.0f, 1.0f};
	vertexData[3].texcoord = {uRight, vTop};
	vertexData[3].normal = {0.0f, 0.0f, -1.0f};

	// スプライトのサイズを反映
	transform.scale = {size.x, size.y, 1.0f};
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	Matrix4x4 viewMatrix = MakeIdentity4x4();
	Matrix4x4 projectionMatrix = MakeOrthographicMatrix(0.0f, 0.0f, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0.0f, 100.0f);
	Matrix4x4 wvpMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
	transformationMatrixData->WVP = wvpMatrix;
	Matrix4x4 uvTransformMatrix = MakeAffineMatrix(uvTransform.scale, uvTransform.rotate, uvTransform.translate);

	// UV変換行列をマテリアルに設定
	materialData->uvTransform = uvTransformMatrix;
}
void Sprite::Draw() {
	spriteCommon->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferview);
	spriteCommon->GetDxCommon()->GetCommandList()->IASetIndexBuffer(&indexBufferView);
	spriteCommon->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
	spriteCommon->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResource->GetGPUVirtualAddress());
	spriteCommon->GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(3, TextureManager::GetInstance()->GetSRVHandleGPU(filepath));
	spriteCommon->GetDxCommon()->GetCommandList()->DrawIndexedInstanced(6, 1, 0, 0, 0);
}
void Sprite::AdjustTextureSize() {
	const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetTextureMetadata(filepath);
	textureSize = {static_cast<float>(metadata.width), static_cast<float>(metadata.height)};
	size = textureSize;

};