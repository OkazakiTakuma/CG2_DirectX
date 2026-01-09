#pragma once
#include <string>
#include <vector>
#include "Matrix.h"

enum BlendMode {
	kBlendModeNone,
	kBlendModeNormal,
	kBlendModeAdd,
	kBlendModeSubtract,
	kBlendModeMultiply,
	kBlendModeScreen,
	kBlendCountblend,
};
struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};
struct MaterialData {
	std::string textureFilePath; // テクスチャファイルのパス
	uint32_t textureIndex = 0;
};

struct ModelData {
	std::vector<VertexData> vertices; // 頂点データ
	MaterialData material;
};
struct Particle {
	Transform transform; // SRT情報
	Vector3 velocity;     // 速度
	Vector4 color;        // 色
	float lifeTimme;
	float currentTime;
};
struct Emitter {
	Transform transform; // エミッタの位置情報
	uint32_t count;       // パーティクルの数
	float frequency;      // 発生頻度（秒間）
	float frequencyTimer; // 発生頻度タイマー
};