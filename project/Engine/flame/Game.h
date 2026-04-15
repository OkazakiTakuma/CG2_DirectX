#pragma once
#include "Audio.h"
#include "Camera.h"
#include "D3DResouceLeakCheker.h"
#include "DirectXCommon.h"
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
#include"FlameWork.h"

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
	bool IsEnd() const override { return endRequest; }	

private:
	std::unique_ptr<D3DResourceLeakCheker> Checker = nullptr;
	std::unique_ptr<ImGuiManager> imguiManager = nullptr;
	std::unique_ptr<Camera> camera = nullptr;
	std::unique_ptr<Sprite> sprite = nullptr;
	std::vector<std::unique_ptr<Sprite>> sprites;
	std::unique_ptr<Object3d> object3d = nullptr;
	std::vector<std::unique_ptr<Object3d>> axisObjects;
	std::unique_ptr<ParticleEmitter> smokeEmitter = nullptr;
	Vector3 cameraPosition;
	Vector3 cameraRotate;

	Transform trsprite;
	Transform trspriteUV;
	Vector4 spriteColor;
	Vector2 spriteSize;
	Vector2 anchor;
	bool isFlipX;
	bool isFlipY;
	Vector2 textureLeftTop;
	Vector2 textureSize;
	// モデルのパラメータ調整
	Vector3 modelPosition;
	Vector3 modelRotate;
	Vector3 modelScale;
	void ImGuiUpdate();
	bool endRequest = false;
};
