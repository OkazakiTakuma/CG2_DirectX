#include "Matrix.h"
#include <assert.h>
#include <cmath>
#include <math.h>
static const int kRowHeight = 20;
static const int kColumnWidth = 60;
static const float p = 3.1415f;
Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 add{};
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			add.m[i][j] = m1.m[i][j] + m2.m[i][j];
		}
	}
	return add;
}
Matrix4x4 operator+(const Matrix4x4& m1, const Matrix4x4& m2) { return Add(m1, m2); };
Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 subtract{};
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			subtract.m[i][j] = m1.m[i][j] - m2.m[i][j];
		}
	}
	return subtract;
}
Matrix4x4 operator-(const Matrix4x4& m1, const Matrix4x4& m2) { return Subtract(m1, m2); };
Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 multiply{};

	// 各行と各列を掛け合わせる
	for (int i = 0; i < 4; i++) {     // 行ループ
		for (int j = 0; j < 4; j++) { // 列ループ
			// 行列の要素を計算して格納
			multiply.m[i][j] = m1.m[i][0] * m2.m[0][j] + m1.m[i][1] * m2.m[1][j] + m1.m[i][2] * m2.m[2][j] + m1.m[i][3] * m2.m[3][j];
		}
	}

	return multiply;
}
Matrix4x4 operator*(const Matrix4x4& m1, const Matrix4x4& m2) { return Multiply(m1, m2); }
Vector3 MultiplyVector3(const Matrix4x4& m, const Vector3& v) {
	Vector3 result;
	result.x = m.m[0][0] * v.x + m.m[0][1] * v.y + m.m[0][2] * v.z + m.m[0][3];
	result.y = m.m[1][0] * v.x + m.m[1][1] * v.y + m.m[1][2] * v.z + m.m[1][3];
	result.z = m.m[2][0] * v.x + m.m[2][1] * v.y + m.m[2][2] * v.z + m.m[2][3];
	if (m.m[3][0] != 0.0f) {
		result.x /= m.m[3][0];
		result.y /= m.m[3][0];
		result.z /= m.m[3][0];
	}
	if (m.m[3][1] != 0.0f) {
		result.x /= m.m[3][1];
		result.y /= m.m[3][1];
		result.z /= m.m[3][1];
	}
	if (m.m[3][2] != 0.0f) {
		result.x /= m.m[3][2];
		result.y /= m.m[3][2];
		result.z /= m.m[3][2];
	}
	if (m.m[3][3] != 0.0f) {
		result.x /= m.m[3][3];
		result.y /= m.m[3][3];
		result.z /= m.m[3][3];
	}
	if (m.m[3][0] == 0.0f && m.m[3][1] == 0.0f && m.m[3][2] == 0.0f && m.m[3][3] == 0.0f) { // 行列がゼロ行列の場合
		assert(false); // エラーを発生させる
	}
	if (m.m[3][0] == 0.0f && m.m[3][1] == 0.0f && m.m[3][2] == 0.0f && m.m[3][3] == 0.0f) {
		assert(false); // エラーを発生させる
	}
	return result;
}
Vector3 operator*(const Matrix4x4& m, const Vector3& v) { return MultiplyVector3(m, v); }
Matrix4x4 Inverse(const Matrix4x4& m) {
	float det =
	    m.m[0][0] * m.m[1][1] * m.m[2][2] * m.m[3][3] + m.m[0][0] * m.m[1][2] * m.m[2][3] * m.m[3][1] + m.m[0][0] * m.m[1][3] * m.m[2][1] * m.m[3][2] - m.m[0][0] * m.m[1][3] * m.m[2][2] * m.m[3][1] -
	    m.m[0][0] * m.m[1][2] * m.m[2][1] * m.m[3][3] - m.m[0][0] * m.m[1][1] * m.m[2][3] * m.m[3][2] - m.m[0][1] * m.m[1][0] * m.m[2][2] * m.m[3][3] - m.m[0][2] * m.m[1][0] * m.m[2][3] * m.m[3][1] -
	    m.m[0][3] * m.m[1][0] * m.m[2][1] * m.m[3][2] + m.m[0][3] * m.m[1][0] * m.m[2][2] * m.m[3][1] + m.m[0][2] * m.m[1][0] * m.m[2][1] * m.m[3][3] + m.m[0][1] * m.m[1][0] * m.m[2][3] * m.m[3][2] +
	    m.m[0][1] * m.m[1][2] * m.m[2][0] * m.m[3][3] + m.m[0][2] * m.m[1][3] * m.m[2][0] * m.m[3][1] + m.m[0][3] * m.m[1][1] * m.m[2][0] * m.m[3][2] - m.m[0][3] * m.m[1][2] * m.m[2][0] * m.m[3][1] -
	    m.m[0][2] * m.m[1][1] * m.m[2][0] * m.m[3][3] - m.m[0][1] * m.m[1][3] * m.m[2][0] * m.m[3][2] - m.m[0][1] * m.m[1][2] * m.m[2][3] * m.m[3][0] - m.m[0][2] * m.m[1][3] * m.m[2][1] * m.m[3][0] -
	    m.m[0][3] * m.m[1][1] * m.m[2][2] * m.m[3][0] + m.m[0][3] * m.m[1][2] * m.m[2][1] * m.m[3][0] + m.m[0][2] * m.m[1][1] * m.m[2][3] * m.m[3][0] + m.m[0][1] * m.m[1][3] * m.m[2][2] * m.m[3][0];

	Matrix4x4 Inverse{};
	Inverse.m[0][0] = 1 / det *
	                  (m.m[1][1] * m.m[2][2] * m.m[3][3] + m.m[1][2] * m.m[2][3] * m.m[3][1] + m.m[1][3] * m.m[2][1] * m.m[3][2] - m.m[1][3] * m.m[2][2] * m.m[3][1] -
	                   m.m[1][2] * m.m[2][1] * m.m[3][3] - m.m[1][1] * m.m[2][3] * m.m[3][2]);
	Inverse.m[0][1] = 1 / det *
	                  (-m.m[0][1] * m.m[2][2] * m.m[3][3] - m.m[0][2] * m.m[2][3] * m.m[3][1] - m.m[0][3] * m.m[2][1] * m.m[3][2] + m.m[0][3] * m.m[2][2] * m.m[3][1] +
	                   m.m[0][2] * m.m[2][1] * m.m[3][3] + m.m[0][1] * m.m[2][3] * m.m[3][2]);
	Inverse.m[0][2] = 1 / det *
	                  (m.m[0][1] * m.m[1][2] * m.m[3][3] + m.m[0][2] * m.m[1][3] * m.m[3][1] + m.m[0][3] * m.m[1][1] * m.m[3][2] - m.m[0][3] * m.m[1][2] * m.m[3][1] -
	                   m.m[0][2] * m.m[1][1] * m.m[3][3] - m.m[0][1] * m.m[1][3] * m.m[3][2]);
	Inverse.m[0][3] = 1 / det *
	                  (-m.m[0][1] * m.m[1][2] * m.m[2][3] - m.m[0][2] * m.m[1][3] * m.m[2][1] - m.m[0][3] * m.m[1][1] * m.m[2][2] + m.m[0][3] * m.m[1][2] * m.m[2][1] +
	                   m.m[0][2] * m.m[1][1] * m.m[2][3] + m.m[0][1] * m.m[1][3] * m.m[2][2]);

	Inverse.m[1][0] = 1 / det *
	                  (-m.m[1][0] * m.m[2][2] * m.m[3][3] - m.m[1][2] * m.m[2][3] * m.m[3][0] - m.m[1][3] * m.m[2][0] * m.m[3][2] + m.m[1][3] * m.m[2][2] * m.m[3][0] +
	                   m.m[1][2] * m.m[2][0] * m.m[3][3] + m.m[1][0] * m.m[2][3] * m.m[3][2]);
	Inverse.m[1][1] = 1 / det *
	                  (m.m[0][0] * m.m[2][2] * m.m[3][3] + m.m[0][2] * m.m[2][3] * m.m[3][0] + m.m[0][3] * m.m[2][0] * m.m[3][2] - m.m[0][3] * m.m[2][2] * m.m[3][0] -
	                   m.m[0][2] * m.m[2][0] * m.m[3][3] - m.m[0][0] * m.m[2][3] * m.m[3][2]);
	Inverse.m[1][2] = 1 / det *
	                  (-m.m[0][0] * m.m[1][2] * m.m[3][3] - m.m[0][2] * m.m[1][3] * m.m[3][0] - m.m[0][3] * m.m[1][0] * m.m[3][2] + m.m[0][3] * m.m[1][2] * m.m[3][0] +
	                   m.m[0][2] * m.m[1][0] * m.m[3][3] + m.m[0][0] * m.m[1][3] * m.m[3][2]);
	Inverse.m[1][3] = 1 / det *
	                  (m.m[0][0] * m.m[1][2] * m.m[2][3] + m.m[0][2] * m.m[1][3] * m.m[2][0] + m.m[0][3] * m.m[1][0] * m.m[2][2] - m.m[0][3] * m.m[1][2] * m.m[2][0] -
	                   m.m[0][2] * m.m[1][0] * m.m[2][3] - m.m[0][0] * m.m[1][3] * m.m[2][2]);

	Inverse.m[2][0] = 1 / det *
	                  (m.m[1][0] * m.m[2][1] * m.m[3][3] + m.m[1][1] * m.m[2][3] * m.m[3][0] + m.m[1][3] * m.m[2][0] * m.m[3][1] - m.m[1][3] * m.m[2][1] * m.m[3][0] -
	                   m.m[1][1] * m.m[2][0] * m.m[3][3] - m.m[1][0] * m.m[2][3] * m.m[3][1]);
	Inverse.m[2][1] = 1 / det *
	                  (-m.m[0][0] * m.m[2][1] * m.m[3][3] - m.m[0][1] * m.m[2][3] * m.m[3][0] - m.m[0][3] * m.m[2][0] * m.m[3][1] + m.m[0][3] * m.m[2][1] * m.m[3][0] +
	                   m.m[0][1] * m.m[2][0] * m.m[3][3] + m.m[0][0] * m.m[2][3] * m.m[3][1]);
	Inverse.m[2][2] = 1 / det *
	                  (m.m[0][0] * m.m[1][1] * m.m[3][3] + m.m[0][1] * m.m[1][3] * m.m[3][0] + m.m[0][3] * m.m[1][0] * m.m[3][1] - m.m[0][3] * m.m[1][1] * m.m[3][0] -
	                   m.m[0][1] * m.m[1][0] * m.m[3][3] - m.m[0][0] * m.m[1][3] * m.m[3][1]);
	Inverse.m[2][3] = 1 / det *
	                  (-m.m[0][0] * m.m[1][1] * m.m[2][3] - m.m[0][1] * m.m[1][3] * m.m[2][0] - m.m[0][3] * m.m[1][0] * m.m[2][1] + m.m[0][3] * m.m[1][1] * m.m[2][0] +
	                   m.m[0][1] * m.m[1][0] * m.m[2][3] + m.m[0][0] * m.m[1][3] * m.m[2][1]);

	Inverse.m[3][0] = 1 / det *
	                  (-m.m[1][0] * m.m[2][1] * m.m[3][2] - m.m[1][1] * m.m[2][2] * m.m[3][0] - m.m[1][2] * m.m[2][0] * m.m[3][1] + m.m[1][2] * m.m[2][1] * m.m[3][0] +
	                   m.m[1][1] * m.m[2][0] * m.m[3][2] + m.m[1][0] * m.m[2][2] * m.m[3][1]);
	Inverse.m[3][1] = 1 / det *
	                  (m.m[0][0] * m.m[2][1] * m.m[3][2] + m.m[0][1] * m.m[2][2] * m.m[3][0] + m.m[0][2] * m.m[2][0] * m.m[3][1] - m.m[0][2] * m.m[2][1] * m.m[3][0] -
	                   m.m[0][1] * m.m[2][0] * m.m[3][2] - m.m[0][0] * m.m[2][2] * m.m[3][1]);
	Inverse.m[3][2] = 1 / det *
	                  (-m.m[0][0] * m.m[1][1] * m.m[3][2] - m.m[0][1] * m.m[1][2] * m.m[3][0] - m.m[0][2] * m.m[1][0] * m.m[3][1] + m.m[0][2] * m.m[1][1] * m.m[3][0] +
	                   m.m[0][1] * m.m[1][0] * m.m[3][2] + m.m[0][0] * m.m[1][2] * m.m[3][1]);
	Inverse.m[3][3] = 1 / det *
	                  (m.m[0][0] * m.m[1][1] * m.m[2][2] + m.m[0][1] * m.m[1][2] * m.m[2][0] + m.m[0][2] * m.m[1][0] * m.m[2][1] - m.m[0][2] * m.m[1][1] * m.m[2][0] -
	                   m.m[0][1] * m.m[1][0] * m.m[2][2] - m.m[0][0] * m.m[1][2] * m.m[2][1]);
	return Inverse;
};
Matrix4x4 Transpose(const Matrix4x4& m) {
	Matrix4x4 trancepose{};
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			trancepose.m[i][j] = m.m[j][i];
		}
	}

	return trancepose;
};
Matrix4x4 MakeIdentity4x4() {
	Matrix4x4 identiy = {0};
	for (int i = 0; i < 4; i++) {
		identiy.m[i][i] = 1.0f;
	}

	return identiy;
}

