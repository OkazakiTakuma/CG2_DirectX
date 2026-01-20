#include "ParticleManager.h"
#include <cassert>
#include"Camera.h"
using namespace Logger;
const uint32_t ParticleManager::kMaxParticle = 512;

ParticleManager* ParticleManager::instance = nullptr;
ParticleManager* ParticleManager::GetInstance() {
	if (instance == nullptr) {
		instance = new ParticleManager;
	}
	return instance;
};

void ParticleManager::Initialize(DirectXCommon* dxCommon, SrvManager* srv) {

	// 1. ポインタをメンバに保存
	dxCommon_ = dxCommon;
	srvManager_ = srv;

	// 2. ランダムエンジン初期化
	std::random_device seedGenerator;
	randomEngine_ = std::mt19937(seedGenerator());

	// 3. 頂点データの初期化（例：最大パーティクル数分確保）
	vertices_.resize(6);

	// 4. 頂点リソースの生成（UploadHeap）
	vertexResource_ = dxCommon_->CreateBufferResource(sizeof(VertexData) * 6);

	// 5. VBV の生成
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * kMaxParticle;
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	// 6. 頂点リソースにデータを書き込む
	VertexData* mapped = nullptr;
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
	std::memcpy(mapped, vertices_.data(), sizeof(VertexData) * vertices_.size());
	vertexResource_->Unmap(0, nullptr);



	// パイプライン生成
	CreatePipelineState();
}

void ParticleManager::CreateParticleGroup(const std::string& groupName, const std::string& textureFilePath) {

	// --- 1. 登録済みの名前かチェック ---
	assert(particleGroups_.find(groupName) == particleGroups_.end() && "ParticleGroup name already exists!");

	// --- 2. 新しいパーティクルグループを作成して登録 ---
	ParticleGroup newGroup{};
	particleGroups_[groupName] = std::move(newGroup);
	ParticleGroup& group = particleGroups_[groupName];

	// --- 3. マテリアルデータにテクスチャファイルパスを設定 ---
	group.material.textureFilePath = textureFilePath;

	// --- 4. テクスチャを読み込む ---
	TextureManager::GetInstance()->LoadTexture(textureFilePath);

	// --- 5. マテリアルデータにテクスチャの SRV インデックスを記録 ---
	group.material.textureIndex = TextureManager::GetInstance()->GetSrvIndex(textureFilePath);

	// --- 6. インスタンシング用リソースの生成 ---
	// 最大インスタンス数分の StructuredBuffer を作る
	const uint32_t maxInstance = kMaxParticle;
	uint32_t bufferSize = sizeof(ParticleForGPU) * maxInstance;

	group.instanceResource = dxCommon_->CreateBufferResource(bufferSize);

	// Map して書き込みポインタを保持
	group.instanceResource->Map(0, nullptr, reinterpret_cast<void**>(&group.instanceDataPtr));

	// --- 7. インスタンシング用 SRV を確保 ---
	group.instanceSrvIndex = srvManager_->Allocate();

	// --- 8. SRV 生成（StructuredBuffer 用） ---
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN; // StructuredBuffer は UNKNOWN
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = maxInstance;
	srvDesc.Buffer.StructureByteStride = sizeof(ParticleForGPU);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	srvManager_->CreateSRVforStructuredBuffer(group.instanceSrvIndex, group.instanceResource.Get(), srvDesc.Buffer.NumElements, srvDesc.Buffer.StructureByteStride);

	// 初期インスタンス数は 0
	group.instanceCount = 5;
}

