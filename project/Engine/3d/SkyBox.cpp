#include "SkyBox.h"
#include "TextureManager.h"

void SkyBox::Initialize(const std::string& filePath) {
	common_ = SkyBoxCommon::GetInstance();
	textureFilePath = filePath;

	CreateVertexData();
	CreateConstantBuffers();
	this->camera = common_->GetDefaultCamera();
}

void SkyBox::Update() {
	// スケールを大きくし、位置をカメラに合わせる
	Vector3 scale = {500.0f, 500.0f, 500.0f};
	Vector3 rotation = {0.0f, 0.0f, 0.0f};
	Vector3 translation = camera->GetTranslate();

	Matrix4x4 worldMatrix = MakeAffineMatrix(scale, rotation, translation);
	Matrix4x4 viewProjectionMatrix = Multiply(camera->GetViewMatrix(), camera->GetProjectionMatrix());

	transformData->WVP = Multiply(worldMatrix, viewProjectionMatrix);
	transformData->world = worldMatrix;
}

void SkyBox::Draw() {
	// 1. 共通の設定をコマンドリストに積む
	common_->SetDraw();

	auto commandList = common_->GetDxCommon()->GetCommandList();

	// 2. 個別のバッファをセット
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(1, transformResource->GetGPUVirtualAddress());
	commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSRVHandleGPU(textureFilePath));

	// 3. 描画
	commandList->DrawInstanced(36, 1, 0, 0);
}

void SkyBox::CreateVertexData() {
    // SkyBox用の立方体の頂点データ (サイズは1.0fの立方体)
    // ※ 描画時にシェーダーや定数バッファでスケールを大きくして遠くに配置します
    const float kSize = 1.0f;
    VertexData vertices[] = {
        // 右(Right)
        {{ kSize, -kSize, -kSize, 1.0f}}, {{ kSize,  kSize, -kSize, 1.0f}}, {{ kSize,  kSize,  kSize, 1.0f}},
        {{ kSize, -kSize, -kSize, 1.0f}}, {{ kSize,  kSize,  kSize, 1.0f}}, {{ kSize, -kSize,  kSize, 1.0f}},
        // 左(Left)
        {{-kSize, -kSize,  kSize, 1.0f}}, {{-kSize,  kSize,  kSize, 1.0f}}, {{-kSize,  kSize, -kSize, 1.0f}},
        {{-kSize, -kSize,  kSize, 1.0f}}, {{-kSize,  kSize, -kSize, 1.0f}}, {{-kSize, -kSize, -kSize, 1.0f}},
        // 上(Up)
        {{-kSize,  kSize, -kSize, 1.0f}}, {{-kSize,  kSize,  kSize, 1.0f}}, {{ kSize,  kSize,  kSize, 1.0f}},
        {{-kSize,  kSize, -kSize, 1.0f}}, {{ kSize,  kSize,  kSize, 1.0f}}, {{ kSize,  kSize, -kSize, 1.0f}},
        // 下(Down)
        {{-kSize, -kSize,  kSize, 1.0f}}, {{-kSize, -kSize, -kSize, 1.0f}}, {{ kSize, -kSize, -kSize, 1.0f}},
        {{-kSize, -kSize,  kSize, 1.0f}}, {{ kSize, -kSize, -kSize, 1.0f}}, {{ kSize, -kSize,  kSize, 1.0f}},
        // 手前(Front)
        {{-kSize, -kSize, -kSize, 1.0f}}, {{-kSize,  kSize, -kSize, 1.0f}}, {{ kSize,  kSize, -kSize, 1.0f}},
        {{-kSize, -kSize, -kSize, 1.0f}}, {{ kSize,  kSize, -kSize, 1.0f}}, {{ kSize, -kSize, -kSize, 1.0f}},
        // 奥(Back)
        {{ kSize, -kSize,  kSize, 1.0f}}, {{ kSize,  kSize,  kSize, 1.0f}}, {{-kSize,  kSize,  kSize, 1.0f}},
        {{ kSize, -kSize,  kSize, 1.0f}}, {{-kSize,  kSize,  kSize, 1.0f}}, {{-kSize, -kSize,  kSize, 1.0f}},
    };

    uint32_t vertexCount = _countof(vertices);

    // 1. 頂点バッファリソースの作成 (既存の dxCommon を使用)
    vertexResource = SkyBoxCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * vertexCount);

    // 2. 頂点バッファビューの作成
    vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = sizeof(VertexData) * vertexCount;
    vertexBufferView.StrideInBytes = sizeof(VertexData);

    // 3. データの書き込み (Map)
    VertexData* mappedVertexData = nullptr;
    vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertexData));
    std::memcpy(mappedVertexData, vertices, sizeof(VertexData) * vertexCount);
    // Unmapはデストラクタで行うか、ここで行うかは他のクラス(Model.cppなど)の設計に合わせてください
}

void SkyBox::CreateConstantBuffers() {
	// 1. マテリアル用のバッファ作成 (b0)
	materialResource = SkyBoxCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(Material));
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));

	// マテリアルの初期値を設定
	materialData->color = {1.0f, 1.0f, 1.0f, 1.0f}; // 白（テクスチャの色をそのまま出す）
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