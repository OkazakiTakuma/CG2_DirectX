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
#include"LineCommon.h"
#include"LineDrawer.h"
#include"InstancingModelCommon.h"
#include <DirectXCommon.h>
#include <ImGuiManager.h>
#include <Input.h>
#include <SrvManager.h>
#include <WinApp.h>
#include"PostEffect.h"
using namespace Logger;
using namespace StringUtility;
using namespace Microsoft::WRL;
static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception) {
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filepath[MAX_PATH] = { 0 };
	StringCchPrintfW(filepath, MAX_PATH, L"Dump\\%04d-%02d-%02d_%02d%02d.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
	HANDLE dumpFileHandle = CreateFile(filepath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	DWORD processID = GetCurrentProcessId();
	DWORD threadID = GetCurrentThreadId();
	MINIDUMP_EXCEPTION_INFORMATION minidumpInformation = { 0 };
	minidumpInformation.ThreadId = threadID;
	minidumpInformation.ExceptionPointers = exception;
	minidumpInformation.ClientPointers = TRUE;
	MiniDumpWriteDump(GetCurrentProcess(), processID, dumpFileHandle, MiniDumpNormal, &minidumpInformation, nullptr, nullptr);

	return EXCEPTION_EXECUTE_HANDLER;
}


void FlameWork::Initialize() {
#pragma region Setup
	SetUnhandledExceptionFilter(ExportDump);

	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
	winApp = std::make_unique<WinApp>();
	winApp->Initialize();


#if _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController1 = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController1)))) {
		debugController1->EnableDebugLayer();
		debugController1->SetEnableGPUBasedValidation(TRUE);
	}

#endif // _DEBUG

	OutputDebugStringA("Hello, World!\n");
	Checker = std::make_unique<D3DResourceLeakCheker>();

	Input::GetInstance()->Initialize(winApp.get());
	dxCommon = std::make_unique<DirectXCommon>();
	dxCommon->Initialize(winApp.get());
	SrvManager::GetInstance()->Initialize(dxCommon.get());
	ImGuiManager::GetInstance()->Initialize(winApp.get(), dxCommon.get());
	TextureManager::GetInstance()->Initialize(dxCommon.get());
	TextureManager::GetInstance()->SetDirectXCommon(dxCommon.get());
	PostEffect::GetInstance()->Initialize(dxCommon.get());
	SpriteCommon::GetInstance()->Initialize(dxCommon.get());
	SkyBoxCommon::GetInstance()->Initialize(dxCommon.get());
	LineCommon::GetInstance()->Initialize(dxCommon.get());
	LineDrawer::GetInstance()->Initialize();
	Object3dCommon::GetInstance()->Initialize(dxCommon.get());
	ModelManager::GetInstance()->Inithialize(dxCommon.get());
	ParticleManager::GetInstance()->Initialize(dxCommon.get());
	InstancingModelCommon::GetInstance()->Initialize(dxCommon.get());

}

void FlameWork::Update() {
	if (winApp->ProcessMessage()) {
		endRequest = true;
	}
}

void FlameWork::Draw() {}

void FlameWork::ToggleFullscreen() {
	if (winApp) {
		winApp->ToggleFullscreen();
	}
}

void FlameWork::Finalize() {
	ParticleManager::GetInstance()->Finalize();
	ModelManager::GetInstance()->Finalize();
	Object3dCommon::GetInstance()->Finalize();
	LineDrawer::GetInstance()->Finalize();
	LineCommon::GetInstance()->Finalize();
	SkyBoxCommon::GetInstance()->Finalize();
	SpriteCommon::GetInstance()->Finalize();
	PostEffect::GetInstance()->Finalize();
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
		if (IsEndRequest() || endRequest) {
			break;
		}
		Draw();
	}
	Finalize();
}
