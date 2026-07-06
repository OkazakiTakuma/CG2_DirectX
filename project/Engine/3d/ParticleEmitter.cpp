#include "ParticleEmitter.h"
#include "LineDrawer.h"
#include "ParticleManager.h"
#include "algorithm.h"
#include <algorithm>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <iomanip>


namespace {
float RandomRange(float min, float max) {
	return min + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * (max - min);
}

Vector3 Lerp(const Vector3& a, const Vector3& b, float t) {
	return {
	    a.x + (b.x - a.x) * t,
	    a.y + (b.y - a.y) * t,
	    a.z + (b.z - a.z) * t,
	};
}

Vector3 AddScaled(const Vector3& a, const Vector3& b, float scale) {
	return {a.x + b.x * scale, a.y + b.y * scale, a.z + b.z * scale};
}

Vector3 RandomPerpendicular(const Vector3& direction, float length) {
	Vector3 axis = std::fabs(direction.y) < 0.9f ? Vector3{0.0f, 1.0f, 0.0f} : Vector3{1.0f, 0.0f, 0.0f};
	Vector3 side = Normalize(Cross(direction, axis));
	Vector3 up = Normalize(Cross(direction, side));
	Vector3 result = AddScaled(RandomRange(-1.0f, 1.0f) * side, up, RandomRange(-1.0f, 1.0f));
	return length * result;
}
}

void ParticleEmitter::EmitLightning(const Vector3& targetPosition) {
	Vector3 startPos = transform_.translate;
	Vector3 endPos = targetPosition;
	Vector3 mainDirection = Normalize(endPos - startPos);
	float distance = Length(endPos - startPos);
	if (distance <= 0.001f) {
		return;
	}

	int generations = 5;
	float displacement = distance * 0.22f;
	std::vector<Vector3> lightningPath = CreateLightningPath(startPos, endPos, displacement, generations);

	lightningLines_.clear();
	const float lineLife = 0.12f;

	for (size_t i = 0; i + 1 < lightningPath.size(); ++i) {
		const Vector3& p0 = lightningPath[i];
		const Vector3& p1 = lightningPath[i + 1];
		lightningLines_.push_back({p0, p1, {0.95f, 0.98f, 1.0f, 1.0f}, lineLife, 0.0f});

		Vector3 glowOffset = RandomPerpendicular(mainDirection, 0.05f);
		lightningLines_.push_back({p0 + glowOffset, p1 + glowOffset, {0.25f, 0.55f, 1.0f, 0.55f}, lineLife, 0.0f});

		if (i > 1 && i + 2 < lightningPath.size() && RandomRange(0.0f, 1.0f) < 0.35f) {
			float branchLength = distance * RandomRange(0.08f, 0.18f);
			Vector3 branchDir = Normalize(AddScaled(mainDirection, RandomPerpendicular(mainDirection, 1.0f), RandomRange(1.4f, 2.4f)));
			Vector3 branchStart = Lerp(p0, p1, RandomRange(0.2f, 0.8f));
			Vector3 branchEnd = AddScaled(branchStart, branchDir, branchLength);
			lightningLines_.push_back({branchStart, branchEnd, {0.55f, 0.78f, 1.0f, 0.75f}, lineLife * 0.8f, 0.0f});

			if (RandomRange(0.0f, 1.0f) < 0.45f) {
				Vector3 childDir = Normalize(AddScaled(branchDir, RandomPerpendicular(branchDir, 1.0f), RandomRange(0.8f, 1.5f)));
				Vector3 childStart = Lerp(branchStart, branchEnd, RandomRange(0.45f, 0.8f));
				Vector3 childEnd = AddScaled(childStart, childDir, branchLength * RandomRange(0.35f, 0.55f));
				lightningLines_.push_back({childStart, childEnd, {0.45f, 0.68f, 1.0f, 0.45f}, lineLife * 0.6f, 0.0f});
			}
		}
	}

	for (const auto& pos : lightningPath) {
		ParticleManager::GetInstance()->Emit(groupName_, pos, 1, emitParam_);
	}
}

