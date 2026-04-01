#include "Object3d.h"
#include "../2d/TextureManager.h"
#include "Model.h"
#include "ModelManager.h"
#include "Object3dCommon.h"

void Object3d::Initialize(Object3dCommon* object3dCommon) {
	this->object3dCommon_ = object3dCommon;
	CreateWVPResource();
	CreateDirectionalLightResource();
	transform = {
	    {1.0f, 1.0f, 1.0f}, // スケール
	    {0.0f, 0.0f, 0.0f}, // 回転
	    {0.0f, 0.0f, 0.0f}  // 平行移動
	};
	this->camera = object3dCommon->GetDefaultCamera();
}

void Object3d::CreateWVPResource() {
	wvpResorceModel = object3dCommon_->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));
	// データを書き込む
	wvpResorceModel->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrix));
	transformationMatrix->WVP = MakeIdentity4x4();
	transformationMatrix->world = MakeIdentity4x4();
}

void Object3d::CreateDirectionalLightResource() {
	lightResource = object3dCommon_->GetDxCommon()->CreateBufferResource(sizeof(DirectionalLight));
	DirectionalLight* directionallightData = nullptr;
	lightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionallightData));

	// 値を設定（白くて上から照らす光）
	directionallightData->color = {1.0f, 1.0f, 1.0f, 1.0f};
	directionallightData->direction = NormalizeReturnVector(Vector3(0.0f, -1.0f, 0.0f));
	directionallightData->intensity = 1.0f;
}

void Object3d::Update() {
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	if (camera) {
		Matrix4x4 wvpMatrix = Multiply(worldMatrix, camera->GetViewProjectionMatrix());
		transformationMatrix->WVP = wvpMatrix;

	} else {
		Matrix4x4 wvpMatrix = worldMatrix;
		transformationMatrix->WVP = wvpMatrix;
	}
	transformationMatrix->world = worldMatrix;
}

void Object3d::Draw() {
	object3dCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(1, wvpResorceModel->GetGPUVirtualAddress());
	object3dCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(2, lightResource->GetGPUVirtualAddress());
	if (model) {
		model->Draw();
	}
}

void Object3d::SetModel(const std::string& filePath) { model = ModelManager::GetInstance()->FindModel(filePath); }

Object3d::~Object3d() {
	// 1. マップ解除 (Unmap)
	// 書き込み用ポインタを開放する前にUnmapを呼びます
	if (wvpResorceModel) {
		wvpResorceModel->Unmap(0, nullptr);
	}
	if (lightResource) {
		lightResource->Unmap(0, nullptr);
	}

	// 2. ComPtr の解放 (Reset)
	// これにより ID3D12Resource の参照カウントが減ります
	wvpResorceModel.Reset();
	lightResource.Reset();

	// 3. ポインタと参照のクリア
	transformationMatrix = nullptr;
	object3dCommon_ = nullptr;
	camera = nullptr;
	model = nullptr; // モデルの実体は ModelManager が管理しているので Reset/delete は不要
}