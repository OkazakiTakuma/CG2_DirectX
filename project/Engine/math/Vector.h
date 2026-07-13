#pragma once
struct Vector3 {
	float x, y, z;
};
/// <summary>
/// 2 つの値を加算した結果を返します。
/// </summary>
/// <param name="v1">計算に使用する値を指定します。</param>
/// <param name="v2">計算に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
Vector3 Add(const Vector3& v1, const Vector3& v2);
/// <summary>
/// operator+ の処理を行います。
/// </summary>
/// <param name="v1">計算に使用する値を指定します。</param>
/// <param name="v2">計算に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
Vector3 operator+(const Vector3& v1, const Vector3& v2);

/// <summary>
/// 2 つの値を減算した結果を返します。
/// </summary>
/// <param name="v1">計算に使用する値を指定します。</param>
/// <param name="v2">計算に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
Vector3 Subtract(const Vector3& v1, const Vector3& v2);
/// <summary>
/// operator- の処理を行います。
/// </summary>
/// <param name="v1">計算に使用する値を指定します。</param>
/// <param name="v2">計算に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
Vector3 operator-(const Vector3& v1, const Vector3& v2) ;
/// <summary>
/// 2 つの値を乗算した結果を返します。
/// </summary>
/// <param name="scalar">scalar に使用する値を指定します。</param>
/// <param name="v">計算に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
Vector3 Multiply(float scalar, const Vector3& v);
/// <summary>
/// operator* の処理を行います。
/// </summary>
/// <param name="scalar">scalar に使用する値を指定します。</param>
/// <param name="v">計算に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
Vector3 operator*(float scalar, const Vector3& v);
/// <summary>
/// Dot の処理を行います。
/// </summary>
/// <param name="v1">計算に使用する値を指定します。</param>
/// <param name="v2">計算に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
float Dot(const Vector3& v1, const Vector3& v2);
/// <summary>
/// Length の処理を行います。
/// </summary>
/// <param name="v">計算に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
float Length(const Vector3& v);
/// <summary>
/// 値を正規化して扱いやすい状態にします。
/// </summary>
/// <param name="v">計算に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
Vector3 Normalize(const Vector3& v);
/// <summary>
/// 値を正規化して扱いやすい状態にします。
/// </summary>
/// <param name="v">計算に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
Vector3 NormalizeReturnVector(const Vector3& v);

/// <summary>
/// Leap の処理を行います。
/// </summary>
/// <param name="v1">計算に使用する値を指定します。</param>
/// <param name="v2">計算に使用する値を指定します。</param>
/// <param name="t">t に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
Vector3 Leap(const Vector3& v1, const Vector3& v2, const float t);

/// <summary>
/// VectorScreenPrintf の処理を行います。
/// </summary>
/// <param name="x">x に使用する値を指定します。</param>
/// <param name="y">y に使用する値を指定します。</param>
/// <param name="vector">計算に使用する値を指定します。</param>
/// <param name="label">label に使用する値を指定します。</param>
void VectorScreenPrintf(int x, int y, const Vector3& vector, const char* label);
/// <summary>
/// Cross の処理を行います。
/// </summary>
/// <param name="v1">計算に使用する値を指定します。</param>
/// <param name="v2">計算に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
Vector3 Cross(const Vector3& v1, const Vector3& v2);
struct Vector4 {
	float x, y, z, w;
	Vector4(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 1.0f) : x(x), y(y), z(z), w(w) {}
	Vector4 operator+(const Vector4& other) const { return Vector4(x + other.x, y + other.y, z + other.z, w + other.w); }
	Vector4 operator-(const Vector4& other) const { return Vector4(x - other.x, y - other.y, z - other.z, w - other.w); }
};

/// <summary>
/// </summary>
struct Vector2 {
	float x;
	float y;
};

