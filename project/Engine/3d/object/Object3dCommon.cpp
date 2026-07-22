#include "Object3dCommon.h"
#include "PipelineStateUtility.h"
#include <cstddef>

using namespace Logger;

/// <summary>
/// 共有インスタンスを取得します。
/// </summary>
Object3dCommon* Object3dCommon::GetInstance() {
	static Object3dCommon instance;
	return &instance;
}

/// <summary>
/// 必要なリソースを準備し、オブジェクトを初期化します。
/// </summary>
/// <param name="dxCommon">DirectX 共通処理へアクセスするための参照を指定します。</param>
void Object3dCommon::Initialize(DirectXCommon* dxCommon) {
	assert(dxCommon);
	this->dxCommon_ = dxCommon;

	CreatePipelineState();
}

void Object3dCommon::EnsureInitialized(DirectXCommon* dxCommon) {
	assert(dxCommon);
	if (dxCommon_ == dxCommon && rootSignature && graphicsPipelineState && shadowPipelineState) {
		return;
	}
	Initialize(dxCommon);
}

void Object3dCommon::SetDraw() {
	dxCommon_->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());
	dxCommon_->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());
	dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Object3dCommon::SetShadowDraw() {
	dxCommon_->GetCommandList()->SetPipelineState(shadowPipelineState.Get());
	dxCommon_->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());
	dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

/// <summary>
/// 確保したリソースを解放し、終了処理を行います。
/// </summary>
void Object3dCommon::Finalize() {
	rootSignature.Reset();
	graphicsPipelineState.Reset();
	shadowPipelineState.Reset();
	dxCommon_ = nullptr;
	defaultCamera = nullptr;
}

/// <summary>
/// RootSignature を作成し、利用できる状態にします。
/// </summary>
void Object3dCommon::CreateRootSignature() {
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE descriptorRangeEnvMap[1] = {};
	descriptorRangeEnvMap[0].BaseShaderRegister = 1;
	descriptorRangeEnvMap[0].NumDescriptors = 1;
	descriptorRangeEnvMap[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRangeEnvMap[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParameters[8] = {};

	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].Descriptor.ShaderRegister = 0;

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[1].Descriptor.ShaderRegister = 1;

	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[2].Descriptor.ShaderRegister = 2;

	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[3].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[3].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

	rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[4].Descriptor.ShaderRegister = 3; // b3

	rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[5].Descriptor.ShaderRegister = 4; // b4

	rootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[6].DescriptorTable.pDescriptorRanges = descriptorRangeEnvMap;
	rootParameters[6].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeEnvMap);

	rootParameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParameters[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[7].Descriptor.ShaderRegister = 2;

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

	rootSignature = PipelineStateUtility::CreateRootSignature(dxCommon_->GetDevice().Get(), descriptionRootSignature);
}
/// <summary>
/// PipelineState を作成し、利用できる状態にします。
/// </summary>
void Object3dCommon::CreatePipelineState() {
	HRESULT hr;
	CreateRootSignature();

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[5] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = static_cast<UINT>(offsetof(::VertexData, position));

	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = static_cast<UINT>(offsetof(::VertexData, texcoord));

	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = static_cast<UINT>(offsetof(::VertexData, normal));

	inputElementDescs[3].SemanticName = "BONEINDEX";
	inputElementDescs[3].Format = DXGI_FORMAT_R32G32B32A32_UINT;
	inputElementDescs[3].AlignedByteOffset = static_cast<UINT>(offsetof(::VertexData, boneIndices));

	inputElementDescs[4].SemanticName = "BONEWEIGHT";
	inputElementDescs[4].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[4].AlignedByteOffset = static_cast<UINT>(offsetof(::VertexData, boneWeights));

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	const D3D12_BLEND_DESC blendDesc = PipelineStateUtility::MakeBlendDesc();
	const D3D12_BLEND_DESC shadowBlendDesc = PipelineStateUtility::MakeBlendDesc(
	    TRUE, D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA);
	const D3D12_RASTERIZER_DESC rasterizerDesc = PipelineStateUtility::MakeRasterizerDesc(D3D12_CULL_MODE_NONE);

	auto vertexShaderBlob = dxCommon_->CompileShader(L"Resources/Shader/Object3d.VS.hlsl", L"vs_6_0");
	auto pixelShaderBlob = dxCommon_->CompileShader(L"Resources/Shader/Object3d.PS.hlsl", L"ps_6_0");
	assert(vertexShaderBlob != nullptr);
	assert(pixelShaderBlob != nullptr);

	// DepthStencilState
	const D3D12_DEPTH_STENCIL_DESC depthStencilDesc =
	    PipelineStateUtility::MakeDepthStencilDesc(TRUE, D3D12_DEPTH_WRITE_MASK_ALL);

	// PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.InputLayout = inputLayoutDesc;
	psoDesc.BlendState = blendDesc;
	psoDesc.RasterizerState = rasterizerDesc;
	psoDesc.VS = {vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize()};
	psoDesc.PS = {pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize()};
	psoDesc.DepthStencilState = depthStencilDesc;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowPsoDesc = psoDesc;
	D3D12_DEPTH_STENCIL_DESC shadowDepthStencilDesc = depthStencilDesc;
	shadowDepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	shadowPsoDesc.BlendState = shadowBlendDesc;
	shadowPsoDesc.DepthStencilState = shadowDepthStencilDesc;
	hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&shadowPsoDesc, IID_PPV_ARGS(&shadowPipelineState));
	assert(SUCCEEDED(hr));
}
