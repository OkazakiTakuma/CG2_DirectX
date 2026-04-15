#pragma once
#include "../base/DirectXCommon.h"
#include "../base/Logger.h"
#include "../base/StringUtility.h"
#include "../base/struct.h"
#include <assert.h>
#include <d3d12.h>
#include <wrl.h>

class SpriteCommon {
public:
	// シングルトンインスタンスの取得
	static SpriteCommon* GetInstance();

	// 初期化
	void Initialize(DirectXCommon* dxCommon);

	// 終了処理
	void Finalize();

	// 描画前設定
	void SetDraw();

	// ゲッター
	DirectXCommon* GetDxCommon() const { return dxCommon_; }

	~SpriteCommon() = default;
private:
	// シングルトンのためコンストラクタはprivate
	SpriteCommon() = default;

	// コピーコンストラクタと代入演算子を無効化
	SpriteCommon(const SpriteCommon&) = delete;
	SpriteCommon& operator=(const SpriteCommon&) = delete;

	// 内部初期化関数
	void CreateRootSignature();
	void CreatePipelineState();

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;
};