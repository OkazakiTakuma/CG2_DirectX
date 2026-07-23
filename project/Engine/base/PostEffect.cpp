#include "PostEffect.h"
#include "PipelineStateUtility.h"
#include "Input.h"
#include "SrvManager.h"
#include "WinApp.h"
#include "imGuiManager.h"
#include "GameTime.h"
#include "object/Object3dCommon.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

/// <summary>
/// 共有インスタンスを取得します。
/// </summary>
PostEffect* PostEffect::GetInstance() {
	static PostEffect instance;
	return &instance;
}

/// <summary>
/// 必要なリソースを準備し、オブジェクトを初期化します。
/// </summary>
/// <param name="dxCommon">DirectX 共通処理へアクセスするための参照を指定します。</param>
void PostEffect::Initialize(DirectXCommon* dxCommon) {
	assert(dxCommon);
	dxCommon_ = dxCommon;
	renderWidth_ = dxCommon_->GetRenderWidth();
	renderHeight_ = dxCommon_->GetRenderHeight();

	CreateTextureResource();
	CreateRtv();
	CreateDsv();
	CreateSrv();
	CreateDissolveMask();
	CreateRootSignature();
	CreatePipelineState();

	CreateColorBuffer();
}

/// <summary>
/// TextureResource を作成し、利用できる状態にします。
/// </summary>
void PostEffect::CreateTextureResource() {
	D3D12_RESOURCE_DESC textureDesc{};
	textureDesc.Width = renderWidth_;
	textureDesc.Height = renderHeight_;
	textureDesc.MipLevels = 1;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	clearValue.Color[0] = 0.1f;
	clearValue.Color[1] = 0.25f;
	clearValue.Color[2] = 0.5f;
	clearValue.Color[3] = 1.0f;

	HRESULT hr = dxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue, IID_PPV_ARGS(&textureResource_));
	assert(SUCCEEDED(hr));
}

/// <summary>
/// Rtv を作成し、利用できる状態にします。
/// </summary>
void PostEffect::CreateRtv() {
	rtvHeap_ = dxCommon_->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1, false);

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	dxCommon_->GetDevice()->CreateRenderTargetView(textureResource_.Get(), &rtvDesc, rtvHeap_->GetCPUDescriptorHandleForHeapStart());
}

/// <summary>
/// Dsv を作成し、利用できる状態にします。
/// </summary>
void PostEffect::CreateDsv() {
	D3D12_RESOURCE_DESC depthDesc{};
	depthDesc.Width = renderWidth_;
	depthDesc.Height = renderHeight_;
	depthDesc.MipLevels = 1;
	depthDesc.DepthOrArraySize = 1;
	depthDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	clearValue.DepthStencil.Depth = 1.0f;

	HRESULT hr = dxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &depthDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&depthBuffer_));
	assert(SUCCEEDED(hr));

	dsvHeap_ = dxCommon_->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	dxCommon_->GetDevice()->CreateDepthStencilView(depthBuffer_.Get(), &dsvDesc, dsvHeap_->GetCPUDescriptorHandleForHeapStart());
}

