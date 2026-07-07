#pragma once
#include "Audio.h"
#include "Camera.h"
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
#include "SpriteComponent.h"
#include "InstancingModel.h"
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
		Sprite,
		LoadedModel
	};

	virtual void Initialize();
	virtual void Update();
	virtual void DrawSkyBox();
	virtual void Draw2D();
	virtual void Draw3D();
	virtual void Finalize();
	virtual ~BaseScene();

	virtual void SetSceneManager(SceneManager* manager) { sceneManager = manager; }
	void UpdateSceneObjects();
	void DrawSceneObjects2D();
	void DrawSceneObjects3D();
	void DrawEditorImGui();
	void SetSceneName(const std::string& sceneName) { sceneName_ = sceneName; }
	void SaveEditorObjects();
	void LoadEditorObjects();

private:
	GameObject* CreateEditorObject(EditorCreateType type, const std::string& modelFilePath = "");
	void DeleteSelectedEditorObject();
	void DrawEditorHierarchy();
	void DrawEditorInspector();
	void DrawEditorGizmo();
	std::string MakeUniqueObjectName(const std::string& baseName) const;
	std::string GetSceneObjectFilePath() const;
	EditorCreateType EditorCreateTypeFromName(const std::string& typeName) const;

	SceneManager* sceneManager = nullptr;
	std::string sceneName_ = "None";
	std::vector<std::unique_ptr<GameObject>> sceneObjects_;
	int selectedObjectIndex_ = -1;
	int nextObjectId_ = 1;
	EditorCreateType createType_ = EditorCreateType::Object3dSphere;
	int selectedLoadedModelIndex_ = 0;
	bool isGizmoEnabled_ = true;
	int gizmoOperationIndex_ = 0;
};
