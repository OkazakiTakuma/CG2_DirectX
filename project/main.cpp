#include "Engine/2d/Sprite.h"
#include "Engine/2d/SpriteCommon.h"
#include "Engine/2d/TextureManager.h"
#include "Engine/3d/Matrix.h"
#include "Engine/3d/Object3d.h"
#include "Engine/3d/Object3dCommon.h"
#include "Engine/3d/Model.h"
#include "Engine/3d/ModelCommon.h"
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


int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

#pragma region 基盤システムの初期化
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
		if (i == 1 || i == 3) {
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
	Object3dCommon* obj3dComoon = new Object3dCommon;
	obj3dComoon->Initialize(dxCommon);
	Object3d* object3d = new Object3d;
	object3d->Initialize(obj3dComoon);
	ModelCommon* modelCommon = new ModelCommon;
	modelCommon->Initialize(dxCommon);
	Model* model = new Model;
	model->Initialize(modelCommon, "Resources", "plane.obj");
	object3d->SetModel(model);


#pragma endregion
	


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

			object3d->Update();
			sprite->Update();
			for (Sprite* s : sprites) {
				s->Update();
				s->SetSize({100.0f, 100.0f});
			}

			const char* modeNames[] = {"Normal", "Add", "Sub", "Multiply"};
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
			Vector2 anchor = sprite->GetAnchorPoint();
			bool isFlipX = sprite->GetIsFlipX();
			bool isFlipY = sprite->GetIsFlipY();
			Vector2 textureLeftTop = sprite->GetTextureLeftTop();
			Vector2 textureSize = sprite->GetTextureSize();
			// モデルのパラメータ調整用
			Vector3 modelPosition = object3d->GetTransformTranslate();
			Vector3 modelRotate = object3d->GetTransformRotate();
			Vector3 modelScale = object3d->GetTransformScale();
			


			ImGui::DragFloat3("camera pos", &cameraPosition.x, 0.1f);
			ImGui::SliderAngle("camera rotate x", &cameraRotate.x);
			ImGui::SliderAngle("camera rotate y", &cameraRotate.y);
			ImGui::SliderAngle("camera rotate z", &cameraRotate.z);
			ImGui::DragFloat3("model pos", &modelPosition.x, 0.1f);
			ImGui::SliderAngle("model rotate x", &modelRotate.x);
			ImGui::SliderAngle("model rotate y", &modelRotate.y);
			ImGui::SliderAngle("model rotate z", &modelRotate.z);
			ImGui::DragFloat3("model scale", &modelScale.x, 0.1f);
			ImGui::DragFloat2("sprite pos", &trsprite.translate.x, 0.3f);
			ImGui::SliderAngle("sprite rotate", &trsprite.rotate.z);
			ImGui::DragFloat2("sprite scale", &spriteSize.x, 0.3f);
			ImGui::ColorEdit4("sprite color", &spriteColor.x, 1.0f); // クリアカラーの編集
			ImGui::DragFloat2("anchor point", &anchor.x, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat2("texture left top", &textureLeftTop.x, 1.0f, 0.0f, 512.0f);
			ImGui::DragFloat2("texture size", &textureSize.x, 1.0f, 0.0f, 512.0f);
			ImGui::Checkbox("Flip X", &isFlipX);
			ImGui::Checkbox("Flip Y", &isFlipY);
			ImGui::DragFloat2("UV translate", &trspriteUV.translate.x, 0.01f, -10.0f, 10.0f);
			ImGui::DragFloat2("UV scale", &trspriteUV.scale.x, 0.01f, 0.0f, 10.0f);
			ImGui::SliderAngle("UV rotate", &trspriteUV.rotate.z);
			// ImGui::ColorEdit4("lighr color", &directionallightData->color.x, 1.0f); // クリアカラーの編
			// ImGui::DragFloat3("light direction", &directionallightData->direction.x, 0.1f);
			// directionallightData->direction = NormalizeReturnVector(directionallightData->direction); // 正規化
			// ImGui::SliderFloat("intensity", &directionallightData->intensity, 0.0f, 1.0f);
			//  ImGuiのウィンドウを作成
			ImGui::Render(); // ImGuiの描画を実行
			object3d->SetTransform(modelPosition);
			object3d->SetTransformRotate(modelRotate);
			object3d->SetTransformScale(modelScale);

			object3d->SetCameraRotate(cameraRotate);
			object3d->SetCameraTranslate(cameraPosition);

			sprite->SetIsFlipX(isFlipX);
			sprite->SetIsFlipY(isFlipY);
			sprite->SetAnchorPoint(anchor);
			sprite->SetTransform(trsprite);
			sprite->SetUVTransform(trspriteUV);
			sprite->SetColor(spriteColor);
			sprite->SetSize(spriteSize);
			sprite->SetTextureLeftTop(textureLeftTop);
			sprite->SetTextureSize(textureSize);

#pragma region コマンドリストのリセット

			dxCommon->PreDraw();
			// モデルの描画
			obj3dComoon->SetDraw();
			object3d->Draw();
			

			// スプライトの描画
			spriteCommon->SetDraw();
			sprite->Draw();
			for (Sprite* s : sprites) {
				s->Draw();
			}
		

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
	delete object3d;
	delete obj3dComoon;
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