/// <summary>
/// Srv を作成し、利用できる状態にします。
/// </summary>
void PostEffect::CreateSrv() {
	SrvManager* srvManager = SrvManager::GetInstance();
	if (srvIndex_ == UINT32_MAX) {
		srvIndex_ = srvManager->Allocate();
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	dxCommon_->GetDevice()->CreateShaderResourceView(textureResource_.Get(), &srvDesc, srvManager->GetCPUDescriptorHandle(srvIndex_));

	srvHandleGPU_ = srvManager->GetGPUDescriptorHandle(srvIndex_);

	if (depthSrvIndex_ == UINT32_MAX) {
		depthSrvIndex_ = srvManager->Allocate();
	}
	D3D12_SHADER_RESOURCE_VIEW_DESC depthSrvDesc{};
	depthSrvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	depthSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	depthSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	depthSrvDesc.Texture2D.MipLevels = 1;
	dxCommon_->GetDevice()->CreateShaderResourceView(depthBuffer_.Get(), &depthSrvDesc, srvManager->GetCPUDescriptorHandle(depthSrvIndex_));
	depthSrvHandleGPU_ = srvManager->GetGPUDescriptorHandle(depthSrvIndex_);
}

void PostEffect::CreateDissolveMask() {
	if (dissolveMaskResource_) {
		return;
	}

	constexpr uint32_t maskSize = 256;
	D3D12_RESOURCE_DESC textureDesc{};
	textureDesc.Width = maskSize;
	textureDesc.Height = maskSize;
	textureDesc.MipLevels = 1;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

	HRESULT hr = dxCommon_->GetDevice()->CreateCommittedResource(
	    &heapProps,
	    D3D12_HEAP_FLAG_NONE,
	    &textureDesc,
	    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
	    nullptr,
	    IID_PPV_ARGS(&dissolveMaskResource_)
	);
	assert(SUCCEEDED(hr));

	std::vector<uint8_t> mask(maskSize * maskSize);
	for (uint32_t y = 0; y < maskSize; ++y) {
		for (uint32_t x = 0; x < maskSize; ++x) {
			const float wave = std::sin(static_cast<float>(x) * 0.13f) + std::sin(static_cast<float>(y) * 0.17f);
			const uint32_t hash = (x * 1973u) ^ (y * 9277u) ^ ((x + y) * 26699u);
			const float noise = static_cast<float>((hash ^ (hash >> 13)) & 0xffu) / 255.0f;
			const float value = std::clamp((wave * 0.18f) + (noise * 0.82f), 0.0f, 1.0f);
			mask[(y * maskSize) + x] = static_cast<uint8_t>(value * 255.0f);
		}
	}

	hr = dissolveMaskResource_->WriteToSubresource(0, nullptr, mask.data(), maskSize, maskSize * maskSize);
	assert(SUCCEEDED(hr));

	SrvManager* srvManager = SrvManager::GetInstance();
	if (dissolveMaskSrvIndex_ == UINT32_MAX) {
		dissolveMaskSrvIndex_ = srvManager->Allocate();
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_R8_UNORM;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	dxCommon_->GetDevice()->CreateShaderResourceView(dissolveMaskResource_.Get(), &srvDesc, srvManager->GetCPUDescriptorHandle(dissolveMaskSrvIndex_));
	dissolveMaskSrvHandleGPU_ = srvManager->GetGPUDescriptorHandle(dissolveMaskSrvIndex_);
}

/// <summary>
/// RootSignature を作成し、利用できる状態にします。
/// </summary>
void PostEffect::CreateRootSignature() {
	D3D12_DESCRIPTOR_RANGE descriptorRanges[3] = {};
	for (uint32_t index = 0; index < _countof(descriptorRanges); ++index) {
		descriptorRanges[index].BaseShaderRegister = index;
		descriptorRanges[index].NumDescriptors = 1;
		descriptorRanges[index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRanges[index].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	}

	D3D12_ROOT_PARAMETER rootParameters[4] = {};

	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].DescriptorTable.pDescriptorRanges = &descriptorRanges[0];
	rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[1].Descriptor.ShaderRegister = 0;

	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[2].DescriptorTable.pDescriptorRanges = &descriptorRanges[1];
	rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;

	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[3].DescriptorTable.pDescriptorRanges = &descriptorRanges[2];
	rootParameters[3].DescriptorTable.NumDescriptorRanges = 1;

	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC descriptionSignature{};
	descriptionSignature.pParameters = rootParameters;
	descriptionSignature.NumParameters = _countof(rootParameters);
	descriptionSignature.pStaticSamplers = staticSamplers;
	descriptionSignature.NumStaticSamplers = 1;
	descriptionSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	rootSignature_ = PipelineStateUtility::CreateRootSignature(dxCommon_->GetDevice().Get(), descriptionSignature);
}

/// <summary>
/// PipelineState を作成し、利用できる状態にします。
/// </summary>
void PostEffect::CreatePipelineState() {
	auto vertexShaderBlob = dxCommon_->CompileShader(L"Resources/Shader/CopyImage.VS.hlsl", L"vs_6_0");
	auto pixelShaderBlob = dxCommon_->CompileShader(L"Resources/Shader/FullScreen.PS.hlsl", L"ps_6_0");
	assert(vertexShaderBlob && pixelShaderBlob);

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = nullptr;
	inputLayoutDesc.NumElements = 0;

	const D3D12_BLEND_DESC blendDesc = PipelineStateUtility::MakeBlendDesc();
	const D3D12_RASTERIZER_DESC rasterizerDesc = PipelineStateUtility::MakeRasterizerDesc(D3D12_CULL_MODE_NONE);
	const D3D12_DEPTH_STENCIL_DESC depthStencilDesc =
	    PipelineStateUtility::MakeDepthStencilDesc(FALSE, D3D12_DEPTH_WRITE_MASK_ZERO, D3D12_COMPARISON_FUNC_NEVER);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = rootSignature_.Get();
	psoDesc.InputLayout = inputLayoutDesc;
	psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
	psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
	psoDesc.BlendState = blendDesc;
	psoDesc.RasterizerState = rasterizerDesc;
	psoDesc.DepthStencilState = depthStencilDesc;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&graphicsPipelineState_));
	assert(SUCCEEDED(hr));
}

