// TextureManager.h
#pragma once
#include "../../extenals/DirectXTex/DirectXTex.h"
#include "../../extenals/DirectXTex/d3dx12.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include <string>
#include <vector>
#include <wrl.h>
#include <d3d12.h>
#include <unordered_map>

class TextureManager {
public:
	void Initialize(DirectXCommon* dxcommon); // 初期化時にセット
	static TextureManager* GetInstance();
	void Finalize();
	void Rerease();
	static uint32_t kSRVIndexTop;
	uint32_t GetTextureIndexByFilePath(const std::string& filepath);
	// TextureManager.h 内の修正例
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandleGPU(const std::string& filePath) {
		// 修正: シングルトンからチェックを行う
		assert(!SrvManager::GetInstance()->IsOverAllocated());
		return textureDatas[filePath].srvHandleGPU;
	}
	void LoadTexture(const std::string& filepath);
	void SetDirectXCommon(DirectXCommon* dxCommon) { dxCommon_ = dxCommon; }
	const DirectX::TexMetadata& GetTextureMetadata(const std::string& filePath) {
		assert(!SrvManager::GetInstance()->IsOverAllocated());
		return textureDatas[filePath].metadata;
	}
	uint32_t GetSrvIndex(const std::string& filePath) {
		assert(!SrvManager::GetInstance()->IsOverAllocated());
		return textureDatas[filePath].srvIndex;
	}
	Microsoft::WRL::ComPtr<ID3D12Resource> GetResource(const std::string& filePath) 
	{
		assert(!SrvManager::GetInstance()->IsOverAllocated());
		return textureDatas[filePath].resource;
	};

private:
	static TextureManager* instance;
	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(TextureManager&) = delete;
	TextureManager& operator=(const TextureManager&) = delete;
	struct TextureData {
		DirectX::TexMetadata metadata;
		Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
		uint32_t srvIndex;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
	};

	std::unordered_map<std::string, TextureData> textureDatas;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12Resource>> textureResources_;
	DirectXCommon* dxCommon_ = nullptr; // メンバとして保持
};