#pragma once

#include "Component.h"
#include "GameObject.h"
#include "MathConstants.h"
#include "../base/GameTime.h"

/// <summary>敵弾の直進移動、攻撃力、寿命、命中状態を管理します。</summary>
class EnemyProjectileComponent : public Component {
public:
	void Update() override {
		if (!GetOwner()) {
			return;
		}
		// 60FPS基準の速度を実際のフレーム時間に合わせて補正する。
		GetOwner()->GetTransform().translate = GetOwner()->GetTransform().translate +
			(speed_ * GameTime::GetFrameScale60()) * direction_;
		elapsedTime_ += GameTime::GetDeltaTime();
	}

	void SetDirection(const Vector3& direction) {
		direction_ = Length(direction) > MathConstants::kDirectionEpsilon ? Normalize(direction) : Vector3{0.0f, 0.0f, 1.0f};
	}
	const Vector3& GetDirection() const { return direction_; }
	void SetSpeed(float speed) { speed_ = (std::max)(0.0f, speed); }
	void SetAttack(float attack) { attack_ = (std::max)(0.0f, attack); }
	float GetAttack() const { return attack_; }
	void SetSize(float size) { size_ = (std::max)(0.01f, size); }
	float GetSize() const { return size_; }
	void SetLifeTime(float lifeTime) { lifeTime_ = (std::max)(0.0f, lifeTime); }
	bool IsExpired() const { return hit_ || elapsedTime_ >= lifeTime_; }
	void MarkHit() { hit_ = true; }

private:
	/// <summary>正規化された弾の進行方向です。</summary>
	Vector3 direction_{0.0f, 0.0f, 1.0f};
	/// <summary>60FPS時の1フレームあたりの移動量です。</summary>
	float speed_ = 0.12f;
	/// <summary>プレイヤーへ与えるダメージ量です。</summary>
	float attack_ = 1.0f;
	/// <summary>表示と衝突判定に使用する弾の大きさです。</summary>
	float size_ = 0.22f;
	/// <summary>未命中でも弾を破棄するまでの秒数です。</summary>
	float lifeTime_ = 6.0f;
	/// <summary>生成後の経過秒数です。</summary>
	float elapsedTime_ = 0.0f;
	/// <summary>衝突処理済みかを表します。</summary>
	bool hit_ = false;
};
