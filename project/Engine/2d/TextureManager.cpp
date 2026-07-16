// TextureManager.cpp
#include "TextureManager.h"
#include "ParticleManager.h"
#include "StringUtility.h"
#include <algorithm>
#include <cstring>

TextureManager* TextureManager::instance = nullptr;
using namespace StringUtility;

uint32_t TextureManager::kSRVIndexTop = 1;

/// <summary>
/// 必要なリソースを準備し、オブジェクトを初期化します。
/// </summary>
/// <param name="dxcommon">DirectX 共通処理へアクセスするための参照を指定します。</param>
void TextureManager::Initialize(DirectXCommon* dxcommon) {
	dxCommon_ = dxcommon;

	assert(dxCommon_ != nullptr);
}

/// <summary>
/// 共有インスタンスを取得します。
/// </summary>
/// <returns>処理結果を返します。</returns>
TextureManager* TextureManager::GetInstance() {
	if (instance == nullptr) {
		instance = new TextureManager;
	}
	return instance;
}

/// <summary>
/// 確保したリソースを解放し、終了処理を行います。
/// </summary>
void TextureManager::Finalize() {
	for (auto& pair : textureDatas) {
		pair.second.resource.Reset();
	}
	textureDatas.clear();
	delete instance;
	instance = nullptr;
}

void TextureManager::Release() {}

/// <summary>
/// TextureIndexByFilePath を取得します。
/// </summary>
/// <param name="filepath">読み込みまたは保存に使用するファイルパスを指定します。</param>
/// <returns>処理結果を返します。</returns>
uint32_t TextureManager::GetTextureIndexByFilePath(const std::string& filepath) {
	assert(!SrvManager::GetInstance()->IsOverAllocated());
	return 0;
}

/// <summary>
/// Texture を読み込み、内部データへ反映します。
/// </summary>
/// <param name="filepath">読み込みまたは保存に使用するファイルパスを指定します。</param>
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
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = UINT_MAX;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

	} else {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = UINT(mipImages.GetMetadata().mipLevels);
	}
	//srvManager->CreateSRVforTexture2D(textureData.srvIndex, textureData.resource.Get(), textureData.metadata.format, static_cast<UINT>(textureData.metadata.mipLevels));
	dxCommon_->GetDevice()->CreateShaderResourceView(textureData.resource.Get(), &srvDesc, textureData.srvHandleCPU);

	dxCommon_->UploadTextureData(textureData.resource, mipImages);
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = textureData.resource.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	dxCommon_->GetCommandList()->ResourceBarrier(1, &barrier);
}

void TextureManager::CreateTextureFromRGBA(const std::string& key, uint32_t width, uint32_t height, const std::vector<uint8_t>& pixels) {
	if (textureDatas.contains(key) || !dxCommon_ || width == 0 || height == 0 || pixels.size() < static_cast<size_t>(width) * height * 4) {
		return;
	}
	DirectX::ScratchImage image;
	HRESULT hr = image.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, width, height, 1, 1);
	assert(SUCCEEDED(hr));
	const DirectX::Image* destination = image.GetImage(0, 0, 0);
	for (uint32_t row = 0; row < height; ++row) {
		std::memcpy(destination->pixels + destination->rowPitch * row, pixels.data() + static_cast<size_t>(width) * row * 4, static_cast<size_t>(width) * 4);
	}

	TextureData& textureData = textureDatas[key];
	textureData.metadata = image.GetMetadata();
	textureData.resource = dxCommon_->CreateTextureResource(textureData.metadata);
	SrvManager* srvManager = SrvManager::GetInstance();
	textureData.srvIndex = srvManager->Allocate();
	textureData.srvHandleCPU = srvManager->GetCPUDescriptorHandle(textureData.srvIndex);
	textureData.srvHandleGPU = srvManager->GetGPUDescriptorHandle(textureData.srvIndex);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = textureData.metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	dxCommon_->GetDevice()->CreateShaderResourceView(textureData.resource.Get(), &srvDesc, textureData.srvHandleCPU);
	dxCommon_->UploadTextureData(textureData.resource, image);

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = textureData.resource.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	dxCommon_->GetCommandList()->ResourceBarrier(1, &barrier);
}

bool TextureManager::ReloadTexture(const std::string& filepath) {
	auto textureIt = textureDatas.find(filepath);
	if (textureIt == textureDatas.end() || !dxCommon_) {
		return false;
	}

	HRESULT hr;
	DirectX::ScratchImage image{};
	if (filepath.ends_with(".dds")) {
		hr = DirectX::LoadFromDDSFile(ConvertString(filepath).c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
	} else {
		hr = DirectX::LoadFromWICFile(ConvertString(filepath).c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	}
	if (FAILED(hr)) {
		return false;
	}

	DirectX::ScratchImage mipImages{};
	if (DirectX::IsCompressed(image.GetMetadata().format)) {
		mipImages = std::move(image);
	} else {
		hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
		if (FAILED(hr)) {
			return false;
		}
	}

	TextureData replacement = textureIt->second;
	replacement.metadata = mipImages.GetMetadata();
	replacement.resource = dxCommon_->CreateTextureResource(replacement.metadata);
	dxCommon_->UploadTextureData(replacement.resource, mipImages);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = replacement.metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	if (replacement.metadata.IsCubemap()) {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = UINT_MAX;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	} else {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = static_cast<UINT>(replacement.metadata.mipLevels);
	}
	dxCommon_->GetDevice()->CreateShaderResourceView(replacement.resource.Get(), &srvDesc, replacement.srvHandleCPU);

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = replacement.resource.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	dxCommon_->GetCommandList()->ResourceBarrier(1, &barrier);

	textureIt->second = std::move(replacement);
	return true;
}

size_t TextureManager::ReloadAllTextures() {
	size_t reloadedCount = 0;
	const std::vector<std::string> filePaths = GetLoadedTextureNames();
	for (const std::string& filePath : filePaths) {
		if (ReloadTexture(filePath)) {
			++reloadedCount;
		}
	}
	return reloadedCount;
}

/// <summary>
/// 読み込み済みテクスチャのファイルパス一覧を取得します。
/// </summary>
/// <returns>読み込み済みテクスチャのファイルパス一覧を返します。</returns>
std::vector<std::string> TextureManager::GetLoadedTextureNames() const {
	std::vector<std::string> names;
	names.reserve(textureDatas.size());
	for (const auto& [filePath, textureData] : textureDatas) {
		if (filePath.starts_with("__runtime_")) {
			continue;
		}
		names.push_back(filePath);
	}
	std::sort(names.begin(), names.end());
	return names;
}