ParticleEmitter::ParticleEmitter() : groupName_(""), count_(0), frequency_(0.0f), frequencyTimer_(0.0f), textureFilePath_(""), isActive_(true), blendMode_(kBlendModeNormal) {
	transform_.scale = { 1.0f, 1.0f, 1.0f };
	transform_.rotate = { 0.0f, 0.0f, 0.0f };
	transform_.translate = { 0.0f, 0.0f, 0.0f };

	emitParam_.scale = { 1.0f, 1.0f, 1.0f };
	emitParam_.endScale = { 0.0f, 0.0f, 0.0f };

	emitParam_.baseVelocity = { 0.0f, 0.0f, 0.0f };
	emitParam_.randomVelocityRange = { 0.0f, 0.0f, 0.0f };
	emitParam_.acceleration = { 0.0f, 0.0f, 0.0f };

	emitParam_.randomPositionRange = { 0.0f, 0.0f, 0.0f };
	emitParam_.lifeTime = 1.0f;

	emitParam_.baseRotate = { 0.0f, 0.0f, 0.0f };
	emitParam_.isRandomRotate = false;
	emitParam_.randomRotateRange = { 0.0f, 0.0f, 0.0f };

	emitParam_.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	emitParam_.endColor = { 1.0f, 1.0f, 1.0f, 0.0f };

	emitParam_.randomScaleRange = { 0.0f, 0.0f, 0.0f };
	emitParam_.count = 1;
	emitParam_.isBillboard = true;
}

void ParticleEmitter::Update(float deltaTime) {
	if (!isActive_) {
		return;
	}

	for (auto& line : lightningLines_) {
		line.currentTime += deltaTime;
		float alpha = 1.0f - (line.currentTime / line.lifeTime);
		if (alpha < 0.0f) {
			alpha = 0.0f;
		}
		Vector4 color = line.color;
		color.w *= alpha;
		LineDrawer::GetInstance()->DrawLine(line.start, line.end, color);
	}
	lightningLines_.erase(
	    std::remove_if(
	        lightningLines_.begin(),
	        lightningLines_.end(),
	        [](const LightningLine& line) { return line.currentTime >= line.lifeTime; }),
	    lightningLines_.end());

	frequencyTimer_ += deltaTime;
	if (frequency_ > 0.0f && frequencyTimer_ >= frequency_) {
		Emit();
		frequencyTimer_ -= frequency_;
	}
}

void ParticleEmitter::Emit() {
	uint32_t emitCount = count_ > 0 ? count_ : emitParam_.count;
	ParticleManager::GetInstance()->Emit(groupName_, transform_.translate, emitCount, emitParam_);

	if (groupName_ == "Slash") {
		lightningLines_.clear();
		const Vector3 center = transform_.translate;
		const float lineLife = 0.09f;
		for (int i = 0; i < 14; ++i) {
			float t = static_cast<float>(i) / 13.0f;
			float angle = -0.9f + t * 1.8f + RandomRange(-0.16f, 0.16f);
			Vector3 directionSource = {
			    static_cast<float>(std::cos(angle)),
			    static_cast<float>(std::sin(angle)) * 0.55f,
			    RandomRange(-0.28f, 0.28f)};
			Vector3 direction = Normalize(directionSource);
			float length = RandomRange(0.8f, 1.8f);
			Vector3 start = AddScaled(center, direction, 0.18f);
			Vector3 end = AddScaled(center, direction, length);
			lightningLines_.push_back({start, end, {1.0f, 0.84f, 0.32f, 0.95f}, lineLife, 0.0f});
		}

		lightningLines_.push_back({
		    {center.x - 1.4f, center.y + 0.7f, center.z},
		    {center.x + 1.4f, center.y - 0.45f, center.z},
		    {1.0f, 0.96f, 0.72f, 1.0f},
		    lineLife,
		    0.0f});
	}
}

