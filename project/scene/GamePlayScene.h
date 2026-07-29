#pragma once
#include "BaseScene.h"
#include "GameObject.h"
#include "object/Object3dComponent.h"
#include "particle/ParticleEmitterComponent.h"
#include "SpriteComponent.h"


class GamePlayScene: public BaseScene {
public:
	/// <summary>
	/// 必要なリソースを準備し、オブジェクトを初期化します。
	/// </summary>
	void Initialize() override;
	/// <summary>
	/// 毎フレームの状態更新を行います。
	/// </summary>
	void Update() override;
	/// <summary>
	/// スカイボックスの描画処理を行います。
	/// </summary>
	void DrawSkyBox() override;
	/// <summary>
	/// 2D 要素の描画処理を行います。
	/// </summary>
	void Draw2D() override;
	/// <summary>
	/// 3D 要素の描画処理を行います。
	/// </summary>
	void Draw3D() override;
	/// <summary>
	/// 確保したリソースを解放し、終了処理を行います。
	/// </summary>
	void Finalize() override;
	void SetSceneManager(SceneManager* manager) override { sceneManager = manager; }
	bool IsParticleRenderingEnabled() const override { return isShowParticles_; }

private:
	bool isShowSkyBox_ = false;
	bool isShowSprite_ = true;
	bool isShowSprites_ = true;
	bool isShowObject3D_ = true;
	bool isShowInstancing_ = true;
	bool isShowSphere_ = true;
	bool isShowCylinder_ = true;
	bool isShowParticles_ = true;
	std::unique_ptr<Audio> audio_ = nullptr;
	std::unique_ptr<GameObject> spriteObject_ = nullptr;
	SpriteComponent* sprite = nullptr;

	std::vector<std::unique_ptr<GameObject>> spriteObjects_;
	std::vector<SpriteComponent*> sprites;
	std::unique_ptr<GameObject> object3dObject_ = nullptr;
	Object3dComponent* object3d = nullptr;
	std::unique_ptr<GameObject> sphereGameObject_ = nullptr;
	Object3dComponent* sphereObject = nullptr;
	std::unique_ptr<GameObject> cylinderGameObject_ = nullptr;
	Object3dComponent* cylinderObject = nullptr;

	std::vector<std::unique_ptr<GameObject>> emitterObjects_;
	std::vector<ParticleEmitterComponent*> emitters_;
	int currentParticleIndex_ = 0;
	std::vector<std::string> availableTextures_;
	Vector3 cameraPosition={0.0f, 0.0f, 0.0f};
	Vector3 cameraRotate={0.0f, 0.0f, 0.0f};

	EulerTransform trsprite={};
	EulerTransform trspriteUV={};
	Vector4 spriteColor={};
	Vector2 spriteSize={};
	Vector2 anchor={0.0f, 0.0f};
	bool isFlipX = false;
	bool isFlipY = false;
	Vector2 textureLeftTop={0.0f, 0.0f};
	Vector2 textureSize={0.0f, 0.0f};
	Vector3 modelPosition={0.0f, 0.0f, 0.0f};
	Vector3 modelRotate={0.0f, 0.0f, 0.0f};
	Vector3 modelScale={1.0f, 1.0f, 1.0f};
	/// <summary>
	/// SceneModels を読み込み、内部データへ反映します。
	/// </summary>
	void LoadSceneModels();
	/// <summary>
	/// ImGui によるデバッグ用 UI の表示と編集処理を行います。
	/// </summary>
	void ImGuiUpdate();
	Vector4 lightColor = {1.0f, 1.0f, 1.0f, 1.0f};
	Vector3 lightDirection = {0.0f, -1.0f, 0.0f};
	float lightIntensity = 1.0f;	
	bool isParticleEmit = false;
	SceneManager* sceneManager = nullptr;
	std::unique_ptr<SkyBox> skyBox = nullptr;
	std::unique_ptr<InstancingModel> instancingModel_ = nullptr;

};
