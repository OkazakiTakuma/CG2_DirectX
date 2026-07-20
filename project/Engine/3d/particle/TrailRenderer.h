#pragma once

#include "../camera/Camera.h"
#include "DirectXCommon.h"
#include "Vector.h"
#include <d3d12.h>
#include <vector>
#include <wrl.h>

struct TrailRenderPoint {
	Vector3 position{};
	float lifeRate = 1.0f;
};

class TrailRenderer {
public:
	static TrailRenderer* GetInstance();

	void Initialize(DirectXCommon* dxCommon);
	void Finalize();
	void Submit(
	    const std::vector<TrailRenderPoint>& points,
	    float width,
	    const Vector4& headColor,
	    const Vector4& tailColor
	);
	void Draw(Camera* camera);

private:
	struct Vertex {
		Vector3 position{};
		Vector4 color{};
	};

	struct Request {
		std::vector<TrailRenderPoint> points;
		float width = 1.0f;
		Vector4 headColor{};
		Vector4 tailColor{};
	};

	static constexpr uint32_t kMaxVertexCount = 32768;

	TrailRenderer() = default;
	~TrailRenderer() = default;
	TrailRenderer(const TrailRenderer&) = delete;
	TrailRenderer& operator=(const TrailRenderer&) = delete;

	void CreateRootSignature();
	void CreatePipelineState();
	void AppendRequestVertices(const Request& request, const Vector3& cameraPosition);

	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	Vertex* vertexData_ = nullptr;
	Matrix4x4* cameraData_ = nullptr;
	uint32_t vertexCount_ = 0;
	std::vector<Request> requests_;
};
