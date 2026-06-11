#pragma once
#include "struct.h" // Transform構造体やVector3などが定義されている前提
#include <string>
#include<json.hpp>

// 前方宣言
class ParticleManager;

class ParticleEmitter {
public:
	
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
	void SetScale(const Vector3& scale) { emitParam_.scale = scale; }
	void SetBaseVelocity(const Vector3& velocity) { emitParam_.baseVelocity = velocity; }
	void SetRandomVelocityRange(const Vector3& range) { emitParam_.randomVelocityRange = range; }
	void SetRandomPositionRange(const Vector3& range) { emitParam_.randomPositionRange = range; }
	void SetLifeTime(float lifeTime) { emitParam_.lifeTime = lifeTime; }
	void SetTexture(const std::string& textureFilePath);
	void SetBaseRotate(const Vector3& rotate) { emitParam_.baseRotate = rotate; }
	void SetIsRandomRotate(bool isRandom) { emitParam_.isRandomRotate = isRandom; }
	void SetRandomRotateRange(const Vector3& range) { emitParam_.randomRotateRange = range; }
	void SetColor(const Vector4& color) { emitParam_.color = color; }
	void SetRandomScaleRange(const Vector3& range) { emitParam_.randomScaleRange = range; }
	void SetCount(uint32_t count) { emitParam_.count = count; } // 既存の count_ = count から変更
	void SetIsBillboard(bool isBillboard) { emitParam_.isBillboard = isBillboard; }
	void SetPalam(const ParticleEmitParam& palam) { emitParam_ = palam; };
	// ステータスのゲッター
	Vector3 GetTlanslate()const { return transform_.translate; }
	Vector3 GetScale() const { return emitParam_.scale; }
	Vector3 GetBaseVelocity() const { return emitParam_.baseVelocity; }
	Vector3 GetRandomVelocityRange() const { return emitParam_.randomVelocityRange; }
	Vector3 GetRandomPositionRange() const { return emitParam_.randomPositionRange; }
	float GetLifeTime() const { return emitParam_.lifeTime; }
	Vector3 GetBaseRotate() const { return emitParam_.baseRotate; }
	bool GetIsRandomRotate() const { return emitParam_.isRandomRotate; }
	Vector3 GetRandomRotateRange() const { return emitParam_.randomRotateRange; }
	Vector4 GetColor() const { return emitParam_.color; }
	Vector3 GetRandomScaleRange() const { return emitParam_.randomScaleRange; }
	uint32_t GetCount() const { return emitParam_.count; }
	bool GetIsBillboard() const { return emitParam_.isBillboard; }
	ParticleEmitParam GetPalam() const { return emitParam_; }

	void SaveToJson(const std::string& filePath = "Resources/Data/emit_status.json");
	void LoadFromJson(const std::string& filePath = "Resources/Data/emit_status.json");
private:
	// --- メンバ変数 ---
	std::string groupName_; // 対象のパーティクルグループ名
	Transform transform_;   // エミッタの座標（ここからパーティクルが出る）
	uint32_t count_;        // 一度の発生数
	float frequency_;       // 発生頻度（回/秒）
	float frequencyTimer_;  // 発生頻度調整用タイマー
	ParticleEmitParam emitParam_;
	std::string textureFilePath_; // テクスチャファイルパス
};