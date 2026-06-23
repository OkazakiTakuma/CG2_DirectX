#include "InstancingModel.h"
#include "Object3dCommon.h" // DirectXCommon を取得するため

void InstancingModel::Initialize(Model* model, uint32_t maxInstanceCount) {
    model_ = model;
    maxInstanceCount_ = maxInstanceCount;

    // transforms_ が頻繁にメモリ再確保されないように予約しておく
    transforms_.reserve(maxInstanceCount_);

    CreateInstanceBuffer();
}

void InstancingModel::CreateInstanceBuffer() {
    auto dxCommon = Object3dCommon::GetInstance()->GetDxCommon();

    // 最大数分の行列データが入るバッファを作成
    uint32_t bufferSize = sizeof(InstancingMatrixData) * maxInstanceCount_;
    instanceBuffer_ = dxCommon->CreateBufferResource(bufferSize);

    // 書き込み用にマップしっぱなしにしておく
    instanceBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedData_));
}

void InstancingModel::AddInstance(const Transform& transform) {
    if (transforms_.size() < maxInstanceCount_) {
        transforms_.push_back(transform);
    }
}

void InstancingModel::Draw(Camera* camera) {
    uint32_t instanceCount = static_cast<uint32_t>(transforms_.size());
    if (instanceCount == 0 || !model_) return;

    // 1. 溜まったTransformを行列に変換してGPUのバッファに書き込む
    for (uint32_t i = 0; i < instanceCount; ++i) {
        Matrix4x4 worldMatrix = MakeAffineMatrix(transforms_[i].scale, transforms_[i].rotate, transforms_[i].translate);

        mappedData_[i].world = worldMatrix;
        mappedData_[i].WVP = Multiply(worldMatrix, camera->GetViewProjectionMatrix());

        // 法線用の逆転置行列（スケールを含まないようにする処理など）
        // ※ここでは簡易的に MakeIdentity4x4() にしていますが、必要に応じて計算関数を入れてください
        mappedData_[i].WorldInverseTranspose = MakeIdentity4x4();
    }

    auto commandList = Object3dCommon::GetInstance()->GetDxCommon()->GetCommandList();

    // 2. インスタンスバッファ(t2)をシェーダーにセット
    // ※ 注意: ルートシグネチャの設定に合わせて番号(RootParameterIndex)を変える必要があります
    commandList->SetGraphicsRootShaderResourceView(2, instanceBuffer_->GetGPUVirtualAddress());

    // 3. モデルの描画準備（頂点バッファやマテリアルのセット）
    model_->Draw();

    // 4. 一括描画（DrawIndexedInstanced）
    // （modelData.indices.size() を取得するゲッターがModelクラスにあると仮定）
    // uint32_t indexCount = model_->GetIndexCount(); 
    uint32_t indexCount = 0; // ※ここは実際のインデックス数を入れてください

    commandList->DrawIndexedInstanced(indexCount, instanceCount, 0, 0, 0);

    // 5. 描画が終わったらリストを空にして、次のフレームに備える
    transforms_.clear();
}