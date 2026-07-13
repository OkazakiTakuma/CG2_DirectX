#pragma once
#include "DirectXCommon.h"

class SrvManager {
public:
	/// <summary>
	/// 必要なリソースを準備し、オブジェクトを初期化します。
	/// </summary>
	/// <param name="dxcommon">DirectX 共通処理へアクセスするための参照を指定します。</param>
	void Initialize(DirectXCommon* dxcommon);
	static const uint32_t kMaxSRVCount;
	/// <summary>
	/// 共有インスタンスを取得します。
	/// </summary>
	/// <returns>処理結果を返します。</returns>
	static SrvManager* GetInstance() {
		static SrvManager instance;
		return &instance;
	}
	/// <summary>
	/// Allocate の処理を行います。
	/// </summary>
	/// <returns>処理結果を返します。</returns>
	uint32_t Allocate();
	/// <summary>
	/// IsOverAllocated の処理を行います。
	/// </summary>
	/// <returns>処理結果を返します。</returns>
	bool IsOverAllocated() const;

	/// <summary>
	/// CPUDescriptorHandle を取得します。
	/// </summary>
	/// <param name="index">対象要素のインデックスを指定します。</param>
	/// <returns>処理結果を返します。</returns>
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap() { return descriptorHeap; }
	
	/// <summary>
	/// GPUDescriptorHandle を取得します。
	/// </summary>
	/// <param name="index">対象要素のインデックスを指定します。</param>
	/// <returns>処理結果を返します。</returns>
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index);
	/// <summary>
	/// SRVforTexture2D を作成し、利用できる状態にします。
	/// </summary>
	/// <param name="srvindex">対象要素のインデックスを指定します。</param>
	/// <param name="pResource">pResource に使用する値を指定します。</param>
	/// <param name="format">format に使用する値を指定します。</param>
	/// <param name="mipLevels">mipLevels に使用する値を指定します。</param>
	void CreateSRVforTexture2D(uint32_t srvindex, ID3D12Resource* pResource, DXGI_FORMAT format, UINT mipLevels);
	/// <summary>
	/// SRVforStructuredBuffer を作成し、利用できる状態にします。
	/// </summary>
	/// <param name="srvindex">対象要素のインデックスを指定します。</param>
	/// <param name="pResource">pResource に使用する値を指定します。</param>
	/// <param name="numElements">numElements に使用する値を指定します。</param>
	/// <param name="structureByteStride">structureByteStride に使用する値を指定します。</param>
	void CreateSRVforStructuredBuffer(uint32_t srvindex, ID3D12Resource* pResource, UINT numElements, UINT structureByteStride);
	/// <summary>
	/// PreDraw の処理を行います。
	/// </summary>
	void PreDraw();
	/// <summary>
	/// GraphicsRootDescriptorTable を設定します。
	/// </summary>
	/// <param name="rootParameterIndex">rootParameterIndex に使用する値を指定します。</param>
	/// <param name="srvIndex">対象要素のインデックスを指定します。</param>
	void SetGraphicsRootDescriptorTable(UINT rootParameterIndex, uint32_t srvIndex);
	/// <summary>
	/// 確保したリソースを解放し、終了処理を行います。
	/// </summary>
	void Finalize();

private:
	DirectXCommon* dxCommon = nullptr;
	uint32_t descriptrSize;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	uint32_t useIndex = 0;

};

