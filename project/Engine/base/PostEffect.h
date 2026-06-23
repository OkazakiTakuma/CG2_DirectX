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

	// ImGuiの描画関数
	void DrawImGui();

	// エフェクトが有効かどうかを取得する関数
	bool IsActive() const { return isActive_; }

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

	// 色変更用の定数バッファ生成関数
	void CreateColorBuffer();

private:
	DirectXCommon* dxCommon_ = nullptr;

	// ポストエフェクトのON/OFFフラグ (初期値はtrue)
	bool isActive_ = true;

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

	// 色変更用の定数バッファ関連
	Microsoft::WRL::ComPtr<ID3D12Resource> colorBuffer_ = nullptr;

	// シェーダーに送るデータと同じ形の構造体
// PostEffect.h の構造体部分を修正
	struct ColorData {
		float r, g, b, a;
		int32_t enableGrayscale; // 💡追加 (1:ON, 0:OFF)
		float padding[3];        // 💡追加 (16バイト境界にするための調整)
	};

	ColorData* colorData_ = nullptr; // 書き込み用のポインタ

	// ImGuiで操作するための色配列 (初期値は白)
	float tintColor_[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
};