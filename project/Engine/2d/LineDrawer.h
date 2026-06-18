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

    // 線を追加する（Update等で何度でも呼べる）
    void DrawLine(const Vector3& p1, const Vector3& p2, const Vector4& color);


    // 溜まった線を一括で描画する（Draw3D等で1回だけ呼ぶ）
    void Draw(Camera* camera, uint32_t blendMode = kBlendModeNormal);

private:
    LineDrawer() = default;
    ~LineDrawer() = default;

    struct VertexLine {
        Vector3 pos;
        Vector4 color;
    };

    static const uint32_t kMaxLineCount = 4096; // 描画できる線の最大数

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
    VertexLine* vertexData = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer;
    Matrix4x4* constData = nullptr;

    uint32_t currentLineCount_ = 0;
};