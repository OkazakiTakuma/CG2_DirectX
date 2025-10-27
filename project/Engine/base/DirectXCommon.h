#pragma once
#include "../3d/Matrix.h"
#include "../3d/Screen.h"
#include "../3d/Vector3.h"
#include "../base/Logger.h"
#include "../base/StringUtility.h"
#include "../base/WinApp.h"
#include "../../extenals/imgui/imgui.h"
#include "../../extenals/imgui/imgui_impl_dx12.h"
#include "../../extenals/imgui/imgui_impl_win32.h"
#include<array>
#include <cassert>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <dxcapi.h>
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

	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);

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

	void CreateImGui();

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
	// DepthStenecilResourceをウィンドウサイズで作成
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStenecilResourceModel;
	// WVP用のリソースを作る。Matrix4x4
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResorce;
	// DepthStenecilResourceをウィンドウサイズで作成
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStenecilResource;
	// リソースの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	// ディスクリプタヒープの生成
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;
	uint32_t descroptorSizeSRV;
	uint32_t descroptorSizeRTV;
	uint32_t descroptorSizeDSV;
	// RTVの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	// スワップチェーンからリソースをもらう
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2> swapChainResources;
	// 初期値0でFenceを生成
	Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;
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
	

	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index);

	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index);
};
