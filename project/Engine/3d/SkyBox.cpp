#include "SkyBox.h"
#include "TextureManager.h"
#include <SrvManager.h>

void SkyBox::Initialize(const std::string& filePath) {
	common_ = SkyBoxCommon::GetInstance();
	textureFilePath = filePath;

	CreateVertexData();
	CreateConstantBuffers();
	this->camera = common_->GetDefaultCamera();
}

void SkyBox::Update() {
    Vector3 scale = { 500.0f, 500.0f, 500.0f };
    Vector3 rotation = { 0.0f, 0.0f, 0.0f };
    Vector3 translation = { 0.0f, 0.0f, 0.0f };

    Matrix4x4 worldMatrix = MakeAffineMatrix(scale, rotation, translation);

    Matrix4x4 viewMatrix = camera->GetViewMatrix();

    viewMatrix.m[3][0] = 0.0f;
    viewMatrix.m[3][1] = 0.0f;
    viewMatrix.m[3][2] = 0.0f;
    // ==================================================================

    Matrix4x4 viewProjectionMatrix = Multiply(viewMatrix, camera->GetProjectionMatrix());

    transformData->WVP = Multiply(worldMatrix, viewProjectionMatrix);
    transformData->world = worldMatrix;
}
void SkyBox::Draw() {
    common_->SetDraw();
    auto commandList = common_->GetDxCommon()->GetCommandList();

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeapComPtr = SrvManager::GetInstance()->GetDescriptorHeap();
    assert(srvHeapComPtr.Get() != nullptr && "Descriptor heap from SrvManager is null.");

    ID3D12DescriptorHeap* ppHeaps[] = { srvHeapComPtr.Get() };
    commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

    commandList->IASetIndexBuffer(&indexBufferView);

    commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, transformResource->GetGPUVirtualAddress());

    D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = TextureManager::GetInstance()->GetSRVHandleGPU(textureFilePath);
    assert(srvHandle.ptr != 0 && "Texture GPU descriptor handle is invalid.");

    commandList->SetGraphicsRootDescriptorTable(2, srvHandle);

    commandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
}
void SkyBox::CreateVertexData() {
    const float kSize = 1.0f;
    VertexData vertices[] = {
        {{-kSize, -kSize, -kSize, 1.0f}},
        {{-kSize,  kSize, -kSize, 1.0f}},
        {{ kSize,  kSize, -kSize, 1.0f}},
        {{ kSize, -kSize, -kSize, 1.0f}},
        {{-kSize, -kSize,  kSize, 1.0f}},
        {{-kSize,  kSize,  kSize, 1.0f}},
        {{ kSize,  kSize,  kSize, 1.0f}},
        {{ kSize, -kSize,  kSize, 1.0f}},
    };
    uint32_t vertexCount = _countof(vertices);

    vertexResource = SkyBoxCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * vertexCount);
    vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = sizeof(VertexData) * vertexCount;
    vertexBufferView.StrideInBytes = sizeof(VertexData);

    VertexData* mappedVertexData = nullptr;
    vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertexData));
    std::memcpy(mappedVertexData, vertices, sizeof(VertexData) * vertexCount);

    uint16_t indices[] = {
        3, 2, 6, 3, 6, 7,
        4, 5, 1, 4, 1, 0,
        1, 5, 6, 1, 6, 2,
        4, 0, 3, 4, 3, 7,
        0, 1, 2, 0, 2, 3, // 謇句燕
        7, 6, 5, 7, 5, 4
    };
    uint32_t indexCount = _countof(indices);

    indexResource = SkyBoxCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(uint16_t) * indexCount);

    indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
    indexBufferView.SizeInBytes = sizeof(uint16_t) * indexCount;
    indexBufferView.Format = DXGI_FORMAT_R16_UINT;

    uint16_t* mappedIndexData = nullptr;
    indexResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedIndexData));
    std::memcpy(mappedIndexData, indices, sizeof(uint16_t) * indexCount);
}

void SkyBox::CreateConstantBuffers() {
    materialResource = SkyBoxCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(Material));
    materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));

    materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    materialData->enableLighting = 0;
    materialData->uvTransform = MakeIdentity4x4();
    materialData->shininess = 1.0f;

    transformResource = SkyBoxCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));
    transformResource->Map(0, nullptr, reinterpret_cast<void**>(&transformData));

    transformData->WVP = MakeIdentity4x4();
    transformData->world = MakeIdentity4x4();
    transformData->WorldInverseTranspose = MakeIdentity4x4();
}
