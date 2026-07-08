#pragma once
#include "../flame/Component.h"
#include "../flame/GameObject.h"
#include "Camera.h"
#include <memory>
#include <string>

class CameraComponent : public Component {
public:
	void Initialize() override {
		camera_ = std::make_unique<Camera>();
		SyncOwnerTransformToCamera();
	}

	void Update() override {
		if (!camera_) {
			return;
		}

		if (followTarget_) {
			EulerTransform& ownerTransform = GetOwner()->GetTransform();
			const EulerTransform& targetTransform = followTarget_->GetTransform();
			ownerTransform.translate = targetTransform.translate;
			ownerTransform.rotate = targetTransform.rotate;
		}

		SyncOwnerTransformToCamera();
		camera_->Update();
	}

	void Finalize() override {
		camera_.reset();
		followTarget_ = nullptr;
	}

	Camera* GetCamera() const { return camera_.get(); }

	void SetFollowTarget(GameObject* target) {
		followTarget_ = target;
		followTargetName_ = target ? target->GetName() : "";
	}
	GameObject* GetFollowTarget() const { return followTarget_; }
	const std::string& GetFollowTargetName() const { return followTargetName_; }
	void SetFollowTargetName(const std::string& name) { followTargetName_ = name; }

	float GetFovY() const { return camera_ ? camera_->GetFovY() : 0.45f; }
	void SetFovY(float fovY) {
		if (camera_) {
			camera_->SetfovY(fovY);
		}
	}
	float GetNearClip() const { return camera_ ? camera_->GetNearClip() : 0.1f; }
	void SetNearClip(float nearClip) {
		if (camera_) {
			camera_->SetNearClip(nearClip);
		}
	}
	float GetFarClip() const { return camera_ ? camera_->GetFarClip() : 100.0f; }
	void SetFarClip(float farClip) {
		if (camera_) {
			camera_->SetFarClip(farClip);
		}
	}

private:
	void SyncOwnerTransformToCamera() {
		if (!camera_ || GetOwner() == nullptr) {
			return;
		}

		const EulerTransform& transform = GetOwner()->GetTransform();
		camera_->SetTranslate(transform.translate);
		camera_->SetRotate(transform.rotate);
	}

	std::unique_ptr<Camera> camera_;
	GameObject* followTarget_ = nullptr;
	std::string followTargetName_;
};
