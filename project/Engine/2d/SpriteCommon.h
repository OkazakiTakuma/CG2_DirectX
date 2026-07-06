#pragma once

#include "../base/DirectXCommon.h"
#include "../base/Logger.h"
#include "../base/StringUtility.h"
#include "../base/struct.h"

#include <array>
#include <assert.h>
#include <d3d12.h>
#include <wrl.h>

class SpriteCommon {
public:
	static SpriteCommon* GetInstance();

	// Prepares the sprite rendering pipeline with shared DirectX resources.
	void Initialize(DirectXCommon* dxCommon);

	// Releases GPU-side pipeline objects.
	void Finalize();

	// Sets the pipeline state used for sprite rendering.
	void SetDraw(uint32_t blendMode = kBlendModeNormal);

	DirectXCommon* GetDxCommon() const { return dxCommon_; }

	~SpriteCommon() = default;

private:
	SpriteCommon() = default;

	SpriteCommon(const SpriteCommon&) = delete;
	SpriteCommon& operator=(const SpriteCommon&) = delete;

	void CreateRootSignature();
	void CreatePipelineState();

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	// Pipeline states are cached by blend mode.
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, kBlendCountblend> graphicsPipelineStates;
};
