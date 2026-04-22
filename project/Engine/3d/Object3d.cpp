#include "Object3d.h"
#include "../2d/TextureManager.h"
#include "Model.h"
#include "ModelManager.h"
#include "Object3dCommon.h"

void Object3d::Initialize() {
	// シングルトンから共通設定とデフォルトカメラを取得
	Object3dCommon* common = Object3dCommon::GetInstance();

	CreateWVPResource();
	CreateDirectionalLightResource();
	CreateCameraResource();

	transform = {
	    {1.0f, 1.0f, 1.0f}, // スケール
	    {0.0f, 0.0f, 0.0f}, // 回転
	    {0.0f, 0.0f, 0.0f}  // 平行移動
	};

	// 共通設定に登録されているデフォルトカメラをセット
	this->camera = common->GetDefaultCamera();
}

void Object3d::CreateWVPResource() {
	// シングルトンから dxCommon を経由してバッファ作成
	wvpResorceModel = Object3dCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));

	wvpResorceModel->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrix));
	transformationMatrix->WVP = MakeIdentity4x4();
	transformationMatrix->world = MakeIdentity4x4();
}
void Object3d::CreateCameraResource() {
	cameraResource = Object3dCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(CameraForGPU));
	cameraResource->Map(0, nullptr, reinterpret_cast<void**>(&cameraData));
}
void Object3d::CreateDirectionalLightResource() {
	// シングルトンから dxCommon を経由してバッファ作成
	lightResource = Object3dCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(DirectionalLight));


	lightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionallightData));

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
		transformationMatrix->WVP = worldMatrix;
	}
	transformationMatrix->world = worldMatrix;
	if (camera) {
		// カメラの現在の座標を転送
		cameraData->worldPosition = camera->GetTranslate();
	}
}

void Object3d::Draw() {
	// 描画コマンドリストを取得
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = Object3dCommon::GetInstance()->GetDxCommon()->GetCommandList();

	commandList->SetGraphicsRootConstantBufferView(1, wvpResorceModel->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(2, lightResource->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(4, cameraResource->GetGPUVirtualAddress()); // ★追加

	if (model) {
		model->Draw();
	}
}

void Object3d::SetModel(const std::string& filePath) { model = ModelManager::GetInstance()->FindModel(filePath); }

Object3d::~Object3d() {
	if (wvpResorceModel) {
		wvpResorceModel->Unmap(0, nullptr);
	}
	if (lightResource) {
		lightResource->Unmap(0, nullptr);
	}

	wvpResorceModel.Reset();
	lightResource.Reset();

	transformationMatrix = nullptr;
	camera = nullptr;
	model = nullptr;
}