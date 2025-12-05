#pragma once

class ModelCommon;
class Model {
	public:
	// 初期化
	void Initialize(ModelCommon* modelCommon);
	// 終了
	void Finalize();
	// 描画前処理
	void SetDraw();

private:
	ModelCommon* modelCommon_ = nullptr;

};
