#pragma once
#include "DirectXTex.h"
#include "d3dx12.h"
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
	static const uint32_t kMaxParticle;
	void CreateParticleGroup(const std::string& groupName, const std::string& textureFilePath);
	void Draw(Camera* camera);
	void Update();
	void SetCamera(Camera* camera) { camera_ = camera; }
	void SetGroupTexture(const std::string& groupName, const std::string& textureFilePath);
	void Emit(const std::string& groupName, const Vector3& position, uint32_t count, const ParticleEmitParam& emitParam);
	void SetGroupBlendMode(const std::string& groupName, BlendMode blendMode);

	struct ParticleGroup {
		MaterialData material;
		std::list<Particle> particles;
		uint32_t instanceSrvIndex = 0;
		Microsoft::WRL::ComPtr<ID3D12Resource> instanceResource;
		uint32_t instanceCount = 0;
		ParticleForGPU* instanceDataPtr;

		// ★追加：このグループがどのブレンドモードで描画されるか（初期値は通常ブレンド）
		BlendMode blendMode = kBlendModeNormal;
	};

private:
	static ParticleManager* instance;
	Camera* camera_ = nullptr;

	DirectXCommon* dxCommon_ = nullptr;
	SrvManager* srvManager_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, kBlendCountblend> graphicsPipelineStates;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;
	std::mt19937 randomEngine_;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	std::vector<VertexData> vertices_;
	float deltaTime = 0;
	void CreateRootSignature();

	void CreatePipelineState();
	
	std::unordered_map<std::string, ParticleGroup> particleGroups_;
};