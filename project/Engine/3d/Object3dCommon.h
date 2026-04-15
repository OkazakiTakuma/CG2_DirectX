#pragma once
#include "Camera.h"
#include "DirectXCommon.h"
#include "Logger.h"
#include "StringUtility.h"
#include "struct.h"
#include <assert.h>
#include <d3d12.h>
#include <wrl.h>

class Object3dCommon {
public:
	// シングルトンインスタンスの取得
	static Object3dCommon* GetInstance();

	// 初期化・終了処理
	void Initialize(DirectXCommon* dxCommon);
	void Finalize();

	// 描画設定
	void SetDraw();

	// ゲッター・セッター
	DirectXCommon* GetDxCommon() const { return dxCommon_; }
	void SetDefaultCamera(Camera* cmr) { defaultCamera = cmr; }
	Camera* GetDefaultCamera() { return defaultCamera; }

	~Object3dCommon() = default;
private:
	// シングルトンのためコンストラクタはprivate
	Object3dCommon() = default;

	// コピー禁止
	Object3dCommon(const Object3dCommon&) = delete;
	Object3dCommon& operator=(const Object3dCommon&) = delete;

	void CreateRootSignature();
	void CreatePipelineState();

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;

	// クラス内で定義されている頂点データ構造体
	struct VertexData {
		Vector4 position; // xyz座標
		Vector3 normal;   // 法線ベクトル
		Vector2 uv;       // uv座標
	};

	Camera* defaultCamera = nullptr;
};