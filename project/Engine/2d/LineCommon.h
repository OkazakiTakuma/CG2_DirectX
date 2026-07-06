#pragma once

#include "DirectXCommon.h"
#include "struct.h"

#include <array>
#include <d3d12.h>
#include <wrl.h>

const uint32_t kCountOfBlendMode = 6;

class LineCommon {
public:
    static LineCommon* GetInstance();

    // Prepares the line rendering pipeline with shared DirectX resources.
    void Initialize(DirectXCommon* dxCommon);

    // Releases GPU-side pipeline objects.
    void Finalize();

    // Sets the pipeline state used for line rendering.
    void SetDraw(uint32_t blendMode = kBlendModeNormal);

    DirectXCommon* GetDxCommon() const { return dxCommon_; }

private:
    LineCommon() = default;
    ~LineCommon() = default;
    LineCommon(const LineCommon&) = delete;
    LineCommon& operator=(const LineCommon&) = delete;

    void CreateRootSignature();
    void CreatePipelineState();

private:
    DirectXCommon* dxCommon_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
    // Pipeline states are cached by blend mode.
    std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, kCountOfBlendMode> graphicsPipelineStates;
};
