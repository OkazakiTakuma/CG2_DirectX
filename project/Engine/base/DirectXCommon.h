#pragma once
#include "DirectXTex.h"
#include "Matrix.h"
#include "Screen.h"
#include "Vector.h"
#include "Logger.h"
#include "StringUtility.h"
#include "WinApp.h"
#include <array>
#include <cassert>
#include <chrono>
#include <d3d12.h>
#include <dxcapi.h>
#include <dxgi1_6.h>
#include <format>
#include <thread>
#include <wrl.h>
#include <xaudio2.h>
#pragma comment(lib, "xaudio2.lib")

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxcompiler.lib")

/// <summary>
/// Direct3D 12デバイス、スワップチェーン、コマンド実行、描画先リソースのライフサイクルを管理します。
/// エンジン内の描画機能へ共通のDirectX基盤を提供します。
/// </summary>
class DirectXCommon {
public:
	struct TransformationMatrix {
		Matrix4x4 WVP;
		Matrix4x4 world;
	};

	// Creates a descriptor heap for RTV, DSV, or shader-visible descriptors.
	/// <summary>
	/// DescriptorHeap を作成し、利用できる状態にします。
	/// </summary>
	/// <param name="type">type に使用する値を指定します。</param>
	/// <param name="numDescriptors">numDescriptors に使用する値を指定します。</param>
	/// <param name="shaderVisible">shaderVisible に使用する値を指定します。</param>
	/// <returns>処理結果を返します。</returns>
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors, bool shaderVisible);

	/// <summary>
	/// 必要なリソースを準備し、オブジェクトを初期化します。
	/// </summary>
	/// <param name="winApp">ウィンドウ管理オブジェクトを指定します。</param>
	void Initialize(WinApp* winApp);
	/// <summary>
	/// PreDraw の処理を行います。
	/// </summary>
	void PreDraw();
	/// <summary>
	/// PostDraw の処理を行います。
	/// </summary>
	void PostDraw();
	void ResizeIfNeeded();
	int32_t GetRenderWidth() const { return renderWidth_; }
	int32_t GetRenderHeight() const { return renderHeight_; }
	/// <summary>
	/// 指定された HLSL シェーダーをコンパイルします。
	/// </summary>
	/// <param name="filepath">読み込みまたは保存に使用するファイルパスを指定します。</param>
	/// <param name="profile">profile に使用する値を指定します。</param>
	/// <returns>処理結果を返します。</returns>
	Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(const std::wstring& filepath, const wchar_t* profile);
	Microsoft::WRL::ComPtr<IDxcBlob> TryCompileShader(const std::wstring& filepath, const wchar_t* profile, std::string& errorMessage);
	void FlushGPU() { WaitForGPU(); }
	/// <summary>
	/// BufferResource を作成し、利用できる状態にします。
	/// </summary>
	/// <param name="sizeInBytes">sizeInBytes に使用する値を指定します。</param>
	/// <returns>処理結果を返します。</returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInBytes);
	/// <summary>
	/// TextureResource を作成し、利用できる状態にします。
	/// </summary>
	/// <param name="metaData">metaData に使用する値を指定します。</param>
	/// <returns>処理結果を返します。</returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metaData);

	Microsoft::WRL::ComPtr<ID3D12Device> GetDevice() { return device.Get(); }
	IDxcCompiler3* GetDxcCompiler() { return dxcCompiler.Get(); }
	IDxcUtils* GetDxcUtils() { return dxcUtils.Get(); }
	IDxcIncludeHandler* GetIncludeHandler() { return includeHandler.Get(); }
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandList() { return commandList.Get(); }
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> GetCommandAllocator() { return commandAllocator.Get(); }
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetCommandQueue() { return commandQueue.Get(); }
	size_t GetSwapChainResourceCount() const { return _countof(swapChainResources); }


	/// <summary>
	/// UploadTextureData の処理を行います。
	/// </summary>
	/// <param name="texture">texture に使用する値を指定します。</param>
	/// <param name="mipImages">mipImages に使用する値を指定します。</param>
	void UploadTextureData(Microsoft::WRL::ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage& mipImages);

	/// <summary>
	/// CPUDescriptorHandle を取得します。
	/// </summary>
	/// <param name="descriptorHeap">descriptorHeap に使用する値を指定します。</param>
	/// <param name="descriptorSize">descriptorSize に使用する値を指定します。</param>
	/// <param name="index">対象要素のインデックスを指定します。</param>
	/// <returns>処理結果を返します。</returns>
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index);

	/// <summary>
	/// GPUDescriptorHandle を取得します。
	/// </summary>
	/// <param name="descriptorHeap">descriptorHeap に使用する値を指定します。</param>
	/// <param name="descriptorSize">descriptorSize に使用する値を指定します。</param>
	/// <param name="index">対象要素のインデックスを指定します。</param>
	/// <returns>処理結果を返します。</returns>
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index);

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetRTVDescriptorHeap() { return rtvDescriptorHeap; }
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetDSVDescriptorHeap() { return dsvDescriptorHeap; }
	/// <summary>
	/// Release の処理を行います。
	/// </summary>
	void Release();

	/// <summary>
	/// RenderTextureResource を作成し、利用できる状態にします。
	/// </summary>
	/// <param name="device">device に使用する値を指定します。</param>
	/// <param name="width">幅を指定します。</param>
	/// <param name="height">高さを指定します。</param>
	/// <param name="format">format に使用する値を指定します。</param>
	/// <param name="color">色を指定します。</param>
	/// <returns>処理結果を返します。</returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateRenderTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, int32_t width, int32_t height, DXGI_FORMAT format, const Vector4 color);

