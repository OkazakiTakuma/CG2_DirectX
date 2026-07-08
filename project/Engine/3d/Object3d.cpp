#include "Object3d.h"
#include "../2d/TextureManager.h"
#include "../2d/LineDrawer.h"
#include "../base/SrvManager.h"
#include "Model.h"
#include "ModelManager.h"
#include "Object3dCommon.h"
#include"Quaternion.h"
#include <cmath>
Vector3 CalculateValue(const std::vector<KeyframeVector3>& keyflames, float time)
{

	assert(!keyflames.empty());
	if (keyflames.size() == 1 || time <= keyflames[0].time) {
		return keyflames[0].value;
	}
	for (size_t index = 0; index < keyflames.size() - 1; index++) {
		size_t newIndex = index + 1;
		if (keyflames[index].time <= time && time <= keyflames[newIndex].time) {
			float t = (time - keyflames[index].time) / (keyflames[newIndex].time - keyflames[index].time);
			return Leap(keyflames[index].value, keyflames[newIndex].value, t);
		}
	}
	return(*keyflames.rbegin()).value;
}

Quaternion CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, float time)
{
	assert(!keyframes.empty());

	if (keyframes.size() == 1 || time <= keyframes[0].time) {
		return keyframes[0].value;
	}

	for (size_t index = 0; index < keyframes.size() - 1; index++) {
		size_t nextIndex = index + 1;

		if (keyframes[index].time <= time && time <= keyframes[nextIndex].time) {
			float t = (time - keyframes[index].time) / (keyframes[nextIndex].time - keyframes[index].time);

			return Slerp(keyframes[index].value, keyframes[nextIndex].value, t);
		}
	}

	return keyframes.back().value;
}

namespace {
Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Quaternion& rotate, const Vector3& translate)
{
	Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);
	Matrix4x4 rotateMatrix = MakeRotateMatrix(rotate);
	Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);
	return Multiply(Multiply(scaleMatrix, rotateMatrix), translateMatrix);
}

Vector3 GetTranslateFromMatrix(const Matrix4x4& matrix)
{
	return {matrix.m[3][0], matrix.m[3][1], matrix.m[3][2]};
}

void DrawDebugWireSphere(const Vector3& center, float radius, const Vector4& color)
{
	constexpr uint32_t kSegmentCount = 8;
	constexpr float kTwoPi = 6.28318530718f;

	for (uint32_t index = 0; index < kSegmentCount; ++index) {
		const float currentAngle = kTwoPi * static_cast<float>(index) / static_cast<float>(kSegmentCount);
		const float nextAngle = kTwoPi * static_cast<float>(index + 1) / static_cast<float>(kSegmentCount);

		const float currentCos = std::cos(currentAngle) * radius;
		const float currentSin = std::sin(currentAngle) * radius;
		const float nextCos = std::cos(nextAngle) * radius;
		const float nextSin = std::sin(nextAngle) * radius;

		LineDrawer::GetInstance()->DrawLine(
		    {center.x + currentCos, center.y + currentSin, center.z},
		    {center.x + nextCos, center.y + nextSin, center.z},
		    color
		);
		LineDrawer::GetInstance()->DrawLine(
		    {center.x, center.y + currentCos, center.z + currentSin},
		    {center.x, center.y + nextCos, center.z + nextSin},
		    color
		);
		LineDrawer::GetInstance()->DrawLine(
		    {center.x + currentCos, center.y, center.z + currentSin},
		    {center.x + nextCos, center.y, center.z + nextSin},
		    color
		);
	}
}
}

void Object3d::Initialize() {
	Object3dCommon* common = Object3dCommon::GetInstance();

	environmentMultiplier = 0.0f;

	CreateWVPResource();
	CreateDirectionalLightResource();
	CreateCameraResource();
	CreatePointLightResource();

	transform = {
		{1.0f, 1.0f, 1.0f},
		{0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f}
	};

	this->camera = common->GetDefaultCamera();
	TextureManager::GetInstance()->LoadTexture(envMapTexturePath);
}
void Object3d::SetEnvironmentMap(const std::string& textureFilePath) {
	envMapTexturePath = textureFilePath;

	// =======================================================
	// =======================================================
	environmentMultiplier = 1.0f;
}
void Object3d::CreateWVPResource() {
	wvpResorceModel = Object3dCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));
	wvpResorceModel->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrix));
	transformationMatrix->WVP = MakeIdentity4x4();
	transformationMatrix->world = MakeIdentity4x4();
	transformationMatrix->WorldInverseTranspose = MakeIdentity4x4();
}

