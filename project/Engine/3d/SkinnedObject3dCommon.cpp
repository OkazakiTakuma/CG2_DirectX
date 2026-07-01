#include "SkinnedObject3dCommon.h"
#include <cassert>
#include "../base/SrvManager.h"
using namespace Logger;

SkinnedObject3dCommon* SkinnedObject3dCommon::GetInstance() {
	static SkinnedObject3dCommon instance;
	return &instance;
}

void SkinnedObject3dCommon::DispatchSkinning(ID3D12GraphicsCommandList* commandList, ID3D12Resource* inStructuredBuffer, ID3D12Resource* outBuffer, D3D12_GPU_VIRTUAL_ADDRESS boneBufferAddress, UINT vertexCount, D3D12_GPU_DESCRIPTOR_HANDLE srvHandle, D3D12_GPU_DESCRIPTOR_HANDLE uavHandle) {
	if (!commandList || !computePipelineState || !computeRootSignature) return;

	// Set compute pipeline and root signature
	commandList->SetPipelineState(computePipelineState.Get());
	commandList->SetComputeRootSignature(computeRootSignature.Get());

	// Set descriptor heaps (SRV/UAV) from global SrvManager
	ID3D12DescriptorHeap* heaps[] = { SrvManager::GetInstance()->GetDescriptorHeap().Get() };
	commandList->SetDescriptorHeaps(1, heaps);

	// Bind SRV/UAV descriptor tables (root 0 = SRV table, root 2 = UAV table)
	commandList->SetComputeRootDescriptorTable(0, srvHandle);
	commandList->SetComputeRootDescriptorTable(2, uavHandle);

	// CBV for bones: set directly as root CBV at slot 1
	commandList->SetComputeRootConstantBufferView(1, boneBufferAddress);

	// Dispatch
	const UINT threadGroupSize = 64;
	UINT dispatchCount = (vertexCount + threadGroupSize - 1) / threadGroupSize;
	commandList->Dispatch(dispatchCount, 1, 1);

	// After dispatch, caller should insert a UAV->VERTEX_AND_CONSTANT_BUFFER barrier before using the output as VB.
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
	HRESULT hr;

	// 既存のテクスチャ(t0)用デスクリプタレンジ
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ★追加：環境マップ(t1)用デスクリプタレンジ
	D3D12_DESCRIPTOR_RANGE descriptorRangeEnvMap[1] = {};
	descriptorRangeEnvMap[0].BaseShaderRegister = 1; // t1レジスタ
	descriptorRangeEnvMap[0].NumDescriptors = 1;
	descriptorRangeEnvMap[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRangeEnvMap[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ルートパラメータの数を必要最小限にする（ボーンCBVはCompute側で扱うためグラフィックス側には含めない）
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

	rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // カメラ用CBV
	rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[4].Descriptor.ShaderRegister = 3; // b3

	rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[5].Descriptor.ShaderRegister = 4; // b4

	// ★追加：環境マップ用のルートパラメータ（インデックス6）
	rootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[6].DescriptorTable.pDescriptorRanges = descriptorRangeEnvMap;
	rootParameters[6].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeEnvMap);

	// 保持: rootParameters[7] は CreateRootSignature で使わないが後方互換のため領域を残す
	rootParameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	// スタティックサンプラー（変更なし）
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;



	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	// その後でルートシグネチャ記述を作る
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumParameters = _countof(rootParameters); // 8 のままでOK
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		Log(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}

	hr = dxCommon_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));	rootParameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	
}

void SkinnedObject3dCommon::CreatePipelineState() {
	CreateRootSignature();

	// ★入力レイアウトにボーン（BLENDINDICES / BLENDWEIGHT）を追加
	// position を float3 に変更して頂点サイズを削減
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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
	// Swap chain のフォーマットに合わせる
	psoDesc.RTVFormats[0] = dxCommon_->GetSwapChainFormat();
	psoDesc.SampleDesc.Count = 1;
	dxCommon_->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&graphicsPipelineState)); // create graphics PSO

	// Compute pipeline 作成（SkinnedSkinComputeCS.hlsl）
	auto computeBlob = dxCommon_->CompileShader(L"Resources/Shader/SkinnedSkinComputeCS.hlsl", L"cs_6_0");
	// シンプルなルートシグネチャを作る: t0 (SRV), b2 (CBV bones), u0 (UAV)
	D3D12_ROOT_PARAMETER computeParams[3] = {};
	// SRV table t0
	D3D12_DESCRIPTOR_RANGE srvRange{};
	srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvRange.NumDescriptors = 1;
	srvRange.BaseShaderRegister = 0;
	srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	computeParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	computeParams[0].DescriptorTable.pDescriptorRanges = &srvRange;
	computeParams[0].DescriptorTable.NumDescriptorRanges = 1;

	// CBV bones b2
	computeParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	computeParams[1].Descriptor.ShaderRegister = 2;

	// UAV table u0
	D3D12_DESCRIPTOR_RANGE uavRange{};
	uavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	uavRange.NumDescriptors = 1;
	uavRange.BaseShaderRegister = 0;
	uavRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	computeParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	computeParams[2].DescriptorTable.pDescriptorRanges = &uavRange;
	computeParams[2].DescriptorTable.NumDescriptorRanges = 1;

	D3D12_ROOT_SIGNATURE_DESC computeRootDesc{};
	computeRootDesc.NumParameters = _countof(computeParams);
	computeRootDesc.pParameters = computeParams;
	computeRootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	ID3DBlob* sigBlob = nullptr;
	ID3DBlob* errBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&computeRootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
	assert(SUCCEEDED(hr));
	hr = dxCommon_->GetDevice()->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&computeRootSignature));
	assert(SUCCEEDED(hr));

	D3D12_COMPUTE_PIPELINE_STATE_DESC cpsd{};
	cpsd.pRootSignature = computeRootSignature.Get();
	cpsd.CS = { computeBlob->GetBufferPointer(), computeBlob->GetBufferSize() };
	hr = dxCommon_->GetDevice()->CreateComputePipelineState(&cpsd, IID_PPV_ARGS(&computePipelineState));
	assert(SUCCEEDED(hr));
}