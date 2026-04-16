#pragma once
#include <memory>

class SceneManager;

class BaseScene {
public:
	virtual void Initialize();
	virtual void Update();
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