private:
	/// <summary>
	/// Device を作成し、利用できる状態にします。
	/// </summary>
	void CreateDevice();

	/// <summary>
	/// Command を作成し、利用できる状態にします。
	/// </summary>
	void CreateCommand();

	/// <summary>
	/// SwapChain を作成し、利用できる状態にします。
	/// </summary>
	void CreateSwapChain();

	/// <summary>
	/// DepthBuffer を作成し、利用できる状態にします。
	/// </summary>
	void CreateDepthBuffer();

	/// <summary>
	/// DescriptorHeap を作成し、利用できる状態にします。
	/// </summary>
	void CreateDescriptorHeap();


	/// <summary>
	/// RenderTargetView を作成し、利用できる状態にします。
	/// </summary>
	void CreateRenderTargetView();

	/// <summary>
	/// DepthStencilView を作成し、利用できる状態にします。
	/// </summary>
	void CreateDepthStencilView();

	/// <summary>
	/// Fence を作成し、利用できる状態にします。
	/// </summary>
	void CreateFence();

	/// <summary>
	/// ViewportRect を作成し、利用できる状態にします。
	/// </summary>
	void CreateViewportRect();

	/// <summary>
	/// ScissorRect を作成し、利用できる状態にします。
	/// </summary>
	void CreateScissorRect();
	void WaitForGPU();
	void ResizeBackBuffers(int32_t width, int32_t height);

	/// <summary>
	/// DXCompiler を作成し、利用できる状態にします。
	/// </summary>
	void CreateDXCompiler();

	/// <summary>
	/// XAudio2 を作成し、利用できる状態にします。
	/// </summary>
	void CreateXAudio2();

	/// <summary>
	/// InitializeFixFPS の処理を行います。
	/// </summary>
	void InitializeFixFPS();

	/// <summary>
	/// UpdateFixFPS の処理を行います。
	/// </summary>
	void UpdateFixFPS();


	WinApp* winApp = nullptr;

	Microsoft::WRL::ComPtr<IDXGIFactory6> dxgiFactory = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Device> device = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResorceModel;
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResorce;
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;
	uint32_t descroptorSizeRTV;
	uint32_t descroptorSizeDSV;
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources[2] = {nullptr};
	Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;
	uint16_t fenceValue = 0;
	HANDLE fenceEvent = nullptr;
	D3D12_VIEWPORT viewport{};
	D3D12_RECT scissorRect{};
	int32_t renderWidth_ = WinApp::kClientWidth;
	int32_t renderHeight_ = WinApp::kClientHeight;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler = nullptr;
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils = nullptr;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;

	std::chrono::steady_clock::time_point reference_;
	Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
	IXAudio2MasteringVoice* masteringVoice = nullptr;
};
