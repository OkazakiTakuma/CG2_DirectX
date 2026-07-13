#pragma once
#include "Audio.h"
#include "Camera.h"
#include "D3DResouceLeakCheker.h"
#include "DirectXCommon.h"
#include "FlameWork.h"
#include "ImGuiManager.h"
#include "Input.h"
#include "Matrix.h"
#include "ModelManager.h"
#include "Object3d.h"
#include "Object3dCommon.h"
#include "ParticleEmitter.h"
#include "ParticleManager.h"
#include "Resource.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include"SkyBoxCommon.h"
#include "SrvManager.h"
#include "TextureManager.h"
#include "Vector.h"
#include "WinApp.h"
#include "struct.h"
#include <Windows.h>
#include <d3d12.h>
#include <string>
#include <strsafe.h>
#include <wrl.h>
#include "AbstractSceneFactory.h"

#pragma comment(lib, "DirectXTex.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

class FlameWork {
public:
	virtual ~FlameWork() = default;

	/// <summary>
	/// 必要なリソースを準備し、オブジェクトを初期化します。
	/// </summary>
	virtual void Initialize();
	/// <summary>
	/// 毎フレームの状態更新を行います。
	/// </summary>
	virtual void Update();
	/// <summary>
	/// 現在の状態をもとに描画処理を行います。
	/// </summary>
	virtual void Draw();
	/// <summary>
	/// 確保したリソースを解放し、終了処理を行います。
	/// </summary>
	virtual void Finalize();
	/// <summary>
	/// Run の処理を行います。
	/// </summary>
	void Run();
	/// <summary>
	/// Fullscreen の状態を切り替えます。
	/// </summary>
	void ToggleFullscreen();
	float GetRenderAspectRatio() const;
	int32_t GetRenderWidth() const;
	int32_t GetRenderHeight() const;

	virtual bool IsEndRequest() const { return endRequest; };


private:
	bool endRequest = false;
	std::unique_ptr<WinApp> winApp = nullptr;
	std::unique_ptr<D3DResourceLeakCheker> Checker = nullptr;
	std::unique_ptr<DirectXCommon> dxCommon = nullptr;
	std::unique_ptr<ImGuiManager> imguiManager = nullptr;
	std::unique_ptr<AbstractSceneFactory> sceneFactory = nullptr;
};
