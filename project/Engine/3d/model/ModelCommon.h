#pragma once

class DirectXCommon;

class ModelCommon {
public:
	// Initializes the shared model rendering resources.
	/// <summary>
	/// 必要なリソースを準備し、オブジェクトを初期化します。
	/// </summary>
	/// <param name="dxCommon">DirectX 共通処理へアクセスするための参照を指定します。</param>
	void Initialize(DirectXCommon* dxCommon);

	// Releases model rendering resources.
	/// <summary>
	/// 確保したリソースを解放し、終了処理を行います。
	/// </summary>
	void Finalize();

	// Sets the pipeline state used for model rendering.
	void SetDraw();

	DirectXCommon* GetDxCommon() { return dxCommon_; }

private:
	DirectXCommon* dxCommon_ = nullptr;
};
