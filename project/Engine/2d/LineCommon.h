#pragma once

#include "DirectXCommon.h"
#include "struct.h"
#include <d3d12.h>
#include <wrl.h>
#include <array>

const uint32_t kCountOfBlendMode = 6;

class LineCommon {
public:
    // シングルトンインスタンスの取得
    static LineCommon* GetInstance();

    // 初期化
    void Initialize(DirectXCommon* dxCommon);

    // 終了処理
    void Finalize();

    // 描画前設定（ブレンドモードを指定）
    void SetDraw(uint32_t blendMode = kBlendModeNormal);

    // ゲッター
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
    // ブレンドモードごとのパイプラインステート
    std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, kCountOfBlendMode> graphicsPipelineStates;
};