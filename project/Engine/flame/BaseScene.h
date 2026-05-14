#pragma once
#include "Audio.h"
#include "Camera.h"
#include "ImGuiManager.h"
#include "Input.h"
#include "Matrix.h"
#include "Object3dCommon.h"
#include "ParticleEmitter.h"
#include "ParticleManager.h"
#include "Resource.h"
#include "SkyBox.h"
#include "SkyBoxCommon.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "SrvManager.h"
#include "Vector.h"
#include "struct.h"
#include <Object3d.h>
#include <memory>
#include <vector>
class SceneManager;

class BaseScene {
public:
	virtual void Initialize();
	virtual void Update();
	virtual void DrawSkyBox();
	virtual void Draw2D();
	virtual void Draw3D();
	virtual void Finalize();
	virtual ~BaseScene();

	// シーンマネージャーの所有権を渡すなら unique_ptr を受け取る方が安全です。
	// 既存コードと互換を保つなら引数は SceneManager* のままにできます。
	virtual void SetSceneManager(SceneManager* manager) { sceneManager = manager; }

private:
	SceneManager* sceneManager = nullptr; // シーンマネージャーへのポインタ（所有権は持たない）
};