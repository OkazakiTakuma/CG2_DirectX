#include "Engine/2d/Sprite.h"
#include "Engine/2d/SpriteCommon.h"
#include "Engine/2d/TextureManager.h"
#include "Engine/3d/Matrix.h"
#include "Engine/3d/Screen.h"
#include "Engine/3d/Vector.h"
#include "Engine/base/D3DResouceLeakCheker.h"
#include "Engine/base/DirectXCommon.h"
#include "Engine/base/Input.h"
#include "Engine/base/Logger.h"
#include "Engine/base/Resource.h"
#include "Engine/base/StringUtility.h"
#include "Engine/base/WinApp.h"
#include "extenals/DirectXTex/DirectXTex.h"
#include <Windows.h>
#include <cassert>
#include <chrono>
#include <codecvt>
#include <cstdint>
#include <d3d12.h>
#include <dbghelp.h>
#include <dxcapi.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <format>
#include <fstream>
#include <locale>
#include <math.h>
#include <sstream>
#include <string>
#include <strsafe.h>
#include <wrl.h>

#include "extenals/imgui/imgui.h"
#include "extenals/imgui/imgui_impl_dx12.h"
#include "extenals/imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

#pragma comment(lib, "DirectXTex.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

using namespace Logger;
using namespace StringUtility;
using namespace Microsoft::WRL;

enum BlendMode {
	kBlendModeNone,
	kBlendModeNormal,
	kBlendModeAdd,
	kBlendModeSubtract,
	kBlendModeMultiply,
	kBlendModeScreen,
	kBlendCountblend,
};

struct DirectionalLight {
	Vector4 color;     // 光の色
	Vector3 direction; // 光の方向
	float intensity;   // 光の強度
};

struct MaterialData {
	std::string textureFilePath; // テクスチャファイルのパス
};

struct ModelData {
	std::vector<VertexData> vertices; // 頂点データ
	MaterialData material;
};

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(const Microsoft::WRL::ComPtr<ID3D12Device>& device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors, bool shaderVisible) {
	assert(device != nullptr);
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
	// ヒープの設定
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Type = type;                                                                                         // ヒープのタイプ
	heapDesc.NumDescriptors = numDescriptors;                                                                     // ヒープに含まれるデスクリプタの数
	heapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // シェーダーからアクセス可能かどうか
	HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorHeap));
	assert(SUCCEEDED(hr));
	return descriptorHeap;
}

static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception) {
	// 時刻を取得して、時刻を名前に入れたファイルを作って、Dumpディレクトリをそこに出力する
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filepath[MAX_PATH] = {0};
	StringCchPrintfW(filepath, MAX_PATH, L"Dump\\%04d-%02d-%02d_%02d%02d.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
	HANDLE dumpFileHandle = CreateFile(filepath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	// processIDとクラッシュしたスレッドIDを取得
	DWORD processID = GetCurrentProcessId();
	DWORD threadID = GetCurrentThreadId();
	// 設定情報を入力
	MINIDUMP_EXCEPTION_INFORMATION minidumpInformation = {0};
	minidumpInformation.ThreadId = threadID;           // クラッシュしたスレッドID
	minidumpInformation.ExceptionPointers = exception; // 例外ポインタ
	minidumpInformation.ClientPointers = TRUE;         // クライアントポインタは使用しない
	// ダンプファイルの書き込み
	MiniDumpWriteDump(GetCurrentProcess(), processID, dumpFileHandle, MiniDumpNormal, &minidumpInformation, nullptr, nullptr);
	// 他に関連付けられているSEH例外ハンドラがあれば実行	なければ終了

	return EXCEPTION_EXECUTE_HANDLER; // 例外を処理するためのハンドラーを返す
}

D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}

MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
	MaterialData materialData;
	std::string line;
	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	while (std::getline(file, line)) {
		std::string idenfire;
		std::istringstream s(line);
		s >> idenfire;

		if (idenfire == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}
	return materialData;
}

ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename) {
	ModelData modelData;
	std::vector<Vector4> positions; // 頂点位置
	std::vector<Vector3> normals;   // 法線ベクトル
	std::vector<Vector2> texcoords; // テクスチャ座標
	std::string line;

	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open() && "Failed to open the OBJ file");

	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;
		if (identifier == "v") { // 頂点位置
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.x *= -1; // X軸を反転

			position.w = 1.0f; // Homogeneous coordinate
			positions.push_back(position);
		} else if (identifier == "vt") { // テクスチャ座標
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			texcoord.x = 1.0f - texcoord.x; // X軸はそのまま
			texcoord.y = 1.0f - texcoord.y; // Y軸を反転
			texcoords.push_back(texcoord);
		} else if (identifier == "vn") { // 法線ベクトル
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			normal.x *= -1; // X軸を反転
			normals.push_back(normal);
		} else if (identifier == "f") { // 面情報
			// 面は三角形限定、他未対応
			for (int32_t faceVertex = 0; faceVertex < 3; faceVertex++) {
				std::string vertexDefinition;
				s >> vertexDefinition;
				// 頂点の情報を分解
				std::istringstream v(vertexDefinition);
				uint32_t elementsIndices[3]; // 頂点、テクスチャ座標、法線のインデックス
				for (int32_t element = 0; element < 3; element++) {
					std::string index;
					std::getline(v, index, '/'); // '/'で区切ってインデックスを取得
					elementsIndices[element] = std::stoi(index);
				}
				// 要素へのIndexから、実際の要素の値を取得して、頂点を構築する
				Vector4 position = positions[elementsIndices[0] - 1];
				Vector2 texcoord = texcoords[elementsIndices[1] - 1];
				Vector3 normal = normals[elementsIndices[2] - 1];
				VertexData vertex = {position, texcoord, normal};
				modelData.vertices.push_back(vertex);
			}
		} else if (identifier == "mtllib") {
			std::string materialFilename;
			s >> materialFilename;
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
		}
	}

	return modelData;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	SetUnhandledExceptionFilter(ExportDump); // 例外ハンドラーを設定44

	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
	// ウィンドウクラスの登録
	WinApp* winApp = new WinApp();
	winApp->Initialize();

	// デバッグレイヤーの有効化
