#pragma once
#include "DirectXCommon.h"
#include <d3d12.h>
#include <wrl.h>

class InstancingModelCommon {
public:
    static InstancingModelCommon* GetInstance();

    void Initialize(DirectXCommon* dxCommon);
    void SetDraw(); // 描画前にパイプラインをセットする

private:
    InstancingModelCommon() = default;
    ~InstancingModelCommon() = default;

    void CreateRootSignature();
    void CreatePipelineState();

    DirectXCommon* dxCommon_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
};