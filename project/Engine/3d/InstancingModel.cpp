#include "InstancingModel.h"
#include "Object3dCommon.h" // DirectXCommon を取得するため
#include "TextureManager.h"

void InstancingModel::Initialize(Model* model, uint32_t maxInstanceCount) {
    model_ = model;
    maxInstanceCount_ = maxInstanceCount;

    // transforms_ が頻繁にメモリ再確保されないように予約しておく
    transforms_.reserve(maxInstanceCount_);

    CreateInstanceBuffer();
    CreateConstantBuffers(); // バッファ作成
}

InstancingModel::~InstancingModel() {
    // マップ解除
    if (instanceBuffer_) instanceBuffer_->Unmap(0, nullptr);
    if (lightResource_) lightResource_->Unmap(0, nullptr);
    if (cameraResource_) cameraResource_->Unmap(0, nullptr);
    if (pointLightResource_) pointLightResource_->Unmap(0, nullptr);
}

void InstancingModel::CreateInstanceBuffer() {
    auto dxCommon = Object3dCommon::GetInstance()->GetDxCommon();

    // 最大数分の行列データが入るバッファを作成
    uint32_t bufferSize = sizeof(InstancingMatrixData) * maxInstanceCount_;
    instanceBuffer_ = dxCommon->CreateBufferResource(bufferSize);

    // 書き込み用にマップしっぱなしにしておく
    instanceBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedData_));
}

void InstancingModel::CreateConstantBuffers() {
    auto dxCommon = Object3dCommon::GetInstance()->GetDxCommon();

    // ライトバッファ作成と初期化
    lightResource_ = dxCommon->CreateBufferResource(sizeof(DirectionalLight));
    lightResource_->Map(0, nullptr, reinterpret_cast<void**>(&lightData_));
    lightData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    lightData_->direction = { 0.0f, -1.0f, 0.0f };
    lightData_->intensity = 1.0f;

    // カメラバッファ作成
    cameraResource_ = dxCommon->CreateBufferResource(sizeof(CameraForGPU));
    cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraData_));
    cameraData_->worldPosition = { 0.0f, 0.0f, -10.0f };
    cameraData_->environmentMultiplier = 0.0f; // 環境マップは無効化

    // ポイントライトバッファ作成（初期状態は強度0で無効化）
    pointLightResource_ = dxCommon->CreateBufferResource(sizeof(PointLight));
    pointLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData_));
    pointLightData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    pointLightData_->position = { 0.0f, 0.0f, 0.0f };
    pointLightData_->intensity = 0.0f;
    pointLightData_->radius = 10.0f;
    pointLightData_->decay = 1.0f;
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
        mappedData_[i].WorldInverseTranspose = MakeIdentity4x4();
    }

    // カメラ座標を最新に更新
    cameraData_->worldPosition = camera->GetTranslate();

    auto commandList = Object3dCommon::GetInstance()->GetDxCommon()->GetCommandList();

    // 2. 描画準備
    SrvManager::GetInstance()->PreDraw();

    // [2] t2: Instancing Data (SRV)
    commandList->SetGraphicsRootShaderResourceView(2, instanceBuffer_->GetGPUVirtualAddress());

    // 頂点バッファのセット
    commandList->IASetVertexBuffers(0, 1, &model_->vertexBufferView);
    // [0] b0: Material
    commandList->SetGraphicsRootConstantBufferView(0, model_->materialResource->GetGPUVirtualAddress());
    // [1] t0: Texture
    commandList->SetGraphicsRootDescriptorTable(1, TextureManager::GetInstance()->GetSRVHandleGPU(model_->modelData.material.textureFilePath));

    // [3] b2: DirectionalLight
    commandList->SetGraphicsRootConstantBufferView(3, lightResource_->GetGPUVirtualAddress());
    // [4] b3: CameraInfo
    commandList->SetGraphicsRootConstantBufferView(4, cameraResource_->GetGPUVirtualAddress());
    // [5] b4: PointLight
    commandList->SetGraphicsRootConstantBufferView(5, pointLightResource_->GetGPUVirtualAddress());

    // [6] t1: EnvironmentMap (バインド漏れエラーを防ぐためテクスチャと同じハンドルを流しておく)
    std::string envPath = envMapTexturePath_.empty()
        ? "Resources/rostock_laage_airport_4k.dds"
        : envMapTexturePath_;
    commandList->SetGraphicsRootDescriptorTable(6, TextureManager::GetInstance()->GetSRVHandleGPU(envPath));

    // 3. 一括描画（DrawInstanced）
    uint32_t vertexCount = model_->GetVertexCount();
    commandList->DrawInstanced(vertexCount, instanceCount, 0, 0);

    // 4. 描画が終わったらリストを空にして、次のフレームに備える
    transforms_.clear();
}