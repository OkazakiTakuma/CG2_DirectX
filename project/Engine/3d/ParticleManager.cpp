#include "ParticleManager.h"
#include "Camera.h"
#include <algorithm> // std::min用
#include <cassert>
#include <numbers>
#include <cmath>

// 頂点データを生成する補助関数
std::vector<VertexData> GenerateRingVerticesForParticle(uint32_t segments, float outerRadius, float innerRadius) {
	std::vector<VertexData> vertices;
	vertices.reserve(segments * 6);

	for (uint32_t i = 0; i < segments; ++i) {
		// 円周をどれくらい進んだかの割合（0.0 ～ 1.0）
		float ratio1 = static_cast<float>(i) / segments;
		float ratio2 = static_cast<float>(i + 1) / segments;

		float angle1 = ratio1 * 2.0f * std::numbers::pi_v<float>;
		float angle2 = ratio2 * 2.0f * std::numbers::pi_v<float>;

		// 座標の計算
		Vector4 outer1 = { outerRadius * std::cos(angle1), outerRadius * std::sin(angle1), 0.0f, 1.0f };
		Vector4 inner1 = { innerRadius * std::cos(angle1), innerRadius * std::sin(angle1), 0.0f, 1.0f };
		Vector4 outer2 = { outerRadius * std::cos(angle2), outerRadius * std::sin(angle2), 0.0f, 1.0f };
		Vector4 inner2 = { innerRadius * std::cos(angle2), innerRadius * std::sin(angle2), 0.0f, 1.0f };

		// ─── ★ここがポイント！UV座標の設定 ───
		// 外側は V = 0.0f (テクスチャの上), 内側は V = 1.0f (テクスチャの下)
		// Uは円周に沿って 0.0f から 1.0f へ進む
		Vector2 uvOuter1 = { ratio1, 0.0f };
		Vector2 uvInner1 = { ratio1, 1.0f };
		Vector2 uvOuter2 = { ratio2, 0.0f };
		Vector2 uvInner2 = { ratio2, 1.0f };

		Vector3 normal = { 0.0f, 0.0f, -1.0f };

		// 1つ目の三角形 (外1, 内1, 外2)
		vertices.push_back({ outer1, uvOuter1, normal });
		vertices.push_back({ inner1, uvInner1, normal });
		vertices.push_back({ outer2, uvOuter2, normal });

		// 2つ目の三角形 (内1, 内2, 外2)
		vertices.push_back({ inner1, uvInner1, normal });
		vertices.push_back({ inner2, uvInner2, normal });
		vertices.push_back({ outer2, uvOuter2, normal });
	}

	return vertices;
}using namespace Logger;

// 定数定義
const uint32_t ParticleManager::kMaxParticle = 512;

ParticleManager* ParticleManager::instance = nullptr;

ParticleManager* ParticleManager::GetInstance() {
	if (instance == nullptr) {
		instance = new ParticleManager;
	}
	return instance;
}

void ParticleManager::Finalize() {
	// インスタンスリソースの破棄
	for (auto& group : particleGroups_) {
		group.second.instanceResource.Reset();
	}
	particleGroups_.clear();

	// パイプラインの破棄（ここが漏れている可能性が高いです）
	rootSignature.Reset();
	for (auto& pso : graphicsPipelineStates) {
		pso.Reset();
	}
	delete instance;
	instance = nullptr;
}
void ParticleManager::Initialize(DirectXCommon* dxCommon) {

	// 1. ポインタ保存
	dxCommon_ = dxCommon;
	srvManager_ = SrvManager::GetInstance();

	// 2. ランダムエンジン初期化
	std::random_device seedGenerator;
	randomEngine_ = std::mt19937(seedGenerator());

	// 3. 頂点データの初期化 (main (2).cpp の VertexData 構造体に合わせる)
	// POSITION(float4), TEXCOORD(float2), NORMAL(float3)
	// ParticleManager.cpp の Initialize 関数内

	// 3. 頂点データの初期化
	vertices_.resize(6);

	// 左下
	vertices_[0] = VertexData{
		{-0.5f, -0.5f, 0.0f, 1.0f},
		{0.0f, 1.0f},
		{0.0f, 0.0f, -1.0f}
	};
	// 左上
	vertices_[1] = VertexData{
		{-0.5f, 0.5f, 0.0f, 1.0f},
		{0.0f, 0.0f},
		{0.0f, 0.0f, -1.0f}
	};
	// 右下
	vertices_[2] = VertexData{
		{0.5f, -0.5f, 0.0f, 1.0f},
		{1.0f, 1.0f},
		{0.0f, 0.0f, -1.0f}
	};
	// 左上
	vertices_[3] = VertexData{
		{-0.5f, 0.5f, 0.0f, 1.0f},
		{0.0f, 0.0f},
		{0.0f, 0.0f, -1.0f}
	};
	// 右上
	vertices_[4] = VertexData{
		{0.5f, 0.5f, 0.0f, 1.0f},
		{1.0f, 0.0f},
		{0.0f, 0.0f, -1.0f}
	};
	// 右下
	vertices_[5] = VertexData{
		{0.5f, -0.5f, 0.0f, 1.0f},
		{1.0f, 1.0f},
		{0.0f, 0.0f, -1.0f}
	};
	// 4. 頂点リソース生成
	vertexResource_ = dxCommon_->CreateBufferResource(sizeof(VertexData) * 6);

	// 5. VBV生成
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * 6;
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	// 6. データ転送
	VertexData* mapped = nullptr;
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
	std::memcpy(mapped, vertices_.data(), sizeof(VertexData) * vertices_.size());
	vertexResource_->Unmap(0, nullptr);

	// パイプライン生成
	CreatePipelineState();
}

