// TextureManager.cpp
#include "TextureManager.h"
#include "ParticleManager.h"
#include "StringUtility.h"

TextureManager* TextureManager::instance = nullptr;
using namespace StringUtility;

uint32_t TextureManager::kSRVIndexTop = 1;

void TextureManager::Initialize(DirectXCommon* dxcommon) {
	dxCommon_ = dxcommon;

	assert(dxCommon_ != nullptr);
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
	assert(!SrvManager::GetInstance()->IsOverAllocated());
	return 0;
}

void TextureManager::LoadTexture(const std::string& filepath) {
	HRESULT hr;
	if (textureDatas.contains(filepath)) {
		return;
	}
	SrvManager* srvManager = SrvManager::GetInstance();
	DirectX::ScratchImage image{};
	if (filepath.ends_with(".dds")) {
		hr = DirectX::LoadFromDDSFile(ConvertString(filepath).c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
	} else {
		hr = DirectX::LoadFromWICFile(ConvertString(filepath).c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	}

	assert(SUCCEEDED(hr));

	DirectX::ScratchImage mipImages{};
	if (DirectX::IsCompressed(image.GetMetadata().format)) {
		mipImages = std::move(image);
	} else {

		hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	}
	assert(SUCCEEDED(hr));

	TextureData& textureData = textureDatas[filepath];
	textureData.metadata = mipImages.GetMetadata();
	textureData.resource = dxCommon_->CreateTextureResource(textureData.metadata);
	textureData.srvIndex = srvManager->Allocate();
	textureData.srvHandleCPU = srvManager->GetCPUDescriptorHandle(textureData.srvIndex);
	textureData.srvHandleGPU = srvManager->GetGPUDescriptorHandle(textureData.srvIndex);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = textureData.metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	if (textureData.metadata.IsCubemap()) {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE; // シェーダーコンポ
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = UINT_MAX; // ミップレベルの数
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

	} else {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;                 // テクスチャの次元
		srvDesc.Texture2D.MipLevels = UINT(mipImages.GetMetadata().mipLevels); // ミップレベルの数
	}
	srvManager->CreateSRVforTexture2D(textureData.srvIndex, textureData.resource.Get(), textureData.metadata.format, static_cast<UINT>(textureData.metadata.mipLevels));
	dxCommon_->UploadTextureData(textureData.resource, mipImages);
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = textureData.resource.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES; // 全ミップレベル対象
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	// dxCommon_ 内のコマンドリスト、または現在ロードに使っているコマンドリストに積む
	// ※dxCommon_->GetCommandList() のようなメソッドがあると仮定しています
	dxCommon_->GetCommandList()->ResourceBarrier(1, &barrier);
}
