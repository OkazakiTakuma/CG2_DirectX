#pragma once
#include "DirectXCommon.h"
#include <wrl.h>
#include "Camera.h"

class SkyBoxCommon {
public:
	/// <summary>
	/// 共有インスタンスを取得します。
	/// </summary>
	/// <returns>処理結果を返します。</returns>
	static SkyBoxCommon* GetInstance();

	/// <summary>
	/// 必要なリソースを準備し、オブジェクトを初期化します。
	/// </summary>
	/// <param name="dxCommon">DirectX 共通処理へアクセスするための参照を指定します。</param>
	void Initialize(DirectXCommon* dxCommon);
	/// <summary>
	/// Draw を設定します。
	/// </summary>
	void SetDraw();
	/// <summary>
	/// 確保したリソースを解放し、終了処理を行います。
	/// </summary>
	void Finalize();
	void ReloadPipelineState() { CreatePipelineState(); }

	DirectXCommon* GetDxCommon() const { return dxCommon_; }
	~SkyBoxCommon() = default;
	void SetDefaultCamera(Camera* cmr) { defaultCamera = cmr; }
	Camera* GetDefaultCamera() const { return defaultCamera; }

private:
	SkyBoxCommon() = default;
	SkyBoxCommon(const SkyBoxCommon&) = delete;
	SkyBoxCommon& operator=(const SkyBoxCommon&) = delete;

	/// <summary>
	/// RootSignature を作成し、利用できる状態にします。
	/// </summary>
	void CreateRootSignature();
	/// <summary>
	/// PipelineState を作成し、利用できる状態にします。
	/// </summary>
	void CreatePipelineState();

	DirectXCommon* dxCommon_ = nullptr;
	Camera* defaultCamera = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;
};
