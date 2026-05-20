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

class DirectXCommon {
public:
	struct TransformationMatrix {
		Matrix4x4 WVP;
		Matrix4x4 world;
	};

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors, bool shaderVisible);

	void Initialize(WinApp* winApp);

	Microsoft::WRL::ComPtr<ID3D12Device> GetDevice() { return device.Get(); }
	IDxcCompiler3* GetDxcCompiler() { return dxcCompiler; }
	IDxcUtils* GetDxcUtils() { return dxcUtils; }
	IDxcIncludeHandler* GetIncludeHandler() { return includeHandler; }
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandList() { return commandList.Get(); }
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> GetCommandAllocator() { return commandAllocator.Get(); }
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetCommandQueue() { return commandQueue.Get(); }
	size_t GetSwapChainResourceCount() const { return _countof(swapChainResources); }
	// 描画前処理
	void PreDraw();

	// 描画後処理
	void PostDraw();

	Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(const std::wstring& filepath, const wchar_t* profile);

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInBytes);

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metaData);


	void UploadTextureData(Microsoft::WRL::ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage& mipImages);

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStenecilTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, int32_t width, int32_t height);

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index);

	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index);

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetRTVDescriptorHeap() { return rtvDescriptorHeap; }
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetDSVDescriptorHeap() { return dsvDescriptorHeap; }
	void Release();

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateRenderTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, int32_t width, int32_t height, DXGI_FORMAT format, const Vector4 color);

private:
	// D3D12デバイスの生成
	void CreateDevice();

	void CreateCommand();

	void CreateSwapChain();

	void CreateDepthBuffer();

	void CreateDescriptorHeap();


	void CreateRenderTargetView();

	void CreateDepthStencilView();

	void CreateFence();

	void CreateViewportRect();

	void CreateScissorRect();

	void CreateDXCompiler();

	void CreateXAudio2();

	void InitializeFixFPS();

	void UpdateFixFPS();


	WinApp* winApp = nullptr;

	// DXGIファクトリーの生成
	Microsoft::WRL::ComPtr<IDXGIFactory6> dxgiFactory = nullptr;
	// D3D12デバイスの生成
	Microsoft::WRL::ComPtr<ID3D12Device> device = nullptr;
	// コマンドアロケーターの生成
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator = nullptr;
	// コマンドキューの生成
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue = nullptr;
	// コマンドリストの生成
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;
	// スワップチェーンの生成
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	// WVP用のリソースを作る。Matrix4x4
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResorceModel;
	// WVP用のリソースを作る。Matrix4x4
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResorce;
	// DepthStenecilResourceをウィンドウサイズで作成
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStenecilResource;
	// リソースの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource = nullptr;
	// ディスクリプタヒープの生成
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;
	uint32_t descroptorSizeRTV;
	uint32_t descroptorSizeDSV;
	// RTVの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	// スワップチェーンからリソースをもらう
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources[2] = {nullptr};
	// 初期値0でFenceを生成
	Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;
	uint16_t fenceValue = 0;
	// FenceのSignal用のイベントハンドルを生成
	HANDLE fenceEvent;
	// ビューポート
	D3D12_VIEWPORT viewport{};
	// シザー矩形
	D3D12_RECT scissorRect{};
	// dxcCompilerの初期化
	IDxcCompiler3* dxcCompiler = nullptr;
	// dxcUtilsの初期化
	IDxcUtils* dxcUtils = nullptr;
	// インクルードハンドラの生成
	IDxcIncludeHandler* includeHandler = nullptr;
	// RTVを2つ作るからディスクリプタも2つ
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;

	// FPS制限用
	std::chrono::steady_clock::time_point reference_;
	Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
	IXAudio2MasteringVoice* masteringVoice;
};
