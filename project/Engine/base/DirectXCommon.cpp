#include "DirectXCommon.h"

using namespace Logger;
using namespace StringUtility;

const uint32_t DirectXCommon::kMaxSRVCount = 512;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DirectXCommon::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors, bool shaderVisible) {
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

void DirectXCommon::Initialize(WinApp* winApp) {
	assert(winApp);
	this->winApp = winApp;
	// 各種初期化処理
	InitializeFixFPS();
	CreateDevice();
	CreateCommand();
	CreateSwapChain();
	CreateDepthBuffer();
	CreateDescriptorHeap();
	CreateRenderTargetView();
	CreateDepthStencilView();
	CreateFence();
	CreateViewportRect();
	CreateScissorRect();
	CreateDXCompiler();
	CreateImGui();
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVCPUDescriptorHandle(uint32_t index) { return GetCPUDescriptorHandle(srvDescriptorHeap, descroptorSizeSRV, index); }

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVGPUDescriptorHandle(uint32_t index) { return GetGPUDescriptorHandle(srvDescriptorHeap, descroptorSizeSRV, index); }

void DirectXCommon::PreDraw() {
	dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// 書き込むバックバッファのインデックスを取得
	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();
	// TransitionBarrierの設定
	// ① 描画前の状態遷移：PRESENT → RENDER_TARGET

	// バリア設定（PRESENT → RENDER_TARGET）
	D3D12_RESOURCE_BARRIER barrier{};

	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	commandList->ResourceBarrier(1, &barrier);
	const float clearColor[4] = {0.1f, 0.25f, 0.5f, 1.0f}; // 青色
	// クリア処理と描画
	commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);
	// 指定した色で画面全体をクリアにする
	commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	//  描画用のDescriptorHeapを設定
	ID3D12DescriptorHeap* heaps[] = {srvDescriptorHeap.Get()};
	commandList->SetDescriptorHeaps(_countof(heaps), heaps);
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
}

void DirectXCommon::PostDraw() {
	HRESULT hr;
	// 書き込むバックバッファのインデックスを取得
	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();
	// バリア設定（RENDER_TARGET → PRESENT）
	D3D12_RESOURCE_BARRIER barrier{};

	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = swapChainResources[backBufferIndex].Get(); // ← これが必要
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	commandList->ResourceBarrier(1, &barrier);

	// コマンド送信とPresent
	hr = commandList->Close();
	assert(SUCCEEDED(hr));
	ID3D12CommandList* commandLists[] = {commandList.Get()};
	commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
	swapChain->Present(1, 0);

	// フェンスでGPU完了待ち（簡略化）
	fenceValue++;
	commandQueue->Signal(fence.Get(), fenceValue);
	if (fence->GetCompletedValue() < fenceValue) {
		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}
	UpdateFixFPS();

	hr = commandAllocator->Reset();
	assert(SUCCEEDED(hr));
	hr = commandList->Reset(commandAllocator.Get(), nullptr);
	assert(SUCCEEDED(hr));
}

Microsoft::WRL::ComPtr<IDxcBlob> DirectXCommon::CompileShader(const std::wstring& filepath, const wchar_t* profile) {
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

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateBufferResource(size_t sizeInBytes) {
	assert(device != nullptr);
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;

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

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateTextureResource(const DirectX::TexMetadata& metaData) {
	assert(device != nullptr);

	// バッファリソースの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metaData.width);
	resourceDesc.Height = UINT(metaData.height);
	resourceDesc.MipLevels = UINT16(metaData.mipLevels);
	resourceDesc.DepthOrArraySize = UINT16(metaData.arraySize);
	resourceDesc.Format = metaData.format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metaData.dimension);

	// アップロードヒープの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;                        // 細かい設定を行う
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK; // CPUからの書き込みを許可
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;          // メモリプールの設定

	// リソースの設定
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
	    &heapProperties,                // ヒープのプロパティ
	    D3D12_HEAP_FLAG_NONE,           // ヒープのフラグ
	    &resourceDesc,                  // リソースの設定
	    D3D12_RESOURCE_STATE_COPY_DEST, // 初期状態
	    nullptr,                        // クリア値はなし
	    IID_PPV_ARGS(&resource)         // リソースのポインタを取得
	);

	assert(SUCCEEDED(hr));
	return resource;
}

void DirectXCommon::UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource>& texture, const DirectX::ScratchImage& mipImage) {

	// テクスチャのメタデータを取得
	const DirectX::TexMetadata& metaData = mipImage.GetMetadata();
	// 全MipMapについて
	for (size_t mipLevel = 0; mipLevel < metaData.mipLevels; mipLevel++) {
		// MipMapLevelを指定して
		const DirectX::Image* img = mipImage.GetImage(mipLevel, 0, 0);
		// Textureに転送
		HRESULT hr = texture->WriteToSubresource(
		    UINT(mipLevel),       // サブリソースインデックス（0は最初のサブリソース）
		    nullptr,              // 全体を転送するのでnullptr
		    img->pixels,          // 転送するピクセルデータ
		    UINT(img->rowPitch),  // 行のピッチ（1行あたりのバイト数）
		    UINT(img->slicePitch) // スライスのピッチ（3Dテクスチャの場合は必要）
		);
		assert(SUCCEEDED(hr));
	}
}

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateDepthStenecilTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, int32_t width, int32_t height) {
	// 生成するリソースの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	// 利用するヒープ
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	// 深度値のクリア設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// リソースの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
	    &heapProperties,                  // ヒープの設定
	    D3D12_HEAP_FLAG_NONE,             // ヒープの特殊設定
	    &resourceDesc,                    // リソースの設定
	    D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値の書き込み可
	    &depthClearValue,                 // クリア最適値
	    IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));

	return resource;
}

