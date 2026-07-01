#pragma once
#include "DirectXCommon.h"

class SrvManager {
public:
	void Initialize(DirectXCommon* dxcommon);
	static const uint32_t kMaxSRVCount;
	static SrvManager* GetInstance() {
		static SrvManager instance;
		return &instance;
	}
	uint32_t Allocate();
	bool IsOverAllocated() const;

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap() { return descriptorHeap; }
	
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index);
	void CreateSRVforTexture2D(uint32_t srvindex, ID3D12Resource* pResource, DXGI_FORMAT format, UINT mipLevels);
	void CreateSRVforStructuredBuffer(uint32_t srvindex, ID3D12Resource* pResource, UINT numElements, UINT structureByteStride);
	void CreateUAVforStructuredBuffer(uint32_t uavindex, ID3D12Resource* pResource, UINT numElements, UINT structureByteStride);
	void PreDraw();
	void SetGraphicsRootDescriptorTable(UINT rootParameterIndex, uint32_t srvIndex);
	void Finalize();

private:
	DirectXCommon* dxCommon = nullptr;
	uint32_t descriptrSize;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	uint32_t useIndex = 0;

};