#ifdef Debug

	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController1 = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController1)))) {
		debugController1->EnableDebugLayer();
		debugController1->SetEnableGPUBasedValidation(TRUE);
	}
#endif

	OutputDebugStringA("Hello, World!\n");
	D3DResourceLeakCheker* Checker;
	Checker = new D3DResourceLeakCheker();

	// DirectInputの初期化
	Input* input = new Input();
	input->Initialize(winApp);
	DirectXCommon* dxCommon = new DirectXCommon();
	dxCommon->Initialize(winApp);
	TextureManager::GetInstance()->Initialize();
	TextureManager::GetInstance()->SetDirectXCommon(dxCommon);
	TextureManager::GetInstance()->LoadTexture("Resources/uvChecker.png");
	SpriteCommon* spriteCommon = new SpriteCommon();
	spriteCommon->Initialize(dxCommon);

	Sprite* sprite = new Sprite();
	sprite->Initialize(spriteCommon, "Resources/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("Resources/monsterball.png");

	std::vector<Sprite*> sprites;
	for (int i = 0; i < 5; i++) {
		Sprite* sprits = new Sprite();
		if (i==1||i==3) {
			sprits->Initialize(spriteCommon, "Resources/uvChecker.png");
		} else {
		sprits->Initialize(spriteCommon, "Resources/monsterball.png");
		}
		sprites.push_back(sprits);
		Transforms transform;
		transform.scale = {50.0f, 50.0f, 1.0f};
		transform.translate = {100.0f + i * 90.0f, 200.0f, 0.0f};
		sprits->SetTransform(transform);
	}

	//	//	assert(SUCCEEDED(hr));
	//	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	//	descriptorRange[0].BaseShaderRegister = 0;
	//	descriptorRange[0].NumDescriptors = 1;
	//	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	//
	//	// RootSignatureの設定
	//	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	//	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; // 入力アセンブラーでの使用を許可
	//
	//	// RootParameterの設定。複数設定できるので配列、今回は結果1つだけなので長さ1の配列
	//	D3D12_ROOT_PARAMETER rootParameters[4] = {};
	//	// ルートパラメーターの設定
	//	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;     // ルートパラメーターのタイプ（CBV）
	//	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  // シェーダーの可視性（ピクセルシェーダー）
	//	rootParameters[0].Descriptor.ShaderRegister = 0;                     // シェーダーレジスタのインデックス
	//	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;     // ルートパラメーターのタイプ（CBV）
	//	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // シェーダーの可視性（バーテックスシェーダー）
	//	rootParameters[1].Descriptor.ShaderRegister = 1;                     // シェーダーレジスタのインデックス
	//	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;     // ルートパラメーターのタイプ（CBV）
	//	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  // シェーダーの可視性（ピクセルシェーダー）
	//	rootParameters[2].Descriptor.ShaderRegister = 2;                     // シェーダーレジスタのインデックス
	//	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//	rootParameters[3].DescriptorTable.pDescriptorRanges = descriptorRange;
	//	rootParameters[3].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);
	//	descriptionRootSignature.pParameters = rootParameters;             // ルートパラメーターの配列
	//	descriptionRootSignature.NumParameters = _countof(rootParameters); // ルートパラメーターの数
	//
	//	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	//	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	//	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	//	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	//	staticSamplers[0].ShaderRegister = 0;
	//	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//	descriptionRootSignature.pStaticSamplers = staticSamplers;
	//	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);
	//
	//	// WVP用のリソースを作る。Matrix4x4
	//	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResorceModel = dxCommon->CreateBufferResource(sizeof(TransformationMatrix));
	//	// DepthStenecilResourceをウィンドウサイズで作成
	//	Microsoft::WRL::ComPtr<ID3D12Resource> depthStenecilResourceModel = dxCommon->CreateDepthStenecilTextureResource(dxCommon->GetDevice().Get(), WinApp::kClientWidth, WinApp::kClientHeight);
	//
	//	// データを書き込む
	//	TransformationMatrix* wvpDataModel = nullptr;
	//	// M書き込むためのアドレスを取得
	//	wvpResorceModel->Map(0, nullptr, reinterpret_cast<void**>(&wvpDataModel));
	//	// 初期値を設定
	//
	//	wvpDataModel->world = MakeIdentity4x4(); // 単位行列を設定
	//	wvpDataModel->WVP = MakeIdentity4x4();   // 単位行列を設定
	//	// WVP用のリソースを作る。Matrix4x4
	//	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResorce = dxCommon->CreateBufferResource(sizeof(TransformationMatrix));
	//	// DepthStenecilResourceをウィンドウサイズで作成
	//	Microsoft::WRL::ComPtr<ID3D12Resource> depthStenecilResource = dxCommon->CreateDepthStenecilTextureResource(dxCommon->GetDevice().Get(), WinApp::kClientWidth, WinApp::kClientHeight);
	//
	//	// データを書き込む
	//	TransformationMatrix* wvpData = nullptr;
	//	// M書き込むためのアドレスを取得
	//	wvpResorce->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
	//	// 初期値を設定
	//
	//	wvpData->world = MakeIdentity4x4(); // 単位行列を設定
	//	wvpData->WVP = MakeIdentity4x4();   // 単位行列を設定
	//	// シリアライズしてバイナリにする
	//	ID3DBlob* signatureBlob = nullptr;
	//	ID3DBlob* errorBlob = nullptr;
	//	hr = D3D12SerializeRootSignature(
	//	    &descriptionRootSignature,    // ルートシグネチャの説明
	//	    D3D_ROOT_SIGNATURE_VERSION_1, // バージョン
	//	    &signatureBlob,               // シリアライズされたバイナリ
	//	    &errorBlob                    // エラー情報
	//	);
	//	if (FAILED(hr)) {
	//		Log(reinterpret_cast<const char*>(errorBlob->GetBufferPointer())); // エラー内容をLogに出力
	//		assert(false);                                                     // シリアライズが失敗した場合はアサート
	//	}
	//	// バイナリをもとにルートシグネチャを生成
	//	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	//	hr = dxCommon->GetDevice()->CreateRootSignature(
	//	    0,                                 // シグネチャのバージョン
	//	    signatureBlob->GetBufferPointer(), // シリアライズされたバイナリのポインタ
	//	    signatureBlob->GetBufferSize(),    // バイナリのサイズ
	//	    IID_PPV_ARGS(&rootSignature)       // 生成したルートシグネチャを受け取る
	//	);
	//	assert(SUCCEEDED(hr)); // ルートシグネチャの生成が成功したか確認
	//
	//	// InputLayoutの設定
	//	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	//	inputElementDescs[0].SemanticName = "POSITION";                        // セマンティック名
	//	inputElementDescs[0].SemanticIndex = 0;                                // セマンティックインデックス
	//	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;          // フォーマット
	//	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT; // アライメントオフセット
	//	inputElementDescs[1].SemanticName = "TEXCOORD";                        // セマンティック名
	//	inputElementDescs[1].SemanticIndex = 0;                                // セマンティックインデックス
	//	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;                // フォーマット
	//	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT; // アライメントオフセット
	//	inputElementDescs[2].SemanticName = "NORMAL";                          // セマンティック名
	//	inputElementDescs[2].SemanticIndex = 0;                                // セマンティックインデックス
	//	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;             // フォーマット
	//	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT; // アライメントオフセット
	//	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	//	inputLayoutDesc.pInputElementDescs = inputElementDescs;    // 入力要素の配列
	//	inputLayoutDesc.NumElements = _countof(inputElementDescs); // 入力要素の数
	//
	//	// BlendStateの設定
	//	D3D12_BLEND_DESC blendDesc{};
	//	// すべての色要素を書き込む
	//	BlendMode blendMode = BlendMode::kBlendModeMultiply;
	//	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	//	blendDesc.RenderTarget[0].BlendEnable = TRUE; // ブレンドを無効にする
	//
	//	switch (blendMode) {
	//	case BlendMode::kBlendModeNormal:
	//		// すべての色要素を書き込む
	//		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;      // ソースのブレンドファクター
	//		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;          // ブレンドの演算
	//		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA; // デスティネーションのブレンドファクター
	//		break;
	//	case BlendMode::kBlendModeAdd:
	//		// すべての色要素を書き込む
	//		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA; // ソースのブレンドファクター
	//		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;     // ブレンドの演算
	//		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;      // デスティネーションのブレンドファクター
	//		break;
	//	case BlendMode::kBlendModeSubtract:
	//		// すべての色要素を書き込む
	//		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;      // ソースのブレンドファクター
	//		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT; // ブレンドの演算
	//		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;           // デスティネーションのブレンドファクター
	//		break;
	//	case BlendMode::kBlendModeMultiply:
	//		// すべての色要素を書き込む
	//		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;  // ソースのブレンドファクター
	//		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;      // ブレンドの演算
	//		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR; // デスティネーションのブレンドファクター
	//		break;
	//	};
	//
	//	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;   // デスティネーションのアルファブレンドファクター
	//	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD; // アルファブレンドの演算
	//	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO; // デスティネーションのアルファブレンドファクター
	//	// RasterizerStateの設定
	//	D3D12_RASTERIZER_DESC rasterizerDesc{};
	//	// 裏面（時計回り）を表示しない
	//	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK; // 裏面をカリング
	//	// 中身を塗りつぶす
	//	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID; // 塗りつぶしモード
	//
	//	// Shaderのコンパイル
	//	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = dxCommon->CompileShader(L"Resources/Shader/Object3d.VS.hlsl", L"vs_6_0");
	//	assert(vertexShaderBlob != nullptr); // Vertex Shaderのコンパイルが成功したか確認
	//	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = dxCommon->CompileShader(L"Resources/Shader/Object3d.PS.hlsl", L"ps_6_0");
	//	assert(pixelShaderBlob != nullptr); // Pixel Shaderのコンパイルが成功したか確認
	//
	//	D3D12_DEPTH_STENCIL_DESC depthStenecilDesc{};
	//	depthStenecilDesc.DepthEnable = true;
	//	depthStenecilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	//	depthStenecilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	//
	//	// PSOの設定
	//	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	//	graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();                                           // ルートシグネチャ
	//	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;                                                  // 入力レイアウト
	//	graphicsPipelineStateDesc.BlendState = blendDesc;                                                         // ブレンドステート
	//	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;                                               // ラスタライザーステート
	//	graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize()}; // Vertex Shader
	//	graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize()};   // Pixel Shader
	//	graphicsPipelineStateDesc.DepthStencilState = depthStenecilDesc;
	//	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	//	// 書き込むRTVの情報
	//	graphicsPipelineStateDesc.NumRenderTargets = 1;                            // レンダーターゲットの数
	//	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // レンダーターゲットのフォーマット
	//	// 利用するトポロジ（形状）のタイプ。三角形
	//	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // トポロジのタイプ
	//	// どのように画面に色を打ち込むかの設定
	//	graphicsPipelineStateDesc.SampleDesc.Count = 1;                   // マルチサンプルの数
	//	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; // サンプルマスク
	//	// 実際に生成
	//	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;
	//	hr = dxCommon->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
	//	assert(SUCCEEDED(hr)); // パイプラインステートの生成が成功したか確認
	//
	//	// 頂点リソース用のヒープの設定
	//	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	//	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // アップロード用のヒープタイプ
	//	// 頂点リソースの設定
	//	D3D12_RESOURCE_DESC vertexResourceDesc{};
	//	// バッファリソース。テクスチャの場合は別設定
	//	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; // リソースの次元
	//	vertexResourceDesc.Width = sizeof(Vector4) * 3;                 // 頂点バッファのサイズ（Vector4 * 3頂点）
	//	vertexResourceDesc.Height = 1;                                  // 高さは1
	//	vertexResourceDesc.DepthOrArraySize = 1;                        // 深さまたは配列サイズ
	//	vertexResourceDesc.MipLevels = 1;                               // ミップレベルは1
	//	vertexResourceDesc.SampleDesc.Count = 1;                        // マルチサンプルの数
	//	// バッファの場合はこれにする
	//	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // レイアウトは行メジャー
	//	// 実際に頂点リソースを生成
	//	// 頂点リソースにデータを書き込む
	//	// 出力リソース
	//	// 平行光のバッファにデータを入れる
	//	Microsoft::WRL::ComPtr<ID3D12Resource> lightResource = dxCommon->CreateBufferResource(sizeof(DirectionalLight));
	//	assert(SUCCEEDED(hr)); // ライトリソースの生成が成功したか確認
	//	DirectionalLight* directionallightData = nullptr;
	//	lightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionallightData));
	//
	//	// 値を設定（白くて上から照らす光）
	//	directionallightData->color = {1.0f, 1.0f, 1.0f, 1.0f};
	//	directionallightData->direction = NormalizeReturnVector(Vector3(0.0f, -1.0f, 0.0f));
	//	directionallightData->intensity = 1.0f;
	//
	// #pragma region マテリアルの描画に必要なデータの作成
	//	const float pi = 3.1415f;                         // 円周率
	//	const uint32_t kSubdivision = 16;                 // 球の細分化数
	//	const float kLonEvery = 2.0f * pi / kSubdivision; // 経度の間隔(φd)
	//	const float kLatEvery = pi / kSubdivision;        // 緯度の間隔(θd)
	//	uint32_t latIndex = 16;
	//	uint32_t lonIndex = 16;
	//	uint32_t startIndex = (kSubdivision * kSubdivision) * 6;
	//	Vector2 tex{};
	//	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = dxCommon->CreateBufferResource(sizeof(VertexData) * kSubdivision * kSubdivision * 6);
	//	assert(SUCCEEDED(hr)); // 頂点リソースの生成が成功したか確認
	//
	//	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource = dxCommon->CreateBufferResource(sizeof(uint32_t) * kSubdivision * kSubdivision * 6);
	//	D3D12_INDEX_BUFFER_VIEW indexBufferView{};
	//	assert(SUCCEEDED(hr)); // インデックスリソースの生成が成功したか確認
	//	// リソースの先頭のアドレスから使う
	//	indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress(); // GPU仮想アドレス
	//	// 使用するリソースのサイズはインデックスのサイズ * インデックス数
	//	indexBufferView.SizeInBytes = sizeof(uint32_t) * kSubdivision * kSubdivision * 6; // インデックスバッファのサイズ
	//	// インデックスはuint32_t型
	//	indexBufferView.Format = DXGI_FORMAT_R32_UINT; // 1インデックスのサイズ
	//
	//	uint32_t* indexData = nullptr;
	//	// 書き込むためのアドレスを取得
	//	indexResource->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
	//
	//	// 頂点バッファビューの作成
	//	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	//	// リソースの先頭のアドレスから使う
	//	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress(); // GPU仮想アドレス
	//	// 使用するリソースのサイズは頂点のサイズ * 頂点数
	//	vertexBufferView.SizeInBytes = sizeof(VertexData) * startIndex; // 頂点バッファのサイズ
	//	// 1頂点のサイズ
	//	vertexBufferView.StrideInBytes = sizeof(VertexData); // 1頂点のサイズ
	//
	//	VertexData* vertexData = nullptr;
	//	// 書き込むためのアドレスを取得
	//	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	//	// 頂点データを設定
	//	for (int latIndex = 0; latIndex < kSubdivision; latIndex++) {
	//		float θA = -pi / 2.0f + latIndex * kLatEvery; // (θ)
	//		float θB = θA + kLatEvery;
	//		for (int lonIndex = 0; lonIndex < kSubdivision; lonIndex++) {
	//			uint32_t start = (latIndex * kSubdivision + lonIndex) * 6;
	//
	//			float φA = lonIndex * kLonEvery; // (φ)
	//			float φB = φA + kLonEvery;
	//
	//			// 座標計算（4頂点：a,b,c,d）
	//			Vector4 a = {cos(θA) * cos(φA), sin(θA), cos(θA) * sin(φA), 1.0f};
	//			Vector4 b = {cos(θB) * cos(φA), sin(θB), cos(θB) * sin(φA), 1.0f};
	//			Vector4 c = {cos(θA) * cos(φB), sin(θA), cos(θA) * sin(φB), 1.0f};
	//			Vector4 d = {cos(θB) * cos(φB), sin(θB), cos(θB) * sin(φB), 1.0f};
	//
	//			Vector2 uv_a = {float(lonIndex) / kSubdivision, 1.0f - float(latIndex) / kSubdivision};
	//			Vector2 uv_b = {float(lonIndex) / kSubdivision, 1.0f - float(latIndex + 1) / kSubdivision};
	//			Vector2 uv_c = {float(lonIndex + 1) / kSubdivision, 1.0f - float(latIndex) / kSubdivision};
	//			Vector2 uv_d = {float(lonIndex + 1) / kSubdivision, 1.0f - float(latIndex + 1) / kSubdivision};
	//
	//			vertexData[start + 0] = {a, uv_a};
	//			vertexData[start + 1] = {b, uv_b};
	//			vertexData[start + 2] = {c, uv_c};
	//			vertexData[start + 3] = {d, uv_d};
	//			vertexData[start + 0].normal = (Vector3(a.x, a.y, a.z)); // 法線ベクトル
	//			vertexData[start + 1].normal = (Vector3(b.x, b.y, b.z));
	//			vertexData[start + 2].normal = (Vector3(c.x, c.y, c.z));
	//			vertexData[start + 3].normal = (Vector3(d.x, d.y, d.z));
	//
	//			// 三角形1: a-b-c
	//			indexData[start + 0] = start + 0; // 三角形1の1頂点目
	//			indexData[start + 1] = start + 1; // 三角形1の2頂点目
	//			indexData[start + 2] = start + 2; // 三角形1の3頂点目
	//
	//			// 三角形2: b-d-c
	//			indexData[start + 3] = start + 1; // 三角形2の1頂点目
	//			indexData[start + 4] = start + 3; // 三角形2の2頂点目
	//			indexData[start + 5] = start + 2; // 三角形2の3頂点目
	//		}
	//	}
	//	// マテリアル用のリソースを作る
	//	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = dxCommon->CreateBufferResource(sizeof(Material));
	//	Material* materialData = nullptr;
	//	// マテリアルリソースにデータを書き込む
	//	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	//	// マテリアルの色を設定
	//	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f); // 赤色
	//	materialData->enableLighting = true;                   // ライティングを有効化
	//	materialData->uvTransform = MakeIdentity4x4();
	//
	// #pragma endregion
	//
	// #pragma region モデルの描画に必要なデータの作成
	//
	//	// モデルの読み込み
	//
	//	ModelData modelData = LoadObjFile("Resources", "fence.obj");
	//
	//	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceModel = dxCommon->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());
	//	assert(SUCCEEDED(hr)); // 頂点リソースの生成が成功したか確認
	//
	//	// 頂点バッファビューの作成
	//	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewModel{};
	//	// リソースの先頭のアドレスから使う
	//	vertexBufferViewModel.BufferLocation = vertexResourceModel->GetGPUVirtualAddress(); // GPU仮想アドレス
	//	// 使用するリソースのサイズは頂点のサイズ * 頂点数
	//	vertexBufferViewModel.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size()); // 頂点バッファのサイズ
	//	// 1頂点のサイズ
	//	vertexBufferViewModel.StrideInBytes = sizeof(VertexData); // 1頂点のサイズ
	//
	//	VertexData* vertexDataModel = nullptr;
	//	// 書き込むためのアドレスを取得
	//	vertexResourceModel->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataModel));
	//	std::memcpy(vertexDataModel, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());
	//
	//	uint32_t* indexDataModel = nullptr;
	//	// 書き込むためのアドレスを取得
	//	Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceModel = dxCommon->CreateBufferResource(sizeof(uint32_t) * modelData.vertices.size());
	//
	//	// マテリアル用のリソースを作る
	//	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceModel = dxCommon->CreateBufferResource(sizeof(Material));
	//	Material* materialDataModel = nullptr;
	//	// マテリアルリソースにデータを書き込む
	//	materialResourceModel->Map(0, nullptr, reinterpret_cast<void**>(&materialDataModel));
	//	// マテリアルの色を設定
	//	materialDataModel->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f); // 赤色
	//	materialDataModel->enableLighting = true;                   // ライティングを有効化
	//	materialDataModel->uvTransform = MakeIdentity4x4();
	//
	// #pragma endregion

#pragma endregion

#pragma region 別の画像の読み込み
	// DirectX::ScratchImage mipImage3 = dxCommon->LoadTexture(modelData.material.textureFilePath);
	// const DirectX::TexMetadata& metaData3 = mipImage3.GetMetadata();
	//// テクスチャリソースの生成
	// Microsoft::WRL::ComPtr<ID3D12Resource> textureResource3 = dxCommon->CreateTextureResource(dxCommon->GetDevice(), metaData3);
	//// テクスチャにデータをアップロード
	// dxCommon->UploadTextureData(textureResource3, mipImage3);

	//// metaDataを基にSRVを生成
	// D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc3{};
	// srvDesc3.Format = metaData3.format;                                          // テクスチャのフォーマット
	// srvDesc3.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // シェーダーコンポーネントのマッピング
	// srvDesc3.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;                      // テクスチャの次元
	// srvDesc3.Texture2D.MipLevels = UINT(metaData3.mipLevels);                    // ミップレベルの数

	//// SRVを生成するためのディスクリプタヒープを取得
	// D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU3 = GetCPUDescriptorHandle(dxCommon->GetSRVDescriptorHeap(), descroptorSizeSRV, 3);
	// D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU3 = GetGPUDescriptorHandle(dxCommon->GetSRVDescriptorHeap(), descroptorSizeSRV, 3);

	//// SRVを生成
	// dxCommon->GetDevice()->CreateShaderResourceView(textureResource3.Get(), &srvDesc3, textureSrvHandleCPU3); // テクスチャリソースにSRVを設定

#pragma endregion

	// 指定した色で画面全体をクリアにする

	Transforms transform{
	    {1.0f, 1.0f, 1.0f}, // スケール
	    {0.0f, 0.0f, 0.0f}, // 回転
	    {0.0f, 0.0f, 0.0f}  // 平行移動
	};

	// Trsnsformの変数を作る
	Transforms transformModel{
	    {1.0f, 1.0f, 1.0f}, // スケール
	    {0.0f, 0.0f, 0.0f}, // 回転
	    {0.0f, 0.0f, 0.0f}  // 平行移動
	};

	Vector3 cameraPosition = {0.0f, 0.0f, -10.00f};
	Vector3 cameraRotate = {0.0f, 0.0f, 0.0f};
	const float clearColor[4] = {0.1f, 0.25f, 0.5f, 1.0f}; // 青色
	                                                       // メッセージループ

	bool useTexture = true;

	MSG msg = {};

	ResourceObject depthStencilResource = dxCommon->CreateDepthStenecilTextureResource(dxCommon->GetDevice(), WinApp::kClientWidth, WinApp::kClientHeight);

	while (msg.message != WM_QUIT) {
		// メッセージを取得
		if (winApp->ProcessMessage()) {

			// ゲームループを抜ける
			break;

		} else {
			// キーボード情報の取得開始
			input->Update();

			if (input->TriggerKey(DIK_0)) {
				OutputDebugStringA("Hit 0\n");
			}

			// ImGuiのフレーム開始
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			/*Matrix4x4 worldMatrixModel = MakeAffineMatrix(transformModel.scale, transformModel.rotate, transformModel.translate);
			Matrix4x4 cameraMatrixModel = MakeAffineMatrix(Vector3{1.0f, 1.0f, 1.0f}, cameraRotate, cameraPosition);
			Matrix4x4 viewMatrixModel = Inverse(cameraMatrixModel);
			Matrix4x4 projectionMatrixModel = MakePerspectiveFovMatrix(0.45f, float(WinApp::kClientWidth) / float(WinApp::kClientHeight), 0.1f, 100.0f);
			Matrix4x4 wvpMatrixModel = Multiply(worldMatrixModel, Multiply(viewMatrixModel, projectionMatrixModel));
			wvpDataModel->WVP = wvpMatrixModel;
			wvpDataModel->world = worldMatrixModel;

			Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
			Matrix4x4 cameraMatrix = MakeAffineMatrix(Vector3{1.0f, 1.0f, 1.0f}, cameraRotate, cameraPosition);
			Matrix4x4 viewMatrix = Inverse(cameraMatrix);
			Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(WinApp::kClientWidth) / float(WinApp::kClientHeight), 0.1f, 100.0f);
			Matrix4x4 wvpMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
			wvpData->WVP = wvpMatrix;
			wvpData->world = worldMatrix;*/
			sprite->Update();
			for (Sprite* s : sprites) {
				s->Update();
				s->SetSize({100.0f, 100.0f});
			}

			const char* modeNames[] = {"Normal", "Add", "Sub", "Multiply"};
			// ImGui::Combo("Select Mode", &currentMode, modeNames, 4);
			//  もしくは
			// ImGui::Combo("Select Mode", &currentMode, modeNames, std::size(modeNames));
			//// enumに戻す場合
			// blendMode = static_cast<BlendMode>(currentMode);
			for (size_t i = 0; i < sprites.size(); ++i) {
				Sprite* s = sprites[i];

				// ImGuiツリーで折りたたみ可能にする
				if (ImGui::TreeNode(("Sprite " + std::to_string(i)).c_str())) {
					Transforms tr = s->GetTransform();
					Transforms uv = s->GetUVTransform();
					Vector4 color = s->GetColor();
					Vector2 size = s->GetSize();

					ImGui::DragFloat2("Position", &tr.translate.x, 0.3f);
					ImGui::SliderAngle("Rotation", &tr.rotate.z);
					ImGui::DragFloat2("Scale", &size.x, 0.3f);
					ImGui::ColorEdit4("Color", &color.x);
					ImGui::DragFloat2("UV Translate", &uv.translate.x, 0.01f, -10.0f, 10.0f);
					ImGui::DragFloat2("UV Scale", &uv.scale.x, 0.01f, 0.0f, 10.0f);
					ImGui::SliderAngle("UV Rotate", &uv.rotate.z);

					s->SetTransform(tr);
					s->SetUVTransform(uv);
					s->SetColor(color);
					s->SetSize(size);

					ImGui::TreePop();
				}
			}

			Transforms trsprite = sprite->GetTransform();
			Transforms trspriteUV = sprite->GetUVTransform();
			Vector4 spriteColor = sprite->GetColor();
			Vector2 spriteSize = sprite->GetSize();
			ImGui::DragFloat3("camera pos", &cameraPosition.x, 0.1f);
			ImGui::SliderAngle("camera rotate x", &cameraRotate.x);
			ImGui::SliderAngle("camera rotate y", &cameraRotate.y);
			ImGui::SliderAngle("camera rotate z", &cameraRotate.z);
			ImGui::DragFloat2("sprite pos", &trsprite.translate.x, 0.3f);
			ImGui::SliderAngle("sprite rotate", &trsprite.rotate.z);
			ImGui::DragFloat2("sprite scale", &spriteSize.x, 0.3f);
			ImGui::ColorEdit4("sprite color", &spriteColor.x, 1.0f); // クリアカラーの編集
			ImGui::DragFloat2("UV translate", &trspriteUV.translate.x, 0.01f, -10.0f, 10.0f);
			ImGui::DragFloat2("UV scale", &trspriteUV.scale.x, 0.01f, 0.0f, 10.0f);
			ImGui::SliderAngle("UV rotate", &trspriteUV.rotate.z);
			// ImGui::ColorEdit4("lighr color", &directionallightData->color.x, 1.0f); // クリアカラーの編
			// ImGui::DragFloat3("light direction", &directionallightData->direction.x, 0.1f);
			// directionallightData->direction = NormalizeReturnVector(directionallightData->direction); // 正規化
			// ImGui::SliderFloat("intensity", &directionallightData->intensity, 0.0f, 1.0f);
			//  ImGuiのウィンドウを作成
			ImGui::Render(); // ImGuiの描画を実行

			sprite->SetTransform(trsprite);
			sprite->SetUVTransform(trspriteUV);
			sprite->SetColor(spriteColor);
			sprite->SetSize(spriteSize);

#pragma region コマンドリストのリセット

			dxCommon->PreDraw();
			// dxCommon->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());
			// dxCommon->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());
			//// インデックスを使った描画
			// dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
			// dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, wvpResorce->GetGPUVirtualAddress());
			// dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(2, lightResource->GetGPUVirtualAddress());
			// dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(3, useTexture ? textureSrvHandleGPU2 : textureSrvHandleGPU);
			// dxCommon->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());
			// dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
			// dxCommon->GetCommandList()->IASetIndexBuffer(&indexBufferView);
			// dxCommon->GetCommandList()->DrawIndexedInstanced(startIndex, 1, 0, 0, 0);

			// モデルの描画
			spriteCommon->SetDraw();
			sprite->Draw();
			for (Sprite* s : sprites) {
				s->Draw();
			}
			/*	dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResourceModel->GetGPUVirtualAddress());
			    dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, wvpResorceModel->GetGPUVirtualAddress());
			    dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(2, lightResource->GetGPUVirtualAddress());
			    dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(3, textureSrvHandleGPU3);
			    dxCommon->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());
			    dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferViewModel);
			    dxCommon->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);*/

			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dxCommon->GetCommandList().Get());

			dxCommon->PostDraw();

#pragma endregion
		}
	}
	// ImGuiの終了処理
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext(); // ImGuiのコンテキストを破棄

	delete Checker;
	delete input; // DirectInputオブジェクトの解放
	for (Sprite* s : sprites) {
		delete s;
	}
	delete sprite; // スプライトの解放
	delete spriteCommon;
	TextureManager::GetInstance()->Finalize();
	delete dxCommon;    // DirectXCommonの解放
	winApp->Finalize(); // ウィンドウの終了処理
	delete winApp;      // ウィンドウクラスの解放
}