/// <summary>
/// ColorBuffer を作成し、利用できる状態にします。
/// </summary>
void PostEffect::CreateColorBuffer() {
	uint32_t sizeIB = (sizeof(ColorData) + 0xff) & ~0xff;

	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeIB;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	HRESULT hr = dxCommon_->GetDevice()->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&colorBuffer_));

	if (FAILED(hr)) {
		assert(false && "Failed to create ColorBuffer");
		return;
	}

	D3D12_RANGE readRange{};
	readRange.Begin = 0;
	readRange.End = 0;

	hr = colorBuffer_->Map(0, &readRange, reinterpret_cast<void**>(&colorData_));

	if (FAILED(hr) || colorData_ == nullptr) {
		assert(false && "Failed to map ColorBuffer");
		return;
	}

	ApplySettingsToBuffer();
}

/// <summary>
/// SettingsToBuffer を現在の状態へ反映します。
/// </summary>
void PostEffect::ApplySettingsToBuffer() {
	if (colorData_ == nullptr) {
		return;
	}

	colorData_->r = tintColor_[0];
	colorData_->g = tintColor_[1];
	colorData_->b = tintColor_[2];
	colorData_->a = tintColor_[3];

	colorData_->enableGrayscale = enableGrayscale_ ? 1 : 0;
	colorData_->enableVignetting = enableVignetting_ ? 1 : 0;
	colorData_->enableSmoothing = enableSmoothing_ ? 1 : 0;
	colorData_->enableGaussianFilter = enableGaussianFilter_ ? 1 : 0;
	colorData_->enableRadialBlur = enableRadialBlur_ ? 1 : 0;
	colorData_->enableRandom = enableRandom_ ? 1 : 0;
	colorData_->radialBlurSamples = radialBlurSamples_;
	colorData_->enableOutline = enableOutline_ ? 1 : 0;
	colorData_->enableDissolve = enableDissolve_ ? 1 : 0;
	colorData_->vignetteIntensity = vignetteIntensity_;
	colorData_->vignetteRadius = vignetteRadius_;
	colorData_->vignetteSoftness = vignetteSoftness_;
	colorData_->radialBlurStrength = radialBlurStrength_;
	colorData_->randomStrength = randomStrength_;
	colorData_->outlineStrength = outlineStrength_;
	colorData_->outlineThreshold = outlineThreshold_;
	colorData_->outlineThickness = outlineThickness_;
	colorData_->dissolveThreshold = dissolveThreshold_;
	colorData_->dissolveEdgeWidth = dissolveEdgeWidth_;
	colorData_->time = time_;
	colorData_->texelSize[0] = 1.0f / static_cast<float>(renderWidth_);
	colorData_->texelSize[1] = 1.0f / static_cast<float>(renderHeight_);
	const Camera* camera = Object3dCommon::GetInstance()->GetDefaultCamera();
	const float nearClip = camera ? camera->GetNearClip() : 0.1f;
	const float farClip = camera ? camera->GetFarClip() : 1000.0f;
	colorData_->cameraNearFar[0] = (std::max)(nearClip, 0.0001f);
	colorData_->cameraNearFar[1] = (std::max)(farClip, colorData_->cameraNearFar[0] + 0.0001f);
	colorData_->outlineColor[0] = outlineColor_[0];
	colorData_->outlineColor[1] = outlineColor_[1];
	colorData_->outlineColor[2] = outlineColor_[2];
	colorData_->outlineColor[3] = outlineColor_[3];
	colorData_->dissolveEdgeColor[0] = dissolveEdgeColor_[0];
	colorData_->dissolveEdgeColor[1] = dissolveEdgeColor_[1];
	colorData_->dissolveEdgeColor[2] = dissolveEdgeColor_[2];
	colorData_->dissolveEdgeColor[3] = dissolveEdgeColor_[3];
	colorData_->damageVignetteIntensity = damageVignetteCurrentIntensity_;
	colorData_->damageVignetteRadius = damageVignetteRadius_;
	colorData_->damageVignetteSoftness = damageVignetteSoftness_;
	colorData_->paddingDamageVignette = 0.0f;
}

