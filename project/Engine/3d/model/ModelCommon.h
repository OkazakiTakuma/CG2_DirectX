#pragma once

class DirectXCommon;

class ModelCommon {
public:
	// モデル描画で共有するリソースを初期化します。
	/// <summary>
	/// 必要なリソースを準備し、オブジェクトを初期化します。
	/// </summary>
	/// <param name="dxCommon">DirectX 共通処理へアクセスするための参照を指定します。</param>
	void Initialize(DirectXCommon* dxCommon);

	// モデル描画用リソースを解放します。
	/// <summary>
	/// 確保したリソースを解放し、終了処理を行います。
	/// </summary>
	void Finalize();

	// モデル描画に使用するパイプラインステートを設定します。
	void SetDraw();

	DirectXCommon* GetDxCommon() { return dxCommon_; }

private:
	DirectXCommon* dxCommon_ = nullptr;
};
