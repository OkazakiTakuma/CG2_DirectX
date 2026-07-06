#pragma once
#include "Model.h"
#include "Camera.h"
#include "struct.h"
#include <vector>
#include <d3d12.h>
#include <wrl.h>

struct InstancingMatrixData {
    Matrix4x4 WVP;
    Matrix4x4 world;
    Matrix4x4 WorldInverseTranspose;
};

class InstancingModel {
public:
    void Initialize(Model* model, uint32_t maxInstanceCount);

    void AddInstance(const Transform& transform);

    void Draw(Camera* camera);

    ~InstancingModel();
    void SetEnvironmentMapPath(const std::string& path) { envMapTexturePath_ = path; }
private:
    Model* model_ = nullptr;
    uint32_t maxInstanceCount_ = 1000;

    std::vector<Transform> transforms_;

    Microsoft::WRL::ComPtr<ID3D12Resource> instanceBuffer_;
    InstancingMatrixData* mappedData_ = nullptr;

    void CreateInstanceBuffer();

    struct DirectionalLight {
        Vector4 color;
        Vector3 direction;
        float intensity;
    };
    struct CameraForGPU {
        Vector3 worldPosition;
        float environmentMultiplier;
    };

    Microsoft::WRL::ComPtr<ID3D12Resource> lightResource_;
    DirectionalLight* lightData_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_;
    CameraForGPU* cameraData_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource_;
    PointLight* pointLightData_ = nullptr;
    std::string envMapTexturePath_;
    void CreateConstantBuffers();
};
