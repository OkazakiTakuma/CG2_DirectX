#pragma once
#include "../../base/struct.h"
#include "../camera/Camera.h"
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
class DirectXCommon;
class Model;

/// <summary>
/// 1つの3Dオブジェクトについて、モデル描画、アニメーション、スキニング、ライト設定を管理します。
/// GPUパイプラインの共有管理はObject3dCommonへ委譲します。
/// </summary>
class Object3d {
public:
	/// <summary>
	/// 必要なリソースを準備し、オブジェクトを初期化します。
	/// </summary>
	void Initialize();
	/// <summary>
	/// 毎フレームの状態更新を行います。
	/// </summary>
	void Update();
	/// <summary>
	/// 現在の状態をもとに描画処理を行います。
	/// </summary>
	void Draw();
	void SetModel(Model* model);
	void SetModel(const std::string& filePath);
	void DrawDebugSkeleton();
	void SetDrawSkeleton(bool isDraw) { isDrawSkeleton_ = isDraw; }
	bool GetDrawSkeleton() const { return isDrawSkeleton_; }
	bool HasSkeleton() const { return hasSkeleton && !skeleton.joints.empty(); }
	bool HasModel() const { return model != nullptr; }
	bool GetJointSkeletonSpaceMatrix(const std::string& jointName, Matrix4x4& jointMatrix) const;
	bool GetJointWorldMatrix(const std::string& jointName, Matrix4x4& jointWorldMatrix) const;
	void SetWorldMatrixOverride(const Matrix4x4& worldMatrix) {
		worldMatrixOverride_ = worldMatrix;
		hasWorldMatrixOverride_ = true;
	}
	void ClearWorldMatrixOverride() { hasWorldMatrixOverride_ = false; }
	bool HasAnimation() const;
	void SetAnimationPlaying(bool isPlaying);
	bool GetAnimationPlaying() const { return isAnimationPlaying_; }
	void RestartAnimation();
	void ResetAnimationPoseToInitial();
	bool SetAnimation(const std::string& animationName, bool restart = true);
	const std::string& GetAnimationName() const { return activeAnimationName_; }
	const std::vector<std::string>& GetAnimationNames() const;
	float GetAnimationTime() const { return animationTime; }
	float GetAnimationDuration() const { return animation.duration; }
	/// <summary>
	/// 破棄時に必要な解放処理を行います。
	/// </summary>
	~Object3d();

	/// <summary>
	/// Cylinder を作成し、利用できる状態にします。
	/// </summary>
	/// <param name="radius">半径を指定します。</param>
	/// <param name="height">高さを指定します。</param>
	void CreateCylinder(float radius = 1.0f, float height = 2.0f, uint32_t subdivision = 16, bool createTopCap = true, bool createBottomCap = true);
	/// <param name="textureFilePath">使用するテクスチャまたはモデルのファイルパスを指定します。</param>
	void SetTexture(const std::string& textureFilePath);
	void SetModelTexture(const std::string& textureFilePath);
	void SetModelTextureOverride(const std::string& textureFilePath);
	std::string GetModelTextureFilePath() const;

	const Vector3& GetTranslate() { return transform.translate; };
	void SetTranslate(const Vector3& newTransform) { transform.translate = newTransform; }
	const Vector3& GetRotate() { return transform.rotate; };
	void SetRotate(const Vector3& newTransformRotate) { transform.rotate = newTransformRotate; }
	const Vector3& GetScale() { return transform.scale; };
	void SetScale(const Vector3& newTransformScale) { transform.scale = newTransformScale; }
	void SetCamera(Camera* cmr) { camera = cmr; }
	/// <param name="color">色を指定します。</param>
	/// <param name="intensity">強度を指定します。</param>
	void SetDirectionalLight(const Vector4& color, const Vector3& direction, float intensity);
	/// <param name="color">色を指定します。</param>
	/// <param name="position">位置を指定します。</param>
	/// <param name="intensity">強度を指定します。</param>
	/// <param name="radius">半径を指定します。</param>
	void SetPointLight(const Vector4& color, const Vector3& position, float intensity, float radius, float decay);
	void SetEnvironmentMultiplier(float multiplier);
	void SetColor(const Vector4& color) { if (materialOverrideData_) materialOverrideData_->color = color; }
	Vector4 GetColor() const { return materialOverrideData_ ? materialOverrideData_->color : Vector4{1.0f, 1.0f, 1.0f, 1.0f}; }
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
	const float GetEnvironmentMultiplier() const { return environmentMultiplier; }
	/// <param name="textureFilePath">使用するテクスチャまたはモデルのファイルパスを指定します。</param>
	void SetEnvironmentMap(const std::string& textureFilePath);

private:
	DirectXCommon* dxCommon_ = nullptr;
	struct CameraForGPU {
		Vector3 worldPosition;
		float environmentMultiplier;
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource;
	CameraForGPU* cameraData = nullptr;

