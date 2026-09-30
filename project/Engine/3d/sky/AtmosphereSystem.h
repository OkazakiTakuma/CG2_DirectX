#pragma once

#include "Vector.h"

/// <summary>
/// 空、太陽光、遠景の霞が同じ太陽方向を参照するための共有設定です。
/// 描画機能は持たず、各レンダラーへ同じ値を渡す役割だけを担当します。
/// </summary>
struct AtmosphereSettings {
	bool enabled = false;
	Vector3 sunDirection{-0.18f, 0.03f, 0.98f};
	float sunAngularRadius = 0.022f;
	float sunIntensity = 3.5f;
	Vector3 rayleighColor{0.20f, 0.42f, 1.0f};
	float rayleighStrength = 0.72f;
	Vector3 mieColor{1.0f, 0.56f, 0.20f};
	float mieStrength = 0.075f;
	float mieG = 0.80f;
	float atmosphereDensity = 0.92f;
	Vector3 horizonColor{0.82f, 0.18f, 0.055f};
	float distanceFogDensity = 0.012f;
	float heightFogDensity = 0.16f;
	float aerialPerspectiveStrength = 0.68f;
};

/// <summary>
/// シーン全体で使用する大気設定を一元管理します。
/// タイトル終了時にResetすることで、距離霞が他シーンへ残ることを防ぎます。
/// </summary>
class AtmosphereSystem final {
public:
	static AtmosphereSystem* GetInstance() {
		static AtmosphereSystem instance;
		return &instance;
	}

	AtmosphereSettings& GetSettings() { return settings_; }
	const AtmosphereSettings& GetSettings() const { return settings_; }

	void Reset() { settings_ = AtmosphereSettings{}; }

private:
	AtmosphereSystem() = default;
	AtmosphereSettings settings_{};
};
