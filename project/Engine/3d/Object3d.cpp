#include "Object3d.h"
#include "../2d/TextureManager.h"
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
}

void Object3d::Initialize() {
	Object3dCommon* common = Object3dCommon::GetInstance();

	CreateWVPResource();
	CreateDirectionalLightResource();
	CreateCameraResource();
	CreatePointLightResource();

	transform = {
		{1.0f, 1.0f, 1.0f},
		{0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f}
	};

	environmentMultiplier = 0.0f;
	this->camera = common->GetDefaultCamera();
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
		worldMatrix = Multiply(model->GetRootNode().localMatrix, worldMatrix);
	}

	if (camera) {
		cameraData->worldPosition = camera->GetTranslate();
		if (model && model->GetIsAnimation() && animation.duration > 0.0f) {
			animationTime += 1.0f / 60.0f;
			animationTime = std::fmod(animationTime, animation.duration);
			auto rootNodeAnimationItr = animation.nodeAnimations.find(model->GetRootNode().name);
			if (rootNodeAnimationItr != animation.nodeAnimations.end()) {
				NodeAnimation& rootNodeAnimation = rootNodeAnimationItr->second;
				Vector3 translate = rootNodeAnimation.translate.keyframes.empty() ? Vector3{ 0.0f, 0.0f, 0.0f } : CalculateValue(rootNodeAnimation.translate.keyframes, animationTime);
				Quaternion rotate = rootNodeAnimation.rotate.keyframes.empty() ? IdentityQuaternion() : CalculateValue(rootNodeAnimation.rotate.keyframes, animationTime);
				Vector3 scale = rootNodeAnimation.scale.keyframes.empty() ? Vector3{ 1.0f, 1.0f, 1.0f } : CalculateValue(rootNodeAnimation.scale.keyframes, animationTime);
				Matrix4x4 animationMatrix = MakeAffineMatrix(scale, rotate, translate);
				worldMatrix = Multiply(animationMatrix, MakeAffineMatrix(transform.scale, transform.rotate, transform.translate));
			}
		}
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

	commandList->SetGraphicsRootConstantBufferView(1, wvpResorceModel->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(2, lightResource->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(4, cameraResource->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(5, pointLightResource->GetGPUVirtualAddress());
	D3D12_GPU_DESCRIPTOR_HANDLE envMapHandle = TextureManager::GetInstance()->GetSRVHandleGPU(envMapTexturePath);
	commandList->SetGraphicsRootDescriptorTable(6, envMapHandle);

	if (model) {
		model->Draw();
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

void Object3d::SetModel(const std::string& filePath) {
	model = ModelManager::GetInstance()->FindModel(filePath);
	if (model && model->GetIsAnimation()) {
		animation = model->GetAnimation();
	}
}



Object3d::~Object3d() {
	if (wvpResorceModel) wvpResorceModel->Unmap(0, nullptr);
	if (lightResource) lightResource->Unmap(0, nullptr);
	if (materialResourceCylinder) materialResourceCylinder->Unmap(0, nullptr);

	wvpResorceModel.Reset();
	lightResource.Reset();
	cameraResource.Reset();
	pointLightResource.Reset();
	vertexResourceCylinder.Reset();
	indexResourceCylinder.Reset();
	materialResourceCylinder.Reset();

	transformationMatrix = nullptr;
	cameraData = nullptr;
	directionallightData = nullptr;
	pointLightData = nullptr;
	materialDataCylinder = nullptr;
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


