#include <Windows.h>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <d3d12.h>
#include <dbghelp.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <format>
#include <strsafe.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
// Windowsアプリケーションのエントリポイント
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void Log(const std::string& message) { OutputDebugStringA(message.c_str()); }

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
	// コマンドキュー、ウィンドウハンドル、設定を渡して生成する
	hr = dxgiFactory->CreateSwapChainForHwnd(
	    commandQueue,                                   // コマンドキュー
	    hwnd,                                           // ウィンドウハンドル
	    &swapChainDesc,                                 // スワップチェーンの設定
	    nullptr,                                        // オプション（nullptrでデフォルト）
	    nullptr,                                        // 共有リソース（nullptrで共有しない）
	    reinterpret_cast<IDXGISwapChain1**>(&swapChain) // スワップチェーンの出力
	);
	assert(SUCCEEDED(hr));

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

	// ② 描画：RTV設定＋Clear
	commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, nullptr);
	commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);

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
