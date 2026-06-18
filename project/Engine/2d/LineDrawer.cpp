#include "LineDrawer.h"
#include "LineCommon.h"
#include <cassert>

LineDrawer* LineDrawer::GetInstance() {
    static LineDrawer instance;
    return &instance;
}

void LineDrawer::Initialize() {
    // LineCommonからdxCommonをもらう
    DirectXCommon* dxCommon = LineCommon::GetInstance()->GetDxCommon();
    assert(dxCommon);

    // 1. 頂点バッファの作成
    vertexResource = dxCommon->CreateBufferResource(sizeof(VertexLine) * kMaxLineCount * 2);
    vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = sizeof(VertexLine) * kMaxLineCount * 2;
    vertexBufferView.StrideInBytes = sizeof(VertexLine);
    vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

    // 2. 定数バッファの作成（カメラの行列用）
    constBuffer = dxCommon->CreateBufferResource(sizeof(Matrix4x4));
    constBuffer->Map(0, nullptr, reinterpret_cast<void**>(&constData));
}

void LineDrawer::DrawLine(const Vector3& p1, const Vector3& p2, const Vector4& color) {
    if (currentLineCount_ >= kMaxLineCount) return;

    uint32_t index = currentLineCount_ * 2;
    // 始点
    vertexData[index].pos = p1;
    vertexData[index].color = color;
    // 終点
    vertexData[index + 1].pos = p2;
    vertexData[index + 1].color = color;

    currentLineCount_++;
}

void LineDrawer::Draw(Camera* camera, uint32_t blendMode) {
    if (currentLineCount_ == 0) return;

    // カメラの行列を更新
    *constData = camera->GetViewProjectionMatrix();

    auto commandList = LineCommon::GetInstance()->GetDxCommon()->GetCommandList();

    // ★ SpriteCommon と同じように、LineCommon に PSO と RootSig をセットしてもらう
    LineCommon::GetInstance()->SetDraw(blendMode);

    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    commandList->SetGraphicsRootConstantBufferView(0, constBuffer->GetGPUVirtualAddress());

    // 描画（登録された線の数 × 2頂点）
    commandList->DrawInstanced(currentLineCount_ * 2, 1, 0, 0);

    // 描画が終わったらカウンタをリセット
    currentLineCount_ = 0;
}

void LineDrawer::Finalize() {
    if (vertexResource) vertexResource->Unmap(0, nullptr);
    if (constBuffer) constBuffer->Unmap(0, nullptr);
}