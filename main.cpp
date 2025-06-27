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
#include <locale>
#include <string>
#include <strsafe.h>
#include <vector>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

struct Vector4 {
	float x, y, z, w;
	Vector4(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 1.0f) : x(x), y(y), z(z), w(w) {}
	Vector4 operator+(const Vector4& other) const { return Vector4(x + other.x, y + other.y, z + other.z, w + other.w); }
	Vector4 operator-(const Vector4& other) const { return Vector4(x - other.x, y - other.y, z - other.z, w - other.w); }
};

// Windowsアプリケーションのエントリポイント
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

std::string ConvertString(const std::wstring& wstr) {
	if (wstr.empty())
		return {};

	int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), result.data(), sizeNeeded, nullptr, nullptr);
	return result;
}

void Log(const std::string& message) { OutputDebugStringA(message.c_str()); }

IDxcBlob* CompileShader(
    // CompieするShaderのファイルパス
    const std::wstring& filepath,
    // Compilerに使用するProfile
    const wchar_t* profile,
    // 初期化で生成したものを3つ
    IDxcUtils* dxcUtils, IDxcCompiler3* dxcCompiler, IDxcIncludeHandler* includeHandler) {
	// これからシェーダーをコンパイルすることをLogに
	Log(ConvertString(std::format(L"Bigin CompileShader, path:{},profile:{}\n", filepath, profile)));
	// hlslファイルを読み込む
	IDxcBlobEncoding* shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filepath.c_str(), nullptr, &shaderSource);
	assert(SUCCEEDED(hr)); // ファイルの読み込みが成功したか確認
	// 読み込んだファイルの内容を設定する
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer(); // 読み込んだファイルのポインタ
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();   // 読み込んだファイルのサイズ
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;                 // UTF-8エンコーディング

	LPCWSTR arguments[] = {
	    filepath.c_str(), // シェーダーファイルのパス
	    L"-E",
	    L"main",
	    L"-T",
	    profile,
	    L"-Zi",
	    L"-Qembed_debug",
	    L"-Od",
	    L"-Zpr"};
	// 実際にShaderをコンパイルする
	IDxcResult* shaderResult = nullptr;

	hr = dxcCompiler->Compile(
	    &shaderSourceBuffer,        // シェーダーソース (IDxcBlob* 型に変更)
	    arguments,                  // コンパイルオプション
	    _countof(arguments),        // オプションの数
	    includeHandler,             // インクルードハンドラー
	    IID_PPV_ARGS(&shaderResult) // 結果を受け取る
	);
	assert(SUCCEEDED(hr)); // コンパイルが成功したか確認
	                       // 警告・エラーのチェック
	IDxcBlobUtf8* shaderError = nullptr;
	IDxcBlobWide* dummyName = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), &dummyName);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
		Log(shaderError->GetStringPointer()); // エラー内容をLogに出力
		assert(false);                        // エラーが発生した場合はアサート
	}
	// コンパイル結果を受け取る
	IDxcBlob* shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));
	Log(ConvertString(std::format(L"Complete CompileShader, path:{},profile:{}\n", filepath, profile)));
	// 後始末
	shaderSource->Release(); // 読み込んだファイルの解放
	shaderResult->Release(); // コンパイル結果の解放
	return shaderBlob;       // コンパイル結果のBlobを返す
}


ID3D12Resource* CreateBufferResource(ID3D12Device* device, size_t sizeInBytes) {
	assert(device != nullptr);
	ID3D12Resource* resource = nullptr;

	// アップロードヒープの設定
	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	// バッファリソースの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeInBytes;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));
	return resource;
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

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	SetUnhandledExceptionFilter(ExportDump); // 例外ハンドラーを設定
	// ウィンドウクラスの設定
	WNDCLASS wc = {};
	// ウィンドウプロシージャ
	wc.lpfnWndProc = WindowProc;
	// ウィンドウのクラス名
	wc.lpszClassName = L"CG2WindowClass";
	// インスタンスハンドル
	wc.hInstance = GetModuleHandle(nullptr);
	// カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	// ウィンドウクラスの登録
	RegisterClass(&wc);

	// クライアント領域のサイズを指定

	const int32_t kClientWidth = 1280; // クライアント領域の幅

	const int32_t kClientHeight = 720; // クライアント領域の高さ

	// ウィンドウサイズを表す構造体にクライアント領域のサイズを設定

	RECT wrc = {0, 0, kClientWidth, kClientHeight};

	// ウィンドウのクライアント領域のサイズを取得

	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
	// ウィンドウの作成
	HWND hwnd = CreateWindow(
	    wc.lpszClassName,     // 拡張スタイル
	    L"CG2 Window",        // ウィンドウタイトル
	    WS_OVERLAPPEDWINDOW,  // スタイル
	    CW_USEDEFAULT,        // ウィンドウの位置x座標
	    CW_USEDEFAULT,        // ウィンドウの位置y座標
	    wrc.right - wrc.left, // ウィンドウの幅
	    wrc.bottom - wrc.top, // ウィンドウの高さ
	    nullptr,              // 親ウィンドウハンドル
	    nullptr,              // メニューハンドル
	    wc.hInstance,         // インスタンスハンドル
	    nullptr               // 追加のアプリケーションデータ
	);

	// デバッグレイヤーの有効化
#ifdef Debug

	ID3D12Debug1* debugController1 = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController1)))) {
		debugController1->EnableDebugLayer();                // デバッグレイヤーを有効化
		debugController1->SetEnableGPUBasedValidation(TRUE); // GPUベースの検証を有効化
	}
