#include "Quaternion.h"
#include "Matrix.h"
#include <cmath>
Quaternion Multiply(const Quaternion& q1, const Quaternion& q2) {
	Quaternion result;
	result.w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z;
	result.x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
	result.y = q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x;
	result.z = q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w;
	return result;
}

Quaternion IdentityQuaternion() {

	Quaternion result = { 0.0f, 0.0f, 0.0f, 1.0f };
	return result;
}

Quaternion Normalize(const Quaternion& q) {
	Quaternion result;
	float norm = Norm(q);
	result.w = q.w / norm;
	result.x = q.x / norm;
	result.y = q.y / norm;
	result.z = q.z / norm;
	return result;
}

Quaternion Conjugate(const Quaternion& q) {
	Quaternion result;
	result.w = q.w;
	result.x = -q.x;
	result.y = -q.y;
	result.z = -q.z;
	return result;
}

float Norm(const Quaternion& q) { return sqrtf(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z); }

Quaternion Inverse(const Quaternion& q) {
	Quaternion result;
	float normSq = q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z;
	Quaternion conj = Conjugate(q);
	result.w = conj.w / normSq;
	result.x = conj.x / normSq;
	result.y = conj.y / normSq;
	result.z = conj.z / normSq;
	return result;
}

Quaternion MakeRotateAxisAngleQuaternion(const Vector3 axis, const float angle) {
	Quaternion result;
	float halfAngle = angle / 2.0f;
	float sinHalfAngle = sinf(halfAngle);
	result.w = cosf(halfAngle);
	result.x = axis.x * sinHalfAngle;
	result.y = axis.y * sinHalfAngle;
	result.z = axis.z * sinHalfAngle;
	return Normalize(result);
}

Vector3 RotateVector(const Vector3& v, const Quaternion& q) {
	Quaternion p = { v.x, v.y, v.z, 0.0f };
	Quaternion qConjugate = Conjugate(q);
	Quaternion rotatedP = Multiply(Multiply(q, p), qConjugate);
	return Vector3{ rotatedP.x, rotatedP.y, rotatedP.z };
}

Matrix4x4 MakeRotateMatrix(const Quaternion& q) {
	Matrix4x4 result = {};
	result.m[0][0] = 1 - 2 * (q.y * q.y + q.z * q.z);
	result.m[0][1] = 2 * (q.x * q.y - q.z * q.w);
	result.m[0][2] = 2 * (q.x * q.z + q.y * q.w);
	result.m[0][3] = 0.0f;
	result.m[1][0] = 2 * (q.x * q.y + q.z * q.w);
	result.m[1][1] = 1 - 2 * (q.x * q.x + q.z * q.z);
	result.m[1][2] = 2 * (q.y * q.z - q.x * q.w);
	result.m[1][3] = 0.0f;
	result.m[2][0] = 2 * (q.x * q.z - q.y * q.w);
	result.m[2][1] = 2 * (q.y * q.z + q.x * q.w);
	result.m[2][2] = 1 - 2 * (q.x * q.x + q.y * q.y);
	result.m[2][3] = 0.0f;
	result.m[3][0] = 0.0f;
	result.m[3][1] = 0.0f;
	result.m[3][2] = 0.0f;
	result.m[3][3] = 1.0f;
	return result;
}

Quaternion Slerp(const Quaternion& q1, const Quaternion& q2, float t) {

	float dot = q1.w * q2.w + q1.x * q2.x + q1.y * q2.y + q1.z * q2.z;
	Quaternion q2Copy = q2;
	if (dot < 0.0f) {
		dot = -dot;
		q2Copy.w = -q2Copy.w;
		q2Copy.x = -q2Copy.x;
		q2Copy.y = -q2Copy.y;
		q2Copy.z = -q2Copy.z;
	}
	const float DOT_THRESHOLD = 0.9995f;
	if (dot > DOT_THRESHOLD) {
		Quaternion result;
		result.w = (1 - t) * q1.w + t * q2Copy.w;
		result.x = (1 - t) * q1.x + t * q2Copy.x;
		result.y = (1 - t) * q1.y + t * q2Copy.y;
		result.z = (1 - t) * q1.z + t * q2Copy.z;
		return Normalize(result);
	}
	float theta_0 = acosf(dot);
	float theta = theta_0 * t;
	float sin_theta = sinf(theta);
	float sin_theta_0 = sinf(theta_0);
	float s0 = cosf(theta) - dot * sin_theta / sin_theta_0;
	float s1 = sin_theta / sin_theta_0;
	Quaternion result;
	result.w = s0 * q1.w + s1 * q2Copy.w;
	result.x = s0 * q1.x + s1 * q2Copy.x;
	result.y = s0 * q1.y + s1 * q2Copy.y;
	result.z = s0 * q1.z + s1 * q2Copy.z;
	return result;
}
