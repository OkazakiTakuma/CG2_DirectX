#pragma once
#include "../base/DirectXCommon.h"
#include "../base/Logger.h"
#include "../base/StringUtility.h"


class SpriteCommon {
public:
	enum BlendMode {
		kBlendModeNone,
		kBlendModeNormal,
		kBlendModeAdd,
		kBlendModeSubtract,
		kBlendModeMultiply,
		kBlendModeScreen,
		kBlendCountblend,
	};
	void Initialize(DirectXCommon* dxCommon);
	DirectXCommon* GetDxCommon() const { return dxCommon_; }
	void SetDraw();

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;



	void CreateRootSignature();

	void CreatePipelineState();
};
