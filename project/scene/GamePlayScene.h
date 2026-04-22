#pragma once
#include "Audio.h"
#include "Camera.h"
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
#include <Object3d.h>
#include "Object3dCommon.h"
#include"BaseScene.h"
#include <memory>
#include <vector>

class GamePlayScene: public BaseScene {
public:
	void Initialize() override;
	void Update() override;
	void Draw2D() override;
	void Draw3D() override;
	void Finalize() override;
	void SetSceneManager(SceneManager* manager) override { sceneManager = manager; }

private:
	std::unique_ptr<Sprite> sprite = nullptr;

	std::vector<std::unique_ptr<Sprite>> sprites;
	std::unique_ptr<Object3d> object3d = nullptr;
	std::vector<std::unique_ptr<Object3d>> axisObjects;
	std::unique_ptr<ParticleEmitter> smokeEmitter = nullptr;
	Vector3 cameraPosition={0.0f, 0.0f, 0.0f};
	Vector3 cameraRotate={0.0f, 0.0f, 0.0f};

	Transform trsprite={};
	Transform trspriteUV={};
	Vector4 spriteColor={};
	Vector2 spriteSize={};
	Vector2 anchor={0.0f, 0.0f};
	bool isFlipX = false;
	bool isFlipY = false;
	Vector2 textureLeftTop={0.0f, 0.0f};
	Vector2 textureSize={0.0f, 0.0f};
	// モデルのパラメータ調整
	Vector3 modelPosition={0.0f, 0.0f, 0.0f};
	Vector3 modelRotate={0.0f, 0.0f, 0.0f};
	Vector3 modelScale={1.0f, 1.0f, 1.0f};
	void ImGuiUpdate();
	Vector4 lightColor = {1.0f, 1.0f, 1.0f, 1.0f};
	Vector3 lightDirection = {0.0f, -1.0f, 0.0f};
	float lightIntensity = 1.0f;	
	bool isParticleEmit = false;          // パーティクル発生のON/OFF
	SceneManager* sceneManager = nullptr; // シーンマネージャーへのポインタ（所有権は持たない）
};
