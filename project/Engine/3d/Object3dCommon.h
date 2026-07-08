#pragma once
#include "Camera.h"
#include "DirectXCommon.h"
#include "Logger.h"
#include "StringUtility.h"
#include "struct.h"
#include <assert.h>
#include <d3d12.h>
#include <wrl.h>

class Object3dCommon {
public:
	static Object3dCommon* GetInstance();

	void Initialize(DirectXCommon* dxCommon);
	void Finalize();

	void SetDraw();

	DirectXCommon* GetDxCommon() const { return dxCommon_; }
	void SetDefaultCamera(Camera* cmr) { defaultCamera = cmr; }
	Camera* GetDefaultCamera() { return defaultCamera; }

	~Object3dCommon() = default;
private:
	Object3dCommon() = default;

	Object3dCommon(const Object3dCommon&) = delete;
	Object3dCommon& operator=(const Object3dCommon&) = delete;

	void CreateRootSignature();
	void CreatePipelineState();

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;

	Camera* defaultCamera = nullptr;
};
