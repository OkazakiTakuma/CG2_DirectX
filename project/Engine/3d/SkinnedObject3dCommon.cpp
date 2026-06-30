#include "SkinnedObject3dCommon.h"
#include <cassert>

SkinnedObject3dCommon* SkinnedObject3dCommon::GetInstance() {
	static SkinnedObject3dCommon instance;
	return &instance;
}

void SkinnedObject3dCommon::Initialize(DirectXCommon* dxCommon) {
	assert(dxCommon);
	this->dxCommon_ = dxCommon;
	CreatePipelineState();
}

void SkinnedObject3dCommon::SetDraw() {
	dxCommon_->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());
	dxCommon_->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());
	dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void SkinnedObject3dCommon::Finalize() {
	rootSignature.Reset();
	graphicsPipelineState.Reset();
	dxCommon_ = nullptr;
	defaultCamera = nullptr;
}

void SkinnedObject3dCommon::CreateRootSignature() {
	// ★Object3dCommonと全く同じなので、Object3dCommon::CreateRootSignature()の
	// 中身を丸ごとコピーしてきてください！

	// ただし、ルートパラメータの数を「8」に変更し、最後に以下を追加します。
	/*
	rootParameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[7].Descriptor.ShaderRegister = 2; // b2
	*/
}

void SkinnedObject3dCommon::CreatePipelineState() {
	CreateRootSignature();

	// ★入力レイアウトにボーン（BLENDINDICES / BLENDWEIGHT）を追加
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	// ★重要：VSは新しく作ったSkinnedObject3d.VS.hlsl、PSは既存のObject3d.PS.hlslを使います
	auto vertexShaderBlob = dxCommon_->CompileShader(L"Resources/Shader/SkinnedObject3d.VS.hlsl", L"vs_6_0");
	auto pixelShaderBlob = dxCommon_->CompileShader(L"Resources/Shader/Object3d.PS.hlsl", L"ps_6_0");

	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.InputLayout = inputLayoutDesc;
	psoDesc.BlendState = blendDesc;
	psoDesc.RasterizerState = rasterizerDesc;
	psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
	psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
	psoDesc.DepthStencilState = depthStencilDesc;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	psoDesc.SampleDesc.Count = 1;

	dxCommon_->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&graphicsPipelineState));
}