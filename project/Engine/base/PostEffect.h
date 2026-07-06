#pragma once
#include "DirectXCommon.h"
#include "struct.h"
#include <d3d12.h>
#include <string>
#include <vector>
#include <wrl/client.h>

class PostEffect {
public:
	static PostEffect* GetInstance();

	void Initialize(DirectXCommon* dxCommon);

	void PreDrawScene();

	void PostDrawScene();

	void Draw();

	void Finalize();

	void DrawImGui();

	bool IsActive() const { return isActive_; }

private:
	PostEffect() = default;
	~PostEffect() = default;
	PostEffect(const PostEffect&) = delete;
	PostEffect& operator=(const PostEffect&) = delete;

	void CreateTextureResource();
	void CreateRtv();
	void CreateDsv();
	void CreateSrv();
	void CreateRootSignature();
	void CreatePipelineState();

	void CreateColorBuffer();

private:
	DirectXCommon* dxCommon_ = nullptr;

	bool isActive_ = true;

	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_ = nullptr;

	uint32_t srvIndex_ = 0;
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU_{};

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> colorBuffer_ = nullptr;

	struct ColorData {
		float r, g, b, a;
		int32_t enableGrayscale;
		float padding[3];
	};

	ColorData* colorData_ = nullptr;

	float tintColor_[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
};
