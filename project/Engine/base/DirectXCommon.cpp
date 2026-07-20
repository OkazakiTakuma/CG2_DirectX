#include "DirectXCommon.h"
#include"SrvManager.h"

using namespace Logger;
using namespace StringUtility;



/// <summary>
/// DescriptorHeap を作成し、利用できる状態にします。
/// </summary>
/// <param name="type">type に使用する値を指定します。</param>
/// <param name="numDescriptors">numDescriptors に使用する値を指定します。</param>
/// <param name="shaderVisible">shaderVisible に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DirectXCommon::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors, bool shaderVisible) {
	assert(device != nullptr);
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Type = type;
	heapDesc.NumDescriptors = numDescriptors;
	heapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorHeap));
	assert(SUCCEEDED(hr));
	return descriptorHeap;
}

/// <summary>
/// 必要なリソースを準備し、オブジェクトを初期化します。
/// </summary>
/// <param name="winApp">ウィンドウ管理オブジェクトを指定します。</param>
void DirectXCommon::Initialize(WinApp* winApp) {
	assert(winApp);
	this->winApp = winApp;
	winApp->UpdateClientSize();
	renderWidth_ = winApp->GetClientWidth();
	renderHeight_ = winApp->GetClientHeight();
	InitializeFixFPS();
	CreateDevice();
	CreateCommand();
	CreateSwapChain();
	CreateDepthBuffer();
	CreateDescriptorHeap();
	CreateRenderTargetView();
	CreateDepthStencilView();
	CreateFence();
	CreateViewportRect();
	CreateScissorRect();
	CreateDXCompiler();
}



/// <summary>
/// PreDraw の処理を行います。
/// </summary>
void DirectXCommon::PreDraw() {
	ResizeIfNeeded();
	dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

	D3D12_RESOURCE_BARRIER barrier{};

	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	commandList->ResourceBarrier(1, &barrier);
	const float clearColor[4] = {0.1f, 0.25f, 0.5f, 1.0f};
	commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);
	commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);


}

/// <summary>
/// PostDraw の処理を行います。
/// </summary>
void DirectXCommon::PostDraw() {
	HRESULT hr;
	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();
	D3D12_RESOURCE_BARRIER barrier{};

	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	commandList->ResourceBarrier(1, &barrier);

	hr = commandList->Close();
	assert(SUCCEEDED(hr));
	ID3D12CommandList* commandLists[] = {commandList.Get()};
	commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
	swapChain->Present(1, 0);

	WaitForGPU();
	UpdateFixFPS();

	hr = commandAllocator->Reset();
	assert(SUCCEEDED(hr));
	hr = commandList->Reset(commandAllocator.Get(), nullptr);
	assert(SUCCEEDED(hr));
}

void DirectXCommon::WaitForGPU() {
	if (!commandQueue || !fence) {
		return;
	}

	fenceValue++;
	commandQueue->Signal(fence.Get(), fenceValue);
	if (fence->GetCompletedValue() < fenceValue) {
		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}
}

void DirectXCommon::ResizeIfNeeded() {
	if (!winApp || !swapChain) {
		return;
	}

	winApp->UpdateClientSize();
	const int32_t width = winApp->GetClientWidth();
	const int32_t height = winApp->GetClientHeight();
	if (width == renderWidth_ && height == renderHeight_) {
		return;
	}

	ResizeBackBuffers(width, height);
}

void DirectXCommon::ResizeBackBuffers(int32_t width, int32_t height) {
	if (width <= 0 || height <= 0) {
		return;
	}

	WaitForGPU();

	for (Microsoft::WRL::ComPtr<ID3D12Resource>& swapChainResource : swapChainResources) {
		swapChainResource.Reset();
	}
	depthStencilResource.Reset();

	HRESULT hr = swapChain->ResizeBuffers(
	    static_cast<UINT>(_countof(swapChainResources)),
	    static_cast<UINT>(width),
	    static_cast<UINT>(height),
	    swapChainDesc.Format,
	    swapChainDesc.Flags
	);
	assert(SUCCEEDED(hr));

	renderWidth_ = width;
	renderHeight_ = height;
	swapChainDesc.Width = static_cast<UINT>(width);
	swapChainDesc.Height = static_cast<UINT>(height);

	CreateRenderTargetView();
	CreateDepthBuffer();
	CreateDepthStencilView();
	CreateViewportRect();
	CreateScissorRect();
}

/// <summary>
/// 指定された HLSL シェーダーをコンパイルします。
/// </summary>
/// <param name="filepath">読み込みまたは保存に使用するファイルパスを指定します。</param>
/// <param name="profile">profile に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
Microsoft::WRL::ComPtr<IDxcBlob> DirectXCommon::CompileShader(const std::wstring& filepath, const wchar_t* profile) {
	std::string errorMessage;
	auto shaderBlob = TryCompileShader(filepath, profile, errorMessage);
	if (!shaderBlob) {
		Log(errorMessage);
		assert(false);
	}
	return shaderBlob;
}

