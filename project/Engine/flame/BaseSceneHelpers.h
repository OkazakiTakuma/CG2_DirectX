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
#include <cstdlib>
#include <cstring>
#include <iomanip>

namespace {
constexpr float kPi = 3.14159265358979323846f;
const char* kParticlePresetFilePath = "Resources/Data/emit_status.json";
const char* kEnemyStatusFilePath = "Resources/Data/enemy_status.json";
const char* kPlayerStatusFilePath = "Resources/Data/player_status.json";
const char* kPlayerAttackStatusFilePath = "Resources/Data/player_attack_status.json";
const char* kPlayerStatusItemFilePath = "Resources/Data/player_status_item_status.json";

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
	json["experience"] = stats.experience;
	json["experienceModel"] = stats.experienceModelFilePath;
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
	stats.experience = json.value("experience", stats.experience);
	stats.experienceModelFilePath = json.value("experienceModel", stats.experienceModelFilePath);
	stats.health = (std::max)(0.0f, stats.health);
	stats.attack = (std::max)(0.0f, stats.attack);
	stats.speed = (std::max)(0.0f, stats.speed);
	stats.shootingInterval = (std::max)(0.0f, stats.shootingInterval);
	stats.spawnsPerMinute = (std::max)(0.0f, stats.spawnsPerMinute);
	stats.experience = (std::max)(0, stats.experience);
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

enum class PlayerStatusItemType {
	Attack,
	Health,
	AttackSpeed,
	Speed,
	Defense,
	AttackSize,
	Experience
};

struct PlayerStatusItemStats {
	std::string name = "AttackUp";
	PlayerStatusItemType type = PlayerStatusItemType::Attack;
	std::array<float, 5> levelAmounts{10.0f, 20.0f, 30.0f, 40.0f, 50.0f};
};

std::vector<std::string> GetPlayerStatusItemLevels() {
	return {"1", "2", "3", "4", "5"};
}

const char* PlayerStatusItemTypeToName(PlayerStatusItemType type) {
	switch (type) {
	case PlayerStatusItemType::Attack:
		return "Attack";
	case PlayerStatusItemType::Health:
		return "HP";
	case PlayerStatusItemType::AttackSpeed:
		return "AttackSpeed";
	case PlayerStatusItemType::Speed:
		return "Speed";
	case PlayerStatusItemType::Defense:
		return "Defense";
	case PlayerStatusItemType::AttackSize:
		return "AttackSize";
	case PlayerStatusItemType::Experience:
		return "Experience";
	}
	return "Attack";
}

PlayerStatusItemType PlayerStatusItemTypeFromName(const std::string& typeName) {
	if (typeName == "HP" || typeName == "Health") {
		return PlayerStatusItemType::Health;
	}
	if (typeName == "AttackSpeed") {
		return PlayerStatusItemType::AttackSpeed;
	}
	if (typeName == "Speed") {
		return PlayerStatusItemType::Speed;
	}
	if (typeName == "Defense") {
		return PlayerStatusItemType::Defense;
	}
	if (typeName == "AttackSize") {
		return PlayerStatusItemType::AttackSize;
	}
	if (typeName == "Experience") {
		return PlayerStatusItemType::Experience;
	}
	return PlayerStatusItemType::Attack;
}

PlayerStatusItemStats MakeDefaultPlayerStatusItemStats() {
	return {};
}

nlohmann::json PlayerStatusItemStatsToJson(const PlayerStatusItemStats& stats) {
	nlohmann::json json;
	json["name"] = stats.name;
	json["type"] = PlayerStatusItemTypeToName(stats.type);
	for (int index = 0; index < static_cast<int>(stats.levelAmounts.size()); ++index) {
		json["levels"][std::to_string(index + 1)] = stats.levelAmounts[index];
	}
	return json;
}

PlayerStatusItemStats JsonToPlayerStatusItemStats(const nlohmann::json& json, const PlayerStatusItemStats& fallback) {
	PlayerStatusItemStats stats = fallback;
	if (!json.is_object()) {
		return stats;
	}
	stats.name = json.value("name", stats.name);
	stats.type = PlayerStatusItemTypeFromName(json.value("type", std::string(PlayerStatusItemTypeToName(stats.type))));
	const nlohmann::json levelsJson = json.value("levels", nlohmann::json::object());
	for (int index = 0; index < static_cast<int>(stats.levelAmounts.size()); ++index) {
		const std::string levelName = std::to_string(index + 1);
		stats.levelAmounts[index] = levelsJson.value(levelName, stats.levelAmounts[index]);
		stats.levelAmounts[index] = (std::max)(0.0f, stats.levelAmounts[index]);
	}
	return stats;
}

nlohmann::json LoadPlayerStatusItemRoot() {
	std::ifstream ifs(kPlayerStatusItemFilePath);
	if (!ifs) {
		nlohmann::json root;
		root["AttackUp"] = PlayerStatusItemStatsToJson(MakeDefaultPlayerStatusItemStats());
		return root;
	}

	nlohmann::json root;
	ifs >> root;
	if (!root.is_object()) {
		root = nlohmann::json::object();
	}
	if (!root.contains("AttackUp")) {
		root["AttackUp"] = PlayerStatusItemStatsToJson(MakeDefaultPlayerStatusItemStats());
	}
	return root;
}

std::vector<std::string> LoadPlayerStatusItemNames() {
	std::vector<std::string> names;
	const nlohmann::json root = LoadPlayerStatusItemRoot();
	for (auto it = root.begin(); it != root.end(); ++it) {
		names.push_back(it.key());
	}
	std::sort(names.begin(), names.end());
	if (names.empty()) {
		names.push_back("AttackUp");
	}
	return names;
}

PlayerStatusItemStats LoadPlayerStatusItemStats(const std::string& itemName) {
	const std::string name = itemName.empty() ? "AttackUp" : itemName;
	const nlohmann::json root = LoadPlayerStatusItemRoot();
	const PlayerStatusItemStats fallback = MakeDefaultPlayerStatusItemStats();
	if (!root.contains(name)) {
		return fallback;
	}
	PlayerStatusItemStats stats = JsonToPlayerStatusItemStats(root.at(name), fallback);
	stats.name = name;
	return stats;
}

void SavePlayerStatusItemStats(const std::string& itemName, const PlayerStatusItemStats& stats) {
	const std::string name = itemName.empty() ? "AttackUp" : itemName;
	nlohmann::json root = LoadPlayerStatusItemRoot();
	PlayerStatusItemStats saveStats = stats;
	saveStats.name = name;
	root[name] = PlayerStatusItemStatsToJson(saveStats);

	std::filesystem::create_directories(std::filesystem::path(kPlayerStatusItemFilePath).parent_path());
	std::ofstream ofs(kPlayerStatusItemFilePath);
	if (ofs) {
		ofs << std::setw(4) << root << std::endl;
	}
}

int PlayerStatusSlotLevelToIndex(const std::string& level) {
	const int parsedLevel = (std::max)(1, std::atoi(level.c_str()));
	return (std::min)(4, parsedLevel - 1);
}

PlayerStats ApplyPlayerStatusItems(const PlayerStats& baseStats) {
	PlayerStats effectiveStats = baseStats;
	for (const PlayerStatusSlot& slot : baseStats.statusSlots) {
		if (!slot.enabled || slot.statusName.empty()) {
			continue;
		}
		const PlayerStatusItemStats itemStats = LoadPlayerStatusItemStats(slot.statusName);
		const float amount = itemStats.levelAmounts[PlayerStatusSlotLevelToIndex(slot.level)];
		switch (itemStats.type) {
		case PlayerStatusItemType::Attack:
			effectiveStats.attack += amount;
			break;
		case PlayerStatusItemType::Health:
			effectiveStats.health += amount;
			break;
		case PlayerStatusItemType::AttackSpeed:
			effectiveStats.attackSpeed += amount;
			break;
		case PlayerStatusItemType::Speed:
			effectiveStats.speed += amount;
			break;
		case PlayerStatusItemType::Defense:
			effectiveStats.defense += amount;
			break;
		case PlayerStatusItemType::AttackSize:
			effectiveStats.attackSize += amount;
			break;
		case PlayerStatusItemType::Experience:
			effectiveStats.experienceCorrection += amount;
			break;
		}
	}
	return effectiveStats;
}

PlayerStats MakeDefaultPlayerStats() {
	PlayerStats stats;
	stats.modelFilePath = "sphere.obj";
	stats.attackSlots[0].enabled = true;
	stats.attackSlots[0].attackName = stats.initialAttackName;
	stats.attackSlots[0].attackLevel = stats.initialAttackLevel;
	return stats;
}

std::vector<std::string> GetPlayerAttackLevels() {
	return {"1", "2", "3", "4", "5", "super"};
}

PlayerAttackLevelStats MakeDefaultPlayerAttackLevelStats(const std::string& level) {
	PlayerAttackLevelStats stats;
	stats.level = level;
	const float levelScale = level == "super" ? 6.0f : static_cast<float>((std::max)(1, std::atoi(level.c_str())));
	stats.attack = 100.0f * levelScale;
	stats.speed = 0.3f + levelScale * 0.02f;
	stats.size = 100.0f + levelScale * 10.0f;
	stats.shotCount = level == "super" ? 5 : 1;
	stats.angles = stats.shotCount > 1 ? std::vector<float>{-30.0f, -15.0f, 0.0f, 15.0f, 30.0f} : std::vector<float>{0.0f};
	stats.modelFilePath = "sphere.obj";
	stats.homing = false;
	stats.homingAccuracy = 1.0f;
	stats.attackInterval = level == "super" ? 0.25f : 0.5f;
	stats.lifeTime = 3.0f;
	stats.pierceCount = level == "super" ? 3 : 0;
	stats.infinitePierce = false;
	return stats;
}

PlayerAttackStats MakeDefaultPlayerAttackStats() {
	PlayerAttackStats stats;
	stats.name = "Straight";
	for (const std::string& level : GetPlayerAttackLevels()) {
		stats.levels.push_back(MakeDefaultPlayerAttackLevelStats(level));
	}
	return stats;
}

nlohmann::json PlayerAttackLevelStatsToJson(const PlayerAttackLevelStats& stats) {
	nlohmann::json json;
	json["level"] = stats.level;
	json["attack"] = stats.attack;
	json["speed"] = stats.speed;
	json["size"] = stats.size;
	json["shotCount"] = stats.shotCount;
	json["angles"] = stats.angles;
	json["model"] = stats.modelFilePath;
	json["homing"] = stats.homing;
	json["homingAccuracy"] = stats.homingAccuracy;
	json["attackInterval"] = stats.attackInterval;
	json["lifeTime"] = stats.lifeTime;
	json["pierceCount"] = stats.pierceCount;
	json["infinitePierce"] = stats.infinitePierce;
	return json;
}

PlayerAttackLevelStats JsonToPlayerAttackLevelStats(const nlohmann::json& json, const PlayerAttackLevelStats& fallback) {
	PlayerAttackLevelStats stats = fallback;
	if (!json.is_object()) {
		return stats;
	}
	stats.level = json.value("level", stats.level);
	stats.attack = (std::max)(0.0f, json.value("attack", stats.attack));
	stats.speed = (std::max)(0.0f, json.value("speed", stats.speed));
	stats.size = (std::max)(0.0f, json.value("size", stats.size));
	stats.shotCount = (std::max)(1, json.value("shotCount", stats.shotCount));
	stats.modelFilePath = json.value("model", stats.modelFilePath);
	stats.homing = json.value("homing", stats.homing);
	stats.homingAccuracy = (std::clamp)(json.value("homingAccuracy", stats.homingAccuracy), 0.0f, 1.0f);
	stats.attackInterval = (std::max)(0.01f, json.value("attackInterval", stats.attackInterval));
	stats.lifeTime = (std::max)(0.01f, json.value("lifeTime", stats.lifeTime));
	stats.pierceCount = (std::max)(0, json.value("pierceCount", stats.pierceCount));
	stats.infinitePierce = json.value("infinitePierce", stats.infinitePierce);
	if (json.contains("angles") && json.at("angles").is_array()) {
		stats.angles.clear();
		for (const nlohmann::json& angleJson : json.at("angles")) {
			stats.angles.push_back(angleJson.get<float>());
		}
	}
	while (static_cast<int>(stats.angles.size()) < stats.shotCount) {
		stats.angles.push_back(0.0f);
	}
	return stats;
}

nlohmann::json PlayerAttackStatsToJson(const PlayerAttackStats& stats) {
	nlohmann::json json;
	json["name"] = stats.name;
	json["superCondition"]["status"] = stats.superConditionStatusName;
	json["superCondition"]["level"] = stats.superConditionStatusLevel;
	for (const PlayerAttackLevelStats& levelStats : stats.levels) {
		json["levels"][levelStats.level] = PlayerAttackLevelStatsToJson(levelStats);
	}
	return json;
}

PlayerAttackStats JsonToPlayerAttackStats(const nlohmann::json& json, const PlayerAttackStats& fallback) {
	PlayerAttackStats stats = fallback;
	if (!json.is_object()) {
		return stats;
	}
	stats.name = json.value("name", stats.name);
	const nlohmann::json superConditionJson = json.value("superCondition", nlohmann::json::object());
	stats.superConditionStatusName = superConditionJson.value("status", stats.superConditionStatusName);
	stats.superConditionStatusLevel = superConditionJson.value("level", stats.superConditionStatusLevel);
	const std::vector<std::string> statusLevels = GetPlayerStatusItemLevels();
	if (std::find(statusLevels.begin(), statusLevels.end(), stats.superConditionStatusLevel) == statusLevels.end()) {
		stats.superConditionStatusLevel = "1";
	}
	stats.levels.clear();
	const nlohmann::json levelsJson = json.value("levels", nlohmann::json::object());
	for (const std::string& level : GetPlayerAttackLevels()) {
		PlayerAttackLevelStats fallbackLevel = MakeDefaultPlayerAttackLevelStats(level);
		for (const PlayerAttackLevelStats& existingLevel : fallback.levels) {
			if (existingLevel.level == level) {
				fallbackLevel = existingLevel;
				break;
			}
		}
		stats.levels.push_back(JsonToPlayerAttackLevelStats(levelsJson.value(level, nlohmann::json::object()), fallbackLevel));
		stats.levels.back().level = level;
	}
	return stats;
}

nlohmann::json LoadPlayerAttackStatusRoot() {
	std::ifstream ifs(kPlayerAttackStatusFilePath);
	if (!ifs) {
		nlohmann::json root;
		root["Straight"] = PlayerAttackStatsToJson(MakeDefaultPlayerAttackStats());
		return root;
	}

	nlohmann::json root;
	ifs >> root;
	if (!root.is_object()) {
		root = nlohmann::json::object();
	}
	if (!root.contains("Straight")) {
		root["Straight"] = PlayerAttackStatsToJson(MakeDefaultPlayerAttackStats());
	}
	return root;
}

std::vector<std::string> LoadPlayerAttackNames() {
	std::vector<std::string> names;
	const nlohmann::json root = LoadPlayerAttackStatusRoot();
	for (auto it = root.begin(); it != root.end(); ++it) {
		names.push_back(it.key());
	}
	std::sort(names.begin(), names.end());
	if (names.empty()) {
		names.push_back("Straight");
	}
	return names;
}

PlayerAttackStats LoadPlayerAttackStats(const std::string& attackName) {
	const std::string name = attackName.empty() ? "Straight" : attackName;
	const nlohmann::json root = LoadPlayerAttackStatusRoot();
	const PlayerAttackStats fallback = MakeDefaultPlayerAttackStats();
	if (!root.contains(name)) {
		return fallback;
	}
	PlayerAttackStats stats = JsonToPlayerAttackStats(root.at(name), fallback);
	stats.name = name;
	return stats;
}

void ApplyPlayerAttackSlots(PlayerAttackComponent* attack, const PlayerStats& playerStats) {
	if (!attack) {
		return;
	}
	attack->ClearAttackSlots();
	for (const PlayerAttackSlot& slot : playerStats.attackSlots) {
		if (slot.attackName.empty()) {
			continue;
		}
		attack->AddAttackSlot(LoadPlayerAttackStats(slot.attackName), slot.attackLevel, slot.enabled);
	}
}

void SavePlayerAttackStats(const std::string& attackName, const PlayerAttackStats& stats) {
	const std::string name = attackName.empty() ? "Straight" : attackName;
	nlohmann::json root = LoadPlayerAttackStatusRoot();
	PlayerAttackStats saveStats = stats;
	saveStats.name = name;
	root[name] = PlayerAttackStatsToJson(saveStats);

	std::filesystem::create_directories(std::filesystem::path(kPlayerAttackStatusFilePath).parent_path());
	std::ofstream ofs(kPlayerAttackStatusFilePath);
	if (ofs) {
		ofs << std::setw(4) << root << std::endl;
	}
}

nlohmann::json PlayerStatsToJson(const PlayerStats& stats) {
	nlohmann::json json;
	json["name"] = stats.name;
	json["baseHealth"] = stats.baseHealth;
	json["health"] = stats.health;
	json["attack"] = stats.attack;
	json["defense"] = stats.defense;
	json["baseSpeed"] = stats.baseSpeed;
	json["speed"] = stats.speed;
	json["attackSpeed"] = stats.attackSpeed;
	json["attackSize"] = stats.attackSize;
	json["damageInvincibilityDuration"] = stats.damageInvincibilityDuration;
	json["level"] = stats.level;
	json["experience"] = stats.experience;
	json["experienceCorrection"] = stats.experienceCorrection;
	json["model"] = stats.modelFilePath;
	json["isAnimationModel"] = stats.isAnimationModel;
	json["initialAttack"] = stats.initialAttackName;
	json["initialAttackLevel"] = stats.initialAttackLevel;
	json["attackSlots"] = nlohmann::json::array();
	for (const PlayerAttackSlot& slot : stats.attackSlots) {
		nlohmann::json slotJson;
		slotJson["enabled"] = slot.enabled;
		slotJson["attack"] = slot.attackName;
		slotJson["level"] = slot.attackLevel;
		json["attackSlots"].push_back(slotJson);
	}
	json["statusSlots"] = nlohmann::json::array();
	for (const PlayerStatusSlot& slot : stats.statusSlots) {
		nlohmann::json slotJson;
		slotJson["enabled"] = slot.enabled;
		slotJson["status"] = slot.statusName;
		slotJson["level"] = slot.level;
		json["statusSlots"].push_back(slotJson);
	}
	return json;
}

PlayerStats JsonToPlayerStats(const nlohmann::json& json, const PlayerStats& fallback) {
	PlayerStats stats = fallback;
	if (!json.is_object()) {
		return stats;
	}

	stats.name = json.value("name", stats.name);
	stats.baseHealth = json.value("baseHealth", stats.baseHealth);
	stats.health = json.value("health", stats.health);
	stats.attack = json.value("attack", stats.attack);
	stats.defense = json.value("defense", stats.defense);
	stats.baseSpeed = json.value("baseSpeed", stats.baseSpeed);
	stats.speed = json.value("speed", stats.speed);
	stats.attackSpeed = json.value("attackSpeed", stats.attackSpeed);
	stats.attackSize = json.value("attackSize", stats.attackSize);
	stats.damageInvincibilityDuration = json.value("damageInvincibilityDuration", stats.damageInvincibilityDuration);
	stats.level = json.value("level", stats.level);
	stats.experience = json.value("experience", stats.experience);
	stats.experienceCorrection = json.value("experienceCorrection", stats.experienceCorrection);
	stats.modelFilePath = json.value("model", stats.modelFilePath);
	stats.isAnimationModel = json.value("isAnimationModel", stats.isAnimationModel);
	stats.initialAttackName = json.value("initialAttack", stats.initialAttackName);
	stats.initialAttackLevel = json.value("initialAttackLevel", stats.initialAttackLevel);
	for (PlayerAttackSlot& slot : stats.attackSlots) {
		slot.enabled = false;
		slot.attackName.clear();
		slot.attackLevel = "1";
	}
	stats.attackSlots[0].enabled = true;
	stats.attackSlots[0].attackName = stats.initialAttackName;
	stats.attackSlots[0].attackLevel = stats.initialAttackLevel;
	if (json.contains("attackSlots") && json.at("attackSlots").is_array()) {
		const nlohmann::json& slotsJson = json.at("attackSlots");
		for (size_t index = 0; index < stats.attackSlots.size() && index < slotsJson.size(); ++index) {
			const nlohmann::json& slotJson = slotsJson.at(index);
			if (!slotJson.is_object()) {
				continue;
			}
			stats.attackSlots[index].enabled = slotJson.value("enabled", stats.attackSlots[index].enabled);
			stats.attackSlots[index].attackName = slotJson.value("attack", stats.attackSlots[index].attackName);
			stats.attackSlots[index].attackLevel = slotJson.value("level", stats.attackSlots[index].attackLevel);
		}
	}
	stats.initialAttackName = stats.attackSlots[0].attackName;
	stats.initialAttackLevel = stats.attackSlots[0].attackLevel;
	for (PlayerStatusSlot& slot : stats.statusSlots) {
		slot.enabled = false;
		slot.statusName.clear();
	}
	if (json.contains("statusSlots") && json.at("statusSlots").is_array()) {
		const nlohmann::json& slotsJson = json.at("statusSlots");
		for (size_t index = 0; index < stats.statusSlots.size() && index < slotsJson.size(); ++index) {
			const nlohmann::json& slotJson = slotsJson.at(index);
			if (!slotJson.is_object()) {
				continue;
			}
			stats.statusSlots[index].enabled = slotJson.value("enabled", stats.statusSlots[index].enabled);
			stats.statusSlots[index].statusName = slotJson.value("status", stats.statusSlots[index].statusName);
			stats.statusSlots[index].level = slotJson.value("level", stats.statusSlots[index].level);
			if (PlayerStatusSlotLevelToIndex(stats.statusSlots[index].level) < 0) {
				stats.statusSlots[index].level = "1";
			}
			if (stats.statusSlots[index].statusName.empty()) {
				stats.statusSlots[index].enabled = false;
			}
		}
	}

	stats.baseHealth = (std::max)(0.0f, stats.baseHealth);
	stats.health = (std::max)(0.0f, stats.health);
	stats.attack = (std::max)(0.0f, stats.attack);
	stats.defense = (std::max)(0.0f, stats.defense);
	stats.baseSpeed = (std::max)(0.0f, stats.baseSpeed);
	stats.speed = (std::max)(0.0f, stats.speed);
	stats.attackSpeed = (std::max)(0.0f, stats.attackSpeed);
	stats.attackSize = (std::max)(0.0f, stats.attackSize);
	stats.damageInvincibilityDuration = (std::max)(0.0f, stats.damageInvincibilityDuration);
	stats.level = (std::max)(1, stats.level);
	stats.experience = (std::max)(0, stats.experience);
	stats.experienceCorrection = (std::max)(0.0f, stats.experienceCorrection);
	return stats;
}

nlohmann::json LoadPlayerStatusRoot() {
	std::ifstream ifs(kPlayerStatusFilePath);
	if (!ifs) {
		nlohmann::json root;
		root["Default"] = PlayerStatsToJson(MakeDefaultPlayerStats());
		return root;
	}

	nlohmann::json root;
	ifs >> root;
	if (!root.is_object()) {
		root = nlohmann::json::object();
	}
	if (!root.contains("Default")) {
		root["Default"] = PlayerStatsToJson(MakeDefaultPlayerStats());
	}
	return root;
}

std::vector<std::string> LoadPlayerTypeNames() {
	std::vector<std::string> names;
	const nlohmann::json root = LoadPlayerStatusRoot();
	for (auto it = root.begin(); it != root.end(); ++it) {
		names.push_back(it.key());
	}
	std::sort(names.begin(), names.end());
	if (names.empty()) {
		names.push_back("Default");
	}
	return names;
}

PlayerStats LoadPlayerStats(const std::string& playerTypeName) {
	const std::string typeName = playerTypeName.empty() ? "Default" : playerTypeName;
	const nlohmann::json root = LoadPlayerStatusRoot();
	const PlayerStats fallback = MakeDefaultPlayerStats();
	if (!root.contains(typeName)) {
		return fallback;
	}
	PlayerStats stats = JsonToPlayerStats(root.at(typeName), fallback);
	stats.name = typeName;
	return stats;
}

void SavePlayerStats(const std::string& playerTypeName, const PlayerStats& stats) {
	const std::string typeName = playerTypeName.empty() ? "Default" : playerTypeName;
	nlohmann::json root = LoadPlayerStatusRoot();
	PlayerStats saveStats = stats;
	saveStats.name = typeName;
	root[typeName] = PlayerStatsToJson(saveStats);

	std::filesystem::create_directories(std::filesystem::path(kPlayerStatusFilePath).parent_path());
	std::ofstream ofs(kPlayerStatusFilePath);
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
	case BaseScene::EditorCreateType::Text:
		return "Text";
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

