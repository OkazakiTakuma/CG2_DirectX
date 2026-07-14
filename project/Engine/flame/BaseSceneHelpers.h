#pragma once
#include "BaseScene.h"
#include "Model.h"
#include "ModelManager.h"
#include "TextureManager.h"
#ifdef USE_IMGUI
#include "../../../imgui/ImGuizmo.h"
#endif
#include <algorithm>
#include <filesystem>
#include <functional>
#include <fstream>
#pragma warning(push)
#pragma warning(disable: 26495)
#include <json.hpp>
#pragma warning(pop)
#include <cmath>
#include <cstring>
#include <iomanip>

namespace {
constexpr float kPi = 3.14159265358979323846f;
const char* kParticlePresetFilePath = "Resources/Data/emit_status.json";
const char* kEnemyStatusFilePath = "Resources/Data/enemy_status.json";

/// <summary>
/// Resources以下と読み込み済みテクスチャから選択可能な画像パスを集めます。
/// </summary>
std::vector<std::string> CollectResourceTexturePaths() {
	std::vector<std::string> paths = TextureManager::GetInstance()->GetLoadedTextureNames();
	const std::filesystem::path resourceRoot = "Resources";
	if (std::filesystem::exists(resourceRoot)) {
		for (const auto& entry : std::filesystem::recursive_directory_iterator(resourceRoot)) {
			if (!entry.is_regular_file()) {
				continue;
			}
			const std::string extension = entry.path().extension().string();
			if (extension != ".png" && extension != ".jpg" && extension != ".jpeg" && extension != ".dds") {
				continue;
			}
			std::string path = entry.path().generic_string();
			if (std::find(paths.begin(), paths.end(), path) == paths.end()) {
				paths.push_back(path);
			}
		}
	}
	std::sort(paths.begin(), paths.end());
	return paths;
}

std::vector<std::string> CollectResourceDdsTexturePaths() {
	std::vector<std::string> paths;
	for (const std::string& path : CollectResourceTexturePaths()) {
		if (std::filesystem::path(path).extension() == ".dds") {
			paths.push_back(path);
		}
	}
	return paths;
}

/// <summary>
/// 読み込み済みモデル名をアニメーション有無で絞り込んで取得します。
/// </summary>
std::vector<std::string> CollectLoadedModelNames(bool isAnimation) {
	std::vector<std::string> result;
	const std::vector<std::string> loadedModels = ModelManager::GetInstance()->GetLoadedModelNames();
	for (const std::string& modelName : loadedModels) {
		Model* model = ModelManager::GetInstance()->FindModel(modelName);
		if (model && model->GetIsAnimation() == isAnimation) {
			result.push_back(modelName);
		}
	}
	return result;
}

/// <summary>
/// 読み込み済みモデル名をすべて取得します。
/// </summary>
std::vector<std::string> CollectAllLoadedModelNames() {
	std::vector<std::string> result = ModelManager::GetInstance()->GetLoadedModelNames();
	std::sort(result.begin(), result.end());
	return result;
}

/// <summary>
/// ImGuiのComboへ渡す文字列ポインタ配列を作成します。
/// </summary>
std::vector<const char*> MakeLabelPointers(const std::vector<std::string>& labels) {
	std::vector<const char*> result;
	result.reserve(labels.size());
	for (const std::string& label : labels) {
		result.push_back(label.c_str());
	}
	return result;
}

/// <summary>
/// 値を指定範囲内に丸めます。
/// </summary>
float ClampFloat(float value, float minValue, float maxValue) {
	if (value < minValue) {
		return minValue;
	}
	if (value > maxValue) {
		return maxValue;
	}
	return value;
}

/// <summary>
/// 方向ベクトルを正規化し、長さがない場合は代替方向を使います。
/// </summary>
Vector3 NormalizeOrFallback(const Vector3& direction, const Vector3& fallback) {
	const float length = Length(direction);
	if (length <= 0.00001f) {
		return Normalize(fallback);
	}
	return Normalize(direction);
}

/// <summary>
/// OBBを指定軸へ射影した半径を計算します。
/// </summary>
float ProjectOBBRadius(const OBBColliderShape& obb, const Vector3& axis) {
	return
	    obb.halfSize.x * std::fabs(Dot(obb.orientation[0], axis)) +
	    obb.halfSize.y * std::fabs(Dot(obb.orientation[1], axis)) +
	    obb.halfSize.z * std::fabs(Dot(obb.orientation[2], axis));
}

/// <summary>
/// 指定点に最も近いOBB上のワールド座標を返します。
/// </summary>
Vector3 ClosestPointOnOBB(const OBBColliderShape& obb, const Vector3& point) {
	const Vector3 diff = point - obb.center;
	Vector3 result = obb.center;
	const float halfSizes[3] = {obb.halfSize.x, obb.halfSize.y, obb.halfSize.z};
	for (int axisIndex = 0; axisIndex < 3; ++axisIndex) {
		const float distance = ClampFloat(Dot(diff, obb.orientation[axisIndex]), -halfSizes[axisIndex], halfSizes[axisIndex]);
		result = result + distance * obb.orientation[axisIndex];
	}
	return result;
}

/// <summary>
/// Sphere同士の押し戻し方向と侵入量を計算します。
/// </summary>
bool CalculateSphereSpherePushBack(const SphereColliderShape& a, const SphereColliderShape& b, Vector3& directionAToB, float& penetration) {
	const Vector3 diff = b.center - a.center;
	const float distance = Length(diff);
	const float radius = a.radius + b.radius;
	penetration = radius - distance;
	if (penetration <= 0.0f) {
		return false;
	}
	directionAToB = NormalizeOrFallback(diff, {1.0f, 0.0f, 0.0f});
	return true;
}

/// <summary>
/// OBBとSphereの押し戻し方向と侵入量を計算します。
/// </summary>
bool CalculateOBBSpherePushBack(const OBBColliderShape& obb, const SphereColliderShape& sphere, Vector3& directionOBBToSphere, float& penetration) {
	const Vector3 closestPoint = ClosestPointOnOBB(obb, sphere.center);
	const Vector3 diff = sphere.center - closestPoint;
	const float distance = Length(diff);
	if (distance > 0.00001f) {
		penetration = sphere.radius - distance;
		if (penetration <= 0.0f) {
			return false;
		}
		directionOBBToSphere = Normalize(diff);
		return true;
	}

	const Vector3 localDiff = sphere.center - obb.center;
	float localPosition[3] = {
	    Dot(localDiff, obb.orientation[0]),
	    Dot(localDiff, obb.orientation[1]),
	    Dot(localDiff, obb.orientation[2])
	};
	const float halfSizes[3] = {obb.halfSize.x, obb.halfSize.y, obb.halfSize.z};
	int nearestAxis = 0;
	float nearestFaceDistance = halfSizes[0] - std::fabs(localPosition[0]);
	for (int axisIndex = 1; axisIndex < 3; ++axisIndex) {
		const float faceDistance = halfSizes[axisIndex] - std::fabs(localPosition[axisIndex]);
		if (faceDistance < nearestFaceDistance) {
			nearestFaceDistance = faceDistance;
			nearestAxis = axisIndex;
		}
	}

	const float sign = localPosition[nearestAxis] < 0.0f ? -1.0f : 1.0f;
	directionOBBToSphere = sign * obb.orientation[nearestAxis];
	penetration = sphere.radius + nearestFaceDistance;
	return penetration > 0.0f;
}

/// <summary>
/// OBB同士の押し戻し方向と侵入量を計算します。
/// </summary>
bool CalculateOBBOBBPushBack(const OBBColliderShape& a, const OBBColliderShape& b, Vector3& directionAToB, float& penetration) {
	Vector3 axes[15] = {
	    a.orientation[0], a.orientation[1], a.orientation[2],
	    b.orientation[0], b.orientation[1], b.orientation[2],
	    Cross(a.orientation[0], b.orientation[0]), Cross(a.orientation[0], b.orientation[1]), Cross(a.orientation[0], b.orientation[2]),
	    Cross(a.orientation[1], b.orientation[0]), Cross(a.orientation[1], b.orientation[1]), Cross(a.orientation[1], b.orientation[2]),
	    Cross(a.orientation[2], b.orientation[0]), Cross(a.orientation[2], b.orientation[1]), Cross(a.orientation[2], b.orientation[2])
	};

	const Vector3 centerDiff = b.center - a.center;
	bool foundAxis = false;
	float minimumOverlap = 0.0f;
	Vector3 minimumAxis{1.0f, 0.0f, 0.0f};
	for (const Vector3& rawAxis : axes) {
		if (Length(rawAxis) <= 0.00001f) {
			continue;
		}
		Vector3 axis = Normalize(rawAxis);
		const float signedDistance = Dot(centerDiff, axis);
		const float overlap = ProjectOBBRadius(a, axis) + ProjectOBBRadius(b, axis) - std::fabs(signedDistance);
		if (overlap <= 0.0f) {
			return false;
		}
		if (!foundAxis || overlap < minimumOverlap) {
			foundAxis = true;
			minimumOverlap = overlap;
			minimumAxis = signedDistance < 0.0f ? -1.0f * axis : axis;
		}
	}

	if (!foundAxis) {
		return false;
	}
	directionAToB = NormalizeOrFallback(minimumAxis, centerDiff);
	penetration = minimumOverlap;
	return true;
}

/// <summary>
/// 押し戻し設定が有効なオブジェクトへ位置補正を適用します。
/// </summary>
void ApplyColliderPushBack(GameObject* objectA, bool isPushBackA, GameObject* objectB, bool isPushBackB, const Vector3& directionAToB, float penetration) {
	if (!objectA || !objectB || objectA == objectB || penetration <= 0.0f || (!isPushBackA && !isPushBackB)) {
		return;
	}

	if (isPushBackA && isPushBackB) {
		objectA->GetTransform().translate = objectA->GetTransform().translate + (-0.5f * penetration) * directionAToB;
		objectB->GetTransform().translate = objectB->GetTransform().translate + (0.5f * penetration) * directionAToB;
		objectA->ApplyCollisionResponse(-1.0f * directionAToB);
		objectB->ApplyCollisionResponse(directionAToB);
		return;
	}
	if (isPushBackA) {
		objectA->GetTransform().translate = objectA->GetTransform().translate + (-penetration) * directionAToB;
		objectA->ApplyCollisionResponse(-1.0f * directionAToB);
		return;
	}
	objectB->GetTransform().translate = objectB->GetTransform().translate + penetration * directionAToB;
	objectB->ApplyCollisionResponse(directionAToB);
}

/// <summary>
/// Vector3をJSON配列へ変換します。
/// </summary>
nlohmann::json Vector3ToJson(const Vector3& value) {
	return nlohmann::json::array({value.x, value.y, value.z});
}

/// <summary>
/// Vector4をJSON配列へ変換します。
/// </summary>
nlohmann::json Vector4ToJson(const Vector4& value) {
	return nlohmann::json::array({value.x, value.y, value.z, value.w});
}

/// <summary>
/// JSON配列からVector3を読み込みます。
/// </summary>
Vector3 JsonToVector3(const nlohmann::json& value, const Vector3& fallback) {
	if (!value.is_array() || value.size() < 3) {
		return fallback;
	}

	return {
	    value.at(0).get<float>(),
	    value.at(1).get<float>(),
	    value.at(2).get<float>()
	};
}

/// <summary>
/// JSON配列からVector4を読み込みます。
/// </summary>
Vector4 JsonToVector4(const nlohmann::json& value, const Vector4& fallback) {
	if (!value.is_array() || value.size() < 4) {
		return fallback;
	}

	return {
	    value.at(0).get<float>(),
	    value.at(1).get<float>(),
	    value.at(2).get<float>(),
	    value.at(3).get<float>()
	};
}

/// <summary>
/// パーティクルプリセットJSONを読み込みます。
/// </summary>
nlohmann::json LoadParticlePresetRoot() {
	std::ifstream ifs(kParticlePresetFilePath);
	if (!ifs) {
		return nlohmann::json::object();
	}

	nlohmann::json root;
	ifs >> root;
	return root.is_object() ? root : nlohmann::json::object();
}

/// <summary>
/// 保存済みパーティクルプリセット名を取得します。
/// </summary>
std::vector<std::string> LoadParticlePresetNames() {
	std::vector<std::string> names;
	const nlohmann::json root = LoadParticlePresetRoot();
	for (auto it = root.begin(); it != root.end(); ++it) {
		names.push_back(it.key());
	}
	return names;
}

/// <summary>
/// プリセットからテクスチャパスとメッシュ種別を取得します。
/// </summary>
void GetParticlePresetResourceInfo(const std::string& presetName, std::string& textureFilePath, ParticleMeshType& meshType) {
	textureFilePath = "Resources/circle.png";
	meshType = kMeshTypeQuad;

	const nlohmann::json root = LoadParticlePresetRoot();
	if (!root.contains(presetName)) {
		return;
	}

	const nlohmann::json preset = root.at(presetName);
	textureFilePath = preset.value("textureFilePath", textureFilePath);
	meshType = static_cast<ParticleMeshType>(preset.value("meshType", static_cast<int>(meshType)));
}

/// <summary>
/// パーティクルプリセットJSONをエミッターへ反映します。
/// </summary>
void ApplyParticlePresetJson(const nlohmann::json& preset, ParticleEmitterComponent* emitter) {
	if (!preset.is_object() || !emitter) {
		return;
	}

	emitter->SetTexture(preset.value("textureFilePath", emitter->GetTextureFilePath()));
	emitter->SetIsActive(preset.value("isActive", emitter->GetIsActive()));
	emitter->SetFrequency(preset.value("frequency", emitter->GetFrequency()));
	emitter->SetBlendMode(static_cast<BlendMode>(preset.value("blendMode", static_cast<int>(emitter->GetBlendMode()))));
	emitter->SetMeshType(static_cast<ParticleMeshType>(preset.value("meshType", static_cast<int>(emitter->GetMeshType()))));

	const nlohmann::json paramJson = preset.value("emitParam", nlohmann::json::object());
	ParticleEmitParam param = emitter->GetPalam();
	param.count = paramJson.value("count", param.count);
	param.lifeTime = paramJson.value("lifeTime", param.lifeTime);
	param.scale = JsonToVector3(paramJson.value("scale", nlohmann::json::array()), param.scale);
	param.endScale = JsonToVector3(paramJson.value("endScale", nlohmann::json::array()), param.endScale);
	param.baseVelocity = JsonToVector3(paramJson.value("baseVelocity", nlohmann::json::array()), param.baseVelocity);
	param.randomVelocityRange = JsonToVector3(paramJson.value("randomVelocityRange", nlohmann::json::array()), param.randomVelocityRange);
	param.acceleration = JsonToVector3(paramJson.value("acceleration", nlohmann::json::array()), param.acceleration);
	param.randomPositionRange = JsonToVector3(paramJson.value("randomPositionRange", nlohmann::json::array()), param.randomPositionRange);
	param.baseRotate = JsonToVector3(paramJson.value("baseRotate", nlohmann::json::array()), param.baseRotate);
	param.isRandomRotate = paramJson.value("isRandomRotate", param.isRandomRotate);
	param.randomRotateRange = JsonToVector3(paramJson.value("randomRotateRange", nlohmann::json::array()), param.randomRotateRange);
	param.color = JsonToVector4(paramJson.value("color", nlohmann::json::array()), param.color);
	param.endColor = JsonToVector4(paramJson.value("endColor", nlohmann::json::array()), param.endColor);
	param.randomScaleRange = JsonToVector3(paramJson.value("randomScaleRange", nlohmann::json::array()), param.randomScaleRange);
	param.isBillboard = paramJson.value("isBillboard", param.isBillboard);
	emitter->SetParam(param);
}

/// <summary>
/// 指定名のパーティクルプリセットをエミッターへ反映します。
/// </summary>
void ApplyParticlePreset(const std::string& presetName, ParticleEmitterComponent* emitter) {
	const nlohmann::json root = LoadParticlePresetRoot();
	if (!root.contains(presetName)) {
		return;
	}

	ApplyParticlePresetJson(root.at(presetName), emitter);
}

/// <summary>
/// エミッター設定をパーティクルプリセットとして保存します。
/// </summary>
void SaveParticlePreset(const std::string& presetName, ParticleEmitterComponent* emitter) {
	if (presetName.empty() || !emitter) {
		return;
	}

	nlohmann::json root = LoadParticlePresetRoot();
	ParticleEmitParam param = emitter->GetPalam();
	nlohmann::json preset;
	preset["blendMode"] = static_cast<int>(emitter->GetBlendMode());
	preset["count"] = param.count;
	preset["frequency"] = emitter->GetFrequency();
	preset["isActive"] = emitter->GetIsActive();
	preset["meshType"] = static_cast<int>(emitter->GetMeshType());
	preset["textureFilePath"] = emitter->GetTextureFilePath();
	preset["emitParam"]["acceleration"] = Vector3ToJson(param.acceleration);
	preset["emitParam"]["baseRotate"] = Vector3ToJson(param.baseRotate);
	preset["emitParam"]["baseVelocity"] = Vector3ToJson(param.baseVelocity);
	preset["emitParam"]["color"] = Vector4ToJson(param.color);
	preset["emitParam"]["count"] = param.count;
	preset["emitParam"]["endColor"] = Vector4ToJson(param.endColor);
	preset["emitParam"]["endScale"] = Vector3ToJson(param.endScale);
	preset["emitParam"]["isBillboard"] = param.isBillboard;
	preset["emitParam"]["isRandomRotate"] = param.isRandomRotate;
	preset["emitParam"]["lifeTime"] = param.lifeTime;
	preset["emitParam"]["randomPositionRange"] = Vector3ToJson(param.randomPositionRange);
	preset["emitParam"]["randomRotateRange"] = Vector3ToJson(param.randomRotateRange);
	preset["emitParam"]["randomScaleRange"] = Vector3ToJson(param.randomScaleRange);
	preset["emitParam"]["randomVelocityRange"] = Vector3ToJson(param.randomVelocityRange);
	preset["emitParam"]["scale"] = Vector3ToJson(param.scale);
	root[presetName] = preset;

	std::filesystem::create_directories(std::filesystem::path(kParticlePresetFilePath).parent_path());
	std::ofstream ofs(kParticlePresetFilePath);
	if (ofs) {
		ofs << std::setw(4) << root << std::endl;
	}
}

EnemyStats MakeDefaultEnemyStats() {
	return {};
}

nlohmann::json EnemyStatsToJson(const EnemyStats& stats) {
	nlohmann::json json;
	json["health"] = stats.health;
	json["attack"] = stats.attack;
	json["speed"] = stats.speed;
	json["shoots"] = stats.shoots;
	json["shootingInterval"] = stats.shootingInterval;
	json["spawnsPerMinute"] = stats.spawnsPerMinute;
	return json;
}

EnemyStats JsonToEnemyStats(const nlohmann::json& json, const EnemyStats& fallback) {
	EnemyStats stats = fallback;
	if (!json.is_object()) {
		return stats;
	}

	stats.health = json.value("health", stats.health);
	stats.attack = json.value("attack", stats.attack);
	stats.speed = json.value("speed", stats.speed);
	stats.shoots = json.value("shoots", stats.shoots);
	stats.shootingInterval = json.value("shootingInterval", stats.shootingInterval);
	stats.spawnsPerMinute = json.value("spawnsPerMinute", stats.spawnsPerMinute);
	stats.health = (std::max)(0.0f, stats.health);
	stats.attack = (std::max)(0.0f, stats.attack);
	stats.speed = (std::max)(0.0f, stats.speed);
	stats.shootingInterval = (std::max)(0.0f, stats.shootingInterval);
	stats.spawnsPerMinute = (std::max)(0.0f, stats.spawnsPerMinute);
	return stats;
}

nlohmann::json LoadEnemyStatusRoot() {
	std::ifstream ifs(kEnemyStatusFilePath);
	if (!ifs) {
		nlohmann::json root;
		root["Default"] = EnemyStatsToJson(MakeDefaultEnemyStats());
		return root;
	}

	nlohmann::json root;
	ifs >> root;
	if (!root.is_object()) {
		root = nlohmann::json::object();
	}
	if (!root.contains("Default")) {
		root["Default"] = EnemyStatsToJson(MakeDefaultEnemyStats());
	}
	return root;
}

std::vector<std::string> LoadEnemyTypeNames() {
	std::vector<std::string> names;
	const nlohmann::json root = LoadEnemyStatusRoot();
	for (auto it = root.begin(); it != root.end(); ++it) {
		names.push_back(it.key());
	}
	std::sort(names.begin(), names.end());
	if (names.empty()) {
		names.push_back("Default");
	}
	return names;
}

EnemyStats LoadEnemyStats(const std::string& enemyTypeName) {
	const std::string typeName = enemyTypeName.empty() ? "Default" : enemyTypeName;
	const nlohmann::json root = LoadEnemyStatusRoot();
	const EnemyStats fallback = MakeDefaultEnemyStats();
	if (!root.contains(typeName)) {
		return fallback;
	}
	return JsonToEnemyStats(root.at(typeName), fallback);
}

void SaveEnemyStats(const std::string& enemyTypeName, const EnemyStats& stats) {
	const std::string typeName = enemyTypeName.empty() ? "Default" : enemyTypeName;
	nlohmann::json root = LoadEnemyStatusRoot();
	root[typeName] = EnemyStatsToJson(stats);

	std::filesystem::create_directories(std::filesystem::path(kEnemyStatusFilePath).parent_path());
	std::ofstream ofs(kEnemyStatusFilePath);
	if (ofs) {
		ofs << std::setw(4) << root << std::endl;
	}
}

/// <summary>
/// エディタ生成タイプを保存用文字列へ変換します。
/// </summary>
const char* EditorCreateTypeName(BaseScene::EditorCreateType type) {
	switch (type) {
	case BaseScene::EditorCreateType::Object3dSphere:
		return "Object3dSphere";
	case BaseScene::EditorCreateType::Object3dCylinder:
		return "Object3dCylinder";
	case BaseScene::EditorCreateType::Object3dCylinderOpen:
		return "Object3dCylinderOpen";
	case BaseScene::EditorCreateType::Sprite:
		return "Sprite";
	case BaseScene::EditorCreateType::LoadedModel:
		return "LoadedModel";
	case BaseScene::EditorCreateType::AnimatedModel:
		return "AnimatedModel";
	case BaseScene::EditorCreateType::Camera:
		return "Camera";
	case BaseScene::EditorCreateType::PointLight:
		return "PointLight";
	case BaseScene::EditorCreateType::ParticleEmitter:
		return "ParticleEmitter";
	case BaseScene::EditorCreateType::Player:
		return "Player";
	case BaseScene::EditorCreateType::EnemySpawnPoint:
		return "EnemySpawnPoint";
	case BaseScene::EditorCreateType::Enemy:
		return "Enemy";
	case BaseScene::EditorCreateType::Empty:
	default:
		return "Empty";
	}
}

/// <summary>
/// ラジアンを度数へ変換します。
/// </summary>
float ToDegrees(float radians) {
	return radians * 180.0f / kPi;
}

/// <summary>
/// 度数をラジアンへ変換します。
/// </summary>
float ToRadians(float degrees) {
	return degrees * kPi / 180.0f;
}

/// <summary>
/// ベクトルを指定軸まわりに回転します。
/// </summary>
Vector3 RotateAroundAxis(const Vector3& value, const Vector3& axis, float radians) {
	const Vector3 normalizedAxis = Normalize(axis);
	const float axisLength = Length(normalizedAxis);
	if (axisLength <= 0.00001f) {
		return value;
	}

	const float cosValue = std::cos(radians);
	const float sinValue = std::sin(radians);
	return
	    cosValue * value +
	    sinValue * Cross(normalizedAxis, value) +
	    Dot(normalizedAxis, value) * (1.0f - cosValue) * normalizedAxis;
}

/// <summary>
/// カメラから位置と回転を取得します。
/// </summary>
bool TryGetCameraTransform(Camera* camera, Vector3& translate, Vector3& rotate) {
	if (!camera) {
		return false;
	}

	translate = camera->GetTranslate();
	rotate = camera->GetRotate();
	return true;
}

/// <summary>
/// 座標を行列で変換し、必要に応じてW除算します。
/// </summary>
Vector3 TransformCoord(const Vector3& vector, const Matrix4x4& matrix) {
	return Transformation(vector, matrix);
}

/// <summary>
/// レイとOBBの交差判定を行います。
/// </summary>
bool IntersectRayToOBB(const Vector3& rayOrigin, const Vector3& rayDirection, const OBBColliderShape& obb, float& distance) {
	constexpr float kEpsilon = 0.00001f;
	float tMin = 0.0f;
	float tMax = 100000.0f;
	const Vector3 centerToRay = rayOrigin - obb.center;

	for (int axisIndex = 0; axisIndex < 3; ++axisIndex) {
		const float origin = Dot(centerToRay, obb.orientation[axisIndex]);
		const float direction = Dot(rayDirection, obb.orientation[axisIndex]);
		const float minValue = -((&obb.halfSize.x)[axisIndex]);
		const float maxValue = (&obb.halfSize.x)[axisIndex];

		if (std::fabs(direction) < kEpsilon) {
			if (origin < minValue || origin > maxValue) {
				return false;
			}
			continue;
		}

		float t1 = (minValue - origin) / direction;
		float t2 = (maxValue - origin) / direction;
		if (t1 > t2) {
			std::swap(t1, t2);
		}
		if (t1 > tMin) {
			tMin = t1;
		}
		if (t2 < tMax) {
			tMax = t2;
		}
		if (tMin > tMax) {
			return false;
		}
	}

	distance = tMin;
	return true;
}

/// <summary>
/// オブジェクト選択用のOBBを生成します。
/// </summary>
OBBColliderShape MakePickOBB(GameObject* object) {
	if (OBBColliderComponent* collider = object->GetComponent<OBBColliderComponent>()) {
		return collider->GetWorldOBB();
	}

	const EulerTransform& transform = object->GetTransform();
	const Matrix4x4 rotateMatrix = MakeRotateXYZMatrix(transform.rotate);
	const Vector3 axisX{rotateMatrix.m[0][0], rotateMatrix.m[0][1], rotateMatrix.m[0][2]};
	const Vector3 axisY{rotateMatrix.m[1][0], rotateMatrix.m[1][1], rotateMatrix.m[1][2]};
	const Vector3 axisZ{rotateMatrix.m[2][0], rotateMatrix.m[2][1], rotateMatrix.m[2][2]};

	OBBColliderShape obb{};
	obb.center = transform.translate;
	obb.orientation[0] = Normalize(axisX);
	obb.orientation[1] = Normalize(axisY);
	obb.orientation[2] = Normalize(axisZ);
	const float halfX = std::fabs(transform.scale.x) * 0.5f;
	const float halfY = std::fabs(transform.scale.y) * 0.5f;
	const float halfZ = std::fabs(transform.scale.z) * 0.5f;
	obb.halfSize = {
	    halfX < 0.25f ? 0.25f : halfX,
	    halfY < 0.25f ? 0.25f : halfY,
	    halfZ < 0.25f ? 0.25f : halfZ
	};
	return obb;
}
}

