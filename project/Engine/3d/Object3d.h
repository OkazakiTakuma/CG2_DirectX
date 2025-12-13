#pragma once
#include "Matrix.h"
#include "Screen.h"
#include "Vector.h"
#include <Windows.h>
#include <d3d12.h>
#include <string>
#include <fstream>
#include <sstream>
#include <cassert>
#include <wrl.h>
#include "../base/struct.h"
#include <vector>
class Object3dCommon;
class Model;
class Object3d {

public:
	void Initialize(Object3dCommon* object3dCommon);
	void Update();
	void Draw();
	void SetModel(Model* model) { this->model = model; }
	void SetModel(const std::string& filePath);
	const Vector3& GetTransformTranslate() { return transform.translate; };
	void SetTransformTranslate(const Vector3& newTransform) { transform.translate = newTransform; }
	const Vector3& GetTransformRotate() { return transform.rotate; };
	void SetTransformRotate(const Vector3& newTransformRotate) { transform.rotate = newTransformRotate; }
	const Vector3& GetTransformScale() { return transform.scale; };
	void SetTransformScale(const Vector3& newTransformScale) { transform.scale = newTransformScale; }
	const Vector3& GetCameraTransformTranslate() { return cameraTransform.translate; };
	void SetCameraTranslate(const Vector3& newCameraTransform) { cameraTransform.translate = newCameraTransform; }
	const Vector3& GetCameraTransformRotate() { return cameraTransform.rotate; };
	void SetCameraRotate(const Vector3& newCameraTransformRotate) { cameraTransform.rotate = newCameraTransformRotate; }

private:
	Object3dCommon* object3dCommon_ = nullptr;
	const float pi = 3.1415f; // 円周率
	const uint32_t kSubdivision = 16;                 // 球の細分化数
	const float kLonEvery = 2.0f * pi / kSubdivision; // 経度の間隔(φd)
	const float kLatEvery = pi / kSubdivision;        // 緯度の間隔(θd)
	uint32_t latIndex = 16;
	uint32_t lonIndex = 16;
	uint32_t startIndex = (kSubdivision * kSubdivision) * 6;
	Vector2 tex{};
	
	
	
	struct TransformationMatrix {
		Matrix4x4 WVP;
		Matrix4x4 world;
	};
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
	Model* model = nullptr;

	
	Transforms transform;

	// Trsnsformの変数を作る
	Transforms cameraTransform;

};
