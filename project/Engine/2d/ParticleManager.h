#pragma once
#include "../../extenals/DirectXTex/DirectXTex.h"
#include "../../extenals/DirectXTex/d3dx12.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "struct.h"
#include <numbers>
#include <random>
#include <string>
#include <vector>


class ParticleManager {
public:
	void Initialize(DirectXCommon* dxCommon, SrvManager* srv); // 初期化時にセット
	static ParticleManager* GetInstance();
	void Finalize();
	void Rerease();

private:
	DirectXCommon* dxCommon_ = nullptr;
	SrvManager* srvManager = nullptr;
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