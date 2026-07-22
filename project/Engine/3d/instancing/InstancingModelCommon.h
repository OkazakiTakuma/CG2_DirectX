#pragma once
#include "DirectXCommon.h"
#include <d3d12.h>
#include <wrl.h>

class InstancingModelCommon {
public:
    /// <summary>
    /// 共有インスタンスを取得します。
    /// </summary>
    static InstancingModelCommon* GetInstance();

    /// <summary>
    /// 必要なリソースを準備し、オブジェクトを初期化します。
    /// </summary>
    /// <param name="dxCommon">DirectX 共通処理へアクセスするための参照を指定します。</param>
    void Initialize(DirectXCommon* dxCommon);
    /// <summary>
    /// 確保したリソースを解放し、終了処理を行います。
    /// </summary>
    void Finalize();
    void ReloadPipelineState() { CreatePipelineState(); }
    void SetDraw();

private:
    InstancingModelCommon() = default;
    ~InstancingModelCommon() = default;

    /// <summary>
    /// RootSignature を作成し、利用できる状態にします。
    /// </summary>
    void CreateRootSignature();
    /// <summary>
    /// PipelineState を作成し、利用できる状態にします。
    /// </summary>
    void CreatePipelineState();

    DirectXCommon* dxCommon_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
};
