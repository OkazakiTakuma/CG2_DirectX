#pragma once
#include "struct.h" // Emitter構造体が定義されている前提
#include <string>

// 前方宣言 (ParticleManager.hをincludeすると循環参照の可能性がある場合)
// class ParticleManager;

class ParticleEmitter {
public:
	// コンストラクタ
	// name: ParticleManagerに登録したグループ名
	// emitterData: 射出頻度や位置などの設定データ
	ParticleEmitter(const std::string& name, const Emitter& emitterData);

	void Update(float deltaTime);
	void Emit();

private:
	Emitter data;          // 射出設定（位置、数、頻度など）
	std::string groupName; // ParticleManager で管理しているグループのキー
	bool isActive = true;
};