void ParticleEmitter::SetTexture(const std::string& textureFilePath) {
	textureFilePath_ = textureFilePath;
	ParticleManager::GetInstance()->SetGroupTexture(groupName_, textureFilePath_);
}

// =========================================================
// =========================================================
void ParticleEmitter::SaveToJson(const std::string& filePath) {
	nlohmann::json root;

	std::ifstream ifs(filePath);
	if (ifs.is_open()) {
		ifs >> root;
		ifs.close();
	}

	nlohmann::json groupJson;
	groupJson["count"] = count_;
	groupJson["frequency"] = frequency_;
	groupJson["textureFilePath"] = textureFilePath_;
	groupJson["isActive"] = isActive_;
	groupJson["blendMode"] = static_cast<int>(blendMode_);
	groupJson["meshType"] = static_cast<int>(meshType_);

	nlohmann::json param;
	param["scale"] = { emitParam_.scale.x, emitParam_.scale.y, emitParam_.scale.z };
	param["endScale"] = { emitParam_.endScale.x, emitParam_.endScale.y, emitParam_.endScale.z };

	param["baseVelocity"] = { emitParam_.baseVelocity.x, emitParam_.baseVelocity.y, emitParam_.baseVelocity.z };
	param["randomVelocityRange"] = { emitParam_.randomVelocityRange.x, emitParam_.randomVelocityRange.y, emitParam_.randomVelocityRange.z };
	param["acceleration"] = { emitParam_.acceleration.x, emitParam_.acceleration.y, emitParam_.acceleration.z };

	param["randomPositionRange"] = { emitParam_.randomPositionRange.x, emitParam_.randomPositionRange.y, emitParam_.randomPositionRange.z };
	param["lifeTime"] = emitParam_.lifeTime;

	param["baseRotate"] = { emitParam_.baseRotate.x, emitParam_.baseRotate.y, emitParam_.baseRotate.z };
	param["isRandomRotate"] = emitParam_.isRandomRotate;
	param["randomRotateRange"] = { emitParam_.randomRotateRange.x, emitParam_.randomRotateRange.y, emitParam_.randomRotateRange.z };

	param["color"] = { emitParam_.color.x, emitParam_.color.y, emitParam_.color.z, emitParam_.color.w };
	param["endColor"] = { emitParam_.endColor.x, emitParam_.endColor.y, emitParam_.endColor.z, emitParam_.endColor.w };

	param["randomScaleRange"] = { emitParam_.randomScaleRange.x, emitParam_.randomScaleRange.y, emitParam_.randomScaleRange.z };
	param["count"] = emitParam_.count;
	param["isBillboard"] = emitParam_.isBillboard;

	groupJson["emitParam"] = param;

	root[groupName_] = groupJson;

	std::ofstream ofs(filePath);
	if (ofs.is_open()) {
		ofs << std::setw(4) << root << std::endl;
		ofs.close();
	}
}

