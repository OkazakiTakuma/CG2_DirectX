#include "ParticleManager.h"
#include <cassert>
using namespace Logger;

void ParticleManager::Initialize(DirectXCommon* dxCommon, SrvManager* srv) {
	dxCommon_ = dxCommon;
	srvManager = srv;
	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());
	CreatePipelineState();
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
	Microsoft::WRL::ComPtr<ID3D12RootSignature> instancingrootSignature = nullptr;
	hr = dxCommon_->GetDevice()->CreateRootSignature(
	    0,                                           // シグネチャのバージョン
	    instancingsignatureBlob->GetBufferPointer(), // シリアライズされたバイナリのポインタ
	    instancingsignatureBlob->GetBufferSize(),    // バイナリのサイズ
	    IID_PPV_ARGS(&instancingrootSignature)       // 生成したルートシグネチャを受け取る
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
	instancinggraphicsPipelineStateDesc.pRootSignature = rootSignature.Get();                                                     // ルートシグネチャ
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