void Object3d::CreateCameraResource() {
	cameraResource = Object3dCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(CameraForGPU));
	cameraResource->Map(0, nullptr, reinterpret_cast<void**>(&cameraData));
	cameraData->worldPosition = {0.0f, 0.0f, 0.0f};
	cameraData->environmentMultiplier = environmentMultiplier;
}

void Object3d::CreateSkinningPaletteResource(uint32_t paletteCount) {
	paletteCount = paletteCount > 0 ? paletteCount : 1;
	if (skinningPaletteData) {
		skinningPaletteResource->Unmap(0, nullptr);
		skinningPaletteData = nullptr;
	}

	skinningPaletteCapacity_ = paletteCount;
	skinningPaletteResource = Object3dCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(Matrix4x4) * skinningPaletteCapacity_);
	skinningPaletteResource->Map(0, nullptr, reinterpret_cast<void**>(&skinningPaletteData));
	for (uint32_t index = 0; index < skinningPaletteCapacity_; index++) {
		skinningPaletteData[index] = MakeIdentity4x4();
	}
}

void Object3d::UpdateSkinningPaletteResource() {
	if (!skinningPaletteData) {
		CreateSkinningPaletteResource(1);
	}

	if (!model || !model->HasSkinCluster()) {
		skinningPaletteData[0] = MakeIdentity4x4();
		return;
	}

	const uint32_t requiredPaletteCount = model->GetSkinningPaletteSize();
	if (requiredPaletteCount > skinningPaletteCapacity_) {
		CreateSkinningPaletteResource(requiredPaletteCount);
	}

	model->BuildSkinningPalette(skeleton, skinningPalette_);
	std::memcpy(skinningPaletteData, skinningPalette_.data(), sizeof(Matrix4x4) * skinningPalette_.size());
}

Skeleton Object3d::CreateSkeleton(const Node& rootNode) {
	Skeleton result;
	result.root = CreateJoint(rootNode, std::nullopt, result.joints, result.jointMap);
	return result;
}

int32_t Object3d::CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints, std::map<std::string, int32_t>& jointMap) {
	Joint joint;
	joint.transform = node.transform;
	joint.localMatrix = node.localMatrix;
	joint.skeletonSpaceMatrix = MakeIdentity4x4();
	joint.name = node.name;
	joint.index = static_cast<int32_t>(joints.size());
	joint.parent = parent;

	jointMap[joint.name] = joint.index;
	joints.push_back(joint);

	for (const Node& child : node.children) {
		const int32_t childIndex = CreateJoint(child, joint.index, joints, jointMap);
		joints[joint.index].children.push_back(childIndex);
	}

	return joint.index;
}

void Object3d::ApplyAnimationToSkeleton() {
	if (!hasSkeleton || animation.duration <= 0.0f) {
		return;
	}

	for (Joint& joint : skeleton.joints) {
		auto animationItr = animation.nodeAnimations.find(joint.name);
		if (animationItr == animation.nodeAnimations.end()) {
			continue;
		}

		NodeAnimation& nodeAnimation = animationItr->second;
		if (!nodeAnimation.translate.keyframes.empty()) {
			joint.transform.translate = CalculateValue(nodeAnimation.translate.keyframes, animationTime);
		}
		if (!nodeAnimation.rotate.keyframes.empty()) {
			joint.transform.rotate = CalculateValue(nodeAnimation.rotate.keyframes, animationTime);
		}
		if (!nodeAnimation.scale.keyframes.empty()) {
			joint.transform.scale = CalculateValue(nodeAnimation.scale.keyframes, animationTime);
		}
	}
}

void Object3d::UpdateSkeleton() {
	if (!hasSkeleton) {
		return;
	}

	for (Joint& joint : skeleton.joints) {
		joint.localMatrix = MakeAffineMatrix(joint.transform.scale, joint.transform.rotate, joint.transform.translate);
		if (joint.parent) {
			joint.skeletonSpaceMatrix = Multiply(joint.localMatrix, skeleton.joints[*joint.parent].skeletonSpaceMatrix);
		} else {
			joint.skeletonSpaceMatrix = joint.localMatrix;
		}
	}
}

void Object3d::CreateDirectionalLightResource() {
	lightResource = Object3dCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(DirectionalLight));
	lightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionallightData));
	directionallightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	directionallightData->direction = NormalizeReturnVector(Vector3(0.0f, -1.0f, 0.0f));
	directionallightData->intensity = 1.0f;
}

