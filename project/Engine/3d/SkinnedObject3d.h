#pragma once
#include "../base/struct.h"
#include "Camera.h"
#include "Matrix.h"
#include "Vector.h"
#include "struct.h"
#include <Windows.h>
#include <cassert>
#include <d3d12.h>
#include <string>
#include <wrl.h>

class SkinnedObject3dCommon;
class SkinnedModel;

class SkinnedObject3d {
public:
	void Initialize();
	void Update();
	void Draw();

	void SetModel(SkinnedModel* model) { this->model = model; }

	// ★追加：再生するアニメーションをセットする
	void SetAnimation(Animation* anim) {
		this->animation = anim;
		this->animationTime = 0.0f; // セットした時に時間をリセット
	}

	~SkinnedObject3d();

	void SetEnvironmentMap(const std::string& textureFilePath);
	void SetDirectionalLight(const Vector4& color, const Vector3& direction, float intensity);
	void SetPointLight(const Vector4& color, const Vector3& position, float intensity, float radius, float decay);

	const Vector3& GetTranslate() { return transform.translate; };
	void SetTranslate(const Vector3& newTransform) { transform.translate = newTransform; }
	const Vector3& GetRotate() { return transform.rotate; };
	void SetRotate(const Vector3& newTransformRotate) { transform.rotate = newTransformRotate; }
	const Vector3& GetScale() { return transform.scale; };
	void SetScale(const Vector3& newTransformScale) { transform.scale = newTransformScale; }

private:
	SkinnedModel* model = nullptr;

	// アニメーション用の変数
	Animation* animation = nullptr;
	float animationTime = 0.0f;

	Transform transform{};
	Camera* camera = nullptr;

	// 環境マップ
	std::string envMapTexturePath;
	float environmentMultiplier = 0.0f;

	// --- DirectXリソース群 ---
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource;
	TransformationMatrix* transformationMatrix = nullptr;
	void CreateWVPResource();

	struct DirectionalLight {
		Vector4 color;     // 光の色
		Vector3 direction; // 光の方向
		float intensity;   // 光の強度
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> lightResource;
	DirectionalLight* directionalLightData = nullptr;
	void CreateDirectionalLightResource();

	Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource;
	PointLight* pointLightData = nullptr;
	void CreatePointLightResource();

	struct CameraInfo {
		Vector3 worldPosition;
		float environmentMultiplier;
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource;
	CameraInfo* cameraData = nullptr;
	void CreateCameraResource();
};