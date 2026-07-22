#pragma once
#include"Vector.h"

struct Matrix4x4;

struct Quaternion {
	float x, y, z, w;
};


/// <summary>
/// 2 つの値を乗算した結果を返します。
/// </summary>
/// <param name="q1">計算に使用するクォータニオンを指定します。</param>
/// <param name="q2">計算に使用するクォータニオンを指定します。</param>
Quaternion Multiply(const Quaternion& q1, const Quaternion& q2);

Quaternion IdentityQuaternion();

/// <summary>
/// 値を正規化して扱いやすい状態にします。
/// </summary>
/// <param name="q">計算に使用するクォータニオンを指定します。</param>
Quaternion Normalize(const Quaternion& q);

/// <param name="q">計算に使用するクォータニオンを指定します。</param>
Quaternion Conjugate(const Quaternion& q);

/// <param name="q">計算に使用するクォータニオンを指定します。</param>
float Norm(const Quaternion& q);

/// <summary>
/// 逆行列または逆元を計算して返します。
/// </summary>
/// <param name="q">計算に使用するクォータニオンを指定します。</param>
Quaternion Inverse(const Quaternion& q);


/// <summary>
/// RotateAxisAngleQuaternion を生成して返します。
/// </summary>
Quaternion MakeRotateAxisAngleQuaternion(const Vector3 axis, const float angle);

/// <param name="q">計算に使用するクォータニオンを指定します。</param>
Vector3 RotateVector(const Vector3& v, const Quaternion& q);

/// <summary>
/// RotateMatrix を生成して返します。
/// </summary>
/// <param name="q">計算に使用するクォータニオンを指定します。</param>
Matrix4x4 MakeRotateMatrix(const Quaternion& q);

/// <summary>
/// 2 つの値を補間して結果を返します。
/// </summary>
/// <param name="q1">計算に使用するクォータニオンを指定します。</param>
/// <param name="q2">計算に使用するクォータニオンを指定します。</param>
Quaternion Slerp(const Quaternion& q1, const Quaternion& q2, float t);
