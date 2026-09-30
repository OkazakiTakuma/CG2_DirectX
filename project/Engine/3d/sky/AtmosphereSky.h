#pragma once

#include "AtmosphereSystem.h"
#include "DirectXCommon.h"
#include "Matrix.h"
#include <d3d12.h>
#include <wrl.h>

class Camera;

/// <summary>
/// テクスチャを使わず、視線方向と太陽方向から空・太陽円盤・散乱光を生成します。
/// 立方体は空を見る方向を補間するためだけに使用し、カメラの移動には追従しません。
/// </summary>
class AtmosphereSky final {
public:
	void Initialize();
	void Draw();
	void Finalize();

private:
	struct Vertex {
		Vector4 position;
	};

	struct TransformForGPU {
		Matrix4x4 WVP;
	};

	// HLSL側も16バイト単位で同じ並びにし、ConstantBufferのずれを防ぎます。
	struct AtmosphereForGPU {
		Vector3 sunDirection;
		float sunAngularRadius;
		Vector3 rayleighColor;
		float rayleighStrength;
		Vector3 mieColor;
		float mieStrength;
		float mieG;
		float atmosphereDensity;
		float sunIntensity;
		float padding;
	};

	void CreateRootSignature();
	void CreatePipelineState();
	void CreateGeometry();
	void CreateConstantBuffers();
	void Update(const Camera* camera);

	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> transformResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> atmosphereResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
	TransformForGPU* transformData_ = nullptr;
	AtmosphereForGPU* atmosphereData_ = nullptr;
};
