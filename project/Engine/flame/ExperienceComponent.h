#pragma once
#include "Component.h"
#include "GameObject.h"
#include "../../Player/Player.h"
#include <string>

class ExperienceComponent : public Component {
public:
	void Update() override {
		GameObject* owner = GetOwner();
		if (!owner || !target_) {
			return;
		}

		Vector3 toTarget = target_->GetTransform().translate - owner->GetTransform().translate;
		const float distance = Length(toTarget);
		if (distance > attractDistance_) {
			return;
		}
		if (distance <= collectDistance_) {
			if (Player* player = target_->GetComponent<Player>()) {
				player->AddExperience(experience_);
			}
			collected_ = true;
			return;
		}

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

private:
	int experience_ = 1;
	float attractDistance_ = 5.0f;
	float collectDistance_ = 0.6f;
	float attractSpeed_ = 0.08f;
	std::string modelFilePath_ = "sphere.obj";
	GameObject* target_ = nullptr;
	bool collected_ = false;
};
