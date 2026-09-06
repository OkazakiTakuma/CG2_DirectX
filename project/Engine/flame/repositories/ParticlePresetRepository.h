#pragma once

#include "../../3d/particle/ParticleEmitterComponent.h"
#include "../helpers/SceneJsonUtility.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>

/// <summary>
/// パーティクルプリセットの読み込み・適用・保存を担当します。
/// </summary>
namespace ParticlePresetRepository {

inline constexpr const char* kFilePath = "Resources/Data/emit_status.json";

inline nlohmann::json LoadRoot() {
	std::ifstream ifs(kFilePath);
	if (!ifs) {
		return nlohmann::json::object();
	}

	nlohmann::json root;
	ifs >> root;
	return root.is_object() ? root : nlohmann::json::object();
}

inline std::vector<std::string> LoadNames() {
	std::vector<std::string> names;
	const nlohmann::json root = LoadRoot();
	for (auto it = root.begin(); it != root.end(); ++it) {
		names.push_back(it.key());
	}
	return names;
}

inline void GetResourceInfo(const std::string& presetName, std::string& textureFilePath, ParticleMeshType& meshType) {
	textureFilePath = "Resources/circle.png";
	meshType = kMeshTypeQuad;

	const nlohmann::json root = LoadRoot();
	if (!root.contains(presetName)) {
		return;
	}

	const nlohmann::json preset = root.at(presetName);
	textureFilePath = preset.value("textureFilePath", textureFilePath);
	meshType = static_cast<ParticleMeshType>(preset.value("meshType", static_cast<int>(meshType)));
}

inline void ApplyJson(const nlohmann::json& preset, ParticleEmitterComponent* emitter) {
	if (!preset.is_object() || !emitter) {
		return;
	}

	emitter->SetTexture(preset.value("textureFilePath", emitter->GetTextureFilePath()));
	emitter->SetIsActive(preset.value("isActive", emitter->GetIsActive()));
	emitter->SetFrequency(preset.value("frequency", emitter->GetFrequency()));
	emitter->SetBlendMode(static_cast<BlendMode>(preset.value("blendMode", static_cast<int>(emitter->GetBlendMode()))));
	emitter->SetMeshType(static_cast<ParticleMeshType>(preset.value("meshType", static_cast<int>(emitter->GetMeshType()))));

	const nlohmann::json paramJson = preset.value("emitParam", nlohmann::json::object());
	ParticleEmitParam param = emitter->GetParam();
	param.count = paramJson.value("count", param.count);
	param.lifeTime = paramJson.value("lifeTime", param.lifeTime);
	param.scale = SceneJsonUtility::JsonToVector3(paramJson.value("scale", nlohmann::json::array()), param.scale);
	param.endScale = SceneJsonUtility::JsonToVector3(paramJson.value("endScale", nlohmann::json::array()), param.endScale);
	param.baseVelocity = SceneJsonUtility::JsonToVector3(paramJson.value("baseVelocity", nlohmann::json::array()), param.baseVelocity);
	param.randomVelocityRange = SceneJsonUtility::JsonToVector3(paramJson.value("randomVelocityRange", nlohmann::json::array()), param.randomVelocityRange);
	param.acceleration = SceneJsonUtility::JsonToVector3(paramJson.value("acceleration", nlohmann::json::array()), param.acceleration);
	param.randomPositionRange = SceneJsonUtility::JsonToVector3(paramJson.value("randomPositionRange", nlohmann::json::array()), param.randomPositionRange);
	param.baseRotate = SceneJsonUtility::JsonToVector3(paramJson.value("baseRotate", nlohmann::json::array()), param.baseRotate);
	param.isRandomRotate = paramJson.value("isRandomRotate", param.isRandomRotate);
	param.randomRotateRange = SceneJsonUtility::JsonToVector3(paramJson.value("randomRotateRange", nlohmann::json::array()), param.randomRotateRange);
	param.color = SceneJsonUtility::JsonToVector4(paramJson.value("color", nlohmann::json::array()), param.color);
	param.endColor = SceneJsonUtility::JsonToVector4(paramJson.value("endColor", nlohmann::json::array()), param.endColor);
	param.randomScaleRange = SceneJsonUtility::JsonToVector3(paramJson.value("randomScaleRange", nlohmann::json::array()), param.randomScaleRange);
	param.isBillboard = paramJson.value("isBillboard", param.isBillboard);
	param.isVortex = paramJson.value("isVortex", param.isVortex);
	param.vortexAngularSpeed = paramJson.value("vortexAngularSpeed", param.vortexAngularSpeed);
	param.vortexBaseRadius = paramJson.value("vortexBaseRadius", param.vortexBaseRadius);
	param.vortexTopRadius = paramJson.value("vortexTopRadius", param.vortexTopRadius);
	param.vortexHeight = paramJson.value("vortexHeight", param.vortexHeight);
	emitter->SetParam(param);
}

inline void Apply(const std::string& presetName, ParticleEmitterComponent* emitter) {
	const nlohmann::json root = LoadRoot();
	if (root.contains(presetName)) {
		ApplyJson(root.at(presetName), emitter);
	}
}

inline void Save(const std::string& presetName, ParticleEmitterComponent* emitter) {
	if (presetName.empty() || !emitter) {
		return;
	}

	nlohmann::json root = LoadRoot();
	const ParticleEmitParam param = emitter->GetParam();
	nlohmann::json preset;
	preset["blendMode"] = static_cast<int>(emitter->GetBlendMode());
	preset["count"] = param.count;
	preset["frequency"] = emitter->GetFrequency();
	preset["isActive"] = emitter->GetIsActive();
	preset["meshType"] = static_cast<int>(emitter->GetMeshType());
	preset["textureFilePath"] = emitter->GetTextureFilePath();
	preset["emitParam"]["acceleration"] = SceneJsonUtility::Vector3ToJson(param.acceleration);
	preset["emitParam"]["baseRotate"] = SceneJsonUtility::Vector3ToJson(param.baseRotate);
	preset["emitParam"]["baseVelocity"] = SceneJsonUtility::Vector3ToJson(param.baseVelocity);
	preset["emitParam"]["color"] = SceneJsonUtility::Vector4ToJson(param.color);
	preset["emitParam"]["count"] = param.count;
	preset["emitParam"]["endColor"] = SceneJsonUtility::Vector4ToJson(param.endColor);
	preset["emitParam"]["endScale"] = SceneJsonUtility::Vector3ToJson(param.endScale);
	preset["emitParam"]["isBillboard"] = param.isBillboard;
	preset["emitParam"]["isVortex"] = param.isVortex;
	preset["emitParam"]["vortexAngularSpeed"] = param.vortexAngularSpeed;
	preset["emitParam"]["vortexBaseRadius"] = param.vortexBaseRadius;
	preset["emitParam"]["vortexTopRadius"] = param.vortexTopRadius;
	preset["emitParam"]["vortexHeight"] = param.vortexHeight;
	preset["emitParam"]["isRandomRotate"] = param.isRandomRotate;
	preset["emitParam"]["lifeTime"] = param.lifeTime;
	preset["emitParam"]["randomPositionRange"] = SceneJsonUtility::Vector3ToJson(param.randomPositionRange);
	preset["emitParam"]["randomRotateRange"] = SceneJsonUtility::Vector3ToJson(param.randomRotateRange);
	preset["emitParam"]["randomScaleRange"] = SceneJsonUtility::Vector3ToJson(param.randomScaleRange);
	preset["emitParam"]["randomVelocityRange"] = SceneJsonUtility::Vector3ToJson(param.randomVelocityRange);
	preset["emitParam"]["scale"] = SceneJsonUtility::Vector3ToJson(param.scale);
	root[presetName] = preset;

	std::filesystem::create_directories(std::filesystem::path(kFilePath).parent_path());
	std::ofstream ofs(kFilePath);
	if (ofs) {
		ofs << std::setw(4) << root << std::endl;
	}
}

} // namespace ParticlePresetRepository