#endif

	// ウィンドウの表示
	ShowWindow(hwnd, SW_SHOW);

	OutputDebugStringA("Hello, World!\n");

	// DXGIファクトリーの生成
	IDXGIFactory6* dxgiFactory = nullptr;
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	assert(SUCCEEDED(hr));
	// DXGIファクトリーのバージョンを確認
	IDXGIAdapter4* useAdapter = nullptr;

	// アダプターの列挙
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i) {
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));
		// ソフトウェアアダプターはスキップ
		if (adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) {

			std::wstring wdesc(adapterDesc.Description);
			std::string desc(wdesc.begin(), wdesc.end()); // ※ 日本語は文字化けする可能性あり
			Log("Use Adapter: " + desc + "\n");
			break;
		}
		useAdapter = nullptr;
	}
	assert(useAdapter != nullptr);

	ID3D12Device* device = nullptr;
	// 機能レベルとログ出力用の文字列
	D3D_FEATURE_LEVEL featureLevel[]{
	    D3D_FEATURE_LEVEL_12_2,
	    D3D_FEATURE_LEVEL_12_1,
	    D3D_FEATURE_LEVEL_12_0,
	};
	const char* featureLevelStrings[] = {
	    "12_2",
	    "12_1",
	    "12_0",
	};
	// 高い順に生成できるか確認
	for (size_t i = 0; i < _countof(featureLevel); i++) {
		// デバイスの生成
		hr = D3D12CreateDevice(useAdapter, featureLevel[i], IID_PPV_ARGS(&device));
		if (SUCCEEDED(hr)) {
			Log(std::string("D3D12 Device created with feature level: ") + featureLevelStrings[i]);
			break; // 成功したらループを抜ける
		}
	}
	assert(device != nullptr);
	// 修正: Log関数の呼び出しを正しい形式に変更
	Log("Comlete create D3D12 Device\n");

#ifdef Debug

	ID3D12InfoQueue* infoQueue = nullptr;
	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
		// デバッグメッセージのフィルターを設定
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		// infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		infoQueue->Release();
		D3D12_MESSAGE_ID denyIds[] = {D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE};
		// 抑制するレベル
		D3D12_MESSAGE_SEVERITY severities[] = {D3D12_MESSAGE_SEVERITY_INFO};
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		// 指定したメッセージを抑制するフィルターを設定
		infoQueue->PushStorageFilter(&filter);
	}