void ParticleManager::CreateParticleGroup(const std::string& groupName, const std::string& textureFilePath, ParticleMeshType meshtype) {

	// 登録済みチェック
	if (particleGroups_.find(groupName) != particleGroups_.end()) {
		assert(false && "ParticleGroup name already exists!");
		return;
	}

	// グループ作成
	ParticleGroup newGroup{};
	particleGroups_[groupName] = std::move(newGroup);
	ParticleGroup& group = particleGroups_[groupName];

	// マテリアルとメッシュタイプの設定
	group.material.textureFilePath = textureFilePath;
	TextureManager::GetInstance()->LoadTexture(textureFilePath);
	group.material.textureIndex = TextureManager::GetInstance()->GetSrvIndex(textureFilePath);
	group.meshType = meshtype; // 渡された形状を保存

	// =========================================================
	// ▼ 形状に応じた頂点データの用意と専用頂点バッファの作成
	// =========================================================
	std::vector<VertexData> vertices;
	if (meshtype == kMeshTypeRing) {
		// リング型ポリゴン (32分割、外径1.0、内径0.8)
		vertices = GenerateRingVerticesForParticle(32, 1.0f, 0.2f);
	}
	else {
		// 通常の四角形ポリゴン
		vertices = {
			{ {-0.5f, -0.5f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f} },
			{ {-0.5f,  0.5f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f} },
			{ { 0.5f, -0.5f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f} },
			{ {-0.5f,  0.5f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f} },
			{ { 0.5f,  0.5f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f} },
			{ { 0.5f, -0.5f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f} },
		};
	}
	group.vertexCount = static_cast<uint32_t>(vertices.size());

	// 専用の頂点バッファを作成 (dxCommon_->CreateBufferResource を活用!)
	UINT sizeVB = static_cast<UINT>(sizeof(VertexData) * vertices.size());
	group.vertBuff = dxCommon_->CreateBufferResource(sizeVB);

	// 頂点データをバッファに書き込む
	VertexData* vertMap = nullptr;
	group.vertBuff->Map(0, nullptr, reinterpret_cast<void**>(&vertMap));
	std::copy(vertices.begin(), vertices.end(), vertMap);
	group.vertBuff->Unmap(0, nullptr);

	// 頂点バッファビューの設定
	group.vbView.BufferLocation = group.vertBuff->GetGPUVirtualAddress();
	group.vbView.SizeInBytes = sizeVB;
	group.vbView.StrideInBytes = sizeof(VertexData);
	// =========================================================

	// インスタンシング用リソース生成
	const uint32_t maxInstance = kMaxParticle;
	uint32_t bufferSize = sizeof(ParticleForGPU) * maxInstance;

	group.instanceResource = dxCommon_->CreateBufferResource(bufferSize);

	// Mapしてポインタ保持
	group.instanceResource->Map(0, nullptr, reinterpret_cast<void**>(&group.instanceDataPtr));

	// SRV生成
	group.instanceSrvIndex = srvManager_->Allocate();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = maxInstance;
	srvDesc.Buffer.StructureByteStride = sizeof(ParticleForGPU);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	srvManager_->CreateSRVforStructuredBuffer(group.instanceSrvIndex, group.instanceResource.Get(), srvDesc.Buffer.NumElements, srvDesc.Buffer.StructureByteStride);

	group.instanceCount = 0;
	// ID3D12Resource* texRes = TextureManager::GetInstance()->GetResource(textureFilePath);

	// 【追加】テクスチャを COPY_DEST から PIXEL_SHADER_RESOURCE へ遷移させる
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = TextureManager::GetInstance()->GetResource(textureFilePath).Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	//dxCommon_->GetCommandList()->ResourceBarrier(1, &barrier);
}
// ==========================================
// ルートシグネチャ生成 (main (2).cpp の構成を参考に整理)
// ==========================================
void ParticleManager::CreateRootSignature() {
	HRESULT hr;

	// [0] テクスチャ (t0)
	D3D12_DESCRIPTOR_RANGE descriptorRangeTexture[1] = {};
	descriptorRangeTexture[0].BaseShaderRegister = 0;
	descriptorRangeTexture[0].NumDescriptors = 1;
	descriptorRangeTexture[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRangeTexture[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// [1] StructuredBuffer (t1)
	D3D12_DESCRIPTOR_RANGE descriptorRangeData[1] = {};
	descriptorRangeData[0].BaseShaderRegister = 1;
	descriptorRangeData[0].NumDescriptors = 1;
	descriptorRangeData[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRangeData[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ルートパラメータ
	D3D12_ROOT_PARAMETER rootParameters[2] = {};

	// Param 0: Texture
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRangeTexture;
	rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeTexture);

	// Param 1: Instancing Data
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRangeData;
	rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeData);

	// サンプラー
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumParameters = _countof(rootParameters);
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	// シリアライズ
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		Log(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}

	hr = dxCommon_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));
}