void DirectXCommon::CreateDevice() {
	HRESULT hr;
	hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	assert(SUCCEEDED(hr));
	// DXGIファクトリーのバージョンを確認
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter = nullptr;

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
		hr = D3D12CreateDevice(useAdapter.Get(), featureLevel[i], IID_PPV_ARGS(&device));
		if (SUCCEEDED(hr)) {
			Log(std::string("D3D12 Device created with feature level: ") + featureLevelStrings[i]);
			break; // 成功したらループを抜ける
		}
	}
	assert(device != nullptr);
	// 修正: Log関数の呼び出しを正しい形式に変更
	Log("Comlete create D3D12 Device\n");

#ifdef Debug

	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
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
}

void DirectXCommon::CreateCommand() {
	HRESULT hr;
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	// コマンドアロケーターの生成が成功したか確認
	assert(SUCCEEDED(hr));

	// Define the missing identifiers and fix the syntax error
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
	// コマンドキューの生成が成功したか確認
	assert(SUCCEEDED(hr));

	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
	// コマンドリストの生成が成功したか確認
	assert(SUCCEEDED(hr));
}

void DirectXCommon::CreateSwapChain() {
	HRESULT hr;
	swapChainDesc.Width = WinApp::kClientWidth;                  // スワップチェーンの幅
	swapChainDesc.Height = WinApp::kClientHeight;                // スワップチェーンの高さ
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;           // スワップチェーンのフォーマット
	swapChainDesc.SampleDesc.Count = 1;                          // マルチサンプルの数
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // スワップチェーンの使用法
	swapChainDesc.BufferCount = 2;                               // バッファの数
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;    // スワップ効果
	// コマンドキュー、ウィンドウハンドル、設定を渡して生成する生成する
	// 修正: スワップチェーンの生成部分での型変換エラーを修正
	// Microsoft::WRL::ComPtr を使用しているため、reinterpret_cast を削除し、GetAddressOf() を使用する
	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1 = nullptr;
	hr = dxgiFactory->CreateSwapChainForHwnd(
	    commandQueue.Get(),       // コマンドキュー
	    winApp->GetHwnd(),        // ウィンドウハンドル
	    &swapChainDesc,           // スワップチェーンの設定
	    nullptr,                  // オプション（nullptrでデフォルト）
	    nullptr,                  // 共有リソース（nullptrで共有しない）
	    swapChain1.GetAddressOf() // スワップチェーンの出力
	);
	assert(SUCCEEDED(hr));

	// IDXGISwapChain1 を IDXGISwapChain4 にクエリして取得
	hr = swapChain1.As(&swapChain);
}

void DirectXCommon::CreateDepthBuffer() {
	// 生成するリソースの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = winApp->kClientWidth;
	resourceDesc.Height = winApp->kClientHeight;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	// 利用するヒープ
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	// 深度値のクリア設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	HRESULT hr = device->CreateCommittedResource(
	    &heapProperties,                  // ヒープの設定
	    D3D12_HEAP_FLAG_NONE,             // ヒープの特殊設定
	    &resourceDesc,                    // リソースの設定
	    D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値の書き込み可
	    &depthClearValue,                 // クリア最適値
	    IID_PPV_ARGS(&depthStencilResource));
	assert(SUCCEEDED(hr));
}

