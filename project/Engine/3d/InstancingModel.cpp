#include "InstancingModel.h"
#include "Object3dCommon.h"
#include "TextureManager.h"

/// <summary>
/// 必要なリソースを準備し、オブジェクトを初期化します。
/// </summary>
/// <param name="model">model に使用する値を指定します。</param>
/// <param name="maxInstanceCount">範囲判定に使用する値を指定します。</param>
void InstancingModel::Initialize(Model* model, uint32_t maxInstanceCount) {
    model_ = model;
    maxInstanceCount_ = maxInstanceCount;

    transforms_.reserve(maxInstanceCount_);

    CreateInstanceBuffer();
    CreateConstantBuffers();
}

/// <summary>
/// 破棄時に必要な解放処理を行います。
/// </summary>
InstancingModel::~InstancingModel() {
    if (instanceBuffer_) instanceBuffer_->Unmap(0, nullptr);
    if (lightResource_) lightResource_->Unmap(0, nullptr);
    if (cameraResource_) cameraResource_->Unmap(0, nullptr);
    if (pointLightResource_) pointLightResource_->Unmap(0, nullptr);
}

/// <summary>
/// InstanceBuffer を作成し、利用できる状態にします。
/// </summary>
void InstancingModel::CreateInstanceBuffer() {
    auto dxCommon = Object3dCommon::GetInstance()->GetDxCommon();

    uint32_t bufferSize = sizeof(InstancingMatrixData) * maxInstanceCount_;
    instanceBuffer_ = dxCommon->CreateBufferResource(bufferSize);

    instanceBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedData_));
}

/// <summary>
/// ConstantBuffers を作成し、利用できる状態にします。
/// </summary>
void InstancingModel::CreateConstantBuffers() {
    auto dxCommon = Object3dCommon::GetInstance()->GetDxCommon();

    lightResource_ = dxCommon->CreateBufferResource(sizeof(DirectionalLight));
    lightResource_->Map(0, nullptr, reinterpret_cast<void**>(&lightData_));
    lightData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    lightData_->direction = { 0.0f, -1.0f, 0.0f };
    lightData_->intensity = 1.0f;

    cameraResource_ = dxCommon->CreateBufferResource(sizeof(CameraForGPU));
    cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraData_));
    cameraData_->worldPosition = { 0.0f, 0.0f, -10.0f };
    cameraData_->environmentMultiplier = 0.0f;

    pointLightResource_ = dxCommon->CreateBufferResource(sizeof(PointLight));
    pointLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData_));
    pointLightData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    pointLightData_->position = { 0.0f, 0.0f, 0.0f };
    pointLightData_->intensity = 0.0f;
    pointLightData_->radius = 10.0f;
    pointLightData_->decay = 1.0f;
}

/// <summary>
/// AddInstance の処理を行います。
/// </summary>
/// <param name="transform">transform に使用する値を指定します。</param>
void InstancingModel::AddInstance(const EulerTransform& transform) {
    if (transforms_.size() < maxInstanceCount_) {
        transforms_.push_back(transform);
    }
}

/// <summary>
/// 現在の状態をもとに描画処理を行います。
/// </summary>
/// <param name="camera">描画や座標変換に使用するカメラを指定します。</param>
void InstancingModel::Draw(Camera* camera) {
    uint32_t instanceCount = static_cast<uint32_t>(transforms_.size());
    if (instanceCount == 0 || !model_) return;

    for (uint32_t i = 0; i < instanceCount; ++i) {
        Matrix4x4 worldMatrix = MakeAffineMatrix(transforms_[i].scale, transforms_[i].rotate, transforms_[i].translate);

        mappedData_[i].world = worldMatrix;
        mappedData_[i].WVP = Multiply(worldMatrix, camera->GetViewProjectionMatrix());
        mappedData_[i].WorldInverseTranspose = MakeIdentity4x4();
    }

    cameraData_->worldPosition = camera->GetTranslate();

    auto commandList = Object3dCommon::GetInstance()->GetDxCommon()->GetCommandList();

    SrvManager::GetInstance()->PreDraw();

    // [2] t2: Instancing Data (SRV)
    commandList->SetGraphicsRootShaderResourceView(2, instanceBuffer_->GetGPUVirtualAddress());

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

    std::string envPath = envMapTexturePath_.empty()
        ? "Resources/rostock_laage_airport_4k.dds"
        : envMapTexturePath_;
    commandList->SetGraphicsRootDescriptorTable(6, TextureManager::GetInstance()->GetSRVHandleGPU(envPath));

    uint32_t vertexCount = model_->GetVertexCount();
    commandList->DrawInstanced(vertexCount, instanceCount, 0, 0);

    transforms_.clear();
}
