#pragma once
#include "DirectXCommon.h"
#include "Logger.h"
#include "StringUtility.h"
#include "struct.h"
#include"Camera.h"
class Object3dCommon {
public:
	void Initialize(DirectXCommon* dxCommon);
	void SetDraw();
	DirectXCommon* GetDxCommon() const { return dxCommon_; }
	void SetDefaultCamera(Camera* cmr) { defaultCamera = cmr; }
	Camera* GetDefaultCamera() { return defaultCamera; }

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;
	struct VertexData {

		Vector4 position; // xyz座標
		Vector3 normal;   // 法線ベクトル
		Vector2 uv;       // uv座標
	};
	Camera* defaultCamera = nullptr;

	void CreateRootSignature();

	void CreatePipelineState();
};