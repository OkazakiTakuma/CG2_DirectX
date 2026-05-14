#include "SkyBoxCommon.h"
#include <cassert>

SkyBoxCommon* SkyBoxCommon::GetInstance() {
	static SkyBoxCommon instance;
	return &instance;
}

void SkyBoxCommon::Initialize(DirectXCommon* dxCommon) {
	assert(dxCommon);
	dxCommon_ = dxCommon;

	CreateRootSignature();
	CreatePipelineState();
}

void SkyBoxCommon::SetDraw() {
	auto commandList = dxCommon_->GetCommandList();
	commandList->SetPipelineState(graphicsPipelineState.Get());
	commandList->SetGraphicsRootSignature(rootSignature.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void SkyBoxCommon::Finalize() {
	rootSignature.Reset();
	graphicsPipelineState.Reset();
}

void SkyBoxCommon::CreateRootSignature() {
	HRESULT hr;

	// デスクリプタレンジ (t0: テクスチャキューブ用)
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0; // t0
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ルートパラメータの構成（3つに減らしました）
	D3D12_ROOT_PARAMETER rootParameters[3] = {};

	// 0: マテリアル (b0) - ピクセルシェーダー用
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].Descriptor.ShaderRegister = 0;

	// 1: 変換行列 (b1) - 頂点シェーダー用
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[1].Descriptor.ShaderRegister = 1;

	// 2: テクスチャ (t0) - ピクセルシェーダー用
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

	// スタティックサンプラー (s0)
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // 線形補間
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0; // s0
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// ルートシグネチャの生成
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = rootParameters;
	rootSignatureDesc.NumParameters = _countof(rootParameters);
	rootSignatureDesc.pStaticSamplers = staticSamplers;
	rootSignatureDesc.NumStaticSamplers = _countof(staticSamplers);

	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
	hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signatureBlob, &errorBlob);
	assert(SUCCEEDED(hr));
	hr = dxCommon_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));
}

void SkyBoxCommon::CreatePipelineState() {
	HRESULT hr;
	CreateRootSignature(); // 先にルートシグネチャを作成

	// ★重要: シェーダーに合わせて POSITION, TEXCOORD, NORMAL の3つを定義
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
	    {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	// ★SkyBox特有の設定 (内側を描画するためFRONTをカリング)
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.CullMode = D3D12_CULL_MODE_FRONT;
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	// ★SkyBox特有の設定 (深度書き込みを無効化)
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // 書き込まない
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL; // 判定は通常通り

	// ブレンドステート
	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// シェーダーのコンパイル
	auto vertexShaderBlob = dxCommon_->CompileShader(L"Resources/Shader/SkyBox.VS.hlsl", L"vs_6_0");
	auto pixelShaderBlob = dxCommon_->CompileShader(L"Resources/Shader/SkyBox.PS.hlsl", L"ps_6_0");
	assert(vertexShaderBlob != nullptr);
	assert(pixelShaderBlob != nullptr);

	// PSOの生成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.InputLayout = inputLayoutDesc;
	psoDesc.VS = {vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize()};
	psoDesc.PS = {pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize()};
	psoDesc.BlendState = blendDesc;
	psoDesc.RasterizerState = rasterizerDesc;
	psoDesc.DepthStencilState = depthStencilDesc;

	// 基本的な構成設定
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr));
}