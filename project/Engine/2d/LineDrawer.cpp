#include "LineDrawer.h"
#include "LineCommon.h"
#include <cassert>

/// <summary>
/// 共有インスタンスを取得します。
/// </summary>
/// <returns>処理結果を返します。</returns>
LineDrawer* LineDrawer::GetInstance() {
    static LineDrawer instance;
    return &instance;
}

/// <summary>
/// 必要なリソースを準備し、オブジェクトを初期化します。
/// </summary>
void LineDrawer::Initialize() {
    DirectXCommon* dxCommon = LineCommon::GetInstance()->GetDxCommon();
    assert(dxCommon);

    vertexResource = dxCommon->CreateBufferResource(sizeof(VertexLine) * kMaxLineCount * 2);
    vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = sizeof(VertexLine) * kMaxLineCount * 2;
    vertexBufferView.StrideInBytes = sizeof(VertexLine);
    vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

    constBuffer = dxCommon->CreateBufferResource(sizeof(Matrix4x4));
    constBuffer->Map(0, nullptr, reinterpret_cast<void**>(&constData));
}

/// <summary>
/// DrawLine の処理を行います。
/// </summary>
/// <param name="p1">p1 に使用する値を指定します。</param>
/// <param name="p2">p2 に使用する値を指定します。</param>
/// <param name="color">色を指定します。</param>
void LineDrawer::DrawLine(const Vector3& p1, const Vector3& p2, const Vector4& color) {
    if (currentLineCount_ >= kMaxLineCount) return;

    uint32_t index = currentLineCount_ * 2;
    // 蟋狗せ
    vertexData[index].pos = p1;
    vertexData[index].color = color;
    vertexData[index + 1].pos = p2;
    vertexData[index + 1].color = color;

    currentLineCount_++;
}

/// <summary>
/// 現在の状態をもとに描画処理を行います。
/// </summary>
/// <param name="camera">描画や座標変換に使用するカメラを指定します。</param>
/// <param name="blendMode">描画時に使用するブレンドモードを指定します。</param>
void LineDrawer::Draw(Camera* camera, uint32_t blendMode) {
    if (currentLineCount_ == 0) return;

    *constData = camera->GetViewProjectionMatrix();

    auto commandList = LineCommon::GetInstance()->GetDxCommon()->GetCommandList();

    LineCommon::GetInstance()->SetDraw(blendMode);

    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    commandList->SetGraphicsRootConstantBufferView(0, constBuffer->GetGPUVirtualAddress());

    commandList->DrawInstanced(currentLineCount_ * 2, 1, 0, 0);

    currentLineCount_ = 0;
}

/// <summary>
/// 確保したリソースを解放し、終了処理を行います。
/// </summary>
void LineDrawer::Finalize() {
    if (vertexResource) vertexResource->Unmap(0, nullptr);
    if (constBuffer) constBuffer->Unmap(0, nullptr);
}
