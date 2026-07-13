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
/// <returns>処理結果を返します。</returns>
Quaternion Multiply(const Quaternion& q1, const Quaternion& q2);

/// <summary>
/// IdentityQuaternion の処理を行います。
/// </summary>
/// <returns>処理結果を返します。</returns>
Quaternion IdentityQuaternion();

/// <summary>
/// 値を正規化して扱いやすい状態にします。
/// </summary>
/// <param name="q">計算に使用するクォータニオンを指定します。</param>
/// <returns>処理結果を返します。</returns>
Quaternion Normalize(const Quaternion& q);

/// <summary>
/// Conjugate の処理を行います。
/// </summary>
/// <param name="q">計算に使用するクォータニオンを指定します。</param>
/// <returns>処理結果を返します。</returns>
Quaternion Conjugate(const Quaternion& q);

/// <summary>
/// Norm の処理を行います。
/// </summary>
/// <param name="q">計算に使用するクォータニオンを指定します。</param>
/// <returns>処理結果を返します。</returns>
float Norm(const Quaternion& q);

/// <summary>
/// 逆行列または逆元を計算して返します。
/// </summary>
/// <param name="q">計算に使用するクォータニオンを指定します。</param>
/// <returns>処理結果を返します。</returns>
Quaternion Inverse(const Quaternion& q);


/// <summary>
/// RotateAxisAngleQuaternion を生成して返します。
/// </summary>
/// <param name="axis">axis に使用する値を指定します。</param>
/// <param name="angle">angle に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
Quaternion MakeRotateAxisAngleQuaternion(const Vector3 axis, const float angle);

/// <summary>
/// RotateVector の処理を行います。
/// </summary>
/// <param name="v">計算に使用する値を指定します。</param>
/// <param name="q">計算に使用するクォータニオンを指定します。</param>
/// <returns>処理結果を返します。</returns>
Vector3 RotateVector(const Vector3& v, const Quaternion& q);

/// <summary>
/// RotateMatrix を生成して返します。
/// </summary>
/// <param name="q">計算に使用するクォータニオンを指定します。</param>
/// <returns>処理結果を返します。</returns>
Matrix4x4 MakeRotateMatrix(const Quaternion& q);

/// <summary>
/// 2 つの値を補間して結果を返します。
/// </summary>
/// <param name="q1">計算に使用するクォータニオンを指定します。</param>
/// <param name="q2">計算に使用するクォータニオンを指定します。</param>
/// <param name="t">t に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
Quaternion Slerp(const Quaternion& q1, const Quaternion& q2, float t);
