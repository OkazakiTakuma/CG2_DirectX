#pragma once
#include "../../extenals/DirectXTex/DirectXTex.h"
#include "../../extenals/DirectXTex/d3dx12.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "struct.h"
#include "TextureManager.h"
#include <numbers>
#include <random>
#include <string>
#include <vector>
#include <list>
#include <Camera.h>


class Camera;
class ParticleManager {
public:
	void Initialize(DirectXCommon* dxCommon); // 初期化時にセット
	static ParticleManager* GetInstance();
	void Finalize();
	void Rerease();
	static const uint32_t kMaxParticle;
	void CreateParticleGroup(const std::string& groupName, const std::string& textureFilePath);
	void Draw(Camera* camera);
	void Update();
	void SetCamera(Camera* camera) { camera_ = camera; }

	void Emit(const std::string& groupName, const Vector3& position, uint32_t count);
	struct ParticleGroup {
		// --- マテリアル情報 ---
		MaterialData material;

		// --- パーティクル本体 ---
		std::list<Particle> particles; // パーティクルのリスト

		// --- インスタンシング関連 ---
		uint32_t instanceSrvIndex = 0; // インスタンシング用 SRV インデックス

		Microsoft::WRL::ComPtr<ID3D12Resource> instanceResource; // インスタンシング用バッファ
		uint32_t instanceCount = 0;                              // インスタンス数

		// インスタンシングデータを書き込むためのポインタ（Map した先）
		ParticleForGPU* instanceDataPtr;
	};

private:
	static ParticleManager* instance;
	Camera* camera_ = nullptr;

	DirectXCommon* dxCommon_ = nullptr;
	SrvManager* srvManager_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;
	
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;
	std::mt19937 randomEngine_;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	std::vector<VertexData> vertices_;
	float deltaTime = 0;
	void CreateRootSignature();

	void CreatePipelineState();
	
	std::unordered_map<std::string, ParticleGroup> particleGroups_;
};