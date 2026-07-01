#include "LineCommon.h"
#include <cassert>

LineCommon* LineCommon::GetInstance() {
    static LineCommon instance;
    return &instance;
}

void LineCommon::Initialize(DirectXCommon* dxCommon) {
    assert(dxCommon);
    dxCommon_ = dxCommon;

    CreateRootSignature();
    CreatePipelineState();
}

void LineCommon::SetDraw(uint32_t blendMode) {
    // 謖・ｮ壹＆繧後◆繝悶Ξ繝ｳ繝峨Δ繝ｼ繝峨・PSO繧偵そ繝・ヨ
    dxCommon_->GetCommandList()->SetPipelineState(graphicsPipelineStates[blendMode].Get());
    // 繝ｫ繝ｼ繝医す繧ｰ繝阪メ繝｣縺ｮ險ｭ螳・
    dxCommon_->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());
    // 笘・・繝ｪ繝溘ユ繧｣繝門ｽ｢迥ｶ縺ｮ險ｭ螳夲ｼ医Λ繧､繝ｳ繝ｪ繧ｹ繝茨ｼ・
    dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
}

void LineCommon::Finalize() {
    rootSignature.Reset();
    for (auto& pso : graphicsPipelineStates) {
        pso.Reset();
    }
    dxCommon_ = nullptr;
}

void LineCommon::CreateRootSignature() {
    // 邱壹・謠冗判縺ｫ縺ｯ繧ｫ繝｡繝ｩ縺ｮ陦悟・(b0)縺縺代′蠢・ｦ√↑縺ｮ縺ｧ縲・縺､縺縺醍畑諢・
    D3D12_ROOT_PARAMETER rootParam{};
    rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParam.Descriptor.ShaderRegister = 0; // b0
    rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_ROOT_SIGNATURE_DESC rsDesc{};
    rsDesc.NumParameters = 1;
    rsDesc.pParameters = &rootParam;
    rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    dxCommon_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
}

void LineCommon::CreatePipelineState() {
    // 笘・＃閾ｪ霄ｫ縺ｮ迺ｰ蠅・・繧ｳ繝ｳ繝代う繝ｫ髢｢謨ｰ・・xCommon_->CompileShader縺ｪ縺ｩ・峨↓鄂ｮ縺肴鋤縺医※縺上□縺輔＞
     auto vertexShaderBlob = dxCommon_->CompileShader(L"Resources/Shader/Line.VS.hlsl", L"vs_6_0");
     auto pixelShaderBlob = dxCommon_->CompileShader(L"Resources/Shader/Line.PS.hlsl", L"ps_6_0");
     assert(vertexShaderBlob != nullptr);
     assert(pixelShaderBlob != nullptr);

    D3D12_INPUT_ELEMENT_DESC inputElements[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    // BlendState縺ｮ險ｭ螳・(Multiply繧偵ョ繝輔か繝ｫ繝医→縺励※險ｭ螳壹＆繧後※縺・◆繧ゅ・繧堤ｶｭ謖・
   
    // RasterizerState縺ｮ險ｭ螳・
    D3D12_RASTERIZER_DESC rasterizerDesc{};
    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK; // 陬城擇繧ｫ繝ｪ繝ｳ繧ｰ
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = rootSignature.Get();
    psoDesc.InputLayout = { inputElements, _countof(inputElements) };
    psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
    psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.RasterizerState = rasterizerDesc;
    psoDesc.DepthStencilState = depthStencilDesc;

    // 笘・㍾隕・ｼ啜opologyType 繧・LINE 縺ｫ縺吶ｋ
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    // SpriteCommon 縺ｨ蜷後§繧医≧縺ｫ繝悶Ξ繝ｳ繝峨Δ繝ｼ繝峨＃縺ｨ縺ｫPSO繧剃ｽ懈・
    for (uint32_t i = 0; i < kCountOfBlendMode; ++i) {
        D3D12_BLEND_DESC blendDesc{};
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

        switch (i) {
        case kBlendModeNone:
            blendDesc.RenderTarget[0].BlendEnable = FALSE;
            break;
        case kBlendModeNormal:
            blendDesc.RenderTarget[0].BlendEnable = TRUE;
            blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
            blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            break;
        case kBlendModeAdd:
            blendDesc.RenderTarget[0].BlendEnable = TRUE;
            blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
            blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
            break;
        case kBlendModeSubtract:
            blendDesc.RenderTarget[0].BlendEnable = TRUE;
            blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
            blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
            blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
            break;
        case kBlendModeMultiply:
            blendDesc.RenderTarget[0].BlendEnable = TRUE;
            blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
            blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR;
            break;
        case kBlendModeScreen:
            blendDesc.RenderTarget[0].BlendEnable = TRUE;
            blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
            blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
            break;
        }
        psoDesc.BlendState = blendDesc;
        dxCommon_->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&graphicsPipelineStates[i]));
    }
}