void Object3d::CreatePointLightResource() {
	pointLightResource = Object3dCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(PointLight));
	pointLightResource->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData));
	pointLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	pointLightData->position = { 0.0f, 0.0f, 0.0f };
	pointLightData->intensity = 1.0f;
	pointLightData->radius = 10.0f;
	pointLightData->decay = 1.0f;
}

void Object3d::CreateCylinder(float radius, float height, uint32_t subdivision, bool createTopCap, bool createBottomCap) {
	std::vector<VertexData> vertices;
	std::vector<uint32_t> indices;

	float halfHeight = height / 2.0f;

	// -------------------------------------------------------
	// -------------------------------------------------------
	for (uint32_t i = 0; i <= subdivision; ++i) {
		float theta = (float)i / (float)subdivision * 2.0f * pi;
		float cosTheta = std::cos(theta);
		float sinTheta = std::sin(theta);

		Vector3 normal = { cosTheta, 0.0f, sinTheta };

		float u = (float)i / (float)subdivision;

		vertices.push_back({ { cosTheta * radius, halfHeight, sinTheta * radius, 1.0f }, { u, 0.0f }, normal });
		vertices.push_back({ { cosTheta * radius, -halfHeight, sinTheta * radius, 1.0f }, { u, 1.0f }, normal });
	}

	for (uint32_t i = 0; i < subdivision; ++i) {
		uint32_t top1 = i * 2;
		uint32_t bottom1 = i * 2 + 1;
		uint32_t top2 = (i + 1) * 2;
		uint32_t bottom2 = (i + 1) * 2 + 1;

		indices.push_back(top1); indices.push_back(top2); indices.push_back(bottom1);
		indices.push_back(bottom1); indices.push_back(top2); indices.push_back(bottom2);
	}

	// -------------------------------------------------------
	// -------------------------------------------------------
	if (createTopCap) {
		uint32_t topCenterIndex = (uint32_t)vertices.size();
		vertices.push_back({ { 0.0f, halfHeight, 0.0f, 1.0f }, { 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } });

		for (uint32_t i = 0; i <= subdivision; ++i) {
			float theta = (float)i / (float)subdivision * 2.0f * pi;
			float cosTheta = std::cos(theta);
			float sinTheta = std::sin(theta);

			vertices.push_back({
				{ cosTheta * radius, halfHeight, sinTheta * radius, 1.0f },
				{ cosTheta * 0.5f + 0.5f, -sinTheta * 0.5f + 0.5f },
				{ 0.0f, 1.0f, 0.0f }
				});
		}

		for (uint32_t i = 0; i < subdivision; ++i) {
			indices.push_back(topCenterIndex);
			indices.push_back(topCenterIndex + 1 + i);
			indices.push_back(topCenterIndex + 2 + i);
		}
	}

	// -------------------------------------------------------
	// -------------------------------------------------------
	if (createBottomCap) {
		uint32_t bottomCenterIndex = (uint32_t)vertices.size();
		vertices.push_back({ { 0.0f, -halfHeight, 0.0f, 1.0f }, { 0.5f, 0.5f }, { 0.0f, -1.0f, 0.0f } });

		for (uint32_t i = 0; i <= subdivision; ++i) {
			float theta = (float)i / (float)subdivision * 2.0f * pi;
			float cosTheta = std::cos(theta);
			float sinTheta = std::sin(theta);

			vertices.push_back({
				{ cosTheta * radius, -halfHeight, sinTheta * radius, 1.0f },
				{ cosTheta * 0.5f + 0.5f, sinTheta * 0.5f + 0.5f },
				{ 0.0f, -1.0f, 0.0f }
				});
		}

		for (uint32_t i = 0; i < subdivision; ++i) {
			indices.push_back(bottomCenterIndex);
			indices.push_back(bottomCenterIndex + 2 + i);
			indices.push_back(bottomCenterIndex + 1 + i);
		}
	}

	cylinderIndexCount = (uint32_t)indices.size();

	// -------------------------------------------------------
	// -------------------------------------------------------
	DirectXCommon* dxCommon = Object3dCommon::GetInstance()->GetDxCommon();

	size_t vertexBufferSize = sizeof(VertexData) * vertices.size();
	vertexResourceCylinder = dxCommon->CreateBufferResource(vertexBufferSize);

	VertexData* vertexData = nullptr;
	vertexResourceCylinder->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	std::memcpy(vertexData, vertices.data(), vertexBufferSize);
	vertexResourceCylinder->Unmap(0, nullptr);

	vertexBufferViewCylinder.BufferLocation = vertexResourceCylinder->GetGPUVirtualAddress();
	vertexBufferViewCylinder.SizeInBytes = static_cast<UINT>(vertexBufferSize);
	vertexBufferViewCylinder.StrideInBytes = sizeof(VertexData);

	size_t indexBufferSize = sizeof(uint32_t) * indices.size();
	indexResourceCylinder = dxCommon->CreateBufferResource(indexBufferSize);

	uint32_t* indexData = nullptr;
	indexResourceCylinder->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
	std::memcpy(indexData, indices.data(), indexBufferSize);
	indexResourceCylinder->Unmap(0, nullptr);

	indexBufferViewCylinder.BufferLocation = indexResourceCylinder->GetGPUVirtualAddress();
	indexBufferViewCylinder.SizeInBytes = static_cast<UINT>(indexBufferSize);
	indexBufferViewCylinder.Format = DXGI_FORMAT_R32_UINT;

	if (!materialResourceCylinder) {
		materialResourceCylinder = dxCommon->CreateBufferResource(sizeof(MaterialData));
		materialResourceCylinder->Map(0, nullptr, reinterpret_cast<void**>(&materialDataCylinder));

		materialDataCylinder->color = { 1.0f, 1.0f, 1.0f, 1.0f };
		materialDataCylinder->enableLighting = 1;
		materialDataCylinder->uvTransform = MakeIdentity4x4();
		materialDataCylinder->shininess = 50.0f;
	}
}

