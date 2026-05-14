#pragma once
#include "DirectXCommon.h"
#include <wrl.h>
#include "Camera.h"

class SkyBoxCommon {
public:
	static SkyBoxCommon* GetInstance();

	void Initialize(DirectXCommon* dxCommon);
	void SetDraw(); // パイプラインとルートシグネチャをコマンドリストにセット
	void Finalize();

	DirectXCommon* GetDxCommon() const { return dxCommon_; }
	~SkyBoxCommon() = default;
	void SetDefaultCamera(Camera* cmr) { defaultCamera = cmr; }
	Camera* GetDefaultCamera() const { return defaultCamera; }

private:
	SkyBoxCommon() = default;
	SkyBoxCommon(const SkyBoxCommon&) = delete;
	SkyBoxCommon& operator=(const SkyBoxCommon&) = delete;

	void CreateRootSignature();
	void CreatePipelineState();

	DirectXCommon* dxCommon_ = nullptr;
	Camera* defaultCamera = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;
};