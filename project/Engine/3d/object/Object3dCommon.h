#pragma once
#include "../camera/Camera.h"
#include "DirectXCommon.h"
#include "Logger.h"
#include "StringUtility.h"
#include "struct.h"
#include <assert.h>
#include <d3d12.h>
#include <wrl.h>

class Object3dCommon {
public:
	/// <summary>
	/// 共有インスタンスを取得します。
	/// </summary>
	/// <returns>処理結果を返します。</returns>
	static Object3dCommon* GetInstance();

	/// <summary>
	/// 必要なリソースを準備し、オブジェクトを初期化します。
	/// </summary>
	/// <param name="dxCommon">DirectX 共通処理へアクセスするための参照を指定します。</param>
	void Initialize(DirectXCommon* dxCommon);
	/// <summary>
	/// 確保したリソースを解放し、終了処理を行います。
	/// </summary>
	void Finalize();
	void ReloadPipelineState() { CreatePipelineState(); }

	/// <summary>
	/// Draw を設定します。
	/// </summary>
	void SetDraw();
	void SetShadowDraw();

	DirectXCommon* GetDxCommon() const { return dxCommon_; }
	void SetDefaultCamera(Camera* cmr) { defaultCamera = cmr; }
	Camera* GetDefaultCamera() { return defaultCamera; }

	~Object3dCommon() = default;
private:
	Object3dCommon() = default;

	Object3dCommon(const Object3dCommon&) = delete;
	Object3dCommon& operator=(const Object3dCommon&) = delete;

	/// <summary>
	/// RootSignature を作成し、利用できる状態にします。
	/// </summary>
	void CreateRootSignature();
	/// <summary>
	/// PipelineState を作成し、利用できる状態にします。
	/// </summary>
	void CreatePipelineState();

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> shadowPipelineState = nullptr;

	Camera* defaultCamera = nullptr;
};
