#pragma once
#include "../flame/Component.h"
#include "../flame/GameObject.h"
#include "Object3d.h"
#include "Object3dCommon.h"
#include <memory>

class Object3dComponent : public Component {
public:
	void Initialize() override {
		object3d_ = std::make_unique<Object3d>();
		object3d_->Initialize();
		SyncOwnerTransformToObject();
	}

	void Update() override {
		if (!object3d_) {
			return;
		}
		SyncOwnerTransformToObject();
		object3d_->Update();
	}

	void Draw3D() override {
		if (object3d_) {
			Object3dCommon::GetInstance()->SetDraw();
			object3d_->Draw();
		}
	}

	void Finalize() override {
		object3d_.reset();
	}

	Object3d* GetObject3d() const { return object3d_.get(); }

	void SetModel(Model* model) { object3d_->SetModel(model); }
	void SetModel(const std::string& filePath) { object3d_->SetModel(filePath); }
	void CreateCylinder(float radius = 1.0f, float height = 2.0f, uint32_t subdivision = 16, bool createTopCap = true, bool createBottomCap = true) {
		object3d_->CreateCylinder(radius, height, subdivision, createTopCap, createBottomCap);
	}
	void SetTexture(const std::string& textureFilePath) { object3d_->SetTexture(textureFilePath); }

	const Vector3& GetTranslate() { return GetOwner()->GetTransform().translate; }
	void SetTranslate(const Vector3& translate) {
		GetOwner()->GetTransform().translate = translate;
		SyncOwnerTransformToObject();
	}
	const Vector3& GetRotate() { return GetOwner()->GetTransform().rotate; }
	void SetRotate(const Vector3& rotate) {
		GetOwner()->GetTransform().rotate = rotate;
		SyncOwnerTransformToObject();
	}
	const Vector3& GetScale() { return GetOwner()->GetTransform().scale; }
	void SetScale(const Vector3& scale) {
		GetOwner()->GetTransform().scale = scale;
		SyncOwnerTransformToObject();
	}

	void SetCamera(Camera* camera) { object3d_->SetCamera(camera); }
	void SetDirectionalLight(const Vector4& color, const Vector3& direction, float intensity) {
		object3d_->SetDirectionalLight(color, direction, intensity);
	}
	void SetPointLight(const Vector4& color, const Vector3& position, float intensity, float radius, float decay) {
		object3d_->SetPointLight(color, position, intensity, radius, decay);
	}
	void SetEnvironmentMultiplier(float multiplier) { object3d_->SetEnvironmentMultiplier(multiplier); }
	void IsPointLightSet(bool isSet) { object3d_->IsPointLightSet(isSet); }
	void SetEnvironmentMap(const std::string& textureFilePath) { object3d_->SetEnvironmentMap(textureFilePath); }
	void SetDrawSkeleton(bool isDraw) { object3d_->SetDrawSkeleton(isDraw); }
	bool GetDrawSkeleton() const { return object3d_->GetDrawSkeleton(); }

	const Vector4 GetLightColor() const { return object3d_->GetLightColor(); }
	const Vector3 GetLightDirection() const { return object3d_->GetLightDirection(); }
	const float GetLightIntensity() const { return object3d_->GetLightIntensity(); }
	const Vector4 GetPointLightColor() const { return object3d_->GetPointLightColor(); }
	const Vector3 GetPointLightPosition() const { return object3d_->GetPointLightPosition(); }
	const float GetPointLightIntensity() const { return object3d_->GetPointLightIntensity(); }
	const float GetPointLightRadius() const { return object3d_->GetPointLightRadius(); }
	const float GetPointLightDecay() const { return object3d_->GetPointLightDecay(); }
	const bool GetIsPointLightSet() const { return object3d_->GetIsPointLightSet(); }
	const float GetEnvironmentMultiplier() const { return object3d_->GetEnvironmentMultiplier(); }

private:
	void SyncOwnerTransformToObject() {
		if (!object3d_ || GetOwner() == nullptr) {
			return;
		}

		const EulerTransform& transform = GetOwner()->GetTransform();
		object3d_->SetScale(transform.scale);
		object3d_->SetRotate(transform.rotate);
		object3d_->SetTranslate(transform.translate);
	}

	std::unique_ptr<Object3d> object3d_;
};
