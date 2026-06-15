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
    // スケールを大きくする（回転はなし、位置は原点固定）
    Vector3 scale = { 500.0f, 500.0f, 500.0f };
    Vector3 rotation = { 0.0f, 0.0f, 0.0f };
    Vector3 translation = { 0.0f, 0.0f, 0.0f }; // ★位置は原点に固定

    // スカイボックス自身のワールド行列を作成
    Matrix4x4 worldMatrix = MakeAffineMatrix(scale, rotation, translation);

    // ====== 【ここを修正】カメラのビュー行列から位置成分を取り除く ======
    Matrix4x4 viewMatrix = camera->GetViewMatrix();

    // ビュー行列の平行移動成分（4行目のx, y, z）を0にする
    // ※お使いのMatrix4x4の構造体のメンバー名（m[3][0] や mat[3][0] など）に合わせて調整してください
    viewMatrix.m[3][0] = 0.0f; // X移動
    viewMatrix.m[3][1] = 0.0f; // Y移動
    viewMatrix.m[3][2] = 0.0f; // Z移動
    // ==================================================================

    // 位置移動が消えたビュー行列と、プロジェクション行列を掛け合わせる
    Matrix4x4 viewProjectionMatrix = Multiply(viewMatrix, camera->GetProjectionMatrix());

    // 最終的な行列をシェーダー転送用バッファにセット
    transformData->WVP = Multiply(worldMatrix, viewProjectionMatrix);
    transformData->world = worldMatrix;
}
void SkyBox::Draw() {
    // 1. 共通の設定をコマンドリストに積む (変更なし)
    common_->SetDraw();
    auto commandList = common_->GetDxCommon()->GetCommandList();

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeapComPtr = SrvManager::GetInstance()->GetDescriptorHeap();
    assert(srvHeapComPtr.Get() != nullptr && "SrvManagerから取得したデスクリプタヒープがnullptrです！");

    ID3D12DescriptorHeap* ppHeaps[] = { srvHeapComPtr.Get() };
    commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    // 2. 個別のバッファをセット
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

    // 【追加】インデックスバッファをセット！
    commandList->IASetIndexBuffer(&indexBufferView);

    commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, transformResource->GetGPUVirtualAddress());

    D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = TextureManager::GetInstance()->GetSRVHandleGPU(textureFilePath);
    assert(srvHandle.ptr != 0 && "テクスチャのGPUハンドルが無効(0)です！");

    commandList->SetGraphicsRootDescriptorTable(2, srvHandle);

    // 3. 描画 【変更】DrawInstanced から DrawIndexedInstanced に変更！
    // 36個のインデックスを使って描画します
    commandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
}
void SkyBox::CreateVertexData() {
    const float kSize = 1.0f;
    // 頂点を36個から8個に削減！
    VertexData vertices[] = {
        {{-kSize, -kSize, -kSize, 1.0f}}, // 0: 左下手前
        {{-kSize,  kSize, -kSize, 1.0f}}, // 1: 左上手前
        {{ kSize,  kSize, -kSize, 1.0f}}, // 2: 右上手前
        {{ kSize, -kSize, -kSize, 1.0f}}, // 3: 右下手前
        {{-kSize, -kSize,  kSize, 1.0f}}, // 4: 左下奥
        {{-kSize,  kSize,  kSize, 1.0f}}, // 5: 左上奥
        {{ kSize,  kSize,  kSize, 1.0f}}, // 6: 右上奥
        {{ kSize, -kSize,  kSize, 1.0f}}, // 7: 右下奥
    };
    uint32_t vertexCount = _countof(vertices);

    // 1. 頂点バッファの作成と書き込み
    vertexResource = SkyBoxCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * vertexCount);
    vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = sizeof(VertexData) * vertexCount;
    vertexBufferView.StrideInBytes = sizeof(VertexData);

    VertexData* mappedVertexData = nullptr;
    vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertexData));
    std::memcpy(mappedVertexData, vertices, sizeof(VertexData) * vertexCount);

    // --- ここから新しくインデックスバッファを追加 ---
    // 頂点を結ぶ順番（36個）
    uint16_t indices[] = {
        3, 2, 6, 3, 6, 7, // 右
        4, 5, 1, 4, 1, 0, // 左
        1, 5, 6, 1, 6, 2, // 上
        4, 0, 3, 4, 3, 7, // 下
        0, 1, 2, 0, 2, 3, // 手前
        7, 6, 5, 7, 5, 4  // 奥
    };
    uint32_t indexCount = _countof(indices);

    // 2. インデックスバッファリソースの作成
    indexResource = SkyBoxCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(uint16_t) * indexCount);

    // 3. インデックスバッファビューの作成
    indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
    indexBufferView.SizeInBytes = sizeof(uint16_t) * indexCount;
    indexBufferView.Format = DXGI_FORMAT_R16_UINT; // uint16_tを使っているのでR16形式

    // 4. データの書き込み
    uint16_t* mappedIndexData = nullptr;
    indexResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedIndexData));
    std::memcpy(mappedIndexData, indices, sizeof(uint16_t) * indexCount);
}

void SkyBox::CreateConstantBuffers() {
    // 1. マテリアル用のバッファ作成 (b0)
    materialResource = SkyBoxCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(Material));
    materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));

    // マテリアルの初期値を設定
    materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f }; // 白（テクスチャの色をそのまま出す）
    materialData->enableLighting = 0;               // SkyBoxにライティングは不要
    materialData->uvTransform = MakeIdentity4x4();
    materialData->shininess = 1.0f;

    // 2. 変換行列用のバッファ作成 (b1)
    transformResource = SkyBoxCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));
    transformResource->Map(0, nullptr, reinterpret_cast<void**>(&transformData));

    transformData->WVP = MakeIdentity4x4();
    transformData->world = MakeIdentity4x4();
    transformData->WorldInverseTranspose = MakeIdentity4x4();
}