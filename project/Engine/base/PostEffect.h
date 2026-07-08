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

	void UpdateHotkeys();

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
	void ApplySettingsToBuffer();

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
		int32_t enableVignetting;
		int32_t enableSmoothing;
		int32_t enableGaussianFilter;
		int32_t enableRadialBlur;
		int32_t enableRandom;
		int32_t radialBlurSamples;
		int32_t enableOutline;
		float vignetteIntensity;
		float vignetteRadius;
		float vignetteSoftness;
		float radialBlurStrength;
		float randomStrength;
		float outlineStrength;
		float outlineThreshold;
		float outlineThickness;
		float time;
		float texelSize[2];
		float outlineColor[4];
		float padding;
	};

	ColorData* colorData_ = nullptr;

	float tintColor_[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	bool enableGrayscale_ = false;
	bool enableVignetting_ = false;
	bool enableSmoothing_ = false;
	bool enableGaussianFilter_ = false;
	bool enableRadialBlur_ = false;
	bool enableRandom_ = false;
	bool enableOutline_ = false;
	float vignetteIntensity_ = 0.65f;
	float vignetteRadius_ = 0.0f;
	float vignetteSoftness_ = 0.35f;
	float radialBlurStrength_ = 0.08f;
	int radialBlurSamples_ = 12;
	float randomStrength_ = 0.04f;
	float outlineStrength_ = 1.0f;
	float outlineThreshold_ = 0.12f;
	float outlineThickness_ = 1.0f;
	float outlineColor_[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float time_ = 0.0f;
};
