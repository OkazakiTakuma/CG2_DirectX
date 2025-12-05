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
	cameraTransform = {
	    {1.0f, 1.0f, 1.0f  }, // スケール
	    {0.0f, 0.0f, 0.0f  }, // 回転
	    {0.0f, 4.0f, -10.0f}  // 平行移動
	};
}

void Object3d::CreateWVPResource() {
	wvpResorceModel = object3dCommon_->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));
	assert(SUCCEEDED(hr)); // WVPリソースの生成が成功したか確認
	// データを書き込む
	wvpResorceModel->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrix));
	assert(SUCCEEDED(hr));
	transformationMatrix->WVP = MakeIdentity4x4();
	transformationMatrix->world = MakeIdentity4x4();
}

void Object3d::CreateDirectionalLightResource() {
	lightResource = object3dCommon_->GetDxCommon()->CreateBufferResource(sizeof(DirectionalLight));
	assert(SUCCEEDED(hr)); // ライトリソースの生成が成功したか確認
	DirectionalLight* directionallightData = nullptr;
	lightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionallightData));

	// 値を設定（白くて上から照らす光）
	directionallightData->color = {1.0f, 1.0f, 1.0f, 1.0f};
	directionallightData->direction = NormalizeReturnVector(Vector3(0.0f, -1.0f, 0.0f));
	directionallightData->intensity = 1.0f;
}

void Object3d::Update() {
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
	Matrix4x4 viewMatrix = Inverse(cameraMatrix);
	Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(WinApp::kClientWidth) / float(WinApp::kClientHeight), 0.1f, 100.0f);
	Matrix4x4 wvpMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
	transformationMatrix->WVP = wvpMatrix;
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
