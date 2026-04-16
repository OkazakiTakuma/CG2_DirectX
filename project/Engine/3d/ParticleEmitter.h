#pragma once
#include "struct.h" // Transform構造体やVector3などが定義されている前提
#include <string>

// 前方宣言
class ParticleManager;

class ParticleEmitter {
public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	/// <param name="name">パーティクルマネージャーに登録したグループ名</param>
	/// <param name="transform">エミッタの位置・回転・サイズ</param>
	/// <param name="count">1回の発生で出るパーティクル数</param>
	/// <param name="frequency">1秒間の発生回数 (例: 60.0fなら1秒に60回)</param>
	ParticleEmitter(const std::string& name, const Transform& transform, uint32_t count, float frequency);

	/// <summary>
	/// 更新処理
	/// </summary>
	/// <param name="deltaTime">フレーム経過時間</param>
	void Update(float deltaTime);

	/// <summary>
	/// パーティクルの発生
	/// </summary>
	void Emit();

	// エミッタ自体の座標を更新するためのセッター
	void SetTranslate(const Vector3& translate) { transform_.translate = translate; }

	// 発生頻度の変更
	void SetFrequency(float frequency) { frequency_ = frequency; }

private:
	// --- メンバ変数 ---
	std::string groupName_; // 対象のパーティクルグループ名
	Transform transform_;   // エミッタの座標（ここからパーティクルが出る）
	uint32_t count_;        // 一度の発生数
	float frequency_;       // 発生頻度（回/秒）
	float frequencyTimer_;  // 発生頻度調整用タイマー
};