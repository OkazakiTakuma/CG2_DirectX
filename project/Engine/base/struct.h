#pragma once
#include <string>
#include <vector>
#include <optional>
#include"Matrix.h"
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
	uint32_t boneIndices[4] = {};
	Vector4 boneWeights = {0.0f, 0.0f, 0.0f, 0.0f};
};

struct Material {
	Vector4 color;
	int32_t enableLighting;
	float padding[3];
	Matrix4x4 uvTransform;
	float shininess;
	float padding2[3];
};

struct VertexWeghtData
{
	float weght;
	uint32_t vertexIndex;
};

struct JointWeghtData
{
	Matrix4x4 inverseBindPoseMatrix;
	std::vector<VertexWeghtData> vertexWeights;
	uint32_t paletteIndex = 0;
};

struct MaterialData {
	std::string textureFilePath;
	uint32_t textureIndex = 0;
};

// Define Node before ModelData so it is a complete type when used
struct Node {
	QuaternionTransform transform;
	Matrix4x4 localMatrix;
	std::string name;
	std::vector<Node> children;
};

struct ModelData {
	std::map<std::string, JointWeghtData> skincluserData;
	std::vector<VertexData> vertices;
	std::vector<uint32_t> indices;
	MaterialData material;
	Node rootNode;
};
struct Particle {
	EulerTransform transform;
	Vector3 velocity;
	Vector4 color;
	float lifeTime;
	float currentTime;
	bool isBillboard;

	Vector3 acceleration;
	Vector4 startColor;
	Vector4 endColor;
	Vector3 startScale;
	Vector3 endScale;
};

struct Emitter {
	EulerTransform transform;
	uint32_t count;
	float frequency;
	float frequencyTimer;
};
struct ParticleForGPU {
	Vector3 translate;
	float isBillboard;
	Vector3 scale;
	float padding0;
	Vector3 rotate;
	float padding1;
	Vector4 color;
};
struct SoundData {
	WAVEFORMATEX wfx;
	std::vector<BYTE> buffer;
};

struct PointLight {
	Vector4 color;
	Vector3 position;
	float intensity;
	float radius;
	float decay;
	float padding[2];
};

struct ParticleEmitParam {
	Vector3 scale = { 1.0f, 1.0f, 1.0f };
	Vector3 endScale = { 0.0f, 0.0f, 0.0f };

	Vector3 baseVelocity = { 0.0f, 0.0f, 0.0f };
	Vector3 randomVelocityRange = { 0.1f, 0.1f, 0.1f };

	Vector3 acceleration = { 0.0f, 0.0f, 0.0f };

	Vector3 randomPositionRange = { 0.0f, 0.0f, 0.0f };
	float lifeTime = 1.0f;
	Vector3 baseRotate = { 0.0f, 0.0f, 0.0f };
	bool isRandomRotate = false;
	Vector3 randomRotateRange = { 0.0f, 0.0f, 0.0f };
	Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

	Vector4 endColor = { 1.0f, 1.0f, 1.0f, 0.0f };

	Vector3 randomScaleRange = { 0.0f, 0.0f, 0.0f };
	uint32_t count = 1;
	bool isBillboard = false;
};

enum ParticleMeshType {
	kMeshTypeQuad,
	kMeshTypeRing,
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
	float time;
	tValue value;
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
	float duration = 0.0f;
	std::map<std::string, NodeAnimation> nodeAnimations;
};

struct Joint
{
	QuaternionTransform transform;
	Matrix4x4 localMatrix;
	Matrix4x4 skeletonSpaceMatrix;
	std::string name;
	std::vector<int32_t> children;
	int32_t index;
	std::optional<int32_t> parent;
};

struct Skeleton
{
	int32_t root;
	std::map<std::string, int32_t> jointMap;
	std::vector<Joint> joints;
};
