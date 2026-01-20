#include "ParticleEmitter.h"
#include "ParticleManager.h"

// コンストラクタ
// メンバ初期化子リストを使うのがC++の推奨作法です（無駄なコピーが走らないため）
ParticleEmitter::ParticleEmitter(const std::string& name, const Emitter& emitterData) : groupName(name), data(emitterData) {}

void ParticleEmitter::Update(float deltaTime) {
	if (!isActive)
		return;

	// 【重要】0除算と無限ループの防止
	// frequency（頻度）が 0 以下だと、interval が無限大あるいは計算不能になり、
	// 下の while ループから抜け出せなくなってフリーズします。
	if (data.frequency <= 0.0f) {
		return;
	}

	// タイマー更新（ヘッダーの変数名と合わせてください。例: frequencyTime）
	data.frequencyTimer += deltaTime;

	// 発生間隔 = 1秒 / 頻度 (例: 10回/秒 なら 0.1秒間隔)
	float interval = 1.0f / data.frequency;

	// 経過時間が間隔を超えている場合、その分だけEmitする
	while (data.frequencyTimer >= interval) {
		data.frequencyTimer -= interval;
		Emit();
	}
}

void ParticleEmitter::Emit() {
	// 【確認】struct.h の Emitter 構造体の定義を確認してください。
	// もし Emitter が Transform 構造体を持っているなら、data.transform.translate になります。
	// 直接 Vector3 を持っているなら data.translate でOKです。

	// パターンA: Emitterの中にTransformがある場合 (よくある構成)
	// ParticleManager::GetInstance()->Emit(groupName, data.transform.translate, data.count);

	// パターンB: Emitterが直接座標を持っている場合 (今のコード)

	ParticleManager::GetInstance()->Emit(groupName, data.transform.translate, data.count);


}