void ParticleManager::Draw(Camera* camera) { // ← カメラを引数で受け取る必要があります
	// 全体の設定
	dxCommon_->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());
	dxCommon_->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());
	dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView_);

	// パーティクルグループごとに描画
	for (auto& [groupName, group] : particleGroups_) {
		// 1. インスタンス数が0なら描画しない
		if (group.particles.empty()) {
			group.instanceCount = 0;
			continue;
		}

		// 2. 上限チェック (kMaxParticleを超えないように)
		uint32_t count = 0;
		for (const auto& particle : group.particles) {
			if (count >= kMaxParticle)
				break;

			// --- GPU用メモリにデータを書き込む (CPU -> GPU) ---
			// ※ここでWorld行列計算や色のセットを行います
			Matrix4x4 worldMatrix = MakeAffineMatrix(particle.transform.scale, particle.transform.rotate, particle.transform.translate);
			Matrix4x4 viewProjection = camera->GetViewProjectionMatrix();
			Matrix4x4 worldViewProjection = Multiply(worldMatrix, viewProjection);

			// 下記はParticleForGPU構造体の定義に合わせて書き換えてください
			group.instanceDataPtr[count].WVP = worldViewProjection;

			count++;
		}
		group.instanceCount = count;

		// 3. ルートパラメータの設定 (※ルートシグネチャの定義順に合わせてください)

		// [0] マテリアル or カメラ (例: 色や共通設定)
		// もしマテリアルCBufferがあるならセット。なければ省略可だが、RootSigと一致させる必要あり
		// dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialConstBuffer->GetGPUVirtualAddress());

		// [1] テクスチャ (SRV)
		// ParticleGroup作成時に保存しておいたテクスチャのSRVインデックスを使う
		// ※ group構造体に textureSrvIndex を追加する必要があります
		srvManager_->SetGraphicsRootDescriptorTable(1, group.material.textureIndex);

		// [2] インスタンシングデータ (StructuredBuffer SRV)
		// ここで instanceSrvIndex を使います
		srvManager_->SetGraphicsRootDescriptorTable(2, group.instanceSrvIndex);

		// 4. 描画コマンド
		dxCommon_->GetCommandList()->DrawInstanced(
		    6,                   // 頂点数 (板ポリゴンなら6)
		    group.instanceCount, // インスタンス数 (現在のパーティクル数)
		    0, 0);
	}
}

