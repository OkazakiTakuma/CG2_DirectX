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

/// <summary>
/// 2 つの値を加算した結果を返します。
/// </summary>
/// <param name="m1">計算に使用する行列を指定します。</param>
/// <param name="m2">計算に使用する行列を指定します。</param>
/// <returns>処理結果を返します。</returns>
Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2);
/// <summary>
/// operator+ の処理を行います。
/// </summary>
/// <param name="m1">計算に使用する行列を指定します。</param>
/// <param name="m2">計算に使用する行列を指定します。</param>
/// <returns>処理結果を返します。</returns>
Matrix4x4 operator+(const Matrix4x4& m1, const Matrix4x4& m2);
/// <summary>
/// 2 つの値を減算した結果を返します。
/// </summary>
/// <param name="m1">計算に使用する行列を指定します。</param>
/// <param name="m2">計算に使用する行列を指定します。</param>
/// <returns>処理結果を返します。</returns>
Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2);
/// <summary>
/// operator- の処理を行います。
/// </summary>
/// <param name="m1">計算に使用する行列を指定します。</param>
/// <param name="m2">計算に使用する行列を指定します。</param>
/// <returns>処理結果を返します。</returns>
Matrix4x4 operator-(const Matrix4x4& m1, const Matrix4x4& m2);
/// <summary>
/// 2 つの値を乗算した結果を返します。
/// </summary>
/// <param name="m1">計算に使用する行列を指定します。</param>
/// <param name="m2">計算に使用する行列を指定します。</param>
/// <returns>処理結果を返します。</returns>
Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);
/// <summary>
/// operator* の処理を行います。
/// </summary>
/// <param name="m1">計算に使用する行列を指定します。</param>
/// <param name="m2">計算に使用する行列を指定します。</param>
/// <returns>処理結果を返します。</returns>
Matrix4x4 operator*(const Matrix4x4& m1, const Matrix4x4& m2);
/// <summary>
/// MultiplyVector3 の処理を行います。
/// </summary>
/// <param name="m">計算に使用する行列を指定します。</param>
/// <param name="v">計算に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
Vector3 MultiplyVector3(const Matrix4x4& m, const Vector3& v);
/// <summary>
/// operator* の処理を行います。
/// </summary>
/// <param name="m">計算に使用する行列を指定します。</param>
/// <param name="v">計算に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
Vector3 operator*(const Matrix4x4& m, const Vector3& v);

// 騾・｡悟・
/// <summary>
/// 逆行列または逆元を計算して返します。
/// </summary>
/// <param name="m">計算に使用する行列を指定します。</param>
/// <returns>処理結果を返します。</returns>
Matrix4x4 Inverse(const Matrix4x4& m);
/// <summary>
/// 転置行列を計算して返します。
/// </summary>
/// <param name="m">計算に使用する行列を指定します。</param>
/// <returns>処理結果を返します。</returns>
Matrix4x4 Transpose(const Matrix4x4& m);
/// <summary>
/// Identity4x4 を生成して返します。
/// </summary>
/// <returns>処理結果を返します。</returns>
Matrix4x4 MakeIdentity4x4();
/// <summary>
/// MatrixScreenPrintf の処理を行います。
/// </summary>
/// <param name="posX">posX に使用する値を指定します。</param>
/// <param name="posY">posY に使用する値を指定します。</param>
/// <param name="matrix">計算に使用する行列を指定します。</param>
/// <param name="label">label に使用する値を指定します。</param>
void MatrixScreenPrintf(int posX, int posY, const Matrix4x4& matrix, const char* label);
/// <summary>
/// TranslateMatrix を生成して返します。
/// </summary>
/// <param name="translate">位置を指定します。</param>
/// <returns>処理結果を返します。</returns>
Matrix4x4 MakeTranslateMatrix(const Vector3& translate);
/// <summary>
/// ScaleMatrix を生成して返します。
/// </summary>
/// <param name="scale">拡大率を指定します。</param>
/// <returns>処理結果を返します。</returns>
Matrix4x4 MakeScaleMatrix(const Vector3& scale);
/// <summary>
/// RotateXMatrix を生成して返します。
/// </summary>
/// <param name="radiun">radiun に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
Matrix4x4 MakeRotateXMatrix(float radiun);
/// <summary>
/// RotateYMatrix を生成して返します。
/// </summary>
/// <param name="radiun">radiun に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
Matrix4x4 MakeRotateYMatrix(float radiun);
/// <summary>
/// RotateZMatrix を生成して返します。
/// </summary>
/// <param name="radiun">radiun に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
Matrix4x4 MakeRotateZMatrix(float radiun);
/// <summary>
/// RotateXYZMatrix を生成して返します。
/// </summary>
/// <param name="radiun">radiun に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
Matrix4x4 MakeRotateXYZMatrix(Vector3 radiun);
/// <summary>
/// AffineMatrix を生成して返します。
/// </summary>
/// <param name="scale">拡大率を指定します。</param>
/// <param name="rotate">回転量を指定します。</param>
/// <param name="translate">位置を指定します。</param>
/// <returns>処理結果を返します。</returns>
Matrix4x4 MakeAffineMatrix(Vector3 scale, Vector3 rotate, Vector3 translate);
/// <summary>
/// Transformation の処理を行います。
/// </summary>
/// <param name="vector">計算に使用する値を指定します。</param>
/// <param name="matrix">計算に使用する行列を指定します。</param>
/// <returns>処理結果を返します。</returns>
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
