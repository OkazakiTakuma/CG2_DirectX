#pragma once

#include "DirectXCommon.h"
#include "struct.h"

#include <array>
#include <d3d12.h>
#include <wrl.h>

const uint32_t kCountOfBlendMode = 6;

class LineCommon {
public:
    /// <summary>
    /// 共有インスタンスを取得します。
    /// </summary>
    static LineCommon* GetInstance();

    // Prepares the line rendering pipeline with shared DirectX resources.
    /// <summary>
    /// 必要なリソースを準備し、オブジェクトを初期化します。
    /// </summary>
    /// <param name="dxCommon">DirectX 共通処理へアクセスするための参照を指定します。</param>
    void Initialize(DirectXCommon* dxCommon);

    // Releases GPU-side pipeline objects.
    /// <summary>
    /// 確保したリソースを解放し、終了処理を行います。
    /// </summary>
    void Finalize();
    void ReloadPipelineState() { CreatePipelineState(); }

    // Sets the pipeline state used for line rendering.
    /// <param name="blendMode">描画時に使用するブレンドモードを指定します。</param>
    void SetDraw(uint32_t blendMode = kBlendModeNormal, bool ignoreDepth = false);

    DirectXCommon* GetDxCommon() const { return dxCommon_; }

private:
    LineCommon() = default;
    ~LineCommon() = default;
    LineCommon(const LineCommon&) = delete;
    LineCommon& operator=(const LineCommon&) = delete;

    /// <summary>
    /// RootSignature を作成し、利用できる状態にします。
    /// </summary>
    void CreateRootSignature();
    /// <summary>
    /// PipelineState を作成し、利用できる状態にします。
    /// </summary>
    void CreatePipelineState();

private:
    DirectXCommon* dxCommon_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
    // Pipeline states are cached by depth mode and blend mode.
    std::array<std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, kCountOfBlendMode>, 2> graphicsPipelineStates;
};
