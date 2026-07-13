#include "BaseScene.h"
#include "BaseSceneHelpers.h"

void BaseScene::ApplyCamera(Camera* camera) {
	if (!camera) {
		return;
	}

	const float clientWidth = static_cast<float>(Input::GetInstance()->GetClientWidth());
	const float clientHeight = static_cast<float>(Input::GetInstance()->GetClientHeight());
	if (clientWidth > 0.0f && clientHeight > 0.0f) {
		camera->SetAspectRatio(clientWidth / clientHeight);
	}
	camera->Update();
	Object3dCommon::GetInstance()->SetDefaultCamera(camera);
	SkyBoxCommon::GetInstance()->SetDefaultCamera(camera);
	ParticleManager::GetInstance()->SetCamera(camera);
	for (const auto& object : sceneObjects_) {
		if (Object3dComponent* object3dComponent = object->GetComponent<Object3dComponent>()) {
			object3dComponent->SetCamera(camera);
			if (object3dComponent->GetObject3d()) {
				object3dComponent->GetObject3d()->Update();
			}
		}
	}
}

/// <summary>
/// 現在選択されているアクティブカメラを反映します。
/// </summary>
void BaseScene::ApplyActiveCamera() {
	if (activeCameraObjectName_.empty()) {
		ApplyCamera(fallbackCamera_);
		return;
	}

	GameObject* cameraObject = FindObjectByName(activeCameraObjectName_);
	if (!cameraObject) {
		activeCameraObjectName_.clear();
		ApplyCamera(fallbackCamera_);
		return;
	}

	CameraComponent* cameraComponent = cameraObject->GetComponent<CameraComponent>();
	if (!cameraComponent || !cameraComponent->IsEnabled() || !cameraComponent->GetCamera()) {
		activeCameraObjectName_.clear();
		ApplyCamera(fallbackCamera_);
		return;
	}

	cameraComponent->Update();
	ApplyCamera(cameraComponent->GetCamera());
}

/// <summary>
/// カメラの追従対象リンクを名前から解決します。
/// </summary>
void BaseScene::ResolveCameraLinks() {
	for (const auto& object : sceneObjects_) {
		CameraComponent* cameraComponent = object->GetComponent<CameraComponent>();
		if (!cameraComponent || !cameraComponent->IsEnabled() || cameraComponent->GetFollowTargetName().empty()) {
			continue;
		}

		GameObject* target = FindObjectByName(cameraComponent->GetFollowTargetName());
		if (target && target != object.get() && target != cameraComponent->GetFollowTarget()) {
			cameraComponent->SetFollowTarget(target);
		}
	}
}

/// <summary>
/// 有効なコライダー同士の当たり判定と押し戻しを行います。
/// </summary>
void BaseScene::UpdateColliderCollisions() {
	std::vector<OBBColliderComponent*> colliders;
	colliders.reserve(sceneObjects_.size());
	std::vector<SphereColliderComponent*> sphereColliders;
	sphereColliders.reserve(sceneObjects_.size());

	for (const auto& object : sceneObjects_) {
		OBBColliderComponent* collider = object->GetComponent<OBBColliderComponent>();
		if (collider && collider->IsEnabled()) {
			collider->SetColliding(false);
			colliders.push_back(collider);
		} else if (collider) {
			collider->SetColliding(false);
		}
		SphereColliderComponent* sphereCollider = object->GetComponent<SphereColliderComponent>();
		if (sphereCollider && sphereCollider->IsEnabled()) {
			sphereCollider->SetColliding(false);
			sphereColliders.push_back(sphereCollider);
		} else if (sphereCollider) {
			sphereCollider->SetColliding(false);
		}
	}

	for (size_t i = 0; i < colliders.size(); ++i) {
		for (size_t j = i + 1; j < colliders.size(); ++j) {
			const OBBColliderShape colliderA = colliders[i]->GetWorldOBB();
			const OBBColliderShape colliderB = colliders[j]->GetWorldOBB();
			if (IsCollisionOBBToOBB(colliderA, colliderB)) {
				colliders[i]->SetColliding(true);
				colliders[j]->SetColliding(true);
				Vector3 direction{};
				float penetration = 0.0f;
				if (CalculateOBBOBBPushBack(colliderA, colliderB, direction, penetration)) {
					ApplyColliderPushBack(
					    colliders[i]->GetOwner(),
					    colliders[i]->GetPushBackEnabled(),
					    colliders[j]->GetOwner(),
					    colliders[j]->GetPushBackEnabled(),
					    direction,
					    penetration
					);
				}
			}
		}
	}

	for (size_t i = 0; i < sphereColliders.size(); ++i) {
		for (size_t j = i + 1; j < sphereColliders.size(); ++j) {
			const SphereColliderShape sphereA = sphereColliders[i]->GetWorldSphere();
			const SphereColliderShape sphereB = sphereColliders[j]->GetWorldSphere();
			if (IsCollisionSphereToSphere(sphereA, sphereB)) {
				sphereColliders[i]->SetColliding(true);
				sphereColliders[j]->SetColliding(true);
				Vector3 direction{};
				float penetration = 0.0f;
				if (CalculateSphereSpherePushBack(sphereA, sphereB, direction, penetration)) {
					ApplyColliderPushBack(
					    sphereColliders[i]->GetOwner(),
					    sphereColliders[i]->GetPushBackEnabled(),
					    sphereColliders[j]->GetOwner(),
					    sphereColliders[j]->GetPushBackEnabled(),
					    direction,
					    penetration
					);
				}
			}
		}
	}

	for (OBBColliderComponent* collider : colliders) {
		for (SphereColliderComponent* sphereCollider : sphereColliders) {
			if (collider->GetOwner() == sphereCollider->GetOwner()) {
				continue;
			}

			const OBBColliderShape obb = collider->GetWorldOBB();
			const SphereColliderShape sphere = sphereCollider->GetWorldSphere();
			if (IsCollisionOBBToSphere(obb, sphere)) {
				collider->SetColliding(true);
				sphereCollider->SetColliding(true);
				Vector3 direction{};
				float penetration = 0.0f;
				if (CalculateOBBSpherePushBack(obb, sphere, direction, penetration)) {
					ApplyColliderPushBack(
					    collider->GetOwner(),
					    collider->GetPushBackEnabled(),
					    sphereCollider->GetOwner(),
					    sphereCollider->GetPushBackEnabled(),
					    direction,
					    penetration
					);
				}
			}
		}
	}
}

