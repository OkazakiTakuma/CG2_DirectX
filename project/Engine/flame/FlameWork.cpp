#include "FlameWork.h"
#include <strsafe.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <minidumpapiset.h>
#include <Windows.h>
#include <wrl/client.h>
#include <excpt.h>
#include <memory>
#include <ParticleManager.h>
#include <SpriteCommon.h>
#include <TextureManager.h>
#include <Camera.h>
#include <ModelManager.h>
#include <Object3dCommon.h>
#include <D3DResouceLeakCheker.h>
#include <DirectXCommon.h>
#include <ImGuiManager.h>
#include <Input.h>
#include <SrvManager.h>
#include <WinApp.h>
using namespace Logger;
using namespace StringUtility;
using namespace Microsoft::WRL;
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


void FlameWork::Initialize() {
#pragma region 基盤システムの初期化
	SetUnhandledExceptionFilter(ExportDump); // 例外ハンドラーを設定44

	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
	// ウィンドウクラスの登録
	winApp = std::make_unique<WinApp>();
	winApp->Initialize();

	// デバッグレイヤーの有効化

#if _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController1 = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController1)))) {
		debugController1->EnableDebugLayer();
		debugController1->SetEnableGPUBasedValidation(TRUE);
	}

#endif // _DEBUG

	OutputDebugStringA("Hello, World!\n");
	Checker = std::make_unique<D3DResourceLeakCheker>();

	// DirectInputの初期化
	Input::GetInstance()->Initialize(winApp.get()); // ImGuiの初期化
	dxCommon = std::make_unique<DirectXCommon>();
	dxCommon->Initialize(winApp.get());
	SrvManager::GetInstance()->Initialize(dxCommon.get());
	ImGuiManager::GetInstance()->Initialize(winApp.get(), dxCommon.get());
	TextureManager::GetInstance()->Initialize(dxCommon.get());
	TextureManager::GetInstance()->SetDirectXCommon(dxCommon.get());
	SpriteCommon::GetInstance()->Initialize(dxCommon.get());
	SkyBoxCommon::GetInstance()->Initialize(dxCommon.get());

	Object3dCommon::GetInstance()->Initialize(dxCommon.get());
	ModelManager::GetInstance()->Inithialize(dxCommon.get());
	ParticleManager::GetInstance()->Initialize(dxCommon.get());
}

void FlameWork::Update() {
	if (winApp->ProcessMessage()) { // ここで WM_QUIT を受け取ったら break する
	endRequest = true;
	}
}

void FlameWork::Draw() {}

void FlameWork::Finalize() {
	ParticleManager::GetInstance()->Finalize();
	ModelManager::GetInstance()->Finalize();
	Object3dCommon::GetInstance()->Finalize();
	SpriteCommon::GetInstance()->Finalize();
	TextureManager::GetInstance()->Finalize();
	
	ImGuiManager::GetInstance()->Finalize();
	SrvManager::GetInstance()->Finalize();

	dxCommon->Release();
	winApp->Finalize();
}

void FlameWork::Run() {
	Initialize();
	while (true) {
		Update();
		if (IsEnd()) {
			break;
		}
		Draw();
	}
	Finalize();
}
