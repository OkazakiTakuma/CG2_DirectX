#pragma once
#include "Audio.h"
#include "Camera.h"
#include "CameraComponent.h"
#include "EnemyComponent.h"
#include "EnemySpawnPointComponent.h"
#include "ExperienceComponent.h"
#include "GameObject.h"
#include "ImGuiManager.h"
#include "Input.h"
#include "Matrix.h"
#include "Object3dCommon.h"
#include "ParticleEmitter.h"
#include "ParticleManager.h"
#include "Resource.h"
#include "SkyBox.h"
#include "SkyBoxCommon.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "SrvManager.h"
#include "Vector.h"
#include "struct.h"
#include <Object3d.h>
#include "Object3dComponent.h"
#include "OBBColliderComponent.h"
#include "ParticleEmitterComponent.h"
#include "PlayerAttackComponent.h"
#include "PlayerProjectileComponent.h"
#include "../../Player/Player.h"
#include "SphereColliderComponent.h"
#include "SpriteComponent.h"
#include "TextComponent.h"
#include "InstancingModel.h"
#include <array>
#include <memory>
#include <string>
#include <vector>
class SceneManager;

class BaseScene {
public:
	enum class EditorCreateType {
		Empty,
		Object3dSphere,
		Object3dCylinder,
		Object3dCylinderOpen,
		Sprite,
		Text,
		LoadedModel,
		AnimatedModel,
		Camera,
		PointLight,
		ParticleEmitter,
		Player,
		EnemySpawnPoint,
		Enemy
	};

/// <summary>
/// シーンで使用するリソースやオブジェクトを初期化します。
/// </summary>
	virtual void Initialize();
/// <summary>
/// シーンの毎フレーム更新を行います。
/// </summary>
	virtual void Update();
/// <summary>
/// スカイボックスを描画します。
/// </summary>
	virtual void DrawSkyBox();
/// <summary>
/// 2D要素を描画します。
/// </summary>
	virtual void Draw2D();
/// <summary>
/// 3D要素を描画します。
/// </summary>
	virtual void Draw3D();
/// <summary>
/// シーンが保持しているリソースを解放します。
/// </summary>
	virtual void Finalize();
/// <summary>
/// ~BaseScene の処理を行います。
/// </summary>
	virtual ~BaseScene();

