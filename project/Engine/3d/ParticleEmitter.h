#pragma once
#include "struct.h" // Transform構造体やVector3、BlendModeなどが定義されている前提
#include <string>
#include <vector>
#include <json.hpp>

// 前方宣言
class ParticleManager;

class ParticleEmitter {
public:

	void EmitLightning(const Vector3& targetPosition);

	ParticleEmitter();

	/// <summary>
	/// 更新処理
	/// </summary>
	/// <param name="deltaTime">フレーム経過時間</param>
	void Update(float deltaTime);

	/// <summary>
	/// パーティクルの発生
	/// </summary>
	void Emit();

	// エミッターの種類（名前）を指定
	void SetGroupName(const std::string& name) { groupName_ = name; }
	void SetTranslate(const Vector3& translate) { transform_.translate = translate; }
	void SetFrequency(float frequency) { frequency_ = frequency; }

	// ステータスのセッター
	void SetEmitParam(const ParticleEmitParam& palam) { emitParam_ = palam; }
	void SetScale(const Vector3& scale) { emitParam_.scale = scale; }
	void SetBaseVelocity(const Vector3& velocity) { emitParam_.baseVelocity = velocity; }
	void SetRandomVelocityRange(const Vector3& range) { emitParam_.randomVelocityRange = range; }
	void SetRandomPositionRange(const Vector3& range) { emitParam_.randomPositionRange = range; }
	void SetLifeTime(float lifeTime) { emitParam_.lifeTime = lifeTime; }
	void SetTexture(const std::string& textureFilePath);
	void SetBaseRotate(const Vector3& baseRotate) { emitParam_.baseRotate = baseRotate; }
	void SetIsActive(bool isActive) { isActive_ = isActive; }
	void SetBlendMode(BlendMode mode) { blendMode_ = mode; }
	void SetParam(ParticleEmitParam param) { emitParam_ = param; }
	void SetMeshType(ParticleMeshType type) { meshType_ = type; }
	// ステータスのゲッター
	float GetFrequency() const { return frequency_; }
	Vector3 GetTlanslate()const { return transform_.translate; }
	Vector3 GetScale() const { return emitParam_.scale; }
	Vector3 GetBaseVelocity() const { return emitParam_.baseVelocity; }
	Vector3 GetRandomVelocityRange() const { return emitParam_.randomVelocityRange; }
	Vector3 GetRandomPositionRange() const { return emitParam_.randomPositionRange; }
	float GetLifeTime() const { return emitParam_.lifeTime; }
	std::string GetTextureFilePath() const { return textureFilePath_; }
	Vector3 GetBaseRotate() const { return emitParam_.baseRotate; }
	bool GetIsRandomRotate() const { return emitParam_.isRandomRotate; }
	Vector3 GetRandomRotateRange() const { return emitParam_.randomRotateRange; }
	Vector4 GetColor() const { return emitParam_.color; }
	Vector3 GetRandomScaleRange() const { return emitParam_.randomScaleRange; }
	uint32_t GetCount() const { return emitParam_.count; }
	bool GetIsBillboard() const { return emitParam_.isBillboard; }
	ParticleEmitParam GetPalam() const { return emitParam_; }
	std::string GetGroupName() const { return groupName_; }
	bool GetIsActive() const { return isActive_; }
	BlendMode GetBlendMode() const { return blendMode_; }
	ParticleMeshType GetMeshType() const { return meshType_; }

	void SaveToJson(const std::string& filePath = "Resources/Data/emit_status.json");
	void LoadFromJson(const std::string& filePath = "Resources/Data/emit_status.json");

private:
	// --- メンバ変数 ---
	std::string groupName_; // 対象のパーティクルグループ名
	Transform transform_;   // エミッタの座標（ここからパーティクルが出る）
	uint32_t count_;        // 一度に発生させる数
	float frequency_;       // 発生頻度（秒に何回か、または何秒に1回か）
	float frequencyTimer_;  // 頻度を管理するタイマー
	std::string textureFilePath_; // 設定されているテクスチャのパス
	bool isActive_;         // エミッターのオン・オフフラグ
	BlendMode blendMode_ = kBlendModeNormal; // ★追加：ブレンドモード（初期値は通常）
	ParticleMeshType meshType_ = kMeshTypeQuad;
	struct LightningLine {
		Vector3 start;
		Vector3 end;
		Vector4 color;
		float lifeTime;
		float currentTime;
	};
	std::vector<LightningLine> lightningLines_;
	ParticleEmitParam emitParam_; // 発生するパーティクルのパラメータ
};
