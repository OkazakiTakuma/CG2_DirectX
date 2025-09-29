#pragma once
#include"Vector3.h"
/// <summary>
/// 4x4の行列
/// </summary>
struct Matrix4x4 {
	float m[4][4];
	
};

/// <summary>
/// SRTの変換を行うための構造体
/// </summary>
struct Transforms {
	Vector3 scale;   // 拡大縮小
	Vector3 rotate;  // 回転
	Vector3 translate; // 平行移動
};

// 行列の加法
Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2);
Matrix4x4 operator+(const Matrix4x4& m1, const Matrix4x4& m2);
// 行列の減法
Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2);
Matrix4x4 operator-(const Matrix4x4& m1, const Matrix4x4& m2);
// 行列の積
Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);
Matrix4x4 operator*(const Matrix4x4& m1, const Matrix4x4& m2);
// 行列とベクトルの積
Vector3 MultiplyVector3(const Matrix4x4& m, const Vector3& v);
Vector3 operator*(const Matrix4x4& m, const Vector3& v);

// 逆行列
Matrix4x4 Inverse(const Matrix4x4& m);
// 転置行列
Matrix4x4 Transpose(const Matrix4x4& m);
// 単位行列の作成
Matrix4x4 MakeIdentity4x4();
// 行列の描画
void MatrixScreenPrintf(int posX, int posY, const Matrix4x4& matrix, const char* label);
// 平行移動行列
Matrix4x4 MakeTranslateMatrix(const Vector3& translate);
// 拡大縮小行列
Matrix4x4 MakeScaleMatrix(const Vector3& scale);
// X軸回転行列
Matrix4x4 MakeRotateXMatrix(float radiun);
// Y軸回転行列
Matrix4x4 MakeRotateYMatrix(float radiun);
// Z軸回転行列
Matrix4x4 MakeRotateZMatrix(float radiun);
// XYZ回転行列
Matrix4x4 MakeRotateXYZMatrix(Vector3 radiun);
// アフィン変換
Matrix4x4 MakeAffineMatrix(Vector3 scale, Vector3 rotate, Vector3 translate);
// 座標変換
Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix);
/// <summary>
/// 正射影行列を作る関数
/// </summary>
/// <param name="left">左端</param>
/// <param name="top">上端</param>
/// <param name="right">右端</param>
/// <param name="bottom">下端</param>
/// <param name="near">手前</param>
/// <param name="far">奥行</param>
/// <returns>正射影行列</returns>
Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom,float near,float far);

/// <summary>
/// 透視投影行列を作る関数
/// </summary>
/// <param name="fovY">縦方向の画角</param>
/// <param name="aspectRatio">アスペクト比</param>
/// <param name="nearClip">手前</param>
/// <param name="farClip">奥手</param>
/// <returns></returns>
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);

/// <summary>
/// ビューポート行列を作る関数
/// </summary>
/// <param name="left">左端</param>
/// <param name="top">上端</param>
/// <param name="width">横幅</param>
/// <param name="height">縦幅</param>
/// <param name="minDepth">最小深度値</param>
/// <param name="maxDepth">最大深度値</param>
/// <returns></returns>
Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDeapth, float maxDepth);