	virtual void SetSceneManager(SceneManager* manager) { sceneManager = manager; }
/// <summary>
/// シーン内オブジェクトの更新と当たり判定を行います。
/// </summary>
	void UpdateSceneObjects();
/// <summary>
/// エディタ用のカメラ操作とオブジェクト選択を更新します。
/// </summary>
	void UpdateEditorTools();
/// <summary>
/// シーン内オブジェクトの2D描画を行います。
/// </summary>
	void DrawSceneObjects2D();
/// <summary>
/// シーン内オブジェクトの3D描画を行います。
/// </summary>
	void DrawSceneObjects3D();
/// <summary>
/// エディタ用ImGuiウィンドウを描画します。
/// </summary>
	void DrawEditorImGui();
	void SetSceneName(const std::string& sceneName) { sceneName_ = sceneName; }
	void SetFallbackCamera(Camera* camera) { fallbackCamera_ = camera; }
/// <summary>
/// エディタで配置したオブジェクト情報をJSONへ保存します。
/// </summary>
	void SaveEditorObjects();
/// <summary>
/// JSONからエディタ配置オブジェクトを読み込みます。
/// </summary>
	void LoadEditorObjects();

private:
/// <summary>
/// 指定された種類のエディタオブジェクトを生成します。
/// </summary>
	GameObject* CreateEditorObject(EditorCreateType type, const std::string& modelFilePath = "");
/// <summary>
/// 現在選択中のエディタオブジェクトを削除します。
/// </summary>
	void DeleteSelectedEditorObject();
/// <summary>
/// シーン内オブジェクトの階層ウィンドウを描画します。
/// </summary>
	void DrawEditorHierarchy();
	void DrawEditorProjectAssets();
	void HandleGameViewAssetDrop();
	void DrawEditorCameraSelector();
/// <summary>
/// 選択中オブジェクトのインスペクタを描画します。
/// </summary>
	void DrawEditorInspector();
	void DrawPlayerInspector();
	void DrawPlayerAttackInspector();
	void ReloadPlayerAttackInspectorCache();
	PlayerAttackStats* FindCachedPlayerAttackStats(const std::string& attackName);
/// <summary>
/// 選択中オブジェクトを操作するギズモを描画します。
/// </summary>
	void DrawEditorGizmo();
/// <summary>
/// カメラコンポーネントの編集UIを描画します。
/// </summary>
	void DrawCameraInspector(GameObject* selectedObject);
/// <summary>
/// コライダーコンポーネントの編集UIを描画します。
/// </summary>
	void DrawOBBColliderInspector(GameObject* selectedObject);
/// <summary>
/// パーティクルエミッターコンポーネントの編集UIを描画します。
/// </summary>
	void DrawParticleEmitterInspector(GameObject* selectedObject);
/// <summary>
/// マウスクリックによるエディタオブジェクト選択を更新します。
/// </summary>
	void UpdateEditorObjectPicking();
/// <summary>
/// エディタ用カメラのマウス操作を更新します。
/// </summary>
	void UpdateEditorCameraControl();
/// <summary>
/// 有効なコライダー同士の当たり判定と押し戻しを行います。
/// </summary>
	void UpdateColliderCollisions();
/// <summary>
/// 指定したカメラを描画用の既定カメラへ反映します。
/// </summary>
	void ApplyCamera(Camera* camera);
/// <summary>
/// 現在選択されているアクティブカメラを反映します。
/// </summary>
	void ApplyActiveCamera();
/// <summary>
/// カメラの追従対象リンクを名前から解決します。
/// </summary>
	void ResolveCameraLinks();
	void ResolveEnemySpawnPointLinks();
	void ResolveEnemyLinks();
	void UpdateEnemySpawning();
	void UpdatePlayerAttacks();
	void UpdatePlayerProjectileHits();
	void CleanupExpiredPlayerProjectiles();
/// <summary>
/// 指定したオブジェクトのカメラをアクティブカメラに設定します。
/// </summary>
	void SetActiveCameraObject(GameObject* object);
/// <summary>
/// 最初に見つかった有効なカメラオブジェクトを返します。
/// </summary>
	GameObject* FindFirstCameraObject();
/// <summary>
/// 名前に一致するシーンオブジェクトを検索します。
/// </summary>
	GameObject* FindObjectByName(const std::string& name) const;
/// <summary>
/// 既存名と重複しないオブジェクト名を生成します。
/// </summary>
	std::string MakeUniqueObjectName(const std::string& baseName) const;
	GameObject* CreateRuntimeEnemy(const std::string& enemyTypeName, const Vector3& position, GameObject* target);
	GameObject* CreateRuntimeExperience(const EnemyStats& enemyStats, const Vector3& position, GameObject* target);
	GameObject* CreateRuntimePlayerProjectile(const PlayerAttackShotRequest& request);
	GameObject* FindNearestEnemy(const Vector3& position) const;
/// <summary>
/// 現在のシーンに対応する配置JSONファイルパスを返します。
/// </summary>
	std::string GetSceneObjectFilePath() const;
/// <summary>
/// 文字列からエディタ生成タイプへ変換します。
/// </summary>
	EditorCreateType EditorCreateTypeFromName(const std::string& typeName) const;
	void CreateOrReloadEditorSkyBox(const std::string& textureFilePath);
	void DrawEditorSkyBoxControls();

	SceneManager* sceneManager = nullptr;
	std::string sceneName_ = "None";
	std::vector<std::unique_ptr<GameObject>> sceneObjects_;
	int selectedObjectIndex_ = -1;
	int nextObjectId_ = 1;
	EditorCreateType createType_ = EditorCreateType::Object3dSphere;
	int selectedLoadedModelIndex_ = 0;
	int selectedAnimatedModelIndex_ = 0;
	int selectedTextureIndex_ = 0;
	int selectedParticlePresetIndex_ = 0;
	int selectedEnemyTypeIndex_ = 0;
	int selectedPlayerModelIndex_ = 0;
	int selectedPlayerTypeIndex_ = 0;
	int selectedPlayerAttackTypeIndex_ = 0;
	int selectedPlayerAttackLevelIndex_ = 0;
	int selectedSkyBoxTextureIndex_ = 0;
	int selectedInspectorComponentIndex_ = 0;
	std::array<char, 64> particlePresetNameBuffer_ = {};
	std::array<char, 64> enemyTypeNameBuffer_ = {};
	std::array<char, 64> playerTypeNameBuffer_ = {};
	std::array<char, 64> playerAttackNameBuffer_ = {};
	std::array<char, 512> textEditBuffer_ = {};
	std::vector<std::string> cachedPlayerAttackNames_;
	std::vector<PlayerAttackStats> cachedPlayerAttackStats_;
	bool isPlayerAttackCacheLoaded_ = false;
	bool isGizmoEnabled_ = true;
	bool isEditorSkyBoxEnabled_ = false;
	int gizmoOperationIndex_ = 0;
	std::string activeCameraObjectName_;
	std::string skyBoxTextureFilePath_;
	std::unique_ptr<SkyBox> editorSkyBox_;
	Camera* fallbackCamera_ = nullptr;
};