Microsoft::WRL::ComPtr<IDxcBlob> DirectXCommon::TryCompileShader(const std::wstring& filepath, const wchar_t* profile, std::string& errorMessage) {
	errorMessage.clear();
	Log(ConvertString(std::format(L"Bigin CompileShader, path:{},profile:{}\n", filepath, profile)));
	Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filepath.c_str(), nullptr, &shaderSource);
	if (FAILED(hr) || !shaderSource) {
		errorMessage = "Could not load shader: " + ConvertString(filepath) + "\n";
		return nullptr;
	}
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;

	LPCWSTR arguments[] = {
	    filepath.c_str(),
	    L"-E",
	    L"main",
	    L"-T",
	    profile,
	    L"-Zi",
#ifdef DEBUG
	    L"-Qembed_debug",
	    L"-Od",
#endif // DEBUG
	    L"-Zpr"};
	Microsoft::WRL::ComPtr<IDxcResult> shaderResult = nullptr;

	hr = dxcCompiler->Compile(
	    &shaderSourceBuffer,
	    arguments,
	    _countof(arguments),
	    includeHandler.Get(),
	    IID_PPV_ARGS(&shaderResult)
	);
	if (FAILED(hr) || !shaderResult) {
		errorMessage = "DXC failed to compile: " + ConvertString(filepath) + "\n";
		return nullptr;
	}
	Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError = nullptr;
	Microsoft::WRL::ComPtr<IDxcBlobWide> dummyName = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), &dummyName);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
		errorMessage.assign(shaderError->GetStringPointer(), shaderError->GetStringLength());
		Log(errorMessage);
	}
	HRESULT compileStatus = E_FAIL;
	shaderResult->GetStatus(&compileStatus);
	if (FAILED(compileStatus)) {
		return nullptr;
	}
	Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	if (FAILED(hr) || !shaderBlob) {
		errorMessage = "DXC did not return shader bytecode: " + ConvertString(filepath) + "\n";
		return nullptr;
	}
	Log(ConvertString(std::format(L"Complete CompileShader, path:{},profile:{}\n", filepath, profile)));
	return shaderBlob;
}

/// <summary>
/// BufferResource を作成し、利用できる状態にします。
/// </summary>
/// <param name="sizeInBytes">sizeInBytes に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateBufferResource(size_t sizeInBytes) {
	HRESULT hr;
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;

	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeInBytes;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));

	return resource;
}
/// <summary>
/// TextureResource を作成し、利用できる状態にします。
/// </summary>
/// <param name="metaData">metaData に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateTextureResource(const DirectX::TexMetadata& metaData) {
	assert(device != nullptr);

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metaData.width);
	resourceDesc.Height = UINT(metaData.height);
	resourceDesc.MipLevels = UINT16(metaData.mipLevels);
	resourceDesc.DepthOrArraySize = UINT16(metaData.arraySize);
	resourceDesc.Format = metaData.format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metaData.dimension);

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
	    &heapProperties,
	    D3D12_HEAP_FLAG_NONE,
	    &resourceDesc,
	    D3D12_RESOURCE_STATE_COPY_DEST,
	    nullptr,
	    IID_PPV_ARGS(&resource)
	);

	assert(SUCCEEDED(hr));
	return resource;
}

/// <summary>
/// UploadTextureData の処理を行います。
/// </summary>
/// <param name="texture">texture に使用する値を指定します。</param>
/// <param name="mipImages">mipImages に使用する値を指定します。</param>
void DirectXCommon::UploadTextureData(Microsoft::WRL::ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage& mipImages) {
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();

	for (size_t mipLevel = 0; mipLevel < metadata.mipLevels; ++mipLevel) {
		for (size_t arrayIndex = 0; arrayIndex < metadata.arraySize; ++arrayIndex) {

			const DirectX::Image* img = mipImages.GetImage(mipLevel, arrayIndex, 0);
			assert(img != nullptr);

			UINT subresourceIndex = static_cast<UINT>(mipLevel + (arrayIndex * metadata.mipLevels));

			HRESULT hr = texture->WriteToSubresource(
			    subresourceIndex,
			    nullptr,
			    img->pixels,
			    static_cast<UINT>(img->rowPitch),
			    static_cast<UINT>(img->slicePitch)
			);
			assert(SUCCEEDED(hr));
		}
	}
}
/// <summary>
/// Device を作成し、利用できる状態にします。
/// </summary>
void DirectXCommon::CreateDevice() {
	HRESULT hr;
	hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	assert(SUCCEEDED(hr));
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter = nullptr;

	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i) {
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));
		if ((adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) == 0) {

			std::wstring wdesc(adapterDesc.Description);
			std::string desc = ConvertString(wdesc);
			Log("Use Adapter: " + desc + "\n");
			break;
		}
		useAdapter = nullptr;
	}
	assert(useAdapter != nullptr);

	D3D_FEATURE_LEVEL featureLevel[]{
	    D3D_FEATURE_LEVEL_12_2,
	    D3D_FEATURE_LEVEL_12_1,
	    D3D_FEATURE_LEVEL_12_0,
	};
	const char* featureLevelStrings[] = {
	    "12_2",
	    "12_1",
	    "12_0",
	};
	for (size_t i = 0; i < _countof(featureLevel); i++) {
		hr = D3D12CreateDevice(useAdapter.Get(), featureLevel[i], IID_PPV_ARGS(&device));
		if (SUCCEEDED(hr)) {
			Log(std::string("D3D12 Device created with feature level: ") + featureLevelStrings[i]);
			break;
		}
	}
	assert(device != nullptr);
	Log("Comlete create D3D12 Device\n");