void MatrixScreenPrintf(int posX, int posY, const Matrix4x4& matrix, const char* label) {}

Matrix4x4 MakeTranslateMatrix(const Vector3& translate) {
	Matrix4x4 translateMatrix = MakeIdentity4x4();
	translateMatrix.m[3][0] = translate.x;
	translateMatrix.m[3][1] = translate.y;
	translateMatrix.m[3][2] = translate.z;

	return translateMatrix;
}

Matrix4x4 MakeScaleMatrix(const Vector3& scale) {
	Matrix4x4 scaleMatrix = MakeIdentity4x4();
	scaleMatrix.m[0][0] = scale.x;
	scaleMatrix.m[1][1] = scale.y;
	scaleMatrix.m[2][2] = scale.z;
	return scaleMatrix;
}

Matrix4x4 MakeRotateXMatrix(float radiun) {
	Matrix4x4 rotateX = MakeIdentity4x4();
	rotateX.m[1][1] = std::cos(radiun);
	rotateX.m[2][1] = -std::sin(radiun);
	rotateX.m[1][2] = std::sin(radiun);
	rotateX.m[2][2] = std::cos(radiun);

	return rotateX;
}

Matrix4x4 MakeRotateYMatrix(float radiun) {
	Matrix4x4 rotateY = MakeIdentity4x4();
	rotateY.m[0][0] = std::cos(radiun);
	rotateY.m[2][0] = std::sin(radiun);
	rotateY.m[0][2] = -std::sin(radiun);
	rotateY.m[2][2] = std::cos(radiun);

	return rotateY;
}

