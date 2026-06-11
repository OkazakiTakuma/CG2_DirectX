#include "Object3d.h"
#include "../2d/TextureManager.h"
#include "Model.h"
#include "ModelManager.h"
#include "Object3dCommon.h"
#include <cmath>

void Object3d::Initialize() {
	// シングルトンから共通設定とデフォルトカメラを取得
	Object3dCommon* common = Object3dCommon::GetInstance();

	CreateWVPResource();
	CreateDirectionalLightResource();
	CreateCameraResource();
	CreatePointLightResource();

	transform = {
		{1.0f, 1.0f, 1.0f}, // スケール
		{0.0f, 0.0f, 0.0f}, // 回転
		{0.0f, 0.0f, 0.0f}  // 平行移動
	};

	// 環境マップの強さを初期化
	environmentMultiplier = 0.0f;
	// 共通設定に登録されているデフォルトカメラをセット
	this->camera = common->GetDefaultCamera();
}
// 外から環境マップのテクスチャを切り替えるための関数
void Object3d::SetEnvironmentMap(const std::string& textureFilePath) {
	envMapTexturePath = textureFilePath;

	// =======================================================
	// ★追加：テクスチャがセットされたので、強さを「1.0f（オン）」にする
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

// 蓋の生成フラグを追加したシリンダー生成
void Object3d::CreateCylinder(float radius, float height, uint32_t subdivision, bool createTopCap, bool createBottomCap) {
	std::vector<VertexData> vertices;
	std::vector<uint32_t> indices;

	float halfHeight = height / 2.0f;

	// -------------------------------------------------------
	// 1. 側面（Side）の生成
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
	// 2. 上面（Top Cap）の生成 (蓋パーツ)
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
	// 3. 底面（Bottom Cap）の生成 (底パーツ)
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
	// 4. DirectX12 バッファの生成とデータ転送
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
	textureHandleCylinder = TextureManager::GetInstance()->GetSRVHandleGPU(textureFilePath);
	isTextureSetCylinder = true;
}

void Object3d::Update() {
	// ★追加：データが作られていない場合はエラーを防ぐため処理を抜ける
	if (!transformationMatrix || !cameraData) {
		return;
	}

	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

	if (model) {
		worldMatrix = Multiply(model->GetRootNode().localMatrix, worldMatrix);
	}

	if (camera) {
		Matrix4x4 wvpMatrix = Multiply(worldMatrix, camera->GetViewProjectionMatrix());
		transformationMatrix->WVP = wvpMatrix;
		transformationMatrix->world = worldMatrix;
		cameraData->worldPosition = camera->GetTranslate();
	}
	else {
		transformationMatrix->WVP = worldMatrix;
		transformationMatrix->world = worldMatrix;
	}

	cameraData->environmentMultiplier = environmentMultiplier;
}

void Object3d::Draw() {
	// 念のため、前回の安全対策（Nullチェック）も追加しておきます
	if (!wvpResorceModel || !lightResource || !cameraResource || !pointLightResource) return;

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = Object3dCommon::GetInstance()->GetDxCommon()->GetCommandList();

	commandList->SetGraphicsRootConstantBufferView(1, wvpResorceModel->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(2, lightResource->GetGPUVirtualAddress()); // ←★2番はライトが使っている！
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
			// ★修正：ここの「2」を「3」に変更してください！！
			// =======================================================
			commandList->SetGraphicsRootDescriptorTable(3, textureHandleCylinder);
		}
		commandList->IASetVertexBuffers(0, 1, &vertexBufferViewCylinder);
		commandList->IASetIndexBuffer(&indexBufferViewCylinder);
		commandList->DrawIndexedInstanced(cylinderIndexCount, 1, 0, 0, 0);
	}
}void Object3d::SetModel(const std::string& filePath) { model = ModelManager::GetInstance()->FindModel(filePath); }

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