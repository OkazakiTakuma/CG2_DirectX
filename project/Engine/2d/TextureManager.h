// TextureManager.h
#pragma once
#include "DirectXTex.h"
#include "d3dx12.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include <string>
#include <vector>
#include <wrl.h>
#include <d3d12.h>
#include <unordered_map>

class TextureManager {
public:
	/// <summary>
	/// 共有インスタンスを取得します。
	/// </summary>
	/// <returns>処理結果を返します。</returns>
	static TextureManager* GetInstance();
	/// <summary>
	/// 必要なリソースを準備し、オブジェクトを初期化します。
	/// </summary>
	/// <param name="dxcommon">DirectX 共通処理へアクセスするための参照を指定します。</param>
	void Initialize(DirectXCommon* dxcommon);
	/// <summary>
	/// 確保したリソースを解放し、終了処理を行います。
	/// </summary>
	void Finalize();
	/// <summary>
	/// Release の処理を行います。
	/// </summary>
	void Release();
	void Rerease() { Release(); }
	/// <summary>
	/// Texture を読み込み、内部データへ反映します。
	/// </summary>
	/// <param name="filepath">読み込みまたは保存に使用するファイルパスを指定します。</param>
	void LoadTexture(const std::string& filepath);
	static uint32_t kSRVIndexTop;
	/// <summary>
	/// TextureIndexByFilePath を取得します。
	/// </summary>
	/// <param name="filepath">読み込みまたは保存に使用するファイルパスを指定します。</param>
	/// <returns>処理結果を返します。</returns>
	uint32_t GetTextureIndexByFilePath(const std::string& filepath);
	/// <summary>
	/// SRVHandleGPU を取得します。
	/// </summary>
	/// <param name="filePath">読み込みまたは保存に使用するファイルパスを指定します。</param>
	/// <returns>処理結果を返します。</returns>
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandleGPU(const std::string& filePath) {
		assert(!SrvManager::GetInstance()->IsOverAllocated());
		assert(textureDatas.contains(filePath));
		return textureDatas.at(filePath).srvHandleGPU;
	}
	void SetDirectXCommon(DirectXCommon* dxCommon) { dxCommon_ = dxCommon; }
	/// <summary>
	/// TextureMetadata を取得します。
	/// </summary>
	/// <param name="filePath">読み込みまたは保存に使用するファイルパスを指定します。</param>
	/// <returns>処理結果を返します。</returns>
	const DirectX::TexMetadata& GetTextureMetadata(const std::string& filePath) {
		assert(!SrvManager::GetInstance()->IsOverAllocated());
		assert(textureDatas.contains(filePath));
		return textureDatas.at(filePath).metadata;
	}
	/// <summary>
	/// SrvIndex を取得します。
	/// </summary>
	/// <param name="filePath">読み込みまたは保存に使用するファイルパスを指定します。</param>
	/// <returns>処理結果を返します。</returns>
	uint32_t GetSrvIndex(const std::string& filePath) {
		assert(!SrvManager::GetInstance()->IsOverAllocated());
		assert(textureDatas.contains(filePath));
		return textureDatas.at(filePath).srvIndex;
	}
	Microsoft::WRL::ComPtr<ID3D12Resource> GetResource(const std::string& filePath) 
	{
		assert(!SrvManager::GetInstance()->IsOverAllocated());
		assert(textureDatas.contains(filePath));
		return textureDatas.at(filePath).resource;
	};
	/// <summary>
	/// 読み込み済みテクスチャのファイルパス一覧を取得します。
	/// </summary>
	/// <returns>読み込み済みテクスチャのファイルパス一覧を返します。</returns>
	std::vector<std::string> GetLoadedTextureNames() const;

private:
	static TextureManager* instance;
	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(TextureManager&) = delete;
	TextureManager& operator=(const TextureManager&) = delete;
	struct TextureData {
		DirectX::TexMetadata metadata;
		Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
		uint32_t srvIndex=0;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
	};

	std::unordered_map<std::string, TextureData> textureDatas;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12Resource>> textureResources_;
	DirectXCommon* dxCommon_ = nullptr;
};
