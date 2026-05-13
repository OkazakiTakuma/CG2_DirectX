#include "SkyBox.h"
#include "../2d/TextureManager.h"
#include "Model.h"
#include "ModelManager.h"
#include "Object3dCommon.h"

void SkyBox::Initialize() {
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

void SkyBox::CreateWVPResource() {
	// シングルトンから dxCommon を経由してバッファ作成
	wvpResorceModel = Object3dCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));

	wvpResorceModel->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrix));
	transformationMatrix->WVP = MakeIdentity4x4();
	transformationMatrix->world = MakeIdentity4x4();
	transformationMatrix->WorldInverseTranspose = MakeIdentity4x4();
}
void SkyBox::CreateCameraResource() {
	cameraResource = Object3dCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(CameraForGPU));
	cameraResource->Map(0, nullptr, reinterpret_cast<void**>(&cameraData));
}
void SkyBox::CreateDirectionalLightResource() {
	// シングルトンから dxCommon を経由してバッファ作成
	lightResource = Object3dCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(DirectionalLight));

	lightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionallightData));

	directionallightData->color = {1.0f, 1.0f, 1.0f, 1.0f};
	directionallightData->direction = NormalizeReturnVector(Vector3(0.0f, -1.0f, 0.0f));
	directionallightData->intensity = 1.0f;
}

void SkyBox::CreatePointLightResource() {
	pointLightResource = Object3dCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(PointLight));
	pointLightResource->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData));
	pointLightData->color = {1.0f, 1.0f, 1.0f, 1.0f};
	pointLightData->position = {0.0f, 0.0f, 0.0f};
	pointLightData->intensity = 1.0f;
	pointLightData->radius = 10.0f;
	pointLightData->decay = 1.0f;
}

void SkyBox::Update() {
	// 1. カメラが存在する場合、スカイボックスの座標を常にカメラと同じにする
	if (camera) {
		transform.translate = camera->GetTranslate();
	}

	// 2. ワールド行列の作成 (回転やスケールが必要な場合はここで行う)
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

	if (camera) {
		// 3. WVP行列の計算
		Matrix4x4 wvpMatrix = Multiply(worldMatrix, camera->GetViewProjectionMatrix());
		transformationMatrix->WVP = wvpMatrix;
		transformationMatrix->world = worldMatrix;
		transformationMatrix->WorldInverseTranspose = Inverse(worldMatrix);

		// 4. カメラ座標を定数バッファに転送 (Shaderの CameraInfo 用)
		cameraData->worldPosition = camera->GetTranslate();
	} else {
		transformationMatrix->WVP = worldMatrix;
	}
}
void SkyBox::Draw() {
	// 1. 各種共通設定とコマンドリストの取得
	Object3dCommon* object3dCommon = Object3dCommon::GetInstance();
	DirectXCommon* dxCommon = object3dCommon->GetDxCommon();
	auto commandList = dxCommon->GetCommandList();

	// 2. スカイボックス専用の RootSignature と PSO をセット
	commandList->SetGraphicsRootSignature(object3dCommon->GetSkyBoxRootSignature());
	commandList->SetPipelineState(object3dCommon->GetSkyBoxPipelineState());

	// 3. レンダーターゲット（RTV）と「読み取り専用 DSV」のセット
	// ※ DirectXCommon に現在の RTV と DSV ハンドルを取得する関数がある想定です
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = dxCommon->GetCurrentBackBufferRTV();
	D3D12_CPU_DESCRIPTOR_HANDLE readOnlyDsvHandle = dxCommon->GetReadOnlyDsvHandle();

	// 深度バッファへの書き込みを禁止しつつ、既存の深度（1.0未満の物体）との比較だけ行う
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &readOnlyDsvHandle);

	// 4. 定数バッファ（CBV）をセット
	// ※ HLSL の register(bX) の番号と RootParameter のインデックスを一致させています

	// RootParam 1 -> register(b1): TransformationMatrix (WVP)
	commandList->SetGraphicsRootConstantBufferView(1, wvpResorceModel->GetGPUVirtualAddress());

	// RootParam 2 -> register(b2): DirectionalLight
	commandList->SetGraphicsRootConstantBufferView(2, lightResource->GetGPUVirtualAddress());

	// RootParam 3 -> register(b3): CameraInfo (カメラの座標)
	commandList->SetGraphicsRootConstantBufferView(3, cameraResource->GetGPUVirtualAddress());

	// RootParam 4 -> register(b4): PointLight
	commandList->SetGraphicsRootConstantBufferView(4, pointLightResource->GetGPUVirtualAddress());

	// 5. モデルの描画実行
	if (model) {
		// model->Draw() 内部で register(b0):Material や register(t0):TextureCube がセットされます
		model->Draw();
	}

	// 6. 後続の描画処理（UIなど）のために、DSV を通常（読み書き可能）の状態に戻す
	D3D12_CPU_DESCRIPTOR_HANDLE normalDsvHandle = dxCommon->GetDsvHandle();
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &normalDsvHandle);
}
void SkyBox::SetModel(const std::string& filePath) { model = ModelManager::GetInstance()->FindModel(filePath); }

void SkyBox::SetModelSphere(const std::string& filePath) { model = ModelManager::GetInstance()->FindModel(filePath); }

SkyBox::~SkyBox() {
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

void SkyBox::SetDirectionalLight(const Vector4& color, const Vector3& direction, float intensity) {
	if (directionallightData) {
		directionallightData->color = color;
		directionallightData->direction = NormalizeReturnVector(direction);
		directionallightData->intensity = intensity;
	}
}

void SkyBox::SetPointLight(const Vector4& color, const Vector3& position, float intensity, float radius, float decay) {
	if (pointLightData) {
		pointLightData->color = color;
		pointLightData->position = position;
		pointLightData->intensity = intensity;
		pointLightData->radius = radius;
		pointLightData->decay = decay;
	}
}
