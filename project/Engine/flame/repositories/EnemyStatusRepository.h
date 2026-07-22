#pragma once

#include "../EnemyComponent.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#pragma warning(push)
#pragma warning(disable: 26495)
#include <json.hpp>
#pragma warning(pop)
#include <string>
#include <vector>

/// <summary>
/// 敵ステータスのJSON変換・読み込み・保存を担当します。
/// </summary>
namespace {

inline constexpr const char* kFilePath = "Resources/Data/enemy_status.json";

inline EnemyStats MakeDefaultStats() {
	return {};
}

inline const char* BehaviorToString(EnemyBehaviorType behavior) {
	switch (behavior) {
	case EnemyBehaviorType::Shooter: return "Shooter";
	case EnemyBehaviorType::Charger: return "Charger";
	default: return "Chase";
	}
}

inline EnemyBehaviorType BehaviorFromString(const std::string& behavior) {
	if (behavior == "Shooter") return EnemyBehaviorType::Shooter;
	if (behavior == "Charger") return EnemyBehaviorType::Charger;
	return EnemyBehaviorType::Chase;
}

inline nlohmann::json ToJson(const EnemyStats& stats) {
	nlohmann::json json;
	json["health"] = stats.health;
	json["attack"] = stats.attack;
	json["speed"] = stats.speed;
	json["shoots"] = stats.shoots;
	json["shootingInterval"] = stats.shootingInterval;
	json["spawnsPerMinute"] = stats.spawnsPerMinute;
	json["experience"] = stats.experience;
	json["experienceModel"] = stats.experienceModelFilePath;
	json["behavior"] = BehaviorToString(stats.behavior);
	json["preferredDistance"] = stats.preferredDistance;
	json["distanceTolerance"] = stats.distanceTolerance;
	json["projectileSpeed"] = stats.projectileSpeed;
	json["projectileSize"] = stats.projectileSize;
	json["projectileLifeTime"] = stats.projectileLifeTime;
	json["chargeTriggerDistance"] = stats.chargeTriggerDistance;
	json["chargeDuration"] = stats.chargeDuration;
	json["dashSpeed"] = stats.dashSpeed;
	json["dashDuration"] = stats.dashDuration;
	json["dashRecovery"] = stats.dashRecovery;
	json["sizeScale"] = stats.sizeScale;
	return json;
}

inline EnemyStats FromJson(const nlohmann::json& json, const EnemyStats& fallback) {
	EnemyStats stats = fallback;
	if (!json.is_object()) {
		return stats;
	}

	stats.health = (std::max)(0.0f, json.value("health", stats.health));
	stats.attack = (std::max)(0.0f, json.value("attack", stats.attack));
	stats.speed = (std::max)(0.0f, json.value("speed", stats.speed));
	stats.shoots = json.value("shoots", stats.shoots);
	stats.shootingInterval = (std::max)(0.0f, json.value("shootingInterval", stats.shootingInterval));
	stats.spawnsPerMinute = (std::max)(0.0f, json.value("spawnsPerMinute", stats.spawnsPerMinute));
	stats.experience = (std::max)(0, json.value("experience", stats.experience));
	stats.experienceModelFilePath = json.value("experienceModel", stats.experienceModelFilePath);
	stats.behavior = BehaviorFromString(json.value("behavior", std::string(BehaviorToString(stats.behavior))));
	if (stats.shoots && !json.contains("behavior")) stats.behavior = EnemyBehaviorType::Shooter;
	stats.preferredDistance = (std::max)(0.0f, json.value("preferredDistance", stats.preferredDistance));
	stats.distanceTolerance = (std::max)(0.0f, json.value("distanceTolerance", stats.distanceTolerance));
	stats.projectileSpeed = (std::max)(0.0f, json.value("projectileSpeed", stats.projectileSpeed));
	stats.projectileSize = (std::max)(0.01f, json.value("projectileSize", stats.projectileSize));
	stats.projectileLifeTime = (std::max)(0.0f, json.value("projectileLifeTime", stats.projectileLifeTime));
	stats.chargeTriggerDistance = (std::max)(0.0f, json.value("chargeTriggerDistance", stats.chargeTriggerDistance));
	stats.chargeDuration = (std::max)(0.0f, json.value("chargeDuration", stats.chargeDuration));
	stats.dashSpeed = (std::max)(0.0f, json.value("dashSpeed", stats.dashSpeed));
	stats.dashDuration = (std::max)(0.0f, json.value("dashDuration", stats.dashDuration));
	stats.dashRecovery = (std::max)(0.0f, json.value("dashRecovery", stats.dashRecovery));
	stats.sizeScale = (std::max)(0.1f, json.value("sizeScale", stats.sizeScale));
	return stats;
}

inline nlohmann::json LoadRoot() {
	std::ifstream ifs(kFilePath);
	if (!ifs) {
		nlohmann::json root;
		root["Default"] = ToJson(MakeDefaultStats());
		return root;
	}

	nlohmann::json root;
	ifs >> root;
	if (!root.is_object()) {
		root = nlohmann::json::object();
	}
	if (!root.contains("Default")) {
		root["Default"] = ToJson(MakeDefaultStats());
	}
	return root;
}

inline std::vector<std::string> LoadEnemyTypeNames() {
	std::vector<std::string> names;
	const nlohmann::json root = LoadRoot();
	for (auto it = root.begin(); it != root.end(); ++it) {
		names.push_back(it.key());
	}
	std::sort(names.begin(), names.end());
	if (names.empty()) {
		names.push_back("Default");
	}
	return names;
}

inline EnemyStats LoadEnemyStats(const std::string& enemyTypeName) {
	const std::string typeName = enemyTypeName.empty() ? "Default" : enemyTypeName;
	const nlohmann::json root = LoadRoot();
	const EnemyStats fallback = MakeDefaultStats();
	return root.contains(typeName) ? FromJson(root.at(typeName), fallback) : fallback;
}

inline void SaveEnemyStats(const std::string& enemyTypeName, const EnemyStats& stats) {
	const std::string typeName = enemyTypeName.empty() ? "Default" : enemyTypeName;
	nlohmann::json root = LoadRoot();
	root[typeName] = ToJson(stats);

	std::filesystem::create_directories(std::filesystem::path(kFilePath).parent_path());
	std::ofstream ofs(kFilePath);
	if (ofs) {
		ofs << std::setw(4) << root << std::endl;
	}
}

}