void Object3d::SetTexture(const std::string& textureFilePath) {
	TextureManager::GetInstance()->LoadTexture(textureFilePath);
	textureHandleCylinder = TextureManager::GetInstance()->GetSRVHandleGPU(textureFilePath);
	isTextureSetCylinder = true;
}

void Object3d::Update() {
	if (!transformationMatrix || !cameraData) {
		return;
	}

	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

	if (model) {
		if (model->GetIsAnimation() && animation.duration > 0.0f) {
			animationTime += 1.0f / 60.0f;
			animationTime = std::fmod(animationTime, animation.duration);
			ApplyAnimationToSkeleton();
		}

		UpdateSkeleton();
		UpdateSkinningPaletteResource();
		const Matrix4x4 modelLocalMatrix = model->HasSkinCluster()
		    ? MakeIdentity4x4()
		    : hasSkeleton && skeleton.root >= 0 && skeleton.root < static_cast<int32_t>(skeleton.joints.size())
		          ? skeleton.joints[skeleton.root].skeletonSpaceMatrix
		          : model->GetRootNode().localMatrix;
		worldMatrix = Multiply(modelLocalMatrix, worldMatrix);
	} else {
		UpdateSkinningPaletteResource();
	}

	if (camera) {
		cameraData->worldPosition = camera->GetTranslate();
		Matrix4x4 wvpMatrix = Multiply(worldMatrix, camera->GetViewProjectionMatrix());
		transformationMatrix->WVP = wvpMatrix;
		transformationMatrix->world = worldMatrix;
		transformationMatrix->WorldInverseTranspose = Transpose(Inverse(worldMatrix));
	}
	else {
		transformationMatrix->WVP = worldMatrix;
		transformationMatrix->world = worldMatrix;
		transformationMatrix->WorldInverseTranspose = Transpose(Inverse(worldMatrix));
	}


	cameraData->environmentMultiplier = environmentMultiplier;
}

void Object3d::UpdateAnimation()
{


}