// ==========================================
// PSO生成 (main (2).cpp の設定値を反映)
// ==========================================
void ParticleManager::CreatePipelineState() {
	HRESULT hr;

	CreateRootSignature();

	// InputLayout (main (2).cpp の VertexData 構造体に合致させる)
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	// BlendState (main (2).cpp の kBlendModeNormal に相当する設定)
	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

	// RasterizerState (main (2).cpp と同じ設定)
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK; // main(2).cppではBACKになっている
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	// Shader Compile
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = dxCommon_->CompileShader(L"Resources/Shader/Particle.VS.hlsl", L"vs_6_0");
	assert(vertexShaderBlob != nullptr);
	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = dxCommon_->CompileShader(L"Resources/Shader/Particle.PS.hlsl", L"ps_6_0");
	assert(pixelShaderBlob != nullptr);

	// PSO設定
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.InputLayout = inputLayoutDesc;
	psoDesc.BlendState = blendDesc;
	psoDesc.RasterizerState = rasterizerDesc;
	psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
	psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };

	// DepthStencilState (main (2).cpp と同じ設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	psoDesc.DepthStencilState = depthStencilDesc;

	// 【重要】DSVフォーマット (main (2).cpp に合わせる)
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	for (int i = 0; i < kBlendCountblend; ++i) {
		// 共通の設定をベースにする
		D3D12_GRAPHICS_PIPELINE_STATE_DESC localDesc = psoDesc;

		// ブレンドの基本有効化設定
		localDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
		localDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		// アルファ（透明度）のブレンド計算式（通常は共通でOKです）
		localDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		localDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		localDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

		// ループインデックス（列挙型 BlendMode）に応じてブレンド式を切り替える
		switch (i) {
		case kBlendModeNone:
			localDesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			break;

		case kBlendModeNormal: // 通常ブレンド（半透明）
			localDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			localDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			localDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			break;

		case kBlendModeAdd: // 加算ブレンド（光らせる演出用）
			localDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			localDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			localDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			break;

		case kBlendModeSubtract: // 減算ブレンド（影や暗くする演出用）
			localDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			localDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			localDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT; // 反転して引く
			break;

		case kBlendModeMultiply: // 乗算ブレンド
			localDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_DEST_COLOR;
			localDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
			localDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			break;

		case kBlendModeScreen: // スクリーンブレンド
			localDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
			localDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			localDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			break;
		}

		// それぞれの設定でパイプラインを生成して配列に格納
		HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&localDesc, IID_PPV_ARGS(&graphicsPipelineStates[i]));
		assert(SUCCEEDED(hr));

		if (FAILED(hr)) {
			OutputDebugStringA("Error: Failed to create GraphicsPipelineState for Particle.\n");
			assert(false);
		}
	}
}

