#include "ParticleEmitter.h"
#include "ParticleManager.h" // Managerの定義が必要

// コンストラクタ
// 引数で受け取った値をメンバ変数に書き込む
ParticleEmitter::ParticleEmitter(const std::string& name, const Transform& transform, uint32_t count, float frequency)
    : groupName_(name), transform_(transform), count_(count), frequency_(frequency), frequencyTimer_(0.0f) { // タイマーは0初期化
}

void ParticleEmitter::Update(float deltaTime) {
	// 頻度が0以下の場合は処理しない（0除算防止）
	if (frequency_ <= 0.0f) {
		return;
	}

	// 1. 時刻を進める
	frequencyTimer_ += deltaTime;

	// 発生間隔（閾値）を計算 (例: frequency=10 なら 0.1秒)
	float interval = 1.0f / frequency_;

	// 2. 発生頻度より大きいなら発生（余剰時間も考慮してループ処理）
	// ラグなどで deltaTime が長く、一度に複数回分の時間が経過した場合でも
	// 適切な回数 Emit を呼ぶために while を使用します。
	while (frequencyTimer_ >= interval) {
		// 発生処理
		Emit();
		ParticleManager::GetInstance()->Emit(groupName_, transform_.translate, count_);

		// 3. 余計に過ぎた時間込みで頻度計算をする
		// タイマーを0にするのではなく、閾値分だけ引くことでズレを防ぐ
		frequencyTimer_ -= interval;
	}
}

void ParticleEmitter::Emit() {
	// エミッタの規定値（現在の座標）に従ってパーティクルマネージャーを呼び出す
	// 引数：グループ名, 発生座標, 発生数
	ParticleManager::GetInstance()->Emit(groupName_, transform_.translate, count_);
}