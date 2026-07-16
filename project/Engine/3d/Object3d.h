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
	/// <summary>
	/// UpdateAnimation の処理を行います。
	/// </summary>
	void UpdateAnimation();
	/// <summary>
	/// Model を設定します。
	/// </summary>
	/// <param name="model">model に使用する値を指定します。</param>
	void SetModel(Model* model);
	/// <summary>
	/// Model を設定します。
	/// </summary>
	/// <param name="filePath">読み込みまたは保存に使用するファイルパスを指定します。</param>
	void SetModel(const std::string& filePath);
	/// <summary>
	/// DrawDebugSkeleton の処理を行います。
	/// </summary>
	void DrawDebugSkeleton();
	void SetDrawSkeleton(bool isDraw) { isDrawSkeleton_ = isDraw; }
	bool GetDrawSkeleton() const { return isDrawSkeleton_; }
	bool HasSkeleton() const { return hasSkeleton && !skeleton.joints.empty(); }
	bool HasModel() const { return model != nullptr; }
	bool HasAnimation() const;
	void SetAnimationPlaying(bool isPlaying);
	bool GetAnimationPlaying() const { return isAnimationPlaying_; }
	void RestartAnimation();
	void ResetAnimationPoseToInitial();
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
	/// <param name="subdivision">subdivision に使用する値を指定します。</param>
	/// <param name="createTopCap">createTopCap に使用する値を指定します。</param>
	/// <param name="createBottomCap">createBottomCap に使用する値を指定します。</param>
	void CreateCylinder(float radius = 1.0f, float height = 2.0f, uint32_t subdivision = 16, bool createTopCap = true, bool createBottomCap = true);
	/// <summary>
	/// Texture を設定します。
	/// </summary>
	/// <param name="textureFilePath">使用するテクスチャまたはモデルのファイルパスを指定します。</param>
	void SetTexture(const std::string& textureFilePath);
	void SetModelTexture(const std::string& textureFilePath);
	std::string GetModelTextureFilePath() const;

	const Vector3& GetTranslate() { return transform.translate; };
	void SetTranslate(const Vector3& newTransform) { transform.translate = newTransform; }
	const Vector3& GetRotate() { return transform.rotate; };
	void SetRotate(const Vector3& newTransformRotate) { transform.rotate = newTransformRotate; }
	const Vector3& GetScale() { return transform.scale; };
	void SetScale(const Vector3& newTransformScale) { transform.scale = newTransformScale; }
	void SetCamera(Camera* cmr) { camera = cmr; }
	/// <summary>
	/// DirectionalLight を設定します。
	/// </summary>
	/// <param name="color">色を指定します。</param>
	/// <param name="direction">direction に使用する値を指定します。</param>
	/// <param name="intensity">強度を指定します。</param>
	void SetDirectionalLight(const Vector4& color, const Vector3& direction, float intensity);
	/// <summary>
	/// PointLight を設定します。
	/// </summary>
	/// <param name="color">色を指定します。</param>
	/// <param name="position">位置を指定します。</param>
	/// <param name="intensity">強度を指定します。</param>
	/// <param name="radius">半径を指定します。</param>
	/// <param name="decay">decay に使用する値を指定します。</param>
	void SetPointLight(const Vector4& color, const Vector3& position, float intensity, float radius, float decay);
	/// <summary>
	/// EnvironmentMultiplier を設定します。
	/// </summary>
	/// <param name="multiplier">multiplier に使用する値を指定します。</param>
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
	/// <summary>
	/// EnvironmentMap を設定します。
	/// </summary>
	/// <param name="textureFilePath">使用するテクスチャまたはモデルのファイルパスを指定します。</param>
	void SetEnvironmentMap(const std::string& textureFilePath);

private:
	const float pi = 3.1415f;
	const uint32_t kSubdivision = 16;
	const float kLonEvery = 2.0f * pi / kSubdivision;
	const float kLatEvery = pi / kSubdivision;
	uint32_t latIndex = 16;
	uint32_t lonIndex = 16;
	uint32_t startIndex = (kSubdivision * kSubdivision) * 6;
	Vector2 tex{};
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
	/// <param name="paletteCount">paletteCount に使用する値を指定します。</param>
	void CreateSkinningPaletteResource(uint32_t paletteCount);
	/// <summary>
	/// UpdateSkinningPaletteResource の処理を行います。
	/// </summary>
	void UpdateSkinningPaletteResource();
	/// <summary>
	/// Skeleton を作成し、利用できる状態にします。
	/// </summary>
	/// <param name="rootNode">rootNode に使用する値を指定します。</param>
	/// <returns>処理結果を返します。</returns>
	Skeleton CreateSkeleton(const Node& rootNode);
	/// <summary>
	/// Joint を作成し、利用できる状態にします。
	/// </summary>
	/// <param name="node">node に使用する値を指定します。</param>
	/// <param name="parent">parent に使用する値を指定します。</param>
	/// <param name="joints">joints に使用する値を指定します。</param>
	/// <param name="jointMap">jointMap に使用する値を指定します。</param>
	/// <returns>処理結果を返します。</returns>
	int32_t CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints, std::map<std::string, int32_t>& jointMap);
	/// <summary>
	/// AnimationToSkeleton を現在の状態へ反映します。
	/// </summary>
	void ApplyAnimationToSkeleton();
	/// <summary>
	/// UpdateSkeleton の処理を行います。
	/// </summary>
	void UpdateSkeleton();

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

	Model* model = nullptr;
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
	Skeleton skeleton;
	bool hasSkeleton = false;
	bool isDrawSkeleton_ = false;
	bool isAnimationPlaying_ = true;
	bool useInitialSkinningPose_ = false;
	bool isShadowEnabled_ = true;
	float shadowPlaneY_ = 0.01f;
	float shadowAlpha_ = 0.35f;
};

/// <summary>
/// CalculateValue の処理を行います。
/// </summary>
/// <param name="keyflames">keyflames に使用する値を指定します。</param>
/// <param name="time">time に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
Vector3 CalculateValue(const std::vector<KeyframeVector3>& keyflames, float time);
/// <summary>
/// CalculateValue の処理を行います。
/// </summary>
/// <param name="keyframes">keyframes に使用する値を指定します。</param>
/// <param name="time">time に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
Quaternion CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, float time);
