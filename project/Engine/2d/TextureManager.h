// TextureManager.h
#pragma once
#include "../../extenals/DirectXTex/DirectXTex.h"
#include "../../extenals/DirectXTex/d3dx12.h"
#include "DirectXCommon.h"
#include <string>
#include <vector>
#include"SrvManager.h"

class TextureManager {
public:
	void Initialize(SrvManager*srv); // 初期化時にセット
	static TextureManager* GetInstance();
	void Finalize();
	void Rerease();
	static uint32_t kSRVIndexTop;
	uint32_t GetTextureIndexByFilePath(const std::string& filepath);
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandleGPU(const std::string& filePath) {
		assert(srvManager->IsOverAllocated());
		return textureDatas[filePath].srvHandleGPU;
	}
	void LoadTexture(const std::string& filepath);
	void SetDirectXCommon(DirectXCommon* dxCommon) { dxCommon_ = dxCommon; }
	const DirectX::TexMetadata& GetTextureMetadata(const std::string& filePath) {
		assert(srvManager->IsOverAllocated());
		return textureDatas[filePath].metadata;
	}
	uint32_t GetSrvIndex(const std::string& filePath) {
		assert(srvManager->IsOverAllocated());
		return textureDatas[filePath].srvIndex;
	}

private:
	static TextureManager* instance;
	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(TextureManager&) = delete;
	TextureManager& operator=(const TextureManager&) = delete;
	SrvManager* srvManager;
	struct TextureData {
		DirectX::TexMetadata metadata;
		Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
		uint32_t srvIndex;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
	};

	std::unordered_map< std::string,TextureData> textureDatas;
	DirectXCommon* dxCommon_ = nullptr; // メンバとして保持
};