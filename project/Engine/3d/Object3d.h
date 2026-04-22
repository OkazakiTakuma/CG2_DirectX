#pragma once
#include "../base/struct.h"
#include "Camera.h"
#include "Matrix.h"
#include "Screen.h"
#include "Vector.h"
#include "struct.h"
#include <Windows.h>
#include <cassert>
#include <d3d12.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <wrl.h>
class Object3dCommon;
class Model;
class Object3d {

public:
	void Initialize();
	void Update();
	void Draw();
	void SetModel(Model* model) { this->model = model; }
	void SetModel(const std::string& filePath);
	~Object3d();
	const Vector3& GetTranslate() { return transform.translate; };
	void SetTranslate(const Vector3& newTransform) { transform.translate = newTransform; }
	const Vector3& GetRotate() { return transform.rotate; };
	void SetRotate(const Vector3& newTransformRotate) { transform.rotate = newTransformRotate; }
	const Vector3& GetScale() { return transform.scale; };
	void SetScale(const Vector3& newTransformScale) { transform.scale = newTransformScale; }
	void SetCamera(Camera* cmr) { camera = cmr; }
	void SetDirectionalLight(const Vector4& color, const Vector3& direction, float intensity);
	void SetPointLight(const Vector4& color, const Vector3& position, float intensity, float radius, float decay);
	void IsPointLightSet(bool isSet) { isPointLightSet = isSet; }

	const Vector4 GetLightColor() const { return directionallightData ? directionallightData->color : Vector4(0.0f, 0.0f, 0.0f, 1.0f); }
	const Vector3 GetLightDirection() const { return directionallightData ? directionallightData->direction : Vector3(0.0f, -1.0f, 0.0f); }
	const float GetLightIntensity() const { return directionallightData ? directionallightData->intensity : 0.0f; }
	const Vector4 GetPointLightColor() const { return pointLightData ? pointLightData->color : Vector4(0.0f, 0.0f, 0.0f, 1.0f); }
	const Vector3 GetPointLightPosition() const { return pointLightData ? pointLightData->position : Vector3(0.0f, 0.0f, 0.0f); }
	const float GetPointLightIntensity() const { return pointLightData ? pointLightData->intensity : 0.0f; }
	const float GetPointLightRadius() const { return pointLightData ? pointLightData->radius : 0.0f; }
	const float GetPointLightDecay() const { return pointLightData ? pointLightData->decay : 0.0f; }
	const bool GetIsPointLightSet() const { return isPointLightSet; }

private:
	const float pi = 3.1415f;                         // 円周率
	const uint32_t kSubdivision = 16;                 // 球の細分化数
	const float kLonEvery = 2.0f * pi / kSubdivision; // 経度の間隔(φd)
	const float kLatEvery = pi / kSubdivision;        // 緯度の間隔(θd)
	uint32_t latIndex = 16;
	uint32_t lonIndex = 16;
	uint32_t startIndex = (kSubdivision * kSubdivision) * 6;
	Vector2 tex{};
	struct CameraForGPU {
		Vector3 worldPosition;
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource;
	CameraForGPU* cameraData = nullptr;

	void CreateCameraResource(); // バッファ作成用

	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResorceModel;
	TransformationMatrix* transformationMatrix = nullptr;
	void CreateWVPResource();
	struct DirectionalLight {
		Vector4 color;     // 光の色
		Vector3 direction; // 光の方向
		float intensity;   // 光の強度
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> lightResource;
	DirectionalLight* directionallightData = nullptr;
	void CreateDirectionalLightResource();
	Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource;
	PointLight* pointLightData = nullptr;
	void CreatePointLightResource();

	Model* model = nullptr;
	bool isPointLightSet = true;

	Transform transform;
	Camera* camera = nullptr;
};