// =========================================================
// =========================================================
void ParticleEmitter::LoadFromJson(const std::string& filePath) {
	count_ = 0;
	frequency_ = 0.0f;
	textureFilePath_ = "";
	isActive_ = true;
	blendMode_ = kBlendModeNormal;
	meshType_ = kMeshTypeQuad;

	emitParam_.scale = { 1.0f, 1.0f, 1.0f };
	emitParam_.endScale = { 0.0f, 0.0f, 0.0f };
	emitParam_.baseVelocity = { 0.0f, 0.0f, 0.0f };
	emitParam_.randomVelocityRange = { 0.0f, 0.0f, 0.0f };
	emitParam_.acceleration = { 0.0f, 0.0f, 0.0f };
	emitParam_.randomPositionRange = { 0.0f, 0.0f, 0.0f };
	emitParam_.lifeTime = 1.0f;
	emitParam_.baseRotate = { 0.0f, 0.0f, 0.0f };
	emitParam_.isRandomRotate = false;
	emitParam_.randomRotateRange = { 0.0f, 0.0f, 0.0f };
	emitParam_.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	emitParam_.endColor = { 1.0f, 1.0f, 1.0f, 0.0f };
	emitParam_.randomScaleRange = { 0.0f, 0.0f, 0.0f };
	emitParam_.count = 1;
	emitParam_.isBillboard = true;

	std::ifstream ifs(filePath);
	if (!ifs.is_open()) {
		return;
	}

	nlohmann::json root;
	ifs >> root;
	ifs.close();

	if (!root.contains(groupName_)) {
		return;
	}

	auto& groupJson = root[groupName_];

	if (groupJson.contains("count")) { count_ = groupJson["count"]; }
	if (groupJson.contains("frequency")) { frequency_ = groupJson["frequency"]; }
	if (groupJson.contains("textureFilePath")) { textureFilePath_ = groupJson["textureFilePath"]; }
	if (groupJson.contains("isActive")) { isActive_ = groupJson["isActive"]; }
	if (groupJson.contains("blendMode")) { blendMode_ = static_cast<BlendMode>(groupJson["blendMode"].get<int>()); }
	if (groupJson.contains("meshType")) { meshType_ = static_cast<ParticleMeshType>(groupJson["meshType"].get<int>()); }

	if (groupJson.contains("emitParam")) {
		auto& param = groupJson["emitParam"];

		if (param.contains("scale")) {
			emitParam_.scale = { param["scale"][0], param["scale"][1], param["scale"][2] };
		}
		if (param.contains("endScale")) {
			emitParam_.endScale = { param["endScale"][0], param["endScale"][1], param["endScale"][2] };
		}
		if (param.contains("baseVelocity")) {
			emitParam_.baseVelocity = { param["baseVelocity"][0], param["baseVelocity"][1], param["baseVelocity"][2] };
		}
		if (param.contains("randomVelocityRange")) {
			emitParam_.randomVelocityRange = { param["randomVelocityRange"][0], param["randomVelocityRange"][1], param["randomVelocityRange"][2] };
		}
		if (param.contains("acceleration")) {
			emitParam_.acceleration = { param["acceleration"][0], param["acceleration"][1], param["acceleration"][2] };
		}
		if (param.contains("randomPositionRange")) {
			emitParam_.randomPositionRange = { param["randomPositionRange"][0], param["randomPositionRange"][1], param["randomPositionRange"][2] };
		}
		if (param.contains("lifeTime")) {
			emitParam_.lifeTime = param["lifeTime"];
		}
		if (param.contains("baseRotate")) {
			emitParam_.baseRotate = { param["baseRotate"][0], param["baseRotate"][1], param["baseRotate"][2] };
		}
		if (param.contains("isRandomRotate")) {
			emitParam_.isRandomRotate = param["isRandomRotate"];
		}
		if (param.contains("randomRotateRange")) {
			emitParam_.randomRotateRange = { param["randomRotateRange"][0], param["randomRotateRange"][1], param["randomRotateRange"][2] };
		}
		if (param.contains("color")) {
			emitParam_.color = { param["color"][0], param["color"][1], param["color"][2], param["color"][3] };
		}
		if (param.contains("endColor")) {
			emitParam_.endColor = { param["endColor"][0], param["endColor"][1], param["endColor"][2], param["endColor"][3] };
		}
		if (param.contains("randomScaleRange")) {
			emitParam_.randomScaleRange = { param["randomScaleRange"][0], param["randomScaleRange"][1], param["randomScaleRange"][2] };
		}
		if (param.contains("count")) {
			emitParam_.count = param["count"];
		}
		if (param.contains("isBillboard")) {
			emitParam_.isBillboard = param["isBillboard"];
		}
	}
}


