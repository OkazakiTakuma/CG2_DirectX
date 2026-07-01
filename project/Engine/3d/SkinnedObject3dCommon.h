#pragma once
#include "Camera.h"
#include "DirectXCommon.h"
#include <d3d12.h>
#include <wrl.h>

class SkinnedObject3dCommon {
public:
	static SkinnedObject3dCommon* GetInstance();
	// Make DispatchSkinning public to be callable from SkinnedModel
	void Initialize(DirectXCommon* dxCommon);
	void Finalize();
	void SetDraw();
	DirectXCommon* GetDxCommon() const { return dxCommon_; }
	void SetDefaultCamera(Camera* cmr) { defaultCamera = cmr; }
	Camera* GetDefaultCamera() { return defaultCamera; }

	// GPUスキニング用Dispatch
	void DispatchSkinning(ID3D12GraphicsCommandList* commandList, ID3D12Resource* inStructuredBuffer, ID3D12Resource* outBuffer, D3D12_GPU_VIRTUAL_ADDRESS boneBufferAddress, UINT vertexCount, D3D12_GPU_DESCRIPTOR_HANDLE srvHandle, D3D12_GPU_DESCRIPTOR_HANDLE uavHandle);

private:
	SkinnedObject3dCommon() = default;
	~SkinnedObject3dCommon() = default;
	SkinnedObject3dCommon(const SkinnedObject3dCommon&) = delete;
	SkinnedObject3dCommon& operator=(const SkinnedObject3dCommon&) = delete;

	void CreateRootSignature();
	void CreatePipelineState();

	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;
	// Compute pipeline for GPU skinning
	Microsoft::WRL::ComPtr<ID3D12RootSignature> computeRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> computePipelineState;
	Camera* defaultCamera = nullptr;
};