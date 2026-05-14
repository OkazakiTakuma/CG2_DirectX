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
	virtual void Initialize();

	virtual void Update();

	virtual void Draw();

	virtual void Finalize();

	virtual bool IsEnd() const { return endRequest; };

	void Run();

private:
	bool endRequest = false;
	std::unique_ptr<WinApp> winApp = nullptr;
	std::unique_ptr<D3DResourceLeakCheker> Checker = nullptr;
	std::unique_ptr<DirectXCommon> dxCommon = nullptr;
	std::unique_ptr<ImGuiManager> imguiManager = nullptr;
	std::unique_ptr<AbstractSceneFactory> sceneFactory = nullptr;
};