#endif // Debug

	// 初期値0でFenceを生成
	ID3D12Fence* fence = nullptr;
	uint16_t fenceValue = 0;
	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	// FenceのSignal用のイベントハンドルを生成
	HANDLE fenceEvent = CreateEvent(nullptr, false, false, nullptr);
	assert(fenceEvent != nullptr); // イベントハンドルの生成が成功したか確認

	// dxcCompilerの初期化
	IDxcUtils* dxcUtils = nullptr;
	IDxcCompiler3* dxcCompiler = nullptr;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	IDxcIncludeHandler* includeHandler = nullptr;
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));

	// コマンドキューの生成
	ID3D12CommandQueue* commandQueue = nullptr;
	// Define the missing identifiers and fix the syntax error
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
	// コマンドキューの生成が成功したか確認
	assert(SUCCEEDED(hr));

	// コマンドアロケーターの生成
	ID3D12CommandAllocator* commandAllocator = nullptr;
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	// コマンドアロケーターの生成が成功したか確認
	assert(SUCCEEDED(hr));
	// コマンドリストの生成
	ID3D12GraphicsCommandList* commandList = nullptr;
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
	// コマンドリストの生成が成功したか確認
	assert(SUCCEEDED(hr));

	// スワップチェーンの生成
	IDXGISwapChain4* swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = kClientWidth;                          // スワップチェーンの幅
	swapChainDesc.Height = kClientHeight;                        // スワップチェーンの高さ
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;           // スワップチェーンのフォーマット
	swapChainDesc.SampleDesc.Count = 1;                          // マルチサンプルの数
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // スワップチェーンの使用法
	swapChainDesc.BufferCount = 2;                               // バッファの数
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;    // スワップ効果
	// コマンドキュー、ウィンドウハンドル、設定を渡して生成する生成する
	hr = dxgiFactory->CreateSwapChainForHwnd(
	    commandQueue,                                   // コマンドキュー
	    hwnd,                                           // ウィンドウハンドル
	    &swapChainDesc,                                 // スワップチェーンの設定
	    nullptr,                                        // オプション（nullptrでデフォルト）
	    nullptr,                                        // 共有リソース（nullptrで共有しない）
	    reinterpret_cast<IDXGISwapChain1**>(&swapChain) // スワップチェーンの出力
	);
	assert(SUCCEEDED(hr));

	// RootSignatureの設定
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; // 入力アセンブラーでの使用を許可

	// RootParameterの設定。複数設定できるので配列、今回は結果1つだけなので長さ1の配列
	D3D12_ROOT_PARAMETER rootParameters[1] = {};
	// ルートパラメーターの設定
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;    // ルートパラメーターのタイプ（CBV）
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // シェーダーの可視性（ピクセルシェーダー）
	rootParameters[0].Descriptor.ShaderRegister = 0;                    // シェーダーレジスタのインデックス
	descriptionRootSignature.pParameters = rootParameters;              // ルートパラメーターの配列
	descriptionRootSignature.NumParameters = _countof(rootParameters);  // ルートパラメーターの数

	// シリアライズしてバイナリにする
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
	ID3D12RootSignature* rootSignature = nullptr;
	hr = device->CreateRootSignature(
	    0,                                 // シグネチャのバージョン
	    signatureBlob->GetBufferPointer(), // シリアライズされたバイナリのポインタ
	    signatureBlob->GetBufferSize(),    // バイナリのサイズ
	    IID_PPV_ARGS(&rootSignature)       // 生成したルートシグネチャを受け取る
	);
	assert(SUCCEEDED(hr)); // ルートシグネチャの生成が成功したか確認

	// InputLayoutの設定
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[1] = {};
	inputElementDescs[0].SemanticName = "POSITION";               // セマンティック名
	inputElementDescs[0].SemanticIndex = 0;                       // セマンティックインデックス
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // フォーマット
	inputElementDescs[0].AlignedByteOffset = 0;                   // アライメントオフセット
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;    // 入力要素の配列
	inputLayoutDesc.NumElements = _countof(inputElementDescs); // 入力要素の数

	// BlendStateの設定
	D3D12_BLEND_DESC blendDesc{};
	// すべての色要素を書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	// 裏面（時計回り）を表示しない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK; // 裏面をカリング
	// 中身を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID; // 塗りつぶしモード

	// Shaderのコンパイル
	IDxcBlob* vertexShaderBlob = CompileShader(L"Object3d.VS.hlsl", L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
	assert(vertexShaderBlob != nullptr); // Vertex Shaderのコンパイルが成功したか確認
	IDxcBlob* pixelShaderBlob = CompileShader(L"Object3d.PS.hlsl", L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
	assert(pixelShaderBlob != nullptr); // Pixel Shaderのコンパイルが成功したか確認

	// PSOの設定
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature;                                                 // ルートシグネチャ
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;                                                  // 入力レイアウト
	graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize()}; // Vertex Shader
	graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize()};   // Pixel Shader
	graphicsPipelineStateDesc.BlendState = blendDesc;                                                         // ブレンドステート
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;                                               // ラスタライザーステート
	// 書き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1;                            // レンダーターゲットの数
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // レンダーターゲットのフォーマット
	// 利用するトポロジ（形状）のタイプ。三角形
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // トポロジのタイプ
	// どのように画面に色を打ち込むかの設定
	graphicsPipelineStateDesc.SampleDesc.Count = 1;                   // マルチサンプルの数
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; // サンプルマスク
	// 実際に生成
	ID3D12PipelineState* graphicsPipelineState = nullptr;
	hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr)); // パイプラインステートの生成が成功したか確認

	// 頂点リソース用のヒープの設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // アップロード用のヒープタイプ
	// 頂点リソースの設定
	D3D12_RESOURCE_DESC vertexResourceDesc{};
	// バッファリソース。テクスチャの場合は別設定
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; // リソースの次元
	vertexResourceDesc.Width = sizeof(Vector4) * 3;                 // 頂点バッファのサイズ（Vector4 * 3頂点）
	vertexResourceDesc.Height = 1;                                  // 高さは1
	vertexResourceDesc.DepthOrArraySize = 1;                        // 深さまたは配列サイズ
	vertexResourceDesc.MipLevels = 1;                               // ミップレベルは1
	vertexResourceDesc.SampleDesc.Count = 1;                        // マルチサンプルの数
	// バッファの場合はこれにする
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // レイアウトは行メジャー
	// 実際に頂点リソースを生成
	// 出力リソース
	ID3D12Resource* vertexResource = CreateBufferResource(device, sizeof(Vector4) * 3);
	hr= device->CreateCommittedResource(
		&uploadHeapProperties, // ヒーププロパティ
		D3D12_HEAP_FLAG_NONE,  // ヒープフラグ
		&vertexResourceDesc,    // リソースの説明
		D3D12_RESOURCE_STATE_GENERIC_READ, // リソースの初期状態
		nullptr, // 初期化用のクリア値（今回はなし）
		IID_PPV_ARGS(&vertexResource) // 出力リソース
	);
	assert(SUCCEEDED(hr)); // 頂点リソースの生成が成功したか確認

	// マテリアル用のリソースを作
	ID3D12Resource* materialResource = CreateBufferResource(device, sizeof(Vector4));
	Vector4* materialData = nullptr;
	// マテリアルリソースにデータを書き込む
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	// マテリアルの色を設定
	materialData[0] = Vector4(1.0f, 0.0f, 0.0f,1.0f); // 赤色

	// 頂点バッファビューの作成
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	// リソースの先頭のアドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress(); // GPU仮想アドレス
	// 使用するリソースのサイズは頂点のサイズ * 頂点数
	vertexBufferView.SizeInBytes = sizeof(Vector4) * 3; // 頂点バッファのサイズ
	// 1頂点のサイズ
	vertexBufferView.StrideInBytes = sizeof(Vector4); // 1頂点のサイズ

	// 頂点リソースにデータを書き込む
	Vector4* vertexData = nullptr;
	// 書き込むためのアドレスを取得
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	// 頂点データを設定
	vertexData[0] = Vector4(-0.5f, -0.5f, 1.0f); // 左
	vertexData[1] = Vector4(0.0f, 0.5f, 1.0f);   // 上
	vertexData[2] = Vector4(0.5f, -0.5f, 1.0f);  // 右

	// ビューポート
	D3D12_VIEWPORT viewport{};
	// クライアント領域のサイズに合わせる
	viewport.Width = static_cast<float>(kClientWidth);   // ビューポートの幅
	viewport.Height = static_cast<float>(kClientHeight); // ビューポートの高さ
	viewport.MinDepth = 0.0f;                            // 最小深度
	viewport.MaxDepth = 1.0f;                            // 最大深度
	viewport.TopLeftX = 0.0f;                            // ビューポートの左上X座標
	viewport.TopLeftY = 0.0f;                            // ビューポートの左上Y座標

	// シザー矩形
	D3D12_RECT scissorRect{};
	// 基本的にビューポートと同じサイズ
	scissorRect.left = 0;               // シザー矩形の左端
	scissorRect.top = 0;                // シザー矩形の上端
	scissorRect.right = kClientWidth;   // シザー矩形の右端
	scissorRect.bottom = kClientHeight; // シザー矩形の下端

	// ディスクリプタヒープの生成
	ID3D12DescriptorHeap* rtvDescriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC rtvDeescriptorHeapDesc{};
	rtvDeescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // レンダーターゲットビューのヒープタイプ
	rtvDeescriptorHeapDesc.NumDescriptors = 2;                    // スワップチェーンのバッファ数と同じ
	hr = device->CreateDescriptorHeap(&rtvDeescriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));
	assert(SUCCEEDED(hr));

	// スワップチェーンからリソースをもらう
	ID3D12Resource* swapChainResources[2] = {nullptr};
	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
	assert(SUCCEEDED(hr));
	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
	assert(SUCCEEDED(hr));

	// RTVの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;      // レンダーターゲットビューのフォーマット
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D; // レンダーターゲットビューの次元
	// ディスクリプタの先頭を取得
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	// RTVを2つ作るからディスクリプタも2つ
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
	rtvHandles[0] = rtvStartHandle;
	device->CreateRenderTargetView(swapChainResources[0], &rtvDesc, rtvHandles[0]);
	rtvHandles[1] = {rtvStartHandle.ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)};
	device->CreateRenderTargetView(swapChainResources[1], &rtvDesc, rtvHandles[1]);

	// 書き込むバックバッファのインデックスを取得
	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();
	// TransitionBarrierの設定
	// ① 描画前の状態遷移：PRESENT → RENDER_TARGET
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = swapChainResources[backBufferIndex];
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	commandList->ResourceBarrier(1, &barrier);
	// 描画先のRTVを設定
	commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, nullptr);
	// 指定した色で画面全体をクリアにする
	const float clearColor[4] = {0.1f, 0.25f, 0.5f, 1.0f}; // 青色
	commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);

	// ここから描画処理を行う
	commandList->RSSetViewports(1, &viewport);       // ビューポートの設定
	commandList->RSSetScissorRects(1, &scissorRect); // シザー矩形の設定
	// RootSignatureの設定
	commandList->SetGraphicsRootSignature(rootSignature);
	// ルートパラメーターの設定
	commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress()); // マテリアルリソースの設定
	commandList->SetPipelineState(graphicsPipelineState);     // パイプラインステートの設定
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView); // 頂点バッファの設定
	// 形状の設定
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // トポロジの設定
	// 描画コマンドの発行
	commandList->DrawInstanced(3, 1, 0, 0); // 3頂点を1回描画

	// ③ 描画後の状態遷移：RENDER_TARGET → PRESENT
	std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
	commandList->ResourceBarrier(1, &barrier);
	// コマンドリストをクローズ
	hr = commandList->Close();
	assert(SUCCEEDED(hr));

	// GPUにコマンドリストの実行を要求
	ID3D12CommandList* commandLists[] = {commandList};
	commandQueue->ExecuteCommandLists(1, commandLists);
	// GPUとOSに画面の交換を要求
	swapChain->Present(1, 0); // 1は垂直同期のための待機、0はオプションフラグ
	// Fenceの値を更新
	fenceValue++;
	// GPUがここまでたどり着いたときに、Fenceの値を指定した値に代入するようにSignalを送る
	commandQueue->Signal(fence, fenceValue);
	// Fenceの値が指定したSignal値にたどり着いているか確認する
	// GetCompletedVlueの初期値はFence作成時のの値の初期値
	if (fence->GetCompletedValue() < fenceValue) {
		// 完了していない場合は、イベントハンドルを待機する
		// Signalで指定した値にたどり着くまで待機
		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		// イベントハンドルがシグナル状態になるまで待機
		WaitForSingleObject(fenceEvent, INFINITE);
	}

	// 次フレーム用のコマンドリスト
	hr = commandAllocator->Reset(); // コマンドアロケーターをリセット
	assert(SUCCEEDED(hr));
	hr = commandList->Reset(commandAllocator, nullptr); // コマンドリストをリセット
	assert(SUCCEEDED(hr));

	// メッセージループ
	MSG msg{};
	while (msg.message != WM_QUIT) {
		// メッセージを取得
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			// メッセージを処理
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else {
			// メッセージがない場合は、ここでアプリケーションの処理を行う
			// 例えば、描画処理など
			//
			// 現在の時刻
		}
	}

	CloseHandle(fenceEvent);          // イベントハンドルを閉じる
	fence->Release();                 // Fenceを解放
	rtvDescriptorHeap->Release();     // ディスクリプタヒープを解放
	vertexResource->Release();        // 頂点リソースを解放
	materialResource->Release();      // マテリアルリソースを解放
	graphicsPipelineState->Release(); // パイプラインステートを解放
	signatureBlob->Release();         // ルートシグネチャのシリアライズされたバイナリを解放
	if (errorBlob) {
		errorBlob->Release(); // エラー情報のバイナリを解放
	}
	rootSignature->Release();         // ルートシグネチャを解放
	pixelShaderBlob->Release();       // ピクセルシェーダーのバイナリを解放
	vertexShaderBlob->Release();      // バーテックスシェーダーのバイナリを解放
	swapChainResources[0]->Release(); // スワップチェーンのリソースを解放
	swapChainResources[1]->Release(); // スワップチェーンのリソースを解放
	swapChain->Release();             // スワップチェーンを解放
	commandList->Release();           // コマンドリストを解放
	commandQueue->Release();          // コマンドキューを解放
	device->Release();                // デバイスを解放
	useAdapter->Release();            // アダプターを解放
	dxgiFactory->Release();           // DXGIファクトリーを解放
#ifdef DEBUG
	debugController1->Release();
#endif                 // DEBUG
	CloseWindow(hwnd); // ウィンドウを閉じる

	// リソースリークチェック
	IDXGIDebug* debug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		debug->Release();
	}

	return 0;
}
