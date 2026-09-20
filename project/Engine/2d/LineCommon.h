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

    // 共通のDirectXリソースを使用して、線描画パイプラインを準備します。
    /// <summary>
    /// 必要なリソースを準備し、オブジェクトを初期化します。
    /// </summary>
    /// <param name="dxCommon">DirectX 共通処理へアクセスするための参照を指定します。</param>
    void Initialize(DirectXCommon* dxCommon);

    // GPU側のパイプラインオブジェクトを解放します。
    /// <summary>
    /// 確保したリソースを解放し、終了処理を行います。
    /// </summary>
    void Finalize();
    /// <summary>変更されたHLSLを反映するため、ライン描画用PSOを再生成します。</summary>
    void ReloadPipelineState() { CreatePipelineState(); }

    // 線描画に使用するパイプラインステートを設定します。
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
    // 深度モードとブレンドモードの組み合わせごとにパイプラインステートをキャッシュします。
    std::array<std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, kCountOfBlendMode>, 2> graphicsPipelineStates;
};
