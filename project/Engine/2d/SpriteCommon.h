#pragma once
#include "../base/DirectXCommon.h"
#include "../base/Logger.h"
#include "../base/StringUtility.h"
#include"../base/struct.h"


class SpriteCommon {
public:

	void Initialize(DirectXCommon* dxCommon);
	DirectXCommon* GetDxCommon() const { return dxCommon_; }
	void SetDraw();
	void Finalize();

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;



	void CreateRootSignature();

	void CreatePipelineState();
};
