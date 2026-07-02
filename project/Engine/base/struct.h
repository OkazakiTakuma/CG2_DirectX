#pragma once
#include <string>
#include <vector>
#include"Quaternion.h"
#include <format>
#include <fstream>
#include <locale>
#include <strsafe.h>
#include <wrl.h>
#include <map>

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
	Transform transform;
	Vector3 velocity;
	Vector4 color;
	float lifeTime;
	float currentTime;
	bool isBillboard;

	// ★以下を追加（時間変化と加速度用）
	Vector3 acceleration; // 加速度（重力など）
	Vector4 startColor;   // 発生時の色
	Vector4 endColor;     // 消える時の色
	Vector3 startScale;   // 発生時の大きさ
	Vector3 endScale;     // 消える時の大きさ
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
	Vector3 scale = { 1.0f, 1.0f, 1.0f };
	// ★追加: 消える時の大きさ（デフォルトは0にしてスッと消えるように）
	Vector3 endScale = { 0.0f, 0.0f, 0.0f };

	Vector3 baseVelocity = { 0.0f, 0.0f, 0.0f };
	Vector3 randomVelocityRange = { 0.1f, 0.1f, 0.1f };

	// ★追加: 加速度（Yをマイナスにすれば重力になります）
	Vector3 acceleration = { 0.0f, 0.0f, 0.0f };

	Vector3 randomPositionRange = { 0.0f, 0.0f, 0.0f };
	float lifeTime = 1.0f;
	Vector3 baseRotate = { 0.0f, 0.0f, 0.0f };
	bool isRandomRotate = false;
	Vector3 randomRotateRange = { 0.0f, 0.0f, 0.0f };
	Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

	// ★追加: 消える時の色（デフォルトはアルファ値を0にして透明にする）
	Vector4 endColor = { 1.0f, 1.0f, 1.0f, 0.0f };

	Vector3 randomScaleRange = { 0.0f, 0.0f, 0.0f };
	uint32_t count = 1;
	bool isBillboard = false;
};

enum ParticleMeshType {
	kMeshTypeQuad, // 通常の四角形
	kMeshTypeRing,  // リング形状
	kMeshTypeCylinder
};

struct ParticleSetting {
	std::string groupName;
	std::string texturePath;
	ParticleMeshType meshType;
	BlendMode blendMode;
};

template <typename tValue>
struct Keyframe {
	float time; // キーフレームの時間
	tValue value; // キーフレームの値
};
using KeyframeVector3 = Keyframe<Vector3>;
using KeyframeQuaternion = Keyframe<Quaternion>;

template <typename tValue>
struct AnimationCurve {
	std::vector<Keyframe<tValue>> keyframes;
};
struct NodeAnimation {
	AnimationCurve<Vector3> translate;
	AnimationCurve<Quaternion> rotate;
	AnimationCurve<Vector3> scale;
};

struct Animation {
	float duration; // アニメーションの総時間
	std::map<std::string, NodeAnimation> nodeAnimations; // ノード名とそのアニメーションのマッピング
};

