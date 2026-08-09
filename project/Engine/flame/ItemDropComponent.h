#pragma once
#include "Component.h"
#include "GameObject.h"
#include "../../Player/Player.h"

/// <summary>敵が落とす特殊アイテムの種類です。</summary>
enum class ItemDropType {
	Health,
	CollectAllExperience
};

/// <summary>特殊アイテムの接触回収と、プレイヤーへ直接作用する効果を管理します。</summary>
class ItemDropComponent : public Component {
public:
	void Update() override {
		GameObject* owner = GetOwner();
		if (!owner || !target_ || collected_) {
			return;
		}

		// キャラクターとアイテムの原点の高さが異なっても拾えるよう、水平距離で判定する。
		Vector3 toTarget = target_->GetTransform().translate - owner->GetTransform().translate;
		toTarget.y = 0.0f;
		const float distance = Length(toTarget);
		if (distance > attractDistance_) {
			return;
		}
		if (distance > collectDistance_) {
			// 回収範囲へ入ったアイテムをプレイヤーの足元へ吸着させる。
			owner->GetTransform().translate = owner->GetTransform().translate + attractSpeed_ * toTarget;
			return;
		}

		Player* player = target_->GetComponent<Player>();
		if (!player) {
			return;
		}
		if (type_ == ItemDropType::Health) {
			player->SetCurrentHealth(player->GetCurrentHealth() + healAmount_);
			effectApplied_ = true;
		}
		collected_ = true;
	}

	void SetType(ItemDropType type) { type_ = type; }
	ItemDropType GetType() const { return type_; }
	void SetTarget(GameObject* target) { target_ = target; }
	GameObject* GetTarget() const { return target_; }
	void SetHealAmount(float amount) { healAmount_ = amount < 0.0f ? 0.0f : amount; }
	float GetHealAmount() const { return healAmount_; }
	void SetAttractDistance(float distance) { attractDistance_ = distance < 0.0f ? 0.0f : distance; }
	float GetAttractDistance() const { return attractDistance_; }
	void SetAttractSpeed(float speed) { attractSpeed_ = speed < 0.0f ? 0.0f : (speed > 1.0f ? 1.0f : speed); }
	float GetAttractSpeed() const { return attractSpeed_; }
	void SetCollectDistance(float distance) { collectDistance_ = distance < 0.0f ? 0.0f : distance; }
	float GetCollectDistance() const { return collectDistance_; }
	bool IsCollected() const { return collected_; }
	bool IsEffectApplied() const { return effectApplied_; }
	void MarkEffectApplied() { effectApplied_ = true; }

private:
	ItemDropType type_ = ItemDropType::Health;
	GameObject* target_ = nullptr;
	float healAmount_ = 25.0f;
	float attractDistance_ = 3.0f;
	float attractSpeed_ = 0.12f;
	float collectDistance_ = 0.65f;
	bool collected_ = false;
	bool effectApplied_ = false;
};