void Object3d::Draw() {
	if (!wvpResorceModel || !lightResource || !cameraResource || !pointLightResource) return;

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = Object3dCommon::GetInstance()->GetDxCommon()->GetCommandList();
	SrvManager::GetInstance()->PreDraw();

	commandList->SetGraphicsRootConstantBufferView(1, wvpResorceModel->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(2, lightResource->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(4, cameraResource->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(5, pointLightResource->GetGPUVirtualAddress());
	if (!skinningPaletteResource) {
		CreateSkinningPaletteResource(1);
	}
	commandList->SetGraphicsRootShaderResourceView(7, skinningPaletteResource->GetGPUVirtualAddress());
	if (envMapTexturePath.empty()) {
		envMapTexturePath = "Resources/rostock_laage_airport_4k.dds";
	}
	TextureManager::GetInstance()->LoadTexture(envMapTexturePath);
	D3D12_GPU_DESCRIPTOR_HANDLE envMapHandle = TextureManager::GetInstance()->GetSRVHandleGPU(envMapTexturePath);
	commandList->SetGraphicsRootDescriptorTable(6, envMapHandle);

	if (model) {
		model->Draw();
		DrawDebugSkeleton();
	}
	else if (cylinderIndexCount > 0) {
		if (materialResourceCylinder) {
			commandList->SetGraphicsRootConstantBufferView(0, materialResourceCylinder->GetGPUVirtualAddress());
		}
		if (isTextureSetCylinder) {
			// =======================================================
			// =======================================================
			commandList->SetGraphicsRootDescriptorTable(3, textureHandleCylinder);
		}
		commandList->IASetVertexBuffers(0, 1, &vertexBufferViewCylinder);
		commandList->IASetIndexBuffer(&indexBufferViewCylinder);
		commandList->DrawIndexedInstanced(cylinderIndexCount, 1, 0, 0, 0);
	}
}

void Object3d::DrawDebugSkeleton() {
	if (!isDrawSkeleton_ || !hasSkeleton || skeleton.joints.empty()) {
		return;
	}

	const Matrix4x4 objectWorldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	const Vector4 boneColor = {1.0f, 0.85f, 0.1f, 1.0f};
	const Vector4 jointColor = {0.2f, 0.8f, 1.0f, 1.0f};
	const float jointSphereRadius = 0.05f;

	for (const Joint& joint : skeleton.joints) {
		const Matrix4x4 jointWorldMatrix = Multiply(joint.skeletonSpaceMatrix, objectWorldMatrix);
		const Vector3 jointPosition = GetTranslateFromMatrix(jointWorldMatrix);

		DrawDebugWireSphere(jointPosition, jointSphereRadius, jointColor);

		if (!joint.parent) {
			continue;
		}

		const Joint& parent = skeleton.joints[*joint.parent];
		const Matrix4x4 parentWorldMatrix = Multiply(parent.skeletonSpaceMatrix, objectWorldMatrix);
		const Vector3 parentPosition = GetTranslateFromMatrix(parentWorldMatrix);
		LineDrawer::GetInstance()->DrawLine(parentPosition, jointPosition, boneColor);
	}
}

void Object3d::SetModel(Model* newModel) {
	model = newModel;
	hasSkeleton = false;
	skeleton = {};
	animationTime = 0.0f;
	if (model && model->GetIsAnimation()) {
		animation = model->GetAnimation();
	}
	if (model) {
		skeleton = CreateSkeleton(model->GetRootNode());
		hasSkeleton = !skeleton.joints.empty();
		UpdateSkeleton();
		CreateSkinningPaletteResource(model->GetSkinningPaletteSize());
		UpdateSkinningPaletteResource();
	} else {
		CreateSkinningPaletteResource(1);
	}
}

void Object3d::SetModel(const std::string& filePath) {
	SetModel(ModelManager::GetInstance()->FindModel(filePath));
}



Object3d::~Object3d() {
	if (wvpResorceModel) wvpResorceModel->Unmap(0, nullptr);
	if (lightResource) lightResource->Unmap(0, nullptr);
	if (materialResourceCylinder) materialResourceCylinder->Unmap(0, nullptr);
	if (skinningPaletteResource) skinningPaletteResource->Unmap(0, nullptr);

	wvpResorceModel.Reset();
	lightResource.Reset();
	cameraResource.Reset();
	pointLightResource.Reset();
	vertexResourceCylinder.Reset();
	indexResourceCylinder.Reset();
	materialResourceCylinder.Reset();
	skinningPaletteResource.Reset();

	transformationMatrix = nullptr;
	cameraData = nullptr;
	directionallightData = nullptr;
	pointLightData = nullptr;
	materialDataCylinder = nullptr;
	skinningPaletteData = nullptr;
	camera = nullptr;
	model = nullptr;
}

void Object3d::SetDirectionalLight(const Vector4& color, const Vector3& direction, float intensity) {
	if (directionallightData) {
		directionallightData->color = color;
		directionallightData->direction = NormalizeReturnVector(direction);
		directionallightData->intensity = intensity;
	}
}

void Object3d::SetPointLight(const Vector4& color, const Vector3& position, float intensity, float radius, float decay) {
	if (pointLightData) {
		pointLightData->color = color;
		pointLightData->position = position;
		pointLightData->intensity = intensity;
		pointLightData->radius = radius;
		pointLightData->decay = decay;
	}
}

void Object3d::SetEnvironmentMultiplier(float multiplier) {
	environmentMultiplier = multiplier;
}


