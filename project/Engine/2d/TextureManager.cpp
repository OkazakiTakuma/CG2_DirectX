// TextureManager.cpp
#include "TextureManager.h"
#include "../base/StringUtility.h"

TextureManager* TextureManager::instance = nullptr;
using namespace StringUtility;

uint32_t TextureManager::kSRVIndexTop = 1;

void TextureManager::Initialize() { textureDatas.reserve(DirectXCommon::kMaxSRVCount); }

TextureManager* TextureManager::GetInstance() {
	if (instance == nullptr) {
		instance = new TextureManager;
	}
	return instance;
}

void TextureManager::Finalize() {
	delete instance;
	instance = nullptr;
}

void TextureManager::Rerease() {}

uint32_t TextureManager::GetTextureIndexByFilePath(const std::string& filepath) {
	auto it = std::find_if(textureDatas.begin(), textureDatas.end(), [&](TextureData& texturedata) { return texturedata.filePath == filepath; });
	if (it != textureDatas.end()) {
		uint32_t textureIndex = static_cast<uint32_t>(std::distance(textureDatas.begin(), it));
		return textureIndex;
	}
	assert(0);
	return 0;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSRVHandleGPU(uint32_t textureIndex) {
	assert(textureIndex < DirectXCommon::kMaxSRVCount);
	assert(textureIndex < textureDatas.size());

	TextureData& textureData = textureDatas[textureIndex];
	return textureData.srvHandleGPU;
}

void TextureManager::LoadTexture(const std::string& filepath) {
	auto it = std::find_if(textureDatas.begin(), textureDatas.end(), [&](TextureData& texturedata) { return texturedata.filePath == filepath; });
	if (it != textureDatas.end()) {
		return;
	}
	assert(textureDatas.size() + kSRVIndexTop < DirectXCommon::kMaxSRVCount);

	DirectX::ScratchImage image{};
	DirectX::ScratchImage mipImages{};

	HRESULT hr = DirectX::LoadFromWICFile(ConvertString(filepath).c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	assert(SUCCEEDED(hr));

	textureDatas.resize(textureDatas.size() + 1);

	TextureData& textureData = textureDatas.back();
	textureData.filePath = filepath;
	textureData.metadata = mipImages.GetMetadata();
	textureData.resource = dxCommon_->CreateTextureResource(textureData.metadata);

	uint32_t srvIndex = static_cast<uint32_t>(textureDatas.size() - 1) + kSRVIndexTop;
	textureData.srvHandleCPU = dxCommon_->GetSRVCPUDescriptorHandle(srvIndex);
	textureData.srvHandleGPU = dxCommon_->GetSRVGPUDescriptorHandle(srvIndex);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = textureData.metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // シェーダーコンポーネントのマッピング
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;                      // テクスチャの次元
	srvDesc.Texture2D.MipLevels = UINT(mipImages.GetMetadata().mipLevels);      // ミップレベルの数
	dxCommon_->GetDevice()->CreateShaderResourceView(textureData.resource.Get(), &srvDesc, textureData.srvHandleCPU);

	dxCommon_->UploadTextureData(textureData.resource, mipImages);
}

const DirectX::TexMetadata& TextureManager::GetTextureMetadata(uint32_t textureIndex) {
	assert(textureIndex < DirectXCommon::kMaxSRVCount);
	assert(textureIndex < textureDatas.size());
	TextureData& textureData = textureDatas[textureIndex];
	return textureData.metadata;
}
