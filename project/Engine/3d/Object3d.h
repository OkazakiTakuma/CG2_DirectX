#pragma once
#include "../3d/Matrix.h"
#include "../3d/Screen.h"
#include "../3d/Vector.h"
#include <Windows.h>
#include <d3d12.h>
#include <string>
#include <fstream>
#include <sstream>
#include <cassert>
#include <wrl.h>
#include "../base/Brend.h"
#include <vector>
struct MaterialData {
	std::string textureFilePath; // テクスチャファイルのパス
	uint32_t textureIndex = 0;
};

struct ModelData {
	std::vector<VertexData> vertices; // 頂点データ
	MaterialData material;
};

class Object3dCommon;
class Object3d {

public:
	void Initialize(Object3dCommon* object3dCommon);
	void Update();
	void Draw();
	const Vector3& GetTransformTranslate() { return transform.translate; };
	void SetTransform(const Vector3& newTransform) { transform.translate = newTransform; }
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
	
	ModelData modelData;
	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
	static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	VertexData* vertexData = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	void CreateVertexdata();
	struct Material {
		Vector4 color;          // 色
		int32_t enableLighting; // ライティングの有効化フラグ
		float padding[3];       // パディング
		Matrix4x4 uvTransform;  // UV変換行列
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
	Material* materialData = nullptr;
	void CreateMaterialData();
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

	
	Transforms transform;

	// Trsnsformの変数を作る
	Transforms cameraTransform;

};
