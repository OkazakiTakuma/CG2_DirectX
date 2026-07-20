#include "SkyBoxCommon.h"
#include "PipelineStateUtility.h"
#include <cassert>

/// <summary>
/// 共有インスタンスを取得します。
/// </summary>
/// <returns>処理結果を返します。</returns>
SkyBoxCommon* SkyBoxCommon::GetInstance() {
	static SkyBoxCommon instance;
	return &instance;
}

/// <summary>
/// 必要なリソースを準備し、オブジェクトを初期化します。
/// </summary>
/// <param name="dxCommon">DirectX 共通処理へアクセスするための参照を指定します。</param>
void SkyBoxCommon::Initialize(DirectXCommon* dxCommon) {
	assert(dxCommon);
	dxCommon_ = dxCommon;

	CreateRootSignature();
	CreatePipelineState();
}

/// <summary>
/// Draw を設定します。
/// </summary>
void SkyBoxCommon::SetDraw() {
	auto commandList = dxCommon_->GetCommandList();

	commandList->SetPipelineState(graphicsPipelineState.Get());
	commandList->SetGraphicsRootSignature(rootSignature.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

/// <summary>
/// 確保したリソースを解放し、終了処理を行います。
/// </summary>
void SkyBoxCommon::Finalize() {
	rootSignature.Reset();
	graphicsPipelineState.Reset();
}

/// <summary>
/// RootSignature を作成し、利用できる状態にします。
/// </summary>
void SkyBoxCommon::CreateRootSignature() {
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0; // t0
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParameters[3] = {};

	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].Descriptor.ShaderRegister = 0;

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[1].Descriptor.ShaderRegister = 1;

	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0; // s0
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = rootParameters;
	rootSignatureDesc.NumParameters = _countof(rootParameters);
	rootSignatureDesc.pStaticSamplers = staticSamplers;
	rootSignatureDesc.NumStaticSamplers = _countof(staticSamplers);

	rootSignature = PipelineStateUtility::CreateRootSignature(
	    dxCommon_->GetDevice().Get(), rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0);
}

/// <summary>
/// PipelineState を作成し、利用できる状態にします。
/// </summary>
void SkyBoxCommon::CreatePipelineState() {
	HRESULT hr;
	CreateRootSignature();

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
	    {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	const D3D12_RASTERIZER_DESC rasterizerDesc = PipelineStateUtility::MakeRasterizerDesc(D3D12_CULL_MODE_FRONT);
	const D3D12_DEPTH_STENCIL_DESC depthStencilDesc =
	    PipelineStateUtility::MakeDepthStencilDesc(TRUE, D3D12_DEPTH_WRITE_MASK_ZERO);
	const D3D12_BLEND_DESC blendDesc = PipelineStateUtility::MakeBlendDesc();

	auto vertexShaderBlob = dxCommon_->CompileShader(L"Resources/Shader/SkyBox.VS.hlsl", L"vs_6_0");
	auto pixelShaderBlob = dxCommon_->CompileShader(L"Resources/Shader/SkyBox.PS.hlsl", L"ps_6_0");
	assert(vertexShaderBlob != nullptr);
	assert(pixelShaderBlob != nullptr);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.InputLayout = inputLayoutDesc;
	psoDesc.VS = {vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize()};
	psoDesc.PS = {pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize()};
	psoDesc.BlendState = blendDesc;
	psoDesc.RasterizerState = rasterizerDesc;
	psoDesc.DepthStencilState = depthStencilDesc;

	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr));
}