void ParticleManager::Draw(Camera* camera) {
	auto commandList = dxCommon_->GetCommandList();
	SrvManager::GetInstance()->PreDraw();

	commandList->SetGraphicsRootSignature(rootSignature.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	Matrix4x4 viewProjection = camera->GetViewProjectionMatrix();
	Matrix4x4 billboardMatrix = camera->GetWorldMatrix();
	billboardMatrix.m[3][0] = 0.0f;
	billboardMatrix.m[3][1] = 0.0f;
	billboardMatrix.m[3][2] = 0.0f;

	for (auto& [name, group] : particleGroups_) {
		if (group.particles.empty()) {
			continue;
		}
		commandList->IASetVertexBuffers(0, 1, &group.vbView);
		commandList->SetPipelineState(graphicsPipelineStates[group.blendMode].Get());
		uint32_t numInstance = std::min<uint32_t>(static_cast<uint32_t>(group.particles.size()), kMaxParticle);
		uint32_t index = 0;

		for (const auto& particle : group.particles) {
			if (index >= numInstance)
				break;

			Matrix4x4 scaleMatrix = MakeScaleMatrix(particle.transform.scale);
			Matrix4x4 rotateMatrix = MakeRotateXYZMatrix(particle.transform.rotate); // 回転行列
			Matrix4x4 translateMatrix = MakeTranslateMatrix(particle.transform.translate);

			// ─── ★追加：ビルボードON/OFFの切り替え ───
			Matrix4x4 finalRotateMatrix;
			if (particle.isBillboard) {
				// ビルボードON：パーティクルの回転にビルボード行列を合成する
				finalRotateMatrix = Multiply(rotateMatrix, billboardMatrix);
			}
			else {
				// ビルボードOFF：パーティクル自身の回転行列のみを使用する（3D空間に配置される）
				finalRotateMatrix = rotateMatrix;
			}

			// 合成した回転行列を使って worldMatrix を計算する
			Matrix4x4 worldMatrix = Multiply(scaleMatrix, Multiply(finalRotateMatrix, translateMatrix));
			Matrix4x4 wvp = Multiply(worldMatrix, viewProjection);

			group.instanceDataPtr[index].WVP = wvp;
			group.instanceDataPtr[index].world = worldMatrix;
			group.instanceDataPtr[index].color = particle.color;

			index++;
		}

		group.instanceCount = numInstance;

		if (group.instanceCount > 0) {
			// [0] Texture
			commandList->SetGraphicsRootDescriptorTable(0, srvManager_->GetGPUDescriptorHandle(group.material.textureIndex));
			// [1] Data
			commandList->SetGraphicsRootDescriptorTable(1, srvManager_->GetGPUDescriptorHandle(group.instanceSrvIndex));

			commandList->DrawInstanced(group.vertexCount, group.instanceCount, 0, 0);
		}
	}
}

// Update関数は提示していただいたままで問題ありません
void ParticleManager::Update() {
	for (auto& groupPair : particleGroups_) {
		ParticleGroup& group = groupPair.second;
		group.instanceCount = 0;

		for (auto it = group.particles.begin(); it != group.particles.end();) {
			if (it->currentTime >= it->lifeTime) {
				it = group.particles.erase(it);
				continue;
			}
			it->transform.translate.x += it->velocity.x;
			it->transform.translate.y += it->velocity.y;
			it->transform.translate.z += it->velocity.z;

			it->velocity.x += it->acceleration.x;
			it->velocity.y += it->acceleration.y;
			it->velocity.z += it->acceleration.z;

			float t = it->currentTime / it->lifeTime;
			if (t < 0.0f) {
				t = 0.0f;
			} else if (t > 1.0f) {
				t = 1.0f;
			}

			it->transform.scale.x = it->startScale.x + (it->endScale.x - it->startScale.x) * t;
			it->transform.scale.y = it->startScale.y + (it->endScale.y - it->startScale.y) * t;
			it->transform.scale.z = it->startScale.z + (it->endScale.z - it->startScale.z) * t;

			it->color.x = it->startColor.x + (it->endColor.x - it->startColor.x) * t;
			it->color.y = it->startColor.y + (it->endColor.y - it->startColor.y) * t;
			it->color.z = it->startColor.z + (it->endColor.z - it->startColor.z) * t;
			it->color.w = it->startColor.w + (it->endColor.w - it->startColor.w) * t;

			it->currentTime += 1.0f / 60.0f;
			++it;
		}
	}
}
void ParticleManager::Emit(const std::string& groupName, const Vector3& position, uint32_t count, const ParticleEmitParam& emitParam) {
	if (particleGroups_.find(groupName) == particleGroups_.end())
		return;

	ParticleGroup& group = particleGroups_[groupName];

	// -1.0 から 1.0 のランダムな数を作る分布
	std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

	for (uint32_t i = 0; i < count; ++i) {
		Particle newParticle;

		// 1. 位置: 基準位置 + (乱数 * 位置の範囲)
		newParticle.transform.translate.x = position.x + dist(randomEngine_) * emitParam.randomPositionRange.x;
		newParticle.transform.translate.y = position.y + dist(randomEngine_) * emitParam.randomPositionRange.y;
		newParticle.transform.translate.z = position.z + dist(randomEngine_) * emitParam.randomPositionRange.z;

		// 2. 大きさ: パラメータをそのまま設定
		newParticle.transform.scale.x = emitParam.scale.x + dist(randomEngine_) * emitParam.randomScaleRange.x;
		newParticle.transform.scale.y = emitParam.scale.y + dist(randomEngine_) * emitParam.randomScaleRange.y;
		newParticle.transform.scale.z = emitParam.scale.z + dist(randomEngine_) * emitParam.randomScaleRange.z;
		newParticle.startScale = newParticle.transform.scale;
		newParticle.endScale = emitParam.endScale;
		// 3. 速度: 基礎速度 + (乱数 * 速度の範囲)
		newParticle.velocity.x = emitParam.baseVelocity.x + dist(randomEngine_) * emitParam.randomVelocityRange.x;
		newParticle.velocity.y = emitParam.baseVelocity.y + dist(randomEngine_) * emitParam.randomVelocityRange.y;
		newParticle.velocity.z = emitParam.baseVelocity.z + dist(randomEngine_) * emitParam.randomVelocityRange.z;
		newParticle.acceleration = emitParam.acceleration;

		// フラグが真なら、基本角度に「乱数 × 範囲」を足す
		newParticle.transform.rotate.x = emitParam.baseRotate.x + dist(randomEngine_) * emitParam.randomRotateRange.x;
		newParticle.transform.rotate.y = emitParam.baseRotate.y + dist(randomEngine_) * emitParam.randomRotateRange.y;
		newParticle.transform.rotate.z = emitParam.baseRotate.z + dist(randomEngine_) * emitParam.randomRotateRange.z;

		// 4. 寿命
		newParticle.lifeTime = emitParam.lifeTime;
		newParticle.currentTime = 0.0f;

		newParticle.isBillboard = emitParam.isBillboard;

		// 色の設定（ここは必要に応じてお好みで変更してください）
		newParticle.startColor = emitParam.color;
		newParticle.endColor = emitParam.endColor;
		newParticle.color = newParticle.startColor;

		group.particles.push_back(newParticle);
	}
}
void ParticleManager::SetGroupTexture(const std::string& groupName, const std::string& textureFilePath) {
	// グループが存在するかチェック
	if (particleGroups_.find(groupName) == particleGroups_.end()) {
		return;
	}

	ParticleGroup& group = particleGroups_[groupName];

	// テクスチャをロードして、SRVインデックスを新しいものに更新する
	group.material.textureFilePath = textureFilePath;
	TextureManager::GetInstance()->LoadTexture(textureFilePath);
	group.material.textureIndex = TextureManager::GetInstance()->GetSrvIndex(textureFilePath);
}

void ParticleManager::SetGroupBlendMode(const std::string& groupName, BlendMode blendMode) {
	// 指定された名前のグループが存在するかチェックし、ブレンドモードを設定
	auto it = particleGroups_.find(groupName);
	if (it != particleGroups_.end()) {
		it->second.blendMode = blendMode;
	}
}

ParticleManager::ParticleGroup* ParticleManager::GetGroup(const std::string& groupName) {
	auto it = particleGroups_.find(groupName);
	if (it != particleGroups_.end()) {
		return &it->second;
	}
	return nullptr;
}
