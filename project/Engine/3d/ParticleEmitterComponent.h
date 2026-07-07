#pragma once
#include "../flame/Component.h"
#include "ParticleEmitter.h"
#include <memory>
#include <string>

class ParticleEmitterComponent : public Component {
public:
	void Initialize() override {
		emitter_ = std::make_unique<ParticleEmitter>();
	}

	void Update() override {
		Update(1.0f / 60.0f);
	}

	void Update(float deltaTime) {
		if (emitter_) {
			emitter_->Update(deltaTime);
		}
	}

	void Finalize() override {
		emitter_.reset();
	}

	ParticleEmitter* GetEmitter() const { return emitter_.get(); }

	void EmitLightning(const Vector3& targetPosition) { emitter_->EmitLightning(targetPosition); }
	void Emit() { emitter_->Emit(); }

	void SetGroupName(const std::string& name) { emitter_->SetGroupName(name); }
	void SetTranslate(const Vector3& translate) { emitter_->SetTranslate(translate); }
	void SetFrequency(float frequency) { emitter_->SetFrequency(frequency); }
	void SetEmitParam(const ParticleEmitParam& param) { emitter_->SetEmitParam(param); }
	void SetScale(const Vector3& scale) { emitter_->SetScale(scale); }
	void SetBaseVelocity(const Vector3& velocity) { emitter_->SetBaseVelocity(velocity); }
	void SetRandomVelocityRange(const Vector3& range) { emitter_->SetRandomVelocityRange(range); }
	void SetRandomPositionRange(const Vector3& range) { emitter_->SetRandomPositionRange(range); }
	void SetLifeTime(float lifeTime) { emitter_->SetLifeTime(lifeTime); }
	void SetTexture(const std::string& textureFilePath) { emitter_->SetTexture(textureFilePath); }
	void SetBaseRotate(const Vector3& baseRotate) { emitter_->SetBaseRotate(baseRotate); }
	void SetIsActive(bool isActive) { emitter_->SetIsActive(isActive); }
	void SetBlendMode(BlendMode mode) { emitter_->SetBlendMode(mode); }
	void SetParam(ParticleEmitParam param) { emitter_->SetParam(param); }
	void SetMeshType(ParticleMeshType type) { emitter_->SetMeshType(type); }

	float GetFrequency() const { return emitter_->GetFrequency(); }
	Vector3 GetTlanslate() const { return emitter_->GetTlanslate(); }
	Vector3 GetScale() const { return emitter_->GetScale(); }
	Vector3 GetBaseVelocity() const { return emitter_->GetBaseVelocity(); }
	Vector3 GetRandomVelocityRange() const { return emitter_->GetRandomVelocityRange(); }
	Vector3 GetRandomPositionRange() const { return emitter_->GetRandomPositionRange(); }
	float GetLifeTime() const { return emitter_->GetLifeTime(); }
	std::string GetTextureFilePath() const { return emitter_->GetTextureFilePath(); }
	Vector3 GetBaseRotate() const { return emitter_->GetBaseRotate(); }
	bool GetIsRandomRotate() const { return emitter_->GetIsRandomRotate(); }
	Vector3 GetRandomRotateRange() const { return emitter_->GetRandomRotateRange(); }
	Vector4 GetColor() const { return emitter_->GetColor(); }
	Vector3 GetRandomScaleRange() const { return emitter_->GetRandomScaleRange(); }
	uint32_t GetCount() const { return emitter_->GetCount(); }
	bool GetIsBillboard() const { return emitter_->GetIsBillboard(); }
	ParticleEmitParam GetPalam() const { return emitter_->GetPalam(); }
	std::string GetGroupName() const { return emitter_->GetGroupName(); }
	bool GetIsActive() const { return emitter_->GetIsActive(); }
	BlendMode GetBlendMode() const { return emitter_->GetBlendMode(); }
	ParticleMeshType GetMeshType() const { return emitter_->GetMeshType(); }

	void SaveToJson(const std::string& filePath = "Resources/Data/emit_status.json") { emitter_->SaveToJson(filePath); }
	void LoadFromJson(const std::string& filePath = "Resources/Data/emit_status.json") { emitter_->LoadFromJson(filePath); }

private:
	std::unique_ptr<ParticleEmitter> emitter_;
};
