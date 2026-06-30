#include "SkinnedObject3d.h"
#include "../2d/TextureManager.h"
#include "SkinnedModel.h"
#include "SkinnedObject3dCommon.h"
#include <cmath>

void SkinnedObject3d::Initialize() {
	SkinnedObject3dCommon* common = SkinnedObject3dCommon::GetInstance();

	CreateWVPResource();
	CreateDirectionalLightResource();
	CreateCameraResource();
	CreatePointLightResource();

	transform = {
		{1.0f, 1.0f, 1.0f}, // スケール
		{0.0f, 0.0f, 0.0f}, // 回転
		{0.0f, 0.0f, 0.0f}  // 平行移動
	};

	environmentMultiplier = 0.0f;
	this->camera = common->GetDefaultCamera();
}

SkinnedObject3d::~SkinnedObject3d() {
	if (wvpResource) wvpResource->Unmap(0, nullptr);
	if (lightResource) lightResource->Unmap(0, nullptr);
	if (cameraResource) cameraResource->Unmap(0, nullptr);
	if (pointLightResource) pointLightResource->Unmap(0, nullptr);
}

void SkinnedObject3d::Update() {
	// ============================================
	// ★追加：アニメーションの時間を進めて計算する処理
	// ============================================
	if (animation && model) {
		animationTime += 1.0f / 60.0f; // フレームレートに合わせて時間を進める（60FPS想定）

		// アニメーションが最後までいったらループさせる
		if (animationTime > animation->duration) {
			animationTime = std::fmod(animationTime, animation->duration);
		}

		// モデルの骨組みを計算して、GPU用の行列バッファを更新
		model->UpdateAnimation(*animation, animationTime);
		model->UpdateBoneMatrix();
	}

	// ============================================
	// 行列とライト情報の更新（既存のObject3dと同じ）
	// ============================================
	if (camera) {
		Matrix4x4 affineMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
		Matrix4x4 viewMatrix = camera->GetViewMatrix();
		Matrix4x4 projectionMatrix = camera->GetProjectionMatrix();
		Matrix4x4 viewProjectionMatrix = Multiply(viewMatrix, projectionMatrix);

		transformationMatrix->WVP = Multiply(affineMatrix, viewProjectionMatrix);
		transformationMatrix->world = affineMatrix;

		// スケーリングが含まれる場合の正確な法線計算のため、逆行列を作成（本来はさらに転置が必要ですが既存の計算に合わせます）
		transformationMatrix->WorldInverseTranspose = Inverse(affineMatrix);

		cameraData->worldPosition = camera->GetTranslate();
		cameraData->environmentMultiplier = environmentMultiplier;
	}
}

void SkinnedObject3d::Draw() {
	// アニメーション用パイプラインのセット
	SkinnedObject3dCommon::GetInstance()->SetDraw();
	ID3D12GraphicsCommandList* commandList = SkinnedObject3dCommon::GetInstance()->GetDxCommon()->GetCommandList().Get();

	// 1: WVP行列 (VS)
	commandList->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());
	// 2: ディレクショナルライト (PS)
	commandList->SetGraphicsRootConstantBufferView(2, lightResource->GetGPUVirtualAddress());
	// 4: カメラ情報 (PS)
	commandList->SetGraphicsRootConstantBufferView(4, cameraResource->GetGPUVirtualAddress());
	// 5: ポイントライト (PS)
	commandList->SetGraphicsRootConstantBufferView(5, pointLightResource->GetGPUVirtualAddress());

	// 6: 環境マップテクスチャ (PS)
	if (environmentMultiplier > 0.0f && !envMapTexturePath.empty()) {
		// TextureManager から GPUハンドルを取得
		D3D12_GPU_DESCRIPTOR_HANDLE envSrvHandle = TextureManager::GetInstance()->GetSRVHandleGPU(envMapTexturePath);
		// コマンドリストの 6番 (t1) に直接セット
		commandList->SetGraphicsRootDescriptorTable(6, envSrvHandle);
	}	if (model) {
		// ============================================
		// ★追加：ボーン行列パレットをルートパラメータ7番にセット
		// ============================================
		commandList->SetGraphicsRootConstantBufferView(7, model->GetBoneBufferVirtualAddress());

		// 引数なしでスッキリとモデルの描画を呼び出す！
		model->Draw();
	}
}

void SkinnedObject3d::CreateWVPResource() {
	wvpResource = SkinnedObject3dCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));
	wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrix));
	transformationMatrix->WVP = MakeIdentity4x4();
	transformationMatrix->world = MakeIdentity4x4();
	transformationMatrix->WorldInverseTranspose = MakeIdentity4x4();
}

void SkinnedObject3d::CreateDirectionalLightResource() {
	lightResource = SkinnedObject3dCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(DirectionalLight));
	lightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));
	directionalLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	directionalLightData->direction = { 0.0f, -1.0f, 0.0f };
	directionalLightData->intensity = 1.0f;
}

void SkinnedObject3d::CreatePointLightResource() {
	pointLightResource = SkinnedObject3dCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(PointLight));
	pointLightResource->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData));
	pointLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	pointLightData->position = { 0.0f, 0.0f, 0.0f };
	pointLightData->intensity = 0.0f;
	pointLightData->radius = 10.0f;
	pointLightData->decay = 1.0f;
}

void SkinnedObject3d::CreateCameraResource() {
	cameraResource = SkinnedObject3dCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(CameraInfo));
	cameraResource->Map(0, nullptr, reinterpret_cast<void**>(&cameraData));
	cameraData->worldPosition = { 0.0f, 0.0f, 0.0f };
	cameraData->environmentMultiplier = 0.0f;
}

void SkinnedObject3d::SetEnvironmentMap(const std::string& textureFilePath) {
	envMapTexturePath = textureFilePath;
	environmentMultiplier = 1.0f;
}

void SkinnedObject3d::SetDirectionalLight(const Vector4& color, const Vector3& direction, float intensity) {
	if (directionalLightData) {
		directionalLightData->color = color;
		// 既存のVector関数に合わせて方向を正規化
		directionalLightData->direction = NormalizeReturnVector(direction);
		directionalLightData->intensity = intensity;
	}
}

void SkinnedObject3d::SetPointLight(const Vector4& color, const Vector3& position, float intensity, float radius, float decay) {
	if (pointLightData) {
		pointLightData->color = color;
		pointLightData->position = position;
		pointLightData->intensity = intensity;
		pointLightData->radius = radius;
		pointLightData->decay = decay;
	}
}