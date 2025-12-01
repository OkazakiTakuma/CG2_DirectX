// TextureManager.h
#pragma once
#include "../../extenals/DirectXTex/DirectXTex.h"
#include "../../extenals/DirectXTex/d3dx12.h"
#include "../base/DirectXCommon.h"
#include <string>
#include <vector>

class TextureManager {
public:
	void Initialize(); // 初期化時にセット
	static TextureManager* GetInstance();
	void Finalize();
	void Rerease();
	static uint32_t kSRVIndexTop;
	uint32_t GetTextureIndexByFilePath(const std::string& filepath);
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandleGPU(uint32_t textureIndex);
	void LoadTexture(const std::string& filepath);
	void SetDirectXCommon(DirectXCommon* dxCommon) { dxCommon_ = dxCommon; }

private:
	static TextureManager* instance;
	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(TextureManager&) = delete;
	TextureManager& operator=(const TextureManager&) = delete;

	struct TextureData {
		std::string filePath;
		DirectX::TexMetadata metadata;
		Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
	};

	std::vector<TextureData> textureDatas;
	DirectXCommon* dxCommon_ = nullptr; // メンバとして保持
};