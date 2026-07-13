#pragma once

#include "BaseScene.h"
class TitleScene: public BaseScene {
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

private:
	/// <summary>
	/// ImGui によるデバッグ用 UI の表示と編集処理を行います。
	/// </summary>
	void ImGuiUpdate();
	SceneManager* sceneManager = nullptr;


};