void PostEffect::ResizeIfNeeded() {
	if (!dxCommon_) {
		return;
	}

	const int32_t width = dxCommon_->GetRenderWidth();
	const int32_t height = dxCommon_->GetRenderHeight();
	if (width == renderWidth_ && height == renderHeight_) {
		return;
	}

	ResizeResources(width, height);
}

void PostEffect::ResizeResources(int32_t width, int32_t height) {
	if (width <= 0 || height <= 0) {
		return;
	}

	renderWidth_ = width;
	renderHeight_ = height;
	sceneTextureReadyAsSrv_ = false;
	depthTextureReadyAsSrv_ = false;
	textureResource_.Reset();
	depthBuffer_.Reset();
	rtvHeap_.Reset();
	dsvHeap_.Reset();

	CreateTextureResource();
	CreateRtv();
	CreateDsv();
	CreateSrv();
	ApplySettingsToBuffer();
}

void PostEffect::UpdateHotkeys() {
	if (damageVignetteTimer_ > 0.0f) {
		damageVignetteTimer_ = (std::max)(0.0f, damageVignetteTimer_ - GameTime::GetDeltaTime());
		const float normalizedTime = damageVignetteDuration_ > 0.0f
			? damageVignetteTimer_ / damageVignetteDuration_
			: 0.0f;
		damageVignetteCurrentIntensity_ = damageVignetteMaxIntensity_ * normalizedTime * normalizedTime;
	} else {
		damageVignetteCurrentIntensity_ = 0.0f;
	}

	Input* input = Input::GetInstance();
	const auto triggered = [input](BYTE key, BYTE numpadKey) {
		return input->TriggerKey(key) || input->TriggerKey(numpadKey);
	};

	if (triggered(DIK_1, DIK_NUMPAD1)) {
		isActive_ = !isActive_;
	}
	if (triggered(DIK_2, DIK_NUMPAD2)) {
		enableGrayscale_ = !enableGrayscale_;
	}
	if (triggered(DIK_3, DIK_NUMPAD3)) {
		enableSmoothing_ = !enableSmoothing_;
	}
	if (triggered(DIK_4, DIK_NUMPAD4)) {
		enableGaussianFilter_ = !enableGaussianFilter_;
	}
	if (triggered(DIK_5, DIK_NUMPAD5)) {
		enableRadialBlur_ = !enableRadialBlur_;
	}
	if (triggered(DIK_6, DIK_NUMPAD6)) {
		enableRandom_ = !enableRandom_;
	}
	if (triggered(DIK_7, DIK_NUMPAD7)) {
		enableVignetting_ = !enableVignetting_;
	}
	if (triggered(DIK_8, DIK_NUMPAD8)) {
		enableOutline_ = !enableOutline_;
	}
	if (triggered(DIK_9, DIK_NUMPAD9)) {
		enableDissolve_ = !enableDissolve_;
	}

	ApplySettingsToBuffer();
}

