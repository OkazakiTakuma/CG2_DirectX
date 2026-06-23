#include "InstancingModelCommon.h"
#include <cassert>

using namespace Logger;

InstancingModelCommon* InstancingModelCommon::GetInstance() {
    static InstancingModelCommon instance;
    return &instance;
}

void InstancingModelCommon::Initialize(DirectXCommon* dxCommon) {
    assert(dxCommon);
    dxCommon_ = dxCommon;

    CreateRootSignature();
    CreatePipelineState();
}

void InstancingModelCommon::SetDraw() {
    auto commandList = dxCommon_->GetCommandList();
    commandList->SetPipelineState(pipelineState_.Get());
    commandList->SetGraphicsRootSignature(rootSignature_.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void InstancingModelCommon::CreateRootSignature() {
    HRESULT hr;

    // 1. デスクリプタレンジの設定
    // t0: テクスチャ用
    D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
    descriptorRange[0].BaseShaderRegister = 0;
    descriptorRange[0].NumDescriptors = 1;
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // t1: 環境マップ用
    D3D12_DESCRIPTOR_RANGE descriptorRangeEnvMap[1] = {};
    descriptorRangeEnvMap[0].BaseShaderRegister = 1;
    descriptorRangeEnvMap[0].NumDescriptors = 1;
    descriptorRangeEnvMap[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRangeEnvMap[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // 2. ルートパラメータの設定
    // ※ ここで b0, b2, b3, b4 などの定数バッファと、t0, t1, t2 の設定を行います。
    D3D12_ROOT_PARAMETER rootParameters[7] = {};

    // [0] b0: Material
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[0].Descriptor.ShaderRegister = 0;

    // [1] t0: Texture (テクスチャ)
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRange;
    rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

    // ★重要！ [2] t2: Instancing Data (StructuredBuffer)
    // 前回 `InstancingModel::Draw` の `SetGraphicsRootShaderResourceView(2, ...)` で指定した番号です。
    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV; // SRVを直接渡す
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // 頂点シェーダーで使用
    rootParameters[2].Descriptor.ShaderRegister = 2; // t2 レジスタ

    // [3] b2: DirectionalLight
    rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[3].Descriptor.ShaderRegister = 2;

    // [4] b3: CameraInfo
    rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[4].Descriptor.ShaderRegister = 3;

    // [5] b4: PointLight
    rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[5].Descriptor.ShaderRegister = 4;

    // [6] t1: EnvironmentMap (環境マップ)
    rootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[6].DescriptorTable.pDescriptorRanges = descriptorRangeEnvMap;
    rootParameters[6].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeEnvMap);

    // 3. サンプラーの設定 (s0)
    D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[0].ShaderRegister = 0;
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // 4. ルートシグネチャの生成
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.NumParameters = _countof(rootParameters);
    rootSignatureDesc.pStaticSamplers = staticSamplers;
    rootSignatureDesc.NumStaticSamplers = _countof(staticSamplers);

    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
        assert(false);
    }

    hr = dxCommon_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
    assert(SUCCEEDED(hr));
}

void InstancingModelCommon::CreatePipelineState() {
    HRESULT hr;

    // ★重要：コンパイルする頂点シェーダーを今回新しく作った Instancing.VS.hlsl に変更します！
    auto vertexShaderBlob = dxCommon_->CompileShader(L"Resources/Shader/Instancing.VS.hlsl", L"vs_6_0");
    // ピクセルシェーダーは既存のものをそのまま使います
    auto pixelShaderBlob = dxCommon_->CompileShader(L"Resources/Shader/Object3d.PS.hlsl", L"ps_6_0");

    assert(vertexShaderBlob != nullptr);
    assert(pixelShaderBlob != nullptr);

    // InputLayout
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
    inputElementDescs[0].SemanticName = "POSITION";
    inputElementDescs[0].SemanticIndex = 0;
    inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    inputElementDescs[1].SemanticName = "TEXCOORD";
    inputElementDescs[1].SemanticIndex = 0;
    inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    inputElementDescs[2].SemanticName = "NORMAL";
    inputElementDescs[2].SemanticIndex = 0;
    inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
    inputLayoutDesc.pInputElementDescs = inputElementDescs;
    inputLayoutDesc.NumElements = _countof(inputElementDescs);

    // BlendState
    D3D12_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    // RasterizerState
    D3D12_RASTERIZER_DESC rasterizerDesc{};
    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

    // DepthStencilState
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    // PSO
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = rootSignature_.Get();
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

    hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState_));
    assert(SUCCEEDED(hr));
}