/// <summary>
/// マウスクリックによるエディタオブジェクト選択を更新します。
/// </summary>
void BaseScene::UpdateEditorObjectPicking() {
#ifdef USE_IMGUI
	constexpr int kLeftMouseButton = 0;
	Input* input = Input::GetInstance();
	if (!input->TriggerMouseButton(kLeftMouseButton)) {
		return;
	}
	if (ImGui::GetIO().WantCaptureMouse || ImGuizmo::IsOver() || ImGuizmo::IsUsing()) {
		return;
	}

	Camera* camera = Object3dCommon::GetInstance()->GetDefaultCamera();
	if (!camera) {
		return;
	}

	const float width = static_cast<float>(input->GetClientWidth());
	const float height = static_cast<float>(input->GetClientHeight());
	const float mouseX = static_cast<float>(input->GetMouseClientX());
	const float mouseY = static_cast<float>(input->GetMouseClientY());
	if (mouseX < 0.0f || mouseX > width || mouseY < 0.0f || mouseY > height) {
		return;
	}

	const float ndcX = (mouseX / width) * 2.0f - 1.0f;
	const float ndcY = 1.0f - (mouseY / height) * 2.0f;
	const Matrix4x4 inverseViewProjection = Inverse(camera->GetViewProjectionMatrix());
	const Vector3 nearPoint = TransformCoord({ndcX, ndcY, 0.0f}, inverseViewProjection);
	const Vector3 farPoint = TransformCoord({ndcX, ndcY, 1.0f}, inverseViewProjection);
	const Vector3 rayDirection = Normalize(farPoint - nearPoint);

	int hitIndex = -1;
	float nearestDistance = 100000.0f;
	for (int index = 0; index < static_cast<int>(sceneObjects_.size()); ++index) {
		float distance = 0.0f;
		if (IntersectRayToOBB(nearPoint, rayDirection, MakePickOBB(sceneObjects_[index].get()), distance) && distance < nearestDistance) {
			nearestDistance = distance;
			hitIndex = index;
		}
	}

	if (hitIndex >= 0) {
		selectedObjectIndex_ = hitIndex;
	}
#endif
}