#ifdef Debug

	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		D3D12_MESSAGE_ID denyIds[] = {D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE};
		D3D12_MESSAGE_SEVERITY severities[] = {D3D12_MESSAGE_SEVERITY_INFO};
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		infoQueue->PushStorageFilter(&filter);
	}
#endif // Debug
}

/// <summary>
/// Command を作成し、利用できる状態にします。
/// </summary>
void DirectXCommon::CreateCommand() {
	HRESULT hr;
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	assert(SUCCEEDED(hr));

	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
	assert(SUCCEEDED(hr));

	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
	assert(SUCCEEDED(hr));
}

/// <summary>
/// SwapChain を作成し、利用できる状態にします。
/// </summary>
void DirectXCommon::CreateSwapChain() {
	HRESULT hr;
	swapChainDesc.Width = renderWidth_;
	swapChainDesc.Height = renderHeight_;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1 = nullptr;
	hr = dxgiFactory->CreateSwapChainForHwnd(
	    commandQueue.Get(),
	    winApp->GetHwnd(),
	    &swapChainDesc,
	    nullptr,
	    nullptr,
	    swapChain1.GetAddressOf()
	);
	assert(SUCCEEDED(hr));

	hr = swapChain1.As(&swapChain);
}

/// <summary>
/// DepthBuffer を作成し、利用できる状態にします。
/// </summary>
void DirectXCommon::CreateDepthBuffer() {
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = renderWidth_;
	resourceDesc.Height = renderHeight_;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	HRESULT hr = device->CreateCommittedResource(
	    &heapProperties,
	    D3D12_HEAP_FLAG_NONE,
	    &resourceDesc,
	    D3D12_RESOURCE_STATE_DEPTH_WRITE,
	    &depthClearValue,
	    IID_PPV_ARGS(&depthStencilResource));
	assert(SUCCEEDED(hr));
}

/// <summary>
/// DescriptorHeap を作成し、利用できる状態にします。
/// </summary>
void DirectXCommon::CreateDescriptorHeap() {
	descroptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	descroptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	rtvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	dsvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
}

/// <summary>
/// RenderTargetView を作成し、利用できる状態にします。
/// </summary>
void DirectXCommon::CreateRenderTargetView() {
	HRESULT hr;
	swapChainResources[0].Reset();
	swapChainResources[1].Reset();
	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
	assert(SUCCEEDED(hr));
	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
	assert(SUCCEEDED(hr));

	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	rtvHandles[0] = rtvStartHandle;
	device->CreateRenderTargetView(swapChainResources->GetAddressOf()[0], &rtvDesc, rtvHandles[0]);
	rtvHandles[1] = {rtvStartHandle.ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)};
	device->CreateRenderTargetView(swapChainResources->GetAddressOf()[1], &rtvDesc, rtvHandles[1]);
}

/// <summary>
/// CPUDescriptorHandle を取得します。
/// </summary>
/// <param name="descriptorHeap">descriptorHeap に使用する値を指定します。</param>
/// <param name="descriptorSize">descriptorSize に使用する値を指定します。</param>
/// <param name="index">対象要素のインデックスを指定します。</param>
/// <returns>処理結果を返します。</returns>
D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

/// <summary>
/// GPUDescriptorHandle を取得します。
/// </summary>
/// <param name="descriptorHeap">descriptorHeap に使用する値を指定します。</param>
/// <param name="descriptorSize">descriptorSize に使用する値を指定します。</param>
/// <param name="index">対象要素のインデックスを指定します。</param>
/// <returns>処理結果を返します。</returns>
D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}