	/// <summary>
	/// CameraResource を作成し、利用できる状態にします。
	/// </summary>
	void CreateCameraResource();
	/// <summary>
	/// SkinningPaletteResource を作成し、利用できる状態にします。
	/// </summary>
	void CreateSkinningPaletteResource(uint32_t paletteCount);
	void UpdateSkinningPaletteResource();

	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResorceModel;
	TransformationMatrix* transformationMatrix = nullptr;
	/// <summary>
	/// WVPResource を作成し、利用できる状態にします。
	/// </summary>
	void CreateWVPResource();
	struct DirectionalLight {
		Vector4 color;
		Vector3 direction;
		float intensity;
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> lightResource;
	DirectionalLight* directionallightData = nullptr;
	/// <summary>
	/// DirectionalLightResource を作成し、利用できる状態にします。
	/// </summary>
	void CreateDirectionalLightResource();
	Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource;
	PointLight* pointLightData = nullptr;
	/// <summary>
	/// PointLightResource を作成し、利用できる状態にします。
	/// </summary>
	void CreatePointLightResource();

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceCylinder;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceCylinder;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewCylinder{};
	D3D12_INDEX_BUFFER_VIEW indexBufferViewCylinder{};
	uint32_t cylinderIndexCount = 0;

	D3D12_GPU_DESCRIPTOR_HANDLE textureHandleCylinder{};
	bool isTextureSetCylinder = false;

	struct MaterialData {
		Vector4 color;
		int32_t enableLighting;
		float padding[3];
		Matrix4x4 uvTransform;
		float shininess;
		float padding2[3];
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceCylinder;
	MaterialData* materialDataCylinder = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialOverrideResource_;
	MaterialData* materialOverrideData_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> shadowMaterialResource;
	MaterialData* shadowMaterialData = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> shadowWvpResource;
	TransformationMatrix* shadowTransformationMatrix = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> skinningPaletteResource;
	Matrix4x4* skinningPaletteData = nullptr;
	uint32_t skinningPaletteCapacity_ = 0;
	std::vector<Matrix4x4> skinningPalette_;
	Matrix4x4 worldMatrixOverride_ = MakeIdentity4x4();
	bool hasWorldMatrixOverride_ = false;

	Model* model = nullptr;
	std::string modelTextureOverridePath_;
	bool isPointLightSet = true;
	float environmentMultiplier = 1.0f;

	EulerTransform transform = {
	    {1.0f, 1.0f, 1.0f},
	    {0.0f, 0.0f, 0.0f},
	    {0.0f, 0.0f, 0.0f}
	};
	Camera* camera = nullptr;
	std::string envMapTexturePath = "Resources/rostock_laage_airport_4k.dds";
	float animationTime = 0.0f;
	float animationBlendWeight_ = 1.0f;
	float animationBlendSpeed_ = 0.12f;
	Animation animation;
	std::string activeAnimationName_;
	Skeleton skeleton;
	bool hasSkeleton = false;
	bool isDrawSkeleton_ = false;
	bool isAnimationPlaying_ = true;
	bool useInitialSkinningPose_ = false;
	bool isShadowEnabled_ = true;
	float shadowPlaneY_ = 0.01f;
	float shadowAlpha_ = 0.35f;
};
