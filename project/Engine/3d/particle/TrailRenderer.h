#pragma once

#include "../camera/Camera.h"
#include "DirectXCommon.h"
#include "Vector.h"
#include <d3d12.h>
#include <vector>
#include <wrl.h>

/// <summary>軌跡を構成する1点の位置と残存率です。</summary>
struct TrailRenderPoint {
	Vector3 position{};
	float lifeRate = 1.0f;
};

/// <summary>各コンポーネントから集めた軌跡をカメラ向きの帯として一括描画します。</summary>
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
		/// <summary>軌跡を構成する頂点のワールド座標です。</summary>
		Vector3 position{};
		/// <summary>寿命と位置から補間された頂点色です。</summary>
		Vector4 color{};
	};

	/// <summary>Drawまで保留する1本分の軌跡描画要求です。</summary>
	struct Request {
		std::vector<TrailRenderPoint> points;
		float width = 1.0f;
		Vector4 headColor{};
		Vector4 tailColor{};
	};

	/// <summary>動的頂点バッファに格納できる最大頂点数です。</summary>
	static constexpr uint32_t kMaxVertexCount = 32768;

	TrailRenderer() = default;
	~TrailRenderer() = default;
	TrailRenderer(const TrailRenderer&) = delete;
	TrailRenderer& operator=(const TrailRenderer&) = delete;

	void CreateRootSignature();
	void CreatePipelineState();
	void AppendRequestVertices(const Request& request, const Vector3& cameraPosition);

	/// <summary>描画コマンドとGPUリソース作成に使用する共通処理です。</summary>
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	Vertex* vertexData_ = nullptr;
	Matrix4x4* cameraData_ = nullptr;
	/// <summary>現在のフレームでGPUへ転送した有効頂点数です。</summary>
	uint32_t vertexCount_ = 0;
	/// <summary>描画時まで蓄積する軌跡要求です。</summary>
	std::vector<Request> requests_;
};