/// <summary>
/// Release の処理を行います。
/// </summary>
void DirectXCommon::Release() {
	if (commandQueue && fence) {
		fenceValue++;
		commandQueue->Signal(fence.Get(), fenceValue);
		if (fence->GetCompletedValue() < fenceValue && fenceEvent) {
			fence->SetEventOnCompletion(fenceValue, fenceEvent);
			WaitForSingleObject(fenceEvent, INFINITE);
		}
	}

	if (fenceEvent) {
		CloseHandle(fenceEvent);
		fenceEvent = nullptr;
	}

	xAudio2.Reset();
	includeHandler.Reset();
	dxcCompiler.Reset();
	dxcUtils.Reset();
	fence.Reset();
	commandList.Reset();
	commandAllocator.Reset();
	commandQueue.Reset();

	for (Microsoft::WRL::ComPtr<ID3D12Resource>& swapChainResource : swapChainResources) {
		swapChainResource.Reset();
	}
	swapChain.Reset();

	depthStencilResource.Reset();
	wvpResorce.Reset();
	wvpResorceModel.Reset();
	rtvDescriptorHeap.Reset();
	dsvDescriptorHeap.Reset();
	device.Reset();
	dxgiFactory.Reset();
	winApp = nullptr;
}

/// <summary>
/// RenderTextureResource を作成し、利用できる状態にします。
/// </summary>
/// <param name="device">device に使用する値を指定します。</param>
/// <param name="width">幅を指定します。</param>
/// <param name="height">高さを指定します。</param>
/// <param name="format">format に使用する値を指定します。</param>
/// <param name="color">色を指定します。</param>
/// <returns>処理結果を返します。</returns>
Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateRenderTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, int32_t width, int32_t height, DXGI_FORMAT format, const Vector4 color) {
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = format;
	clearValue.Color[0] = color.x;
	clearValue.Color[1] = color.y;
	clearValue.Color[2] = color.z;
	clearValue.Color[3] = color.w;

	Microsoft::WRL::ComPtr<ID3D12Resource> renderTexture;
	HRESULT hr = device->CreateCommittedResource(
	    &heapProperties,
	    D3D12_HEAP_FLAG_NONE,
	    &resourceDesc,
	    D3D12_RESOURCE_STATE_RENDER_TARGET,
	    &clearValue,
	    IID_PPV_ARGS(&renderTexture));
	assert(SUCCEEDED(hr));

	return renderTexture;
}

/// <summary>
/// DepthStencilView を作成し、利用できる状態にします。
/// </summary>
void DirectXCommon::CreateDepthStencilView() {
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	device->CreateDepthStencilView(depthStencilResource.Get(), &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

/// <summary>
/// Fence を作成し、利用できる状態にします。
/// </summary>
void DirectXCommon::CreateFence() {
	HRESULT hr;
	uint16_t fenceValue = 0;
	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	fenceEvent = CreateEvent(nullptr, false, false, nullptr);
	assert(fenceEvent != nullptr);
}

/// <summary>
/// ViewportRect を作成し、利用できる状態にします。
/// </summary>
void DirectXCommon::CreateViewportRect() {

	viewport.Width = static_cast<float>(renderWidth_);
	viewport.Height = static_cast<float>(renderHeight_);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
}

/// <summary>
/// ScissorRect を作成し、利用できる状態にします。
/// </summary>
void DirectXCommon::CreateScissorRect() {
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = renderWidth_;
	scissorRect.bottom = renderHeight_;
}

/// <summary>
/// DXCompiler を作成し、利用できる状態にします。
/// </summary>
void DirectXCommon::CreateDXCompiler() {
	HRESULT hr;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));
}

/// <summary>
/// XAudio2 を作成し、利用できる状態にします。
/// </summary>
void DirectXCommon::CreateXAudio2() {
	HRESULT hr;
	hr = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	assert(SUCCEEDED(hr));
	hr = xAudio2->CreateMasteringVoice(&masteringVoice);
	assert(SUCCEEDED(hr));
}


void DirectXCommon::InitializeFixFPS() { reference_ = std::chrono::high_resolution_clock::now(); }

/// <summary>
/// UpdateFixFPS の処理を行います。
/// </summary>
void DirectXCommon::UpdateFixFPS() {
	const std::chrono::microseconds kMinTime(uint64_t(1000000.0f / 60.0f));
	const std::chrono::microseconds kMinCheckTime(uint64_t(1000000.0f / 65.0f));
	std::chrono::steady_clock::time_point now = std::chrono::high_resolution_clock::now();
	std::chrono::microseconds elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - reference_);

	if (elapsed < kMinTime) {
		while (std::chrono::steady_clock::now() - reference_ < kMinTime) {
			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}
	}
	reference_ = std::chrono::high_resolution_clock::now();
}
