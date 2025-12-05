#pragma 

class DirectXCommon;
class ModelCommon {
	public:
	// 初期化
	void Initialize(DirectXCommon* dxCommon);
	// 終了
	void Finalize();
	// 描画前処理
	void SetDraw();

	DirectXCommon* GetDirectXCommon() { return dxCommon_; }

private:
	DirectXCommon* dxCommon_ = nullptr;
};
