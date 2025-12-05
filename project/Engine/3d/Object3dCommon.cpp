#include "Object3dCommon.h"

using namespace Logger;

void Object3dCommon::Initialize(DirectXCommon* dxCommon) {
	this->dxCommon_ = dxCommon;
	CreatePipelineState();
};

void Object3dCommon::SetDraw() {
	// パイプラインステートの設定
	dxCommon_->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());
	// ルートシグネチャの設定
	dxCommon_->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());
	// プリミティブ形状の設定（三角形リスト）
	dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Object3dCommon::CreateRootSignature() {
	HRESULT hr;
	//	assert(SUCCEEDED(hr));
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// RootSignatureの設定
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; // 入力アセンブラーでの使用を許可

	// RootParameterの設定。複数設定できるので配列、今回は結果1つだけなので長さ1の配列
	D3D12_ROOT_PARAMETER rootParameters[4] = {};
	// ルートパラメーターの設定
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;     // ルートパラメーターのタイプ（CBV）
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  // シェーダーの可視性（ピクセルシェーダー）
	rootParameters[0].Descriptor.ShaderRegister = 0;                     // シェーダーレジスタのインデックス
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;     // ルートパラメーターのタイプ（CBV）
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // シェーダーの可視性（バーテックスシェーダー）
	rootParameters[1].Descriptor.ShaderRegister = 1;                     // シェーダーレジスタのインデックス
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;     // ルートパラメーターのタイプ（CBV）
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  // シェーダーの可視性（ピクセルシェーダー）
	rootParameters[2].Descriptor.ShaderRegister = 2;                     // シェーダーレジスタのインデックス
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[3].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[3].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);
	descriptionRootSignature.pParameters = rootParameters;             // ルートパラメーターの配列
	descriptionRootSignature.NumParameters = _countof(rootParameters); // ルートパラメーターの数

	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(
	    &descriptionRootSignature,    // ルートシグネチャの説明
	    D3D_ROOT_SIGNATURE_VERSION_1, // バージョン
	    &signatureBlob,               // シリアライズされたバイナリ
	    &errorBlob                    // エラー情報
	);
	if (FAILED(hr)) {
		Log(reinterpret_cast<const char*>(errorBlob->GetBufferPointer())); // エラー内容をLogに出力
		assert(false);                                                     // シリアライズが失敗した場合はアサート
	}
	// バイナリをもとにルートシグネチャを生成
	hr = dxCommon_->GetDevice()->CreateRootSignature(
	    0,                                 // シグネチャのバージョン
	    signatureBlob->GetBufferPointer(), // シリアライズされたバイナリのポインタ
	    signatureBlob->GetBufferSize(),    // バイナリのサイズ
	    IID_PPV_ARGS(&rootSignature)       // 生成したルートシグネチャを受け取る
	);
	assert(SUCCEEDED(hr)); // ルートシグネチャの生成が成功したか確認
}

void Object3dCommon::CreatePipelineState() {
	HRESULT hr;
	CreateRootSignature();
	// InputLayoutの設定
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0].SemanticName = "POSITION";                        // セマンティック名
	inputElementDescs[0].SemanticIndex = 0;                                // セマンティックインデックス
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;          // フォーマット
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT; // アライメントオフセット
	inputElementDescs[1].SemanticName = "TEXCOORD";                        // セマンティック名
	inputElementDescs[1].SemanticIndex = 0;                                // セマンティックインデックス
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;                // フォーマット
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT; // アライメントオフセット
	inputElementDescs[2].SemanticName = "NORMAL";                          // セマンティック名
	inputElementDescs[2].SemanticIndex = 0;                                // セマンティックインデックス
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;             // フォーマット
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT; // アライメントオフセット
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;    // 入力要素の配列
	inputLayoutDesc.NumElements = _countof(inputElementDescs); // 入力要素の数

	// BlendStateの設定
	D3D12_BLEND_DESC blendDesc{};
	// すべての色要素を書き込む
	BlendMode blendMode = BlendMode::kBlendModeMultiply;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].BlendEnable = TRUE; // ブレンドを無効にする

	switch (blendMode) {
	case BlendMode::kBlendModeNormal:
		// すべての色要素を書き込む
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;      // ソースのブレンドファクター
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;          // ブレンドの演算
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA; // デスティネーションのブレンドファクター
		break;
	case BlendMode::kBlendModeAdd:
		// すべての色要素を書き込む
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA; // ソースのブレンドファクター
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;     // ブレンドの演算
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;      // デスティネーションのブレンドファクター
		break;
	case BlendMode::kBlendModeSubtract:
		// すべての色要素を書き込む
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;      // ソースのブレンドファクター
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT; // ブレンドの演算
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;           // デスティネーションのブレンドファクター
		break;
	case BlendMode::kBlendModeMultiply:
		// すべての色要素を書き込む
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;  // ソースのブレンドファクター
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;      // ブレンドの演算
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR; // デスティネーションのブレンドファクター
		break;
	};

	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;   // デスティネーションのアルファブレンドファクター
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD; // アルファブレンドの演算
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO; // デスティネーションのアルファブレンドファクター
	// RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	// 裏面（時計回り）を表示しない
	rasterizerDesc.CullMode =D3D12_CULL_MODE_BACK; // 裏面をカリング
	// 中身を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID; // 塗りつぶしモード

	// Shaderのコンパイル
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = dxCommon_->CompileShader(L"Resources/Shader/Object3d.VS.hlsl", L"vs_6_0");
	assert(vertexShaderBlob != nullptr); // Vertex Shaderのコンパイルが成功したか確認
	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = dxCommon_->CompileShader(L"Resources/Shader/Object3d.PS.hlsl", L"ps_6_0");
	assert(pixelShaderBlob != nullptr); // Pixel Shaderのコンパイルが成功したか確認

	D3D12_DEPTH_STENCIL_DESC depthStenecilDesc{};
	depthStenecilDesc.DepthEnable = true;
	depthStenecilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStenecilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// PSOの設定
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();                                           // ルートシグネチャ
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;                                                  // 入力レイアウト
	graphicsPipelineStateDesc.BlendState = blendDesc;                                                         // ブレンドステート
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;                                               // ラスタライザーステート
	graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize()}; // Vertex Shader
	graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize()};   // Pixel Shader
	graphicsPipelineStateDesc.DepthStencilState = depthStenecilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	// 書き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1;                            // レンダーターゲットの数
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // レンダーターゲットのフォーマット
	// 利用するトポロジ（形状）のタイプ。三角形
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // トポロジのタイプ
	// どのように画面に色を打ち込むかの設定
	graphicsPipelineStateDesc.SampleDesc.Count = 1;                   // マルチサンプルの数
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; // サンプルマスク
	// 実際に生成
	hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr)); // パイプラインステートの生成が成功したか確認
}