void PostEffect::TriggerDamageVignette() {
	damageVignetteTimer_ = damageVignetteDuration_;
	damageVignetteCurrentIntensity_ = damageVignetteMaxIntensity_;
	ApplySettingsToBuffer();
}
void PostEffect::PreDrawScene() {
	ResizeIfNeeded();
	auto commandList = dxCommon_->GetCommandList();

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = textureResource_.Get();
	barrier.Transition.StateBefore = sceneTextureReadyAsSrv_ ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	if (barrier.Transition.StateBefore != barrier.Transition.StateAfter) {
		commandList->ResourceBarrier(1, &barrier);
	}

	if (depthTextureReadyAsSrv_) {
		D3D12_RESOURCE_BARRIER depthBarrier{};
		depthBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		depthBarrier.Transition.pResource = depthBuffer_.Get();
		depthBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		depthBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		depthBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &depthBarrier);
		depthTextureReadyAsSrv_ = false;
	}

	sceneTextureReadyAsSrv_ = false;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	const float clearColor[] = {0.1f, 0.25f, 0.5f, 1.0f};
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	D3D12_VIEWPORT viewport{};
	viewport.Width = static_cast<float>(renderWidth_);
	viewport.Height = static_cast<float>(renderHeight_);
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	commandList->RSSetViewports(1, &viewport);

	D3D12_RECT scissorRect{};
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = renderWidth_;
	scissorRect.bottom = renderHeight_;
	commandList->RSSetScissorRects(1, &scissorRect);
}

void PostEffect::PostDrawScene() {
	auto commandList = dxCommon_->GetCommandList();

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = textureResource_.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	commandList->ResourceBarrier(1, &barrier);
	sceneTextureReadyAsSrv_ = true;

	D3D12_RESOURCE_BARRIER depthBarrier{};
	depthBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	depthBarrier.Transition.pResource = depthBuffer_.Get();
	depthBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	depthBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	depthBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	commandList->ResourceBarrier(1, &depthBarrier);
	depthTextureReadyAsSrv_ = true;
}

/// <summary>
/// 現在の状態をもとに描画処理を行います。
/// </summary>
void PostEffect::Draw() {
	ResizeIfNeeded();
	auto commandList = dxCommon_->GetCommandList();
	SrvManager::GetInstance()->PreDraw();
	time_ += GameTime::GetDeltaTime();
	ApplySettingsToBuffer();

	commandList->SetGraphicsRootSignature(rootSignature_.Get());
	commandList->SetPipelineState(graphicsPipelineState_.Get());

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->SetGraphicsRootDescriptorTable(0, srvHandleGPU_);
	commandList->SetGraphicsRootConstantBufferView(1, colorBuffer_->GetGPUVirtualAddress());
	commandList->SetGraphicsRootDescriptorTable(2, depthSrvHandleGPU_);
	commandList->SetGraphicsRootDescriptorTable(3, dissolveMaskSrvHandleGPU_);

	commandList->DrawInstanced(3, 1, 0, 0);
}

