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
    /// <summary>
    /// 共有インスタンスを取得します。
    /// </summary>
    /// <returns>処理結果を返します。</returns>
    static LineDrawer* GetInstance();

    /// <summary>
    /// 必要なリソースを準備し、オブジェクトを初期化します。
    /// </summary>
    void Initialize();
    /// <summary>
    /// 確保したリソースを解放し、終了処理を行います。
    /// </summary>
    void Finalize();

    /// <summary>
    /// DrawLine の処理を行います。
    /// </summary>
    /// <param name="p1">p1 に使用する値を指定します。</param>
    /// <param name="p2">p2 に使用する値を指定します。</param>
    /// <param name="color">色を指定します。</param>
    void DrawLine(const Vector3& p1, const Vector3& p2, const Vector4& color);


    /// <summary>
    /// 現在の状態をもとに描画処理を行います。
    /// </summary>
    /// <param name="camera">描画や座標変換に使用するカメラを指定します。</param>
    /// <param name="blendMode">描画時に使用するブレンドモードを指定します。</param>
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
