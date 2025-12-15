// TextureManager.cpp
#include "TextureManager.h"
#include "StringUtility.h"

TextureManager* TextureManager::instance = nullptr;
using namespace StringUtility;

uint32_t TextureManager::kSRVIndexTop = 1;

void TextureManager::Initialize(SrvManager* srv) {
	textureDatas.reserve(SrvManager::kMaxSRVCount);
	srvManager = srv;
}

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
	assert(srvManager->IsOverAllocated());
	assert(0);
	return 0;
}


void TextureManager::LoadTexture(const std::string& filepath) {
	if (textureDatas.contains(filepath)){
		return;
	}
	assert(srvManager->IsOverAllocated());

	DirectX::ScratchImage image{};
	DirectX::ScratchImage mipImages{};

	HRESULT hr = DirectX::LoadFromWICFile(ConvertString(filepath).c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	assert(SUCCEEDED(hr));


	TextureData& textureData = textureDatas[filepath];
	textureData.metadata = mipImages.GetMetadata();
	textureData.resource = dxCommon_->CreateTextureResource(textureData.metadata);
	textureData.srvIndex = srvManager->Allcate();
	textureData.srvHandleCPU = srvManager->GetCPUDescriptorHandle(textureData.srvIndex);
	textureData.srvHandleGPU = srvManager->GetGPUDescriptorHandle(textureData.srvIndex);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = textureData.metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // シェーダーコンポーネントのマッピング
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;                      // テクスチャの次元
	srvDesc.Texture2D.MipLevels = UINT(mipImages.GetMetadata().mipLevels);      // ミップレベルの数
	dxCommon_->GetDevice()->CreateShaderResourceView(textureData.resource.Get(), &srvDesc, textureData.srvHandleCPU);

	dxCommon_->UploadTextureData(textureData.resource, mipImages);
}

