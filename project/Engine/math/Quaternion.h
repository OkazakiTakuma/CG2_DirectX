#pragma once
#include"Vector.h"

struct Matrix4x4;

struct Quaternion {
	float x, y, z, w;
};


Quaternion Multiply(const Quaternion& q1, const Quaternion& q2);

Quaternion IdentityQuaternion();

Quaternion Normalize(const Quaternion& q);

Quaternion Conjugate(const Quaternion& q);

float Norm(const Quaternion& q);

Quaternion Inverse(const Quaternion& q);


Quaternion MakeRotateAxisAngleQuaternion(const Vector3 axis, const float angle);

Vector3 RotateVector(const Vector3& v, const Quaternion& q);

Matrix4x4 MakeRotateMatrix(const Quaternion& q);

Quaternion Slerp(const Quaternion& q1, const Quaternion& q2, float t);