void ParticleManager::Update() {
	// デルタタイム（固定値またはEngineから取得。ここでは60FPS想定）
	const float kDeltaTime = 1.0f / 60.0f;

	// ---------------------------------------------------------
	// 1. ビュー行列とプロジェクション行列をカメラから取得
	// ---------------------------------------------------------

	Matrix4x4 viewMatrix = camera_->GetViewMatrix();
	Matrix4x4 projectionMatrix = camera_->GetProjectionMatrix();

	// ---------------------------------------------------------
	// 2. ビルボード行列の計算
	// ---------------------------------------------------------
	// カメラのワールド行列を取得
	Matrix4x4 billboardMatrix = camera_->GetWorldMatrix();

	// 平行移動成分を削除して、回転成分のみにする
	// (これで「カメラと同じ向き」を持つ回転行列になる)
	billboardMatrix.m[3][0] = 0.0f;
	billboardMatrix.m[3][1] = 0.0f;
	billboardMatrix.m[3][2] = 0.0f;
	billboardMatrix.m[3][3] = 1.0f;

	// ---------------------------------------------------------
	// 3. 全グループ・全パーティクルの処理 (二重ループ)
	// ---------------------------------------------------------

	// ▼ 外側のループ：パーティクルグループごとの処理
	for (auto& [groupName, group] : particleGroups_) {

		// インスタンス数をリセット
		group.instanceCount = 0;

		// 書き込みのためにバッファをマップ (Map)
		// ※ 既に永続的にMapしている設計の場合は不要ですが、一般的にはここでMapします
		HRESULT hr = group.instanceResource->Map(0, nullptr, reinterpret_cast<void**>(&group.instanceDataPtr));
		assert(SUCCEEDED(hr));

		// ▼ 内側のループ：グループ内の全パーティクル処理
		for (auto it = group.particles.begin(); it != group.particles.end();) {
			Particle& p = *it;

			// --- 3-1. 寿命に達していたらグループから外す ---
			if (p.currentTime >= p.lifeTime) {
				it = group.particles.erase(it); // 削除してイテレータを進める
				continue;                       // 次のループへ
			}

			// --- 3-2. 場の影響を計算（加速） ---
			// 例: 重力を加算
			// Vector3 acceleration = { 0.0f, -9.8f, 0.0f };
			// p.velocity = p.velocity + (acceleration * kDeltaTime);
			// ※Vectorの演算子定義に合わせて調整してください

			// --- 3-3. 移動処理（速度を座標に加算） ---
			p.transform.translate.x += p.velocity.x * kDeltaTime;
			p.transform.translate.y += p.velocity.y * kDeltaTime;
			p.transform.translate.z += p.velocity.z * kDeltaTime;

			// --- 3-4. 経過時間を加算 ---
			p.currentTime += kDeltaTime;

			// --- 3-5. ワールド行列を計算 ---
			// 順序: Scale -> Rotate(Z) -> Billboard -> Translate

			// 1. スケール
			Matrix4x4 scaleMat = MakeScaleMatrix(p.transform.scale);

			// 2. 回転 (パーティクル自体の回転は通常Z軸のみ使用)
			Matrix4x4 rotateMat = MakeRotateZMatrix(p.transform.rotate.z);

			// 3. 平行移動
			Matrix4x4 translateMat = MakeTranslateMatrix(p.transform.translate);

			// 4. 合成 (SRT = Scale * Rotate * Billboard * Translate)
			// ※ 行列積の関数名(Multiply)は環境に合わせてください
			Matrix4x4 worldMatrix = Multiply(scaleMat, rotateMat); // 自前の変形
			worldMatrix = Multiply(worldMatrix, billboardMatrix);  // カメラの方を向く
			worldMatrix = Multiply(worldMatrix, translateMat);     // その場所へ

			// --- 3-6. ワールドビュープロジェクション行列を合成 ---
			// WVP = World * View * Projection
			Matrix4x4 worldViewProjection = Multiply(worldMatrix, viewMatrix);
			worldViewProjection = Multiply(worldViewProjection, projectionMatrix);

			// --- 3-7. インスタンシング用データ1個分の書き込み ---
			// バッファの最大数を超えないようにチェック
			if (group.instanceCount < kMaxParticle) {
				group.instanceDataPtr[group.instanceCount].WVP = worldViewProjection;
				group.instanceDataPtr[group.instanceCount].world = worldMatrix;
				group.instanceDataPtr[group.instanceCount].color = p.color;

				group.instanceCount++;
			}

			// 次のパーティクルへ
			++it;
		}

		// 書き込み終了 (Unmap)
		group.instanceResource->Unmap(0, nullptr);
	}
}
void ParticleManager::Emit(const std::string& groupName, const Vector3& position, uint32_t count) {
	// --- 1. パーティクルグループの取得 ---
	auto groupIt = particleGroups_.find(groupName);
	assert(groupIt != particleGroups_.end() && "ParticleGroup not found!");
	ParticleGroup& group = groupIt->second;

	// 分布生成（ループ内で何度も作ると重くなるため、外に出すのがベター）
	std::uniform_real_distribution<float> distPos(-0.5f, 0.5f);   // 発生位置のばらつき
	std::uniform_real_distribution<float> distVelXZ(-0.1f, 0.1f); // 水平速度
	std::uniform_real_distribution<float> distVelY(0.1f, 0.3f);   // 上昇速度
	std::uniform_real_distribution<float> distTime(1.0f, 3.0f);   // 寿命

	// --- 2. パーティクルの生成 ---
	for (uint32_t i = 0; i < count; ++i) {
		// 最大数チェック
		if (group.particles.size() >= kMaxParticle) {
			break;
		}

		Particle newParticle{};

		// 【重要】サイズと色を必ず設定する！
		// これがないとサイズ0または透明になり見えません
		newParticle.transform.scale = {1.0f, 1.0f, 1.0f};
		newParticle.transform.rotate = {0.0f, 0.0f, 0.0f};
		newParticle.transform.translate = {position.x + distPos(randomEngine_), position.y + distPos(randomEngine_), position.z + distPos(randomEngine_)};

		// 色（白、不透明）
		newParticle.color = {1.0f, 1.0f, 1.0f, 1.0f};

		// 速度と寿命
		newParticle.velocity = {distVelXZ(randomEngine_), distVelY(randomEngine_), distVelXZ(randomEngine_)};
		newParticle.lifeTime = distTime(randomEngine_);
		newParticle.currentTime = 0.0f;

		// リストに追加
		group.particles.push_back(newParticle);
	}
}


