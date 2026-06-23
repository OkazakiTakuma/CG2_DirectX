#pragma once
#include "Model.h"
#include "Camera.h"
#include "struct.h"
#include <vector>
#include <d3d12.h>
#include <wrl.h>

// インスタンシング描画用のGPUに送る行列データ
struct InstancingMatrixData {
    Matrix4x4 WVP;
    Matrix4x4 world;
    Matrix4x4 WorldInverseTranspose;
};

class InstancingModel {
public:
    // 初期化（最大描画数を決めてバッファを作る）
    void Initialize(Model* model, uint32_t maxInstanceCount);

    // 毎フレーム、描画したい座標を追加する
    void AddInstance(const Transform& transform);

    // 溜まった座標データをGPUに転送し、一括で描画する
    void Draw(Camera* camera);

private:
    Model* model_ = nullptr;
    uint32_t maxInstanceCount_ = 1000; // 最大描画数

    // 追加された座標データのリスト
    std::vector<Transform> transforms_;

    // GPUへ送るための配列バッファ（StructuredBuffer）
    Microsoft::WRL::ComPtr<ID3D12Resource> instanceBuffer_;
    InstancingMatrixData* mappedData_ = nullptr;

    // バッファを生成する内部関数
    void CreateInstanceBuffer();
};