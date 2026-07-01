#include "SrvManager.h"
const uint32_t SrvManager::kMaxSRVCount = 512;

void SrvManager::Initialize(DirectXCommon* dxcommon) {
	this->dxCommon = dxcommon;
	descriptorHeap = dxCommon->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kMaxSRVCount, true);
	descriptrSize = dxCommon->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void SrvManager::CreateUAVforStructuredBuffer(uint32_t uavindex, ID3D12Resource* pResource, UINT numElements, UINT structureByteStride) {
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.NumElements = numElements;
	uavDesc.Buffer.StructureByteStride = structureByteStride;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;

	dxCommon->GetDevice()->CreateUnorderedAccessView(
		pResource,
		nullptr,
		&uavDesc,
		GetCPUDescriptorHandle(uavindex)
	);
}

uint32_t SrvManager::Allocate() {
	assert(kMaxSRVCount > useIndex);
	uint32_t index = useIndex;
	useIndex++;
	return index;
}

bool SrvManager::IsOverAllocated() const { return useIndex > kMaxSRVCount; }

D3D12_CPU_DESCRIPTOR_HANDLE SrvManager::GetCPUDescriptorHandle(uint32_t index) {
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptrSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE SrvManager::GetGPUDescriptorHandle(uint32_t index) {
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptrSize * index);
	return handleGPU;
}

void SrvManager::CreateSRVforTexture2D(uint32_t srvindex, ID3D12Resource* pResource, DXGI_FORMAT format, UINT mipLevels) {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // シェーダーコンポーネントのマッピング
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;                      // テクスチャの次元
	srvDesc.Texture2D.MipLevels = mipLevels;      // ミップレベルの数
	dxCommon->GetDevice()->CreateShaderResourceView(pResource, &srvDesc, GetCPUDescriptorHandle(srvindex));
}

void SrvManager::CreateSRVforStructuredBuffer(uint32_t srvindex, ID3D12Resource* pResource, UINT numElements, UINT structureByteStride) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN; // 構造化バッファは通常フォーマット不要
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = numElements;
    srvDesc.Buffer.StructureByteStride = structureByteStride;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    dxCommon->GetDevice()->CreateShaderResourceView(
        pResource,
        &srvDesc,
        GetCPUDescriptorHandle(srvindex)
    );
}

void SrvManager::PreDraw() {
	ID3D12DescriptorHeap* heaps[] = {descriptorHeap.Get()};
	dxCommon->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);
}

void SrvManager::SetGraphicsRootDescriptorTable(UINT rootParameterIndex, uint32_t srvIndex) {
	dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(rootParameterIndex, GetGPUDescriptorHandle(srvIndex));
}

void SrvManager::Finalize() {
	// ヒープを明示的にリセット
	descriptorHeap.Reset();

}