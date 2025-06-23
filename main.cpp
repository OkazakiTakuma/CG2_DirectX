#include <Windows.h>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <d3d12.h>
#include <dbghelp.h>
#include <dxgi1_6.h>
#include <format>
#include <strsafe.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "Dbghelp.lib")
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
			// 現在の時刻
		}
	}

	return 0;
}
