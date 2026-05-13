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
	CreatePointLightResource();

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
	transformationMatrix->WorldInverseTranspose = MakeIdentity4x4();
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

void Object3d::CreatePointLightResource() {
	pointLightResource = Object3dCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(PointLight));
	pointLightResource->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData));
	pointLightData->color = {1.0f, 1.0f, 1.0f, 1.0f};
	pointLightData->position = {0.0f, 0.0f, 0.0f};
	pointLightData->intensity = 1.0f;
	pointLightData->radius = 10.0f;
	pointLightData->decay = 1.0f;
}

void Object3d::Update() {
	// 1. Object3d 自身のトランスフォーム（SRT）から行列を作成
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

	// --- ここで RootNode の Matrix を適用 ---
	if (model) {
		// Model から RootNode の localMatrix を取得して掛け合わせる
		// 順序はエンジンの仕様によりますが、一般的には [Rootの計算結果] * [Object3dの行列] です
		worldMatrix = Multiply(model->GetRootNode().localMatrix, worldMatrix);
	}
	// ---------------------------------------

	if (camera) {
		// 合成された worldMatrix を使って WVP を計算
		Matrix4x4 wvpMatrix = Multiply(worldMatrix, camera->GetViewProjectionMatrix());
		transformationMatrix->WVP = wvpMatrix;
		transformationMatrix->world = worldMatrix;

		// カメラの座標も転送
		cameraData->worldPosition = camera->GetTranslate();
	} else {
		transformationMatrix->WVP = worldMatrix;
		transformationMatrix->world = worldMatrix;
	}
}
void Object3d::Draw() {
	// 描画コマンドリストを取得
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = Object3dCommon::GetInstance()->GetDxCommon()->GetCommandList();

	commandList->SetGraphicsRootConstantBufferView(1, wvpResorceModel->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(2, lightResource->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(4, cameraResource->GetGPUVirtualAddress());     // ★追加
	commandList->SetGraphicsRootConstantBufferView(5, pointLightResource->GetGPUVirtualAddress()); // ★追加
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