void ParticleManager::CreateRootSignature() {
	HRESULT hr;
	//	assert(SUCCEEDED(hr));
	D3D12_DESCRIPTOR_RANGE instancingdescriptorRange[1] = {};
	instancingdescriptorRange[0].BaseShaderRegister = 0;
	instancingdescriptorRange[0].NumDescriptors = 2;
	instancingdescriptorRange[0].RegisterSpace = 0;
	instancingdescriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	instancingdescriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// RootSignatureの設定
	D3D12_ROOT_SIGNATURE_DESC instancingdescriptionRootSignature{};
	instancingdescriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; // 入力アセンブラーでの使用を許可
	// RootParameterの設定。複数設定できるので配列、今回は結果1つだけなので長さ1の配列
	D3D12_ROOT_PARAMETER instancingrootParameters[4] = {};
	// ルートパラメーターの設定
	instancingrootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;    // ルートパラメーターのタイプ（CBV）
	instancingrootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // シェーダーの可視性（ピクセルシェーダー）
	instancingrootParameters[0].Descriptor.ShaderRegister = 0;                    // シェーダーレジスタのインデックス
	instancingrootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	instancingrootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	instancingrootParameters[1].DescriptorTable.pDescriptorRanges = instancingdescriptorRange;
	instancingrootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(instancingdescriptorRange);
	instancingrootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;             // ルートパラメーターのタイプ（CBV）
	instancingrootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;         // シェーダーの可視性（バーテックスシェーダー）
	instancingrootParameters[2].Descriptor.ShaderRegister = 1;                             // シェーダーレジスタのインデックス
	instancingrootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;             // ルートパラメーターのタイプ（CBV）
	instancingrootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;          // シェーダーの可視性（ピクセルシェーダー）
	instancingrootParameters[3].Descriptor.ShaderRegister = 2;                             // シェーダーレジスタのインデックス
	instancingdescriptionRootSignature.pParameters = instancingrootParameters;             // ルートパラメーターの配列
	instancingdescriptionRootSignature.NumParameters = _countof(instancingrootParameters); // ルートパラメーターの数
	D3D12_STATIC_SAMPLER_DESC instancingstaticSamplers[1] = {};
	instancingstaticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	instancingstaticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	instancingstaticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	instancingstaticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	instancingstaticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	instancingstaticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	instancingstaticSamplers[0].ShaderRegister = 0;
	instancingstaticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	instancingdescriptionRootSignature.pStaticSamplers = instancingstaticSamplers;
	instancingdescriptionRootSignature.NumStaticSamplers = _countof(instancingstaticSamplers);

	// シリアライズしてバイナリにする
	ID3DBlob* instancingsignatureBlob = nullptr;
	ID3DBlob* instancingerrorBlob = nullptr;

	hr = D3D12SerializeRootSignature(
	    &instancingdescriptionRootSignature, // ルートシグネチャの説明
	    D3D_ROOT_SIGNATURE_VERSION_1,        // バージョン
	    &instancingsignatureBlob,            // シリアライズされたバイナリ
	    &instancingerrorBlob                 // エラー情報
	);
	if (FAILED(hr)) {
		Log(reinterpret_cast<const char*>(instancingerrorBlob->GetBufferPointer())); // エラー内容をLogに出力
		assert(false);                                                               // シリアライズが失敗した場合はアサート
	}
	// バイナリをもとにルートシグネチャを生成
	hr = dxCommon_->GetDevice()->CreateRootSignature(
	    0,                                           // シグネチャのバージョン
	    instancingsignatureBlob->GetBufferPointer(), // シリアライズされたバイナリのポインタ
	    instancingsignatureBlob->GetBufferSize(),    // バイナリのサイズ
	    IID_PPV_ARGS(&rootSignature)       // 生成したルートシグネチャを受け取る
	);
	assert(SUCCEEDED(hr)); // ルートシグネチャの生成が成功したか確認
}

void ParticleManager::CreatePipelineState() {
	HRESULT hr;
	CreateRootSignature();
	// InputLayoutの設定
	D3D12_INPUT_ELEMENT_DESC instancinginputElementDescs[3] = {};
	instancinginputElementDescs[0].SemanticName = "POSITION";                        // セマンティック名
	instancinginputElementDescs[0].SemanticIndex = 0;                                // セマンティックインデックス
	instancinginputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;          // フォーマット
	instancinginputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT; // アライメントオフセット
	instancinginputElementDescs[1].SemanticName = "TEXCOORD";                        // セマンティック名
	instancinginputElementDescs[1].SemanticIndex = 0;                                // セマンティックインデックス
	instancinginputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;                // フォーマット
	instancinginputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT; // アライメントオフセット
	instancinginputElementDescs[2].SemanticName = "NORMAL";                          // セマンティック名
	instancinginputElementDescs[2].SemanticIndex = 0;                                // セマンティックインデックス
	instancinginputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;             // フォーマット
	instancinginputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT; // アライメントオフセット
	D3D12_INPUT_LAYOUT_DESC instancinginputLayoutDesc{};
	instancinginputLayoutDesc.pInputElementDescs = instancinginputElementDescs;    // 入力要素の配列
	instancinginputLayoutDesc.NumElements = _countof(instancinginputElementDescs); // 入力要素の数

	D3D12_BLEND_DESC instancingblendDesc{};
	// すべての色要素を書き込む
	instancingblendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	instancingblendDesc.RenderTarget[0].BlendEnable = TRUE; // ブレンドを無効にする
	                                                        // すべての色要素を書き込む
	// すべての色要素を書き込む
	BlendMode blendMode = BlendMode::kBlendModeMultiply;

	switch (blendMode) {
	case BlendMode::kBlendModeNormal:
		// すべての色要素を書き込む
		instancingblendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;      // ソースのブレンドファクター
		instancingblendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;          // ブレンドの演算
		instancingblendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA; // デスティネーションのブレンドファクター
		break;
	case BlendMode::kBlendModeAdd:
		// すべての色要素を書き込む
		instancingblendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA; // ソースのブレンドファクター
		instancingblendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;     // ブレンドの演算
		instancingblendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;      // デスティネーションのブレンドファクター
		break;
	case BlendMode::kBlendModeSubtract:
		// すべての色要素を書き込む
		instancingblendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;      // ソースのブレンドファクター
		instancingblendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT; // ブレンドの演算
		instancingblendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;           // デスティネーションのブレンドファクター
		break;
	case BlendMode::kBlendModeMultiply:
		// すべての色要素を書き込む
		instancingblendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;  // ソースのブレンドファクター
		instancingblendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;      // ブレンドの演算
		instancingblendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR; // デスティネーションのブレンドファクター
		break;
	};

	instancingblendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;   // デスティネーションのアルファブレンドファクター
	instancingblendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD; // アルファブレンドの演算
	instancingblendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO; // デスティネーションのアルファブレンドファクター
	// RasterizerStateの設定
	// RasterizerStateの設定
	D3D12_RASTERIZER_DESC instancingrasterizerDesc{};
	// 裏面（時計回り）を表示しない
	instancingrasterizerDesc.CullMode = D3D12_CULL_MODE_BACK; // 裏面をカリング
	// 中身を塗りつぶす
	instancingrasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID; // 塗りつぶしモード
	// Shaderのコンパイル
	Microsoft::WRL::ComPtr<IDxcBlob> instancingvertexShaderBlob = dxCommon_->CompileShader(L"Resources/Shader/Particle.VS.hlsl", L"vs_6_0");
	assert(instancingvertexShaderBlob != nullptr); // Vertex Shaderのコンパイルが成功したか確認
	Microsoft::WRL::ComPtr<IDxcBlob> instancingpixelShaderBlob = dxCommon_->CompileShader(L"Resources/Shader/Particle.PS.hlsl", L"ps_6_0");
	assert(instancingpixelShaderBlob != nullptr); // Pixel Shaderのコンパイルが成功したか確認
	D3D12_DEPTH_STENCIL_DESC instancingdepthStenecilDesc{};
	instancingdepthStenecilDesc.DepthEnable = true;
	instancingdepthStenecilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	instancingdepthStenecilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC instancinggraphicsPipelineStateDesc{};
	instancinggraphicsPipelineStateDesc.pRootSignature = rootSignature.Get();                                                               // ルートシグネチャ
	instancinggraphicsPipelineStateDesc.InputLayout = instancinginputLayoutDesc;                                                            // 入力レイアウト
	instancinggraphicsPipelineStateDesc.BlendState = instancingblendDesc;                                                                   // ブレンドステート
	instancinggraphicsPipelineStateDesc.RasterizerState = instancingrasterizerDesc;                                                         // ラスタライザーステート
	instancinggraphicsPipelineStateDesc.VS = {instancingvertexShaderBlob->GetBufferPointer(), instancingvertexShaderBlob->GetBufferSize()}; // Vertex Shader
	instancinggraphicsPipelineStateDesc.PS = {instancingpixelShaderBlob->GetBufferPointer(), instancingpixelShaderBlob->GetBufferSize()};   // Pixel Shader
	instancinggraphicsPipelineStateDesc.DepthStencilState = instancingdepthStenecilDesc;
	instancinggraphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	// 書き込むRTVの情報
	instancinggraphicsPipelineStateDesc.NumRenderTargets = 1;                            // レンダーターゲットの数
	instancinggraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // レンダーターゲットのフォーマット
	// 利用するトポロジ（形状）のタイプ。三角形
	instancinggraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // トポロジのタイプ
	// どのように画面に色を打ち込むかの設定
	instancinggraphicsPipelineStateDesc.SampleDesc.Count = 1;                   // マルチサンプルの数
	instancinggraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; // サンプルマスク
	// 実際に生成
	hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&instancinggraphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr)); // パイプラインステートの生成が成功したか確認
}
