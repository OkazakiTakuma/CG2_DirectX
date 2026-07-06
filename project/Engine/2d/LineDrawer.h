#pragma once
#include "Vector.h"
#include "Matrix.h"
#include "Camera.h"
#include <wrl.h>
#include <d3d12.h>
#include <cstdint>
#include"struct.h"

class LineDrawer {
public:
    static LineDrawer* GetInstance();

    void Initialize();
    void Finalize();

    void DrawLine(const Vector3& p1, const Vector3& p2, const Vector4& color);


    void Draw(Camera* camera, uint32_t blendMode = kBlendModeNormal);

private:
    LineDrawer() = default;
    ~LineDrawer() = default;

    struct VertexLine {
        Vector3 pos;
        Vector4 color;
    };

    static const uint32_t kMaxLineCount = 4096;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
    VertexLine* vertexData = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer;
    Matrix4x4* constData = nullptr;

    uint32_t currentLineCount_ = 0;
};