/// <summary>
/// ImGui によるデバッグ用 UI の表示と編集処理を行います。
/// </summary>
void PostEffect::DrawImGui() {
#ifdef USE_IMGUI
#ifndef IMGUI_HAS_DOCK
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	const float width = displaySize.x > 0.0f ? displaySize.x : 1280.0f;
	const float panelWidth = width < 900.0f ? 280.0f : 320.0f;
	const float leftWidth = width * 0.18f;
	const float x = (width - panelWidth) * 0.5f;

	ImGui::SetNextWindowPos(ImVec2(x > leftWidth ? x : leftWidth, 0.0f), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(panelWidth, 420.0f), ImGuiCond_Always);
	ImGui::Begin(
	    "PostEffect Settings",
	    nullptr,
	    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings
	);
#else
	ImGui::Begin("PostEffect Settings");
#endif

	const ImVec4 enabledColor{0.2f, 1.0f, 0.35f, 1.0f};
	const ImVec4 disabledColor{1.0f, 0.25f, 0.2f, 1.0f};
	const auto drawEffectToggle = [&enabledColor, &disabledColor](const char* label, bool* enabled) {
		const bool changed = ImGui::Checkbox(label, enabled);
		ImGui::SameLine();
		ImGui::TextColored(*enabled ? enabledColor : disabledColor, *enabled ? "[ON]" : "[OFF]");
		return changed;
	};

	const int enabledEffectCount =
		static_cast<int>(enableGrayscale_) + static_cast<int>(enableSmoothing_) +
		static_cast<int>(enableGaussianFilter_) + static_cast<int>(enableRadialBlur_) +
		static_cast<int>(enableRandom_) + static_cast<int>(enableOutline_) +
		static_cast<int>(enableDissolve_) + static_cast<int>(enableVignetting_);

	drawEffectToggle("Enable PostEffect", &isActive_);
	ImGui::TextColored(
		isActive_ ? enabledColor : disabledColor,
		isActive_ ? "APPLYING: %d effect(s)" : "MASTER OFF: effects are not applied",
		enabledEffectCount
	);
	ImGui::TextDisabled("Hotkeys: 1 Master / 2-9 Effects");
	ImGui::Separator();

	drawEffectToggle("Apply Grayscale", &enableGrayscale_);

	if (ImGui::ColorEdit4("Tint Color", tintColor_)) {
		colorData_->r = tintColor_[0];
		colorData_->g = tintColor_[1];
		colorData_->b = tintColor_[2];
		colorData_->a = tintColor_[3];
	}

	ImGui::Separator();
	if (drawEffectToggle("Apply Smoothing", &enableSmoothing_)) {
		colorData_->enableSmoothing = enableSmoothing_ ? 1 : 0;
	}

	if (drawEffectToggle("Apply Gaussian Filter", &enableGaussianFilter_)) {
		colorData_->enableGaussianFilter = enableGaussianFilter_ ? 1 : 0;
	}

	if (drawEffectToggle("Apply Radial Blur", &enableRadialBlur_)) {
		colorData_->enableRadialBlur = enableRadialBlur_ ? 1 : 0;
	}
	if (!enableRadialBlur_) {
		ImGui::BeginDisabled();
	}
	if (ImGui::SliderFloat("Radial Strength", &radialBlurStrength_, 0.0f, 0.3f)) {
		colorData_->radialBlurStrength = radialBlurStrength_;
	}
	if (ImGui::SliderInt("Radial Samples", &radialBlurSamples_, 2, 32)) {
		colorData_->radialBlurSamples = radialBlurSamples_;
	}
	if (!enableRadialBlur_) {
		ImGui::EndDisabled();
	}

	if (drawEffectToggle("Apply Random", &enableRandom_)) {
		colorData_->enableRandom = enableRandom_ ? 1 : 0;
	}
	if (!enableRandom_) {
		ImGui::BeginDisabled();
	}
	if (ImGui::SliderFloat("Random Strength", &randomStrength_, 0.0f, 0.3f)) {
		colorData_->randomStrength = randomStrength_;
	}
	if (!enableRandom_) {
		ImGui::EndDisabled();
	}

	ImGui::Separator();
	if (drawEffectToggle("Apply Outline", &enableOutline_)) {
		colorData_->enableOutline = enableOutline_ ? 1 : 0;
	}
	if (!enableOutline_) {
		ImGui::BeginDisabled();
	}
	ImGui::TextDisabled("Depth-based outline (lower threshold = more edges)");
	ImGui::ColorEdit4("Outline Color", outlineColor_);
	ImGui::SliderFloat("Outline Strength", &outlineStrength_, 0.0f, 2.0f);
	ImGui::SliderFloat("Outline Threshold", &outlineThreshold_, 0.001f, 0.5f, "%.3f");
	ImGui::SliderFloat("Outline Thickness", &outlineThickness_, 1.0f, 8.0f, "%.1f px");
	if (ImGui::Button("Reset Outline Settings")) {
		outlineColor_[0] = 0.0f;
		outlineColor_[1] = 0.0f;
		outlineColor_[2] = 0.0f;
		outlineColor_[3] = 1.0f;
		outlineStrength_ = 1.0f;
		outlineThreshold_ = 0.05f;
		outlineThickness_ = 2.0f;
	}
	if (!enableOutline_) {
		ImGui::EndDisabled();
	}

	ImGui::Separator();
	if (drawEffectToggle("Apply Dissolve", &enableDissolve_)) {
		colorData_->enableDissolve = enableDissolve_ ? 1 : 0;
	}
	if (!enableDissolve_) {
		ImGui::BeginDisabled();
	}
	ImGui::SliderFloat("Dissolve Threshold", &dissolveThreshold_, 0.0f, 1.0f);
	ImGui::SliderFloat("Dissolve Edge Width", &dissolveEdgeWidth_, 0.001f, 0.3f);
	ImGui::ColorEdit4("Dissolve Edge Color", dissolveEdgeColor_);
	if (!enableDissolve_) {
		ImGui::EndDisabled();
	}

	ImGui::Separator();
	if (drawEffectToggle("Apply Vignetting", &enableVignetting_)) {
		colorData_->enableVignetting = enableVignetting_ ? 1 : 0;
	}
	if (!enableVignetting_) {
		ImGui::BeginDisabled();
	}
	if (ImGui::SliderFloat("Vignette Intensity", &vignetteIntensity_, 0.0f, 1.0f)) {
		colorData_->vignetteIntensity = vignetteIntensity_;
	}
	if (ImGui::SliderFloat("Vignette Radius", &vignetteRadius_, 0.0f, 1.5f)) {
		colorData_->vignetteRadius = vignetteRadius_;
	}
	if (ImGui::SliderFloat("Vignette Softness", &vignetteSoftness_, 0.01f, 1.0f)) {
		colorData_->vignetteSoftness = vignetteSoftness_;
	}
	if (!enableVignetting_) {
		ImGui::EndDisabled();
	}

	ImGui::Separator();
	ImGui::Text("Damage Vignette");
	ImGui::SliderFloat("Damage Intensity", &damageVignetteMaxIntensity_, 0.0f, 1.0f);
	ImGui::SliderFloat("Damage Duration", &damageVignetteDuration_, 0.05f, 2.0f);
	ImGui::SliderFloat("Damage Radius", &damageVignetteRadius_, 0.0f, 0.5f);
	ImGui::SliderFloat("Damage Softness", &damageVignetteSoftness_, 0.01f, 0.5f);
	if (ImGui::Button("Test Damage Vignette")) {
		TriggerDamageVignette();
	}

	ApplySettingsToBuffer();
	ImGui::End();
#endif
}
/// <summary>
/// 確保したリソースを解放し、終了処理を行います。
/// </summary>
void PostEffect::Finalize() {
	textureResource_.Reset();
	depthBuffer_.Reset();
	dissolveMaskResource_.Reset();
	rtvHeap_.Reset();
	dsvHeap_.Reset();
	rootSignature_.Reset();
	graphicsPipelineState_.Reset();
	colorBuffer_.Reset();
}
