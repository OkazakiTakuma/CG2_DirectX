#pragma once
#include "Component.h"
#include "GameObject.h"
#include "../../Player/Player.h"
#include <string>

/// <summary>経験値アイテムをプレイヤーへ吸着させ、接触時に経験値を付与します。</summary>
class ExperienceComponent : public Component {
public:
	void Update() override {
		GameObject* owner = GetOwner();
		if (!owner || !target_) {
			return;
		}

		// 対象との距離に応じて、待機・吸着・回収の3状態を切り替える。
		Vector3 toTarget = target_->GetTransform().translate - owner->GetTransform().translate;
		const float distance = Length(toTarget);
		if (distance > attractDistance_) {
			return;
		}
		if (distance <= collectDistance_) {
			// 対象がPlayerを持つ場合だけ経験値を反映し、アイテムを回収済みにする。
			if (Player* player = target_->GetComponent<Player>()) {
				player->AddExperience(experience_);
			}
			collected_ = true;
			return;
		}

		// 1フレームあたりの割合で対象位置へ近づける。
		const float lerpRate = attractSpeed_;
		owner->GetTransform().translate = owner->GetTransform().translate + lerpRate * toTarget;
	}

	void SetExperience(int experience) { experience_ = experience < 0 ? 0 : experience; }
	int GetExperience() const { return experience_; }
	void SetModelFilePath(const std::string& modelFilePath) { modelFilePath_ = modelFilePath; }
	const std::string& GetModelFilePath() const { return modelFilePath_; }
	void SetTarget(GameObject* target) { target_ = target; }
	GameObject* GetTarget() const { return target_; }
	void SetAttractDistance(float distance) { attractDistance_ = distance < 0.0f ? 0.0f : distance; }
	float GetAttractDistance() const { return attractDistance_; }
	void SetCollectDistance(float distance) { collectDistance_ = distance < 0.0f ? 0.0f : distance; }
	float GetCollectDistance() const { return collectDistance_; }
	void SetAttractSpeed(float speed) { attractSpeed_ = speed < 0.0f ? 0.0f : (speed > 1.0f ? 1.0f : speed); }
	float GetAttractSpeed() const { return attractSpeed_; }
	bool IsCollected() const { return collected_; }
	void MarkConsumedByCompression() { consumedByCompression_ = true; }
	bool IsConsumedByCompression() const { return consumedByCompression_; }

private:
	/// <summary>回収時にプレイヤーへ与える経験値です。</summary>
	int experience_ = 1;
	/// <summary>プレイヤーへの吸着を開始する距離です。</summary>
	float attractDistance_ = 5.0f;
	/// <summary>回収完了と判定する距離です。</summary>
	float collectDistance_ = 0.6f;
	/// <summary>1フレームで目標まで進む割合です。</summary>
	float attractSpeed_ = 0.08f;
	std::string modelFilePath_ = "sphere.obj";
	/// <summary>吸着対象となるGameObjectへの非所有参照です。</summary>
	GameObject* target_ = nullptr;
	/// <summary>通常の接触回収が完了したかを表します。</summary>
	bool collected_ = false;
	/// <summary>圧縮攻撃によって消費され、通常回収から除外されたかを表します。</summary>
	bool consumedByCompression_ = false;
};
