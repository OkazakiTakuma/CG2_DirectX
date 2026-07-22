#include "Camera.h"
#include"WinApp.h"
Camera::Camera() {
	transform = {
	    {1.0f, 1.0f, 1.0f  },
	    {0.0f, 0.0f, 0.0f  },
	    {0.0f, 4.0f, -10.0f}
	};
	fovY = 0.45f;
	aspectRatio = {float(WinApp::kClientWidth) / float(WinApp::kClientHeight)};
	nearClip = 0.1f;
	farClip = 100.0f;
	worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	viewMatrix = Inverse(worldMatrix);
	projectionMatrix = MakePerspectiveFovMatrix(fovY, aspectRatio, nearClip, farClip);
	viewProjectionMatrix = Multiply(viewMatrix, projectionMatrix);
}
/// <summary>
/// 毎フレームの状態更新を行います。
/// </summary>
void Camera::Update() {
	worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	viewMatrix = Inverse(worldMatrix);
	projectionMatrix = MakePerspectiveFovMatrix(fovY, aspectRatio, nearClip, farClip);
	viewProjectionMatrix = Multiply(viewMatrix, projectionMatrix);
}
