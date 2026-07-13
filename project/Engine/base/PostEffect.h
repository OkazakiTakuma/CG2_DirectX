#pragma once
#include "DirectXCommon.h"
#include "struct.h"
#include <d3d12.h>
#include <string>
#include <vector>
#include <wrl/client.h>

class PostEffect {
public:
	/// <summary>
	/// 共有インスタンスを取得します。
	/// </summary>
	/// <returns>処理結果を返します。</returns>
	static PostEffect* GetInstance();

	/// <summary>
	/// 必要なリソースを準備し、オブジェクトを初期化します。
	/// </summary>
	/// <param name="dxCommon">DirectX 共通処理へアクセスするための参照を指定します。</param>
	void Initialize(DirectXCommon* dxCommon);

	/// <summary>
	/// PreDrawScene の処理を行います。
	/// </summary>
	void PreDrawScene();

	/// <summary>
	/// PostDrawScene の処理を行います。
	/// </summary>
	void PostDrawScene();

	/// <summary>
	/// 現在の状態をもとに描画処理を行います。
	/// </summary>
	void Draw();
	void ResizeIfNeeded();

	/// <summary>
	/// 確保したリソースを解放し、終了処理を行います。
	/// </summary>
	void Finalize();

	/// <summary>
	/// ImGui によるデバッグ用 UI の表示と編集処理を行います。
	/// </summary>
	void DrawImGui();

	/// <summary>
	/// UpdateHotkeys の処理を行います。
	/// </summary>
	void UpdateHotkeys();

	bool IsActive() const { return isActive_; }

private:
	PostEffect() = default;
	~PostEffect() = default;
	PostEffect(const PostEffect&) = delete;
	PostEffect& operator=(const PostEffect&) = delete;

	/// <summary>
	/// TextureResource を作成し、利用できる状態にします。
	/// </summary>
	void CreateTextureResource();
	/// <summary>
	/// Rtv を作成し、利用できる状態にします。
	/// </summary>
	void CreateRtv();
	/// <summary>
	/// Dsv を作成し、利用できる状態にします。
	/// </summary>
	void CreateDsv();
	/// <summary>
	/// Srv を作成し、利用できる状態にします。
	/// </summary>
	void CreateSrv();
	void CreateDissolveMask();
	/// <summary>
	/// RootSignature を作成し、利用できる状態にします。
	/// </summary>
	void CreateRootSignature();
	/// <summary>
	/// PipelineState を作成し、利用できる状態にします。
	/// </summary>
	void CreatePipelineState();

	/// <summary>
	/// ColorBuffer を作成し、利用できる状態にします。
	/// </summary>
	void CreateColorBuffer();
	/// <summary>
	/// SettingsToBuffer を現在の状態へ反映します。
	/// </summary>
	void ApplySettingsToBuffer();
	void ResizeResources(int32_t width, int32_t height);

private:
	DirectXCommon* dxCommon_ = nullptr;

	bool isActive_ = true;

	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_ = nullptr;

	uint32_t srvIndex_ = UINT32_MAX;
	uint32_t depthSrvIndex_ = UINT32_MAX;
	uint32_t dissolveMaskSrvIndex_ = UINT32_MAX;
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU_{};
	D3D12_GPU_DESCRIPTOR_HANDLE depthSrvHandleGPU_{};
	D3D12_GPU_DESCRIPTOR_HANDLE dissolveMaskSrvHandleGPU_{};
	int32_t renderWidth_ = 1;
	int32_t renderHeight_ = 1;
	bool sceneTextureReadyAsSrv_ = false;
	bool depthTextureReadyAsSrv_ = false;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> colorBuffer_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> dissolveMaskResource_ = nullptr;

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
		int32_t enableDissolve;
		float vignetteIntensity;
		float vignetteRadius;
		float vignetteSoftness;
		float radialBlurStrength;
		float randomStrength;
		float outlineStrength;
		float outlineThreshold;
		float outlineThickness;
		float dissolveThreshold;
		float dissolveEdgeWidth;
		float time;
		float texelSize[2];
		float paddingTexel[2];
		float outlineColor[4];
		float dissolveEdgeColor[4];
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
	bool enableDissolve_ = false;
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
	float dissolveThreshold_ = 0.5f;
	float dissolveEdgeWidth_ = 0.08f;
	float dissolveEdgeColor_[4] = { 1.0f, 0.45f, 0.05f, 1.0f };
	float time_ = 0.0f;
};
