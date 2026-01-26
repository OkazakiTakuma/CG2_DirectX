#include "Camera.h"
#include "D3DResouceLeakCheker.h"
#include "DirectXCommon.h"
#include "Input.h"
#include "Logger.h"
#include "Matrix.h"
#include "Model.h"
#include "ModelCommon.h"
#include "ModelManager.h"
#include "Object3d.h"
#include "Object3dCommon.h"
#include "ParticleManager.h"
#include "ParticleEmitter.h"
#include "Resource.h"
#include "Screen.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "SrvManager.h"
#include "StringUtility.h"
#include "TextureManager.h"
#include "Vector.h"
#include "WinApp.h"
#include "extenals/DirectXTex/DirectXTex.h"
#include "extenals/imgui/imgui.h"
#include "extenals/imgui/imgui_impl_dx12.h"
#include "extenals/imgui/imgui_impl_win32.h"
#include "struct.h"

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
	SrvManager* srvManager = nullptr;
	srvManager = new SrvManager();
	srvManager->Initialize(dxCommon);
#pragma endregion
	TextureManager::GetInstance()->Initialize(srvManager);
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
		Transform transform;
		transform.scale = {50.0f, 50.0f, 1.0f};
		transform.translate = {100.0f + i * 90.0f, 200.0f, 0.0f};
		sprits->SetTransform(transform);
	}

	Object3dCommon* obj3dComoon = new Object3dCommon;
	obj3dComoon->Initialize(dxCommon);
	Camera* camera = new Camera();
	camera->SetTranslate({0.0f, 0.0f, -20.0f});
	obj3dComoon->SetDefaultCamera(camera);
	Object3d* object3d = new Object3d;
	object3d->Initialize(obj3dComoon);
	ModelManager::GetInstance()->Inithialize(dxCommon);
	ModelManager::GetInstance()->LoadModel("plane.obj");
	object3d->SetModel("plane.obj");
	ModelManager::GetInstance()->LoadModel("axis.obj");
	std::vector<Object3d*> axisObjects;
	const int axisCount = 5;
	for (int i = 0; i < axisCount; ++i) {
		Object3d* axisObj = new Object3d();
		axisObj->Initialize(obj3dComoon);
		axisObj->SetModel("axis.obj");
		// 位置をずらして配置
		axisObj->SetScale({1.0f, 1.0f, 1.0f});
		axisObj->SetTranslate({float(i) * 2.0f, float(i) * 2.0f, 3.0f});
		axisObjects.push_back(axisObj);
	}
	// 1. 初期化
	// 1. パーティクルグループを作成（テクスチャのロードなどもここで行われます）
	// 引数：グループ名, テクスチャパス
	// 2. グループ作成（テクスチャロード）
	ParticleManager::GetInstance()->Initialize(dxCommon, srvManager);
	ParticleManager::GetInstance()->CreateParticleGroup("Smoke", "Resources/uvChecker.png");
	ParticleManager::GetInstance()->CreateParticleGroup("Fire", "Resources/uvChecker.png");
	ParticleManager::GetInstance()->SetCamera(camera);

	// =================================================
	// ▼ 追加: エミッタの作成
	// =================================================
	// エミッタ用の座標設定
	Transform emitterTransform;
	emitterTransform.translate = {0.0f, 0.0f, 0.0f}; // 原点
	emitterTransform.rotate = {0.0f, 0.0f, 0.0f};
	emitterTransform.scale = {1.0f, 1.0f, 1.0f};

	// "Smoke" グループ用のエミッタを生成
	// (グループ名, Transform, 1回の発生数, 1秒間の発生頻度)
	ParticleEmitter* smokeEmitter = new ParticleEmitter("Smoke", emitterTransform, 5, 60.0f);
	// ※ここでは「1秒間に10回、1回につき5個発生」という設定にしています
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
			if (input->TriggerKey(DIK_SPACE)) {
			}
			// ImGuiのフレーム開始
			/*ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();*/
			// 更新
			smokeEmitter->Update(1.0f / 60.0f);
			ParticleManager::GetInstance()->Update();
			camera->Update();
			camera->Update();
			object3d->Update();
			for (Object3d* axisObj : axisObjects) {
				axisObj->Update();
			}

			sprite->Update();
			for (Sprite* s : sprites) {
				s->Update();
				s->SetSize({100.0f, 100.0f});
			}

			const char* modeNames[] = {"Normal", "Add", "Sub", "Multiply"};
			for (size_t i = 0; i < sprites.size(); ++i) {
				Sprite* s = sprites[i];

				// ImGuiツリーで折りたたみ可能にする
				/*if (ImGui::TreeNode(("Sprite " + std::to_string(i)).c_str())) {
				    Transform tr = s->GetTransform();
				    Transform uv = s->GetUVTransform();
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
				}*/
			}
			Vector3 cameraPosition = camera->GetTranslate();
			Vector3 cameraRotate = camera->GetRotate();

			Transform trsprite = sprite->GetTransform();
			Transform trspriteUV = sprite->GetUVTransform();
			Vector4 spriteColor = sprite->GetColor();
			Vector2 spriteSize = sprite->GetSize();
			Vector2 anchor = sprite->GetAnchorPoint();
			bool isFlipX = sprite->GetIsFlipX();
			bool isFlipY = sprite->GetIsFlipY();
			Vector2 textureLeftTop = sprite->GetTextureLeftTop();
			Vector2 textureSize = sprite->GetTextureSize();
			// モデルのパラメータ調整用
			Vector3 modelPosition = object3d->GetTranslate();
			Vector3 modelRotate = object3d->GetRotate();
			Vector3 modelScale = object3d->GetScale();

			// ImGui::DragFloat3("camera pos", &cameraPosition.x, 0.1f);
			// ImGui::SliderAngle("camera rotate x", &cameraRotate.x);
			// ImGui::SliderAngle("camera rotate y", &cameraRotate.y);
			// ImGui::SliderAngle("camera rotate z", &cameraRotate.z);
			// ImGui::DragFloat3("model pos", &modelPosition.x, 0.1f);
			// ImGui::SliderAngle("model rotate x", &modelRotate.x);
			// ImGui::SliderAngle("model rotate y", &modelRotate.y);
			// ImGui::SliderAngle("model rotate z", &modelRotate.z);
			// ImGui::DragFloat3("model scale", &modelScale.x, 0.1f);
			// ImGui::DragFloat2("sprite pos", &trsprite.translate.x, 0.3f);
			// ImGui::SliderAngle("sprite rotate", &trsprite.rotate.z);
			// ImGui::DragFloat2("sprite scale", &spriteSize.x, 0.3f);
			// ImGui::ColorEdit4("sprite color", &spriteColor.x, 1.0f); // クリアカラーの編集
			// ImGui::DragFloat2("anchor point", &anchor.x, 0.01f, 0.0f, 1.0f);
			// ImGui::DragFloat2("texture left top", &textureLeftTop.x, 1.0f, 0.0f, 512.0f);
			// ImGui::DragFloat2("texture size", &textureSize.x, 1.0f, 0.0f, 512.0f);
			// ImGui::Checkbox("Flip X", &isFlipX);
			// ImGui::Checkbox("Flip Y", &isFlipY);
			// ImGui::DragFloat2("UV translate", &trspriteUV.translate.x, 0.01f, -10.0f, 10.0f);
			// ImGui::DragFloat2("UV scale", &trspriteUV.scale.x, 0.01f, 0.0f, 10.0f);
			// ImGui::SliderAngle("UV rotate", &trspriteUV.rotate.z);
			////  ImGuiのウィンドウを作成
			// ImGui::Render(); // ImGuiの描画を実行
			camera->SetTranslate(cameraPosition);
			camera->SetRotate(cameraRotate);
			object3d->SetTranslate(modelPosition);
			object3d->SetRotate(modelRotate);
			object3d->SetScale(modelScale);

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
			//object3d->Draw();
			// 複数axis.obj描画
			ParticleManager::GetInstance()->Draw(camera);
			for (Object3d* axisObj : axisObjects) {
		//		axisObj->Draw();
			}

			// スプライトの描画
			spriteCommon->SetDraw();
			for (Sprite* s : sprites) {
			//	s->Draw();
			}
			// ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dxCommon->GetCommandList().Get());

			dxCommon->PostDraw();

#pragma endregion
		}
	}
	// ImGuiの終了処理
	// ImGui_ImplDX12_Shutdown();
	// ImGui_ImplWin32_Shutdown();
	// ImGui::DestroyContext(); // ImGuiのコンテキストを破棄

	delete Checker;
	delete input; // DirectInputオブジェクトの解放

	ModelManager::GetInstance()->Finalize();

	delete smokeEmitter;
	// --- 終了処理 ---
	for (Object3d* axisObj : axisObjects) {
		delete axisObj;
	}

	delete object3d;
	delete obj3dComoon;
	delete sprite; // スプライトの解放
	delete spriteCommon;
	TextureManager::GetInstance()->Finalize();
	delete srvManager;
	delete dxCommon;    // DirectXCommonの解放
	winApp->Finalize(); // ウィンドウの終了処理
	delete winApp;      // ウィンドウクラスの解放
}
