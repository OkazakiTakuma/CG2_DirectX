#include "PostEffect.h"
#include "Input.h"
#include "SrvManager.h"
#include "WinApp.h"
#include "imGuiManager.h"
#include <cassert>

PostEffect* PostEffect::GetInstance() {
	static PostEffect instance;
	return &instance;
}

void PostEffect::Initialize(DirectXCommon* dxCommon) {
	assert(dxCommon);
	dxCommon_ = dxCommon;

	CreateTextureResource();
	CreateRtv();
	CreateDsv();
	CreateSrv();
	CreateRootSignature();
	CreatePipelineState();

	CreateColorBuffer();
}

void PostEffect::CreateTextureResource() {
	D3D12_RESOURCE_DESC textureDesc{};
	textureDesc.Width = WinApp::kClientWidth;
	textureDesc.Height = WinApp::kClientHeight;
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

void PostEffect::CreateRtv() {
	rtvHeap_ = dxCommon_->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1, false);

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	dxCommon_->GetDevice()->CreateRenderTargetView(textureResource_.Get(), &rtvDesc, rtvHeap_->GetCPUDescriptorHandleForHeapStart());
}

void PostEffect::CreateDsv() {
	D3D12_RESOURCE_DESC depthDesc{};
	depthDesc.Width = WinApp::kClientWidth;
	depthDesc.Height = WinApp::kClientHeight;
	depthDesc.MipLevels = 1;
	depthDesc.DepthOrArraySize = 1;
	depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
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

void PostEffect::CreateSrv() {
	SrvManager* srvManager = SrvManager::GetInstance();
	srvIndex_ = srvManager->Allocate();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	dxCommon_->GetDevice()->CreateShaderResourceView(textureResource_.Get(), &srvDesc, srvManager->GetCPUDescriptorHandle(srvIndex_));

	srvHandleGPU_ = srvManager->GetGPUDescriptorHandle(srvIndex_);
}

void PostEffect::CreateRootSignature() {
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParameters[2] = {};

	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[1].Descriptor.ShaderRegister = 0;

	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC descriptionSignature{};
	descriptionSignature.pParameters = rootParameters;
	descriptionSignature.NumParameters = 2;
	descriptionSignature.pStaticSamplers = staticSamplers;
	descriptionSignature.NumStaticSamplers = 1;
	descriptionSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&descriptionSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	assert(SUCCEEDED(hr));

	hr = dxCommon_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));
}

void PostEffect::CreatePipelineState() {
	auto vertexShaderBlob = dxCommon_->CompileShader(L"Resources/Shader/CopyImage.VS.hlsl", L"vs_6_0");
	auto pixelShaderBlob = dxCommon_->CompileShader(L"Resources/Shader/FullScreen.PS.hlsl", L"ps_6_0");
	assert(vertexShaderBlob && pixelShaderBlob);

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = nullptr;
	inputLayoutDesc.NumElements = 0;

	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = false;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

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
	colorData_->vignetteIntensity = vignetteIntensity_;
	colorData_->vignetteRadius = vignetteRadius_;
	colorData_->vignetteSoftness = vignetteSoftness_;
	colorData_->radialBlurStrength = radialBlurStrength_;
	colorData_->randomStrength = randomStrength_;
	colorData_->time = time_;
	colorData_->texelSize[0] = 1.0f / static_cast<float>(WinApp::kClientWidth);
	colorData_->texelSize[1] = 1.0f / static_cast<float>(WinApp::kClientHeight);
	colorData_->padding = 0.0f;
}

void PostEffect::UpdateHotkeys() {
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

	ApplySettingsToBuffer();
}
void PostEffect::PreDrawScene() {
	auto commandList = dxCommon_->GetCommandList();

	static bool isFirstFrame = true;

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = textureResource_.Get();
	barrier.Transition.StateBefore = isFirstFrame ? D3D12_RESOURCE_STATE_RENDER_TARGET : D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	if (barrier.Transition.StateBefore != barrier.Transition.StateAfter) {
		commandList->ResourceBarrier(1, &barrier);
	}

	isFirstFrame = false;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	const float clearColor[] = {0.1f, 0.25f, 0.5f, 1.0f};
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	D3D12_VIEWPORT viewport{};
	viewport.Width = static_cast<float>(WinApp::kClientWidth);
	viewport.Height = static_cast<float>(WinApp::kClientHeight);
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	commandList->RSSetViewports(1, &viewport);

	D3D12_RECT scissorRect{};
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = WinApp::kClientWidth;
	scissorRect.bottom = WinApp::kClientHeight;
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
}

void PostEffect::Draw() {
	auto commandList = dxCommon_->GetCommandList();
	SrvManager::GetInstance()->PreDraw();
	time_ += 1.0f / 60.0f;
	ApplySettingsToBuffer();

	commandList->SetGraphicsRootSignature(rootSignature_.Get());
	commandList->SetPipelineState(graphicsPipelineState_.Get());

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->SetGraphicsRootDescriptorTable(0, srvHandleGPU_);
	commandList->SetGraphicsRootConstantBufferView(1, colorBuffer_->GetGPUVirtualAddress());

	commandList->DrawInstanced(3, 1, 0, 0);
}

void PostEffect::DrawImGui() {
#ifdef USE_IMGUI
#ifndef IMGUI_HAS_DOCK
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	const float width = displaySize.x > 0.0f ? displaySize.x : 1280.0f;
	const float panelWidth = width < 900.0f ? 220.0f : 260.0f;
	const float leftWidth = width * 0.18f;
	const float x = (width - panelWidth) * 0.5f;

	ImGui::SetNextWindowPos(ImVec2(x > leftWidth ? x : leftWidth, 0.0f), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(panelWidth, 130.0f), ImGuiCond_Always);
	ImGui::Begin(
	    "PostEffect Settings",
	    nullptr,
	    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings
	);
#else
	ImGui::Begin("PostEffect Settings");
#endif

	ImGui::Checkbox("Enable PostEffect", &isActive_);

	ImGui::Checkbox("Apply Grayscale", &enableGrayscale_);

	if (ImGui::ColorEdit4("Tint Color", tintColor_)) {
		colorData_->r = tintColor_[0];
		colorData_->g = tintColor_[1];
		colorData_->b = tintColor_[2];
		colorData_->a = tintColor_[3];
	}

	ImGui::Separator();
	if (ImGui::Checkbox("Apply Smoothing", &enableSmoothing_)) {
		colorData_->enableSmoothing = enableSmoothing_ ? 1 : 0;
	}

	if (ImGui::Checkbox("Apply Gaussian Filter", &enableGaussianFilter_)) {
		colorData_->enableGaussianFilter = enableGaussianFilter_ ? 1 : 0;
	}

	if (ImGui::Checkbox("Apply Radial Blur", &enableRadialBlur_)) {
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

	if (ImGui::Checkbox("Apply Random", &enableRandom_)) {
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
	if (ImGui::Checkbox("Apply Vignetting", &enableVignetting_)) {
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

	ApplySettingsToBuffer();
	ImGui::End();
#endif
}
void PostEffect::Finalize() {
	textureResource_.Reset();
	depthBuffer_.Reset();
	rtvHeap_.Reset();
	dsvHeap_.Reset();
	rootSignature_.Reset();
	graphicsPipelineState_.Reset();
	colorBuffer_.Reset();
}