/// <summary>
/// エディタ用カメラのマウス操作を更新します。
/// </summary>
void BaseScene::UpdateEditorCameraControl() {
#ifdef USE_IMGUI
	constexpr int kLeftMouseButton = 0;
	constexpr int kMiddleMouseButton = 2;
	constexpr float kRotateSpeed = 0.004f;
	constexpr float kPanSpeed = 0.0015f;

	Input* input = Input::GetInstance();
	const bool isLeftDragging = input->PushMouseButton(kLeftMouseButton);
	const bool isMiddleDragging = input->PushMouseButton(kMiddleMouseButton);
	if (ImGui::GetIO().WantCaptureMouse || (!isLeftDragging && !isMiddleDragging)) {
		return;
	}
	if (isLeftDragging && (ImGuizmo::IsOver() || ImGuizmo::IsUsing())) {
		return;
	}

	const float moveX = static_cast<float>(input->GetMouseMoveX());
	const float moveY = static_cast<float>(input->GetMouseMoveY());
	if (moveX == 0.0f && moveY == 0.0f) {
		return;
	}

	GameObject* selectedObject =
	    selectedObjectIndex_ >= 0 && selectedObjectIndex_ < static_cast<int>(sceneObjects_.size())
	        ? sceneObjects_[selectedObjectIndex_].get()
	        : nullptr;

	GameObject* activeCameraObject = FindObjectByName(activeCameraObjectName_);
	EulerTransform* cameraTransform = nullptr;
	Camera* fallbackCamera = nullptr;
	Vector3 cameraPosition{};
	Vector3 cameraRotate{};

	CameraComponent* activeCameraComponent =
	    activeCameraObject ? activeCameraObject->GetComponent<CameraComponent>() : nullptr;
	if (activeCameraComponent && activeCameraComponent->IsEnabled()) {
		cameraTransform = &activeCameraObject->GetTransform();
		cameraPosition = cameraTransform->translate;
		cameraRotate = cameraTransform->rotate;
	} else if (TryGetCameraTransform(fallbackCamera_, cameraPosition, cameraRotate)) {
		fallbackCamera = fallbackCamera_;
	} else {
		return;
	}

	if (isMiddleDragging) {
		const float pitchDelta = moveY * kRotateSpeed;
		const float yawDelta = moveX * kRotateSpeed;
		if (selectedObject) {
			const Vector3 target = selectedObject->GetTransform().translate;
			Vector3 offset = cameraPosition - target;
			if (Length(offset) > 0.0001f) {
				const Vector3 worldUp{0.0f, 1.0f, 0.0f};
				offset = RotateAroundAxis(offset, worldUp, yawDelta);

				Vector3 right = Cross(worldUp, Normalize(offset));
				if (Length(right) > 0.0001f) {
					offset = RotateAroundAxis(offset, right, pitchDelta);
				}
				cameraPosition = target + offset;
			}
		}

		cameraRotate.x += pitchDelta;
		cameraRotate.y += yawDelta;
	} else if (isLeftDragging) {
		const Matrix4x4 rotateMatrix = MakeRotateXYZMatrix(cameraRotate);
		const Vector3 rightAxis{rotateMatrix.m[0][0], rotateMatrix.m[0][1], rotateMatrix.m[0][2]};
		const Vector3 upAxis{rotateMatrix.m[1][0], rotateMatrix.m[1][1], rotateMatrix.m[1][2]};
		const Vector3 right = Normalize(rightAxis);
		const Vector3 up = Normalize(upAxis);
		float distanceScale = 10.0f;
		if (selectedObject) {
			distanceScale = Length(cameraPosition - selectedObject->GetTransform().translate);
			if (distanceScale < 1.0f) {
				distanceScale = 1.0f;
			}
		}
		cameraPosition = cameraPosition + (moveX * distanceScale * kPanSpeed) * right - (moveY * distanceScale * kPanSpeed) * up;
	}

	if (cameraTransform) {
		cameraTransform->translate = cameraPosition;
		cameraTransform->rotate = cameraRotate;
	} else if (fallbackCamera) {
		fallbackCamera->SetTranslate(cameraPosition);
		fallbackCamera->SetRotate(cameraRotate);
	}
#endif
}

/// <summary>
/// 指定したオブジェクトのカメラをアクティブカメラに設定します。
/// </summary>
void BaseScene::SetActiveCameraObject(GameObject* object) {
	CameraComponent* cameraComponent = object ? object->GetComponent<CameraComponent>() : nullptr;
	if (!cameraComponent || !cameraComponent->IsEnabled()) {
		return;
	}

	activeCameraObjectName_ = object->GetName();
	ApplyActiveCamera();
}

/// <summary>
/// 最初に見つかった有効なカメラオブジェクトを返します。
/// </summary>
GameObject* BaseScene::FindFirstCameraObject() {
	for (const auto& object : sceneObjects_) {
		CameraComponent* cameraComponent = object->GetComponent<CameraComponent>();
		if (cameraComponent && cameraComponent->IsEnabled()) {
			return object.get();
		}
	}
	return nullptr;
}

/// <summary>
/// 名前に一致するシーンオブジェクトを検索します。
/// </summary>
GameObject* BaseScene::FindObjectByName(const std::string& name) const {
	if (name.empty()) {
		return nullptr;
	}

	for (const auto& object : sceneObjects_) {
		if (object->GetName() == name) {
			return object.get();
		}
	}
	return nullptr;
}

