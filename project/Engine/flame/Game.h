#pragma once
#include "Audio.h"
#include "Camera.h"
#include "D3DResouceLeakCheker.h"
#include "ImGuiManager.h"
#include "Input.h"
#include "Matrix.h"
#include "ParticleEmitter.h"
#include "ParticleManager.h"
#include "Resource.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "SrvManager.h"
#include "Vector.h"
#include "struct.h"
#include <Windows.h>
#include <d3d12.h>
#include <string>
#include <strsafe.h>
#include <wrl.h>
#include"FlameWork.h"
#include"BaseScene.h"
#include"SceneManager.h"
#include"AbstractSceneFactory.h"
#include"SkyBox.h"

#pragma comment(lib, "DirectXTex.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")


class Game : public FlameWork {
public:
	void Initialize() override;
	void Update() override;
	void Draw() override;
	void Finalize() override;
	bool IsEndRequest() const override { return endRequest; }	

private:
	std::unique_ptr<D3DResourceLeakCheker> Checker = nullptr;
	std::unique_ptr<Camera> camera = nullptr;
	bool endRequest = false;
	AbstractSceneFactory* sceneFactory = nullptr;

};
