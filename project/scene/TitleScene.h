#pragma once
#include "Audio.h"
#include "Camera.h"
#include "D3DResouceLeakCheker.h"
#include "ImGuiManager.h"
#include "Input.h"
#include "Matrix.h"
#include "Object3dCommon.h"
#include "ParticleEmitter.h"
#include "ParticleManager.h"
#include "Resource.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "SrvManager.h"
#include "Vector.h"
#include "struct.h"
#include <Object3d.h>
#include "BaseScene.h"
class TitleScene: public BaseScene {
public:
	void Initialize() override;
	void Update() override;
	void Draw2D() override;
	void Draw3D() override;
	void Finalize() override;
	void SetSceneManager(SceneManager* manager) override { sceneManager = manager; }

private:
	void ImGuiUpdate();
	SceneManager* sceneManager = nullptr; // シーンマネージャーへのポインタ（所有権は持たない）


};