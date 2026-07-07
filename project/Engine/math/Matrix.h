#pragma once
#include"Vector.h"
#include"Quaternion.h"

/// <summary>
/// </summary>
struct Matrix4x4 {
	float m[4][4];
	
};

/// <summary>
/// </summary>
struct EulerTransform {
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;
};

struct QuaternionTransform {
	Vector3 scale;
	Quaternion rotate;
	Vector3 translate;
};

Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2);
Matrix4x4 operator+(const Matrix4x4& m1, const Matrix4x4& m2);
Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2);
Matrix4x4 operator-(const Matrix4x4& m1, const Matrix4x4& m2);
Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);
Matrix4x4 operator*(const Matrix4x4& m1, const Matrix4x4& m2);
Vector3 MultiplyVector3(const Matrix4x4& m, const Vector3& v);
Vector3 operator*(const Matrix4x4& m, const Vector3& v);

// 騾・｡悟・
Matrix4x4 Inverse(const Matrix4x4& m);
Matrix4x4 Transpose(const Matrix4x4& m);
Matrix4x4 MakeIdentity4x4();
void MatrixScreenPrintf(int posX, int posY, const Matrix4x4& matrix, const char* label);
Matrix4x4 MakeTranslateMatrix(const Vector3& translate);
Matrix4x4 MakeScaleMatrix(const Vector3& scale);
Matrix4x4 MakeRotateXMatrix(float radiun);
Matrix4x4 MakeRotateYMatrix(float radiun);
Matrix4x4 MakeRotateZMatrix(float radiun);
Matrix4x4 MakeRotateXYZMatrix(Vector3 radiun);
Matrix4x4 MakeAffineMatrix(Vector3 scale, Vector3 rotate, Vector3 translate);
Vector3 Transformation(const Vector3& vector, const Matrix4x4& matrix);
/// <summary>
/// </summary>
/// <param name="near">謇句燕</param>
Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom,float near,float far);

/// <summary>
/// </summary>
/// <param name="nearClip">謇句燕</param>
/// <returns></returns>
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);

/// <summary>
/// </summary>
/// <returns></returns>
Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDeapth, float maxDepth);
