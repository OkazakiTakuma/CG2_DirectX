#pragma once
struct Vector3 {
	float x, y, z;
};
// 加算
Vector3 Add(const Vector3& v1, const Vector3& v2);
Vector3 operator+(const Vector3& v1, const Vector3& v2);

// 減算
Vector3 Subtract(const Vector3& v1, const Vector3& v2);
Vector3 operator-(const Vector3& v1, const Vector3& v2) ;
// スカラー倍
Vector3 Multiply(float scalar, const Vector3& v);
Vector3 operator*(float scalar, const Vector3& v);
// 内積
float Dot(const Vector3& v1, const Vector3& v2);
// 長さ (ノルム)
float Length(const Vector3& v);
// 正規化
Vector3 Normalize(const Vector3& v);
// 数値の表記
void VectorScreenPrintf(int x, int y, const Vector3& vector, const char* label);
// クロス積
Vector3 Cross(const Vector3& v1, const Vector3& v2);