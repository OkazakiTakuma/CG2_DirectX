#pragma once
#include "../base/DirectXCommon.h"
#include "../base/Logger.h"
#include "../base/StringUtility.h"
#include "../base/struct.h"
class Object3dCommon {
public:
	void Initialize(DirectXCommon* dxCommon);
	void SetDraw();
	DirectXCommon* GetDxCommon() const { return dxCommon_; }

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;
	struct VertexData {

		Vector4 position; // xyz座標
		Vector3 normal;   // 法線ベクトル
		Vector2 uv;       // uv座標
	};

	void CreateRootSignature();

	void CreatePipelineState();
};