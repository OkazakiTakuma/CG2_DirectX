#pragma once
#include "Camera.h"
#include "DirectXCommon.h"
#include <d3d12.h>
#include <wrl.h>

class SkinnedObject3dCommon {
public:
	static SkinnedObject3dCommon* GetInstance();
	void Initialize(DirectXCommon* dxCommon);
	void Finalize();
	void SetDraw();
	DirectXCommon* GetDxCommon() const { return dxCommon_; }
	void SetDefaultCamera(Camera* cmr) { defaultCamera = cmr; }
	Camera* GetDefaultCamera() { return defaultCamera; }

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
	Camera* defaultCamera = nullptr;
};