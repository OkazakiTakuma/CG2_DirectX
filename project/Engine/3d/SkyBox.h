#pragma once
#include "Camera.h"
#include "SkyBoxCommon.h"
#include "struct.h"
#include <string>
#include <wrl.h>

class SkyBox {
public:
	/// <summary>
	/// 必要なリソースを準備し、オブジェクトを初期化します。
	/// </summary>
	/// <param name="filePath">読み込みまたは保存に使用するファイルパスを指定します。</param>
	void Initialize(const std::string& filePath);
	/// <summary>
	/// 毎フレームの状態更新を行います。
	/// </summary>
	void Update();
	/// <summary>
	/// 現在の状態をもとに描画処理を行います。
	/// </summary>
	void Draw();

private:
	/// <summary>
	/// VertexData を作成し、利用できる状態にします。
	/// </summary>
	void CreateVertexData();
	/// <summary>
	/// ConstantBuffers を作成し、利用できる状態にします。
	/// </summary>
	void CreateConstantBuffers();

	SkyBoxCommon* common_ = nullptr;
	std::string textureFilePath;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};

	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;
	D3D12_INDEX_BUFFER_VIEW indexBufferView{};

	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
	Material* materialData = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> transformResource;
	TransformationMatrix* transformData = nullptr;
	Camera* camera = nullptr;
};