Matrix4x4 MakeRotateZMatrix(float radiun) {
	Matrix4x4 rotateZ = MakeIdentity4x4();
	rotateZ.m[0][0] = std::cos(radiun);
	rotateZ.m[1][0] = -std::sin(radiun);
	rotateZ.m[0][1] = std::sin(radiun);
	rotateZ.m[1][1] = std::cos(radiun);

	return rotateZ;
}

Matrix4x4 MakeRotateXYZMatrix(Vector3 rotate) {
	Matrix4x4 XMatrix = MakeRotateXMatrix(rotate.x);
	Matrix4x4 YMatrix = MakeRotateYMatrix(rotate.y);
	Matrix4x4 ZMatrix = MakeRotateZMatrix(rotate.z);
	Matrix4x4 XYZMatrix = Multiply(XMatrix, Multiply(YMatrix, ZMatrix));

	return XYZMatrix;
}

Matrix4x4 MakeAffineMatrix(Vector3 scale, Vector3 rotate, Vector3 translate) {
	Matrix4x4 affineMatrix;
	Matrix4x4 rotateMatrix = MakeRotateXYZMatrix(rotate);
	affineMatrix.m[0][0] = scale.x * rotateMatrix.m[0][0];
	affineMatrix.m[1][0] = scale.y * rotateMatrix.m[1][0];
	affineMatrix.m[2][0] = scale.z * rotateMatrix.m[2][0];
	affineMatrix.m[3][0] = translate.x;

	affineMatrix.m[0][1] = scale.x * rotateMatrix.m[0][1];
	affineMatrix.m[1][1] = scale.y * rotateMatrix.m[1][1];
	affineMatrix.m[2][1] = scale.z * rotateMatrix.m[2][1];
	affineMatrix.m[3][1] = translate.y;

	affineMatrix.m[0][2] = scale.x * rotateMatrix.m[0][2];
	affineMatrix.m[1][2] = scale.y * rotateMatrix.m[1][2];
	affineMatrix.m[2][2] = scale.z * rotateMatrix.m[2][2];
	affineMatrix.m[3][2] = translate.z;

	affineMatrix.m[0][3] = 0.0f;
	affineMatrix.m[1][3] = 0.0f;
	affineMatrix.m[2][3] = scale.z * rotateMatrix.m[2][3];
	affineMatrix.m[3][3] = 1.0f;
	return affineMatrix;
}
Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix) {
	Vector3 result{};

	result.x = vector.x * matrix.m[0][0] + vector.y * matrix.m[1][0] + vector.z * matrix.m[2][0] + 1.0f * matrix.m[3][0];
	result.y = vector.x * matrix.m[0][1] + vector.y * matrix.m[1][1] + vector.z * matrix.m[2][1] + 1.0f * matrix.m[3][1];
	result.z = vector.x * matrix.m[0][2] + vector.y * matrix.m[1][2] + vector.z * matrix.m[2][2] + 1.0f * matrix.m[3][2];
	float w = vector.x * matrix.m[0][3] + vector.y * matrix.m[1][3] + vector.z * matrix.m[2][3] + 1.0f * matrix.m[3][3];

	// wが0の場合の処理
	if (w != 0.0f) {
		result.x /= w;
		result.y /= w;
		result.z /= w;
	} else {
		// デフォルト値を設定するか、エラー処理を行う
		result.x = 0.0f;
		result.y = 0.0f;
		result.z = 0.0f;
	}
	return result;
}

Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float zFar, float zNear) {
	Matrix4x4 orthographicMatrix = {0};

	orthographicMatrix.m[0][0] = 2.0f / (right - left);
	orthographicMatrix.m[3][0] = -(left + right) / (right - left);

	orthographicMatrix.m[1][1] = 2.0f / (top - bottom);
	orthographicMatrix.m[3][1] = -(top + bottom) / (top - bottom);

	orthographicMatrix.m[2][2] = -2.0f / (zFar - zNear); // 奥行きを -1 ～ 1 にマッピング
	orthographicMatrix.m[3][2] = -(zFar + zNear) / (zFar - zNear);

	orthographicMatrix.m[3][3] = 1;

	return orthographicMatrix;
}

Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip) {
	Matrix4x4 perspectiveFovMatrix{};

	// コタンジェントを計算
	float cot = 1.0f / tanf(fovY / 2.0f);

	// X方向とY方向のスケール設定
	perspectiveFovMatrix.m[0][0] = cot / aspectRatio;
	perspectiveFovMatrix.m[1][1] = cot;

	// Z方向のクリップ面の設定
	perspectiveFovMatrix.m[2][2] = farClip / (nearClip - farClip);
	perspectiveFovMatrix.m[3][2] = (nearClip * farClip) / (nearClip - farClip);

	// 透視投影効果を追加
	perspectiveFovMatrix.m[2][3] = 1.0f;

	return perspectiveFovMatrix;
}
Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDeapth, float maxDepth) {
	Matrix4x4 viewportMatrix{};
	viewportMatrix.m[0][0] = width / 2;
	viewportMatrix.m[3][0] = left + width / 2;

	viewportMatrix.m[1][1] = -height / 2;
	viewportMatrix.m[3][1] = top + height / 2;

	viewportMatrix.m[2][2] = maxDepth - minDeapth;
	viewportMatrix.m[3][2] = minDeapth;

	viewportMatrix.m[3][3] = 1.0f;
	return viewportMatrix;
}
