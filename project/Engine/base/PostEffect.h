#pragma once
#include "DirectXCommon.h"
#include "struct.h"
#include <d3d12.h>
#include <string>
#include <vector>
#include <wrl/client.h>

class PostEffect {
public:
	// シングルトンインスタンスの取得
	static PostEffect* GetInstance();

	// 初期化
	void Initialize(DirectXCommon* dxCommon);

	// 描画前処理（描画先をこのポストエフェクト用テクスチャに切り替える）
	void PreDrawScene();

	// 描画後処理（テクスチャへの書き込みを終了するリソースバリア）
	void PostDrawScene();

	// 画面全体にテクスチャを描画する
	void Draw();

	// 終了処理
	void Finalize();

private:
	PostEffect() = default;
	~PostEffect() = default;
	PostEffect(const PostEffect&) = delete;
	PostEffect& operator=(const PostEffect&) = delete;

	// 各種リソース生成関数
	void CreateTextureResource();
	void CreateRtv();
	void CreateDsv();
	void CreateSrv();
	// ※CreateVertexData() はシェーダー内で頂点生成するため削除
	void CreateRootSignature();
	void CreatePipelineState();

private:
	DirectXCommon* dxCommon_ = nullptr;

	// テクスチャリソースとRTV/DSV用ヒープ
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_ = nullptr;

	// SRV用（SrvManagerで管理するインデックスとハンドル）
	uint32_t srvIndex_ = 0;
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU_{};

	// パイプライン
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr;
};