void DirectXCommon::CreateDescriptorHeap() {
	descroptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descroptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	descroptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	srvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kMaxSRVCount, true);
	rtvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	dsvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
}

void DirectXCommon::CreateRenderTargetView() {
	HRESULT hr;
	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
	assert(SUCCEEDED(hr));
	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
	assert(SUCCEEDED(hr));

	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;      // レンダーターゲットビューのフォーマット
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D; // レンダーターゲットビューの次元
	// ディスクリプタの先頭を取得
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	rtvHandles[0] = rtvStartHandle;
	device->CreateRenderTargetView(swapChainResources->GetAddressOf()[0], &rtvDesc, rtvHandles[0]);
	rtvHandles[1] = {rtvStartHandle.ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)};
	device->CreateRenderTargetView(swapChainResources->GetAddressOf()[1], &rtvDesc, rtvHandles[1]);
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}

void DirectXCommon::CreateDepthStencilView() {
	// DSVの設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	// DSVHeapの先頭にDSVを作る
	device->CreateDepthStencilView(depthStencilResource.Get(), &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

void DirectXCommon::CreateFence() {
	HRESULT hr;
	uint16_t fenceValue = 0;
	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	// FenceのSignal用のイベントハンドルを生成
	fenceEvent = CreateEvent(nullptr, false, false, nullptr);
	assert(fenceEvent != nullptr); // イベントハンドルの生成が成功したか確認
}

void DirectXCommon::CreateViewportRect() {

	// クライアント領域のサイズに合わせる
	viewport.Width = static_cast<float>(WinApp::kClientWidth);   // ビューポートの幅
	viewport.Height = static_cast<float>(WinApp::kClientHeight); // ビューポートの高さ
	viewport.MinDepth = 0.0f;                                    // 最小深度
	viewport.MaxDepth = 1.0f;                                    // 最大深度
	viewport.TopLeftX = 0.0f;                                    // ビューポートの左上X座標
	viewport.TopLeftY = 0.0f;                                    // ビューポートの左上Y座標
}

void DirectXCommon::CreateScissorRect() {
	// 基本的にビューポートと同じサイズ
	scissorRect.left = 0;                       // シザー矩形の左端
	scissorRect.top = 0;                        // シザー矩形の上端
	scissorRect.right = WinApp::kClientWidth;   // シザー矩形の右端
	scissorRect.bottom = WinApp::kClientHeight; // シザー矩形の下端
}

void DirectXCommon::CreateDXCompiler() {
	HRESULT hr;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));
}

void DirectXCommon::CreateImGui() {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();                  // ImGuiのコンテキストを作成
	ImGui::StyleColorsDark();                // ImGuiのスタイルをダークに設定
	ImGui_ImplWin32_Init(winApp->GetHwnd()); // ImGuiのWin32バックエンドを初期化
	ImGui_ImplDX12_Init(
	    device.Get(),
	    swapChainDesc.BufferCount,                               // スワップチェーンのバッファ数
	    rtvDesc.Format,                                          // レンダーターゲットビューのフォーマット
	    srvDescriptorHeap.Get(),                                 // レンダーターゲットビューのディスクリプタヒープ
	    srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), // CPUディスクリプタハンドル
	    srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()  // GPUディスクリプタハンドル
	);
}

void DirectXCommon::InitializeFixFPS() { reference_ = std::chrono::high_resolution_clock::now(); }

void DirectXCommon::UpdateFixFPS() {
	const std::chrono::microseconds kMinTime(uint64_t(1000000.0f / 60.0f));
	const std::chrono::microseconds kMinCheckTime(uint64_t(1000000.0f / 65.0f));
	// 経過時間を取得
	std::chrono::steady_clock::time_point now = std::chrono::high_resolution_clock::now();
	// 経過時間
	std::chrono::microseconds elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - reference_);

	// 最低時間未満なら待機
	if (elapsed < kMinTime) {
		// 経過時間が最低時間に達するまでループ
		while (std::chrono::steady_clock::now() - reference_ < kMinTime) {
			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}
	}
	// 現在の時間を記録
	reference_ = std::chrono::high_resolution_clock::now();
}
