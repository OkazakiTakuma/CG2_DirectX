#pragma once

#include "../base/DirectXCommon.h"
#include "../base/Logger.h"
#include "../base/StringUtility.h"
#include "../base/struct.h"

#include <array>
#include <assert.h>
#include <d3d12.h>
#include <wrl.h>

class SpriteCommon {
public:
	/// <summary>
	/// 共有インスタンスを取得します。
	/// </summary>
	static SpriteCommon* GetInstance();

	// Prepares the sprite rendering pipeline with shared DirectX resources.
	/// <summary>
	/// 必要なリソースを準備し、オブジェクトを初期化します。
	/// </summary>
	/// <param name="dxCommon">DirectX 共通処理へアクセスするための参照を指定します。</param>
	void Initialize(DirectXCommon* dxCommon);

	// Releases GPU-side pipeline objects.
	/// <summary>
	/// 確保したリソースを解放し、終了処理を行います。
	/// </summary>
	void Finalize();
	void ReloadPipelineState() { CreatePipelineState(); }

	// Sets the pipeline state used for sprite rendering.
	/// <param name="blendMode">描画時に使用するブレンドモードを指定します。</param>
	void SetDraw(uint32_t blendMode = kBlendModeNormal);

	DirectXCommon* GetDxCommon() const { return dxCommon_; }

	~SpriteCommon() = default;

private:
	SpriteCommon() = default;

	SpriteCommon(const SpriteCommon&) = delete;
	SpriteCommon& operator=(const SpriteCommon&) = delete;

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
	// Pipeline states are cached by blend mode.
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, kBlendCountblend> graphicsPipelineStates;
};
