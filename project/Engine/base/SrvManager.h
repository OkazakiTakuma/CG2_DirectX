#pragma once
#include "DirectXCommon.h"
class SrvManager {
public:
	void Initialize(DirectXCommon* dxcommon);
	static const uint32_t kMaxSRVCount;
	uint32_t Allcate();
	bool IsOverAllocated() const;

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);
	
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index);
	void CreateSRVforTexture2D(uint32_t srvindex, ID3D12Resource* pResource, DXGI_FORMAT format, UINT mipLevels);
	void CreateSRVforStructuredBuffer(uint32_t srvindex, ID3D12Resource* pResource, UINT numElements, UINT structureByteStride);
	void preDraw();
	void SetGraphicsRootDescriptrTable(UINT rootParameterIndex, uint32_t srvIndex);

private:
	DirectXCommon* dxCommon = nullptr;
	uint32_t descriptrSize;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	uint32_t useIndex = 0;

};

