#pragma once
#include <string>
#include <vector>
#include "Matrix.h"
#include <format>
#include <fstream>
#include <locale>
#include <strsafe.h>
#include <wrl.h>

struct TransformationMatrix {
	Matrix4x4 WVP;
	Matrix4x4 world;
	Matrix4x4 WorldInverseTranspose;
};

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

struct Material {
	Vector4 color;
	int32_t enableLighting;
	float padding[3];
	Matrix4x4 uvTransform;
	float shininess;   // ★追加
	float padding2[3]; // 16バイトアライメントのためのパディング
};

struct MaterialData {
	std::string textureFilePath; // テクスチャファイルのパス
	uint32_t textureIndex = 0;
};

// Define Node before ModelData so it is a complete type when used
struct Node {
	Matrix4x4 localMatrix;
	std::string name;
	std::vector<Node> children;
};

struct ModelData {
	std::vector<VertexData> vertices; // 頂点データ
	MaterialData material;
	Node rootNode;
};
struct Particle {
	Transform transform; // SRT情報
	Vector3 velocity;     // 速度
	Vector4 color;        // 色
	float lifeTime;
	float currentTime;
};
struct Emitter {
	Transform transform; // エミッタの位置情報
	uint32_t count;       // パーティクルの数
	float frequency;      // 発生頻度（秒間）
	float frequencyTimer; // 発生頻度タイマー
};
struct ParticleForGPU {
	Matrix4x4 WVP;
	Matrix4x4 world;
	Vector4 color;
};
struct SoundData {
	WAVEFORMATEX wfx;
	std::vector<BYTE> buffer;
};

struct PointLight {
	Vector4 color;    // 光の色
	Vector3 position; // 光の位置
	float intensity;  // 光の強度
	float radius;     // 光の半径
	float decay;      // 光の減衰率
	float padding[2]; // ★16バイトアライメントのためのパディング
};

struct ParticleEmitParam {
	Vector3 scale = { 1.0f, 1.0f, 1.0f };               // 大きさ
	Vector3 baseVelocity = { 0.0f, 0.0f, 0.0f };        // 基礎速度
	Vector3 randomVelocityRange = { 0.1f, 0.1f, 0.1f }; // 乱数で加算される速度の範囲
	Vector3 randomPositionRange = { 0.5f, 0.5f, 0.5f }; // 発生位置の範囲
	float lifeTime = 1.0f;
	Vector3 baseRotate;         // 角度の基本ステータス
	bool isRandomRotate;        // 角度を乱数にするかどうかのフラグ
	Vector3 randomRotateRange;  // 乱数の範囲// 存在時間
	Vector4 color;
	Vector3 randomScaleRange;  // スケールの乱数幅
	uint32_t count;            // 発生数（パラメータとして一括管理）
};