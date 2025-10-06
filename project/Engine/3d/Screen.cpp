#include "Screen.h"
#include <assert.h>
#include <math.h>
Vector2 Transform(Vector2 vector, Matrix3x3 matrix) {
	Vector2 result;

	result.x = vector.x * matrix.m[0][0] + vector.y * matrix.m[1][0] + 1.0f * matrix.m[2][0];
	result.y = vector.x * matrix.m[0][1] + vector.y * matrix.m[1][1] + 1.0f * matrix.m[2][1];
	float w = vector.x * matrix.m[0][2] + vector.y * matrix.m[1][2] + 1.0f * matrix.m[2][2];
	assert(w != 0);
	result.x /= w;
	result.y /= w;
	return result;
};
Matrix3x3 MakeAffineMatrix(Vector2 scale, float rotate, Vector2 translate) {

	Matrix3x3 matrix;
	matrix.m[0][0] = scale.x * cosf(rotate);
	matrix.m[0][1] = scale.x * sinf(rotate);
	matrix.m[0][2] = 0;

	matrix.m[1][0] = scale.y * -sinf(rotate);
	matrix.m[1][1] = scale.y * cosf(rotate);
	matrix.m[1][2] = 0;

	matrix.m[2][0] = translate.x;
	matrix.m[2][1] = translate.y;
	matrix.m[2][2] = 1;

	return matrix;
};
Matrix3x3 Multply(Matrix3x3 matrix1, Matrix3x3 matrix2) {
	Matrix3x3 matrixMultply;

	matrixMultply.m[0][0] = matrix1.m[0][0] * matrix2.m[0][0] + matrix1.m[0][1] * matrix2.m[1][0] + matrix1.m[0][2] * matrix2.m[2][0];
	matrixMultply.m[0][1] = matrix1.m[0][0] * matrix2.m[0][1] + matrix1.m[0][1] * matrix2.m[1][1] + matrix1.m[0][2] * matrix2.m[2][1];
	matrixMultply.m[0][2] = matrix1.m[0][0] * matrix2.m[0][2] + matrix1.m[0][1] * matrix2.m[1][2] + matrix1.m[0][2] * matrix2.m[2][2];

	matrixMultply.m[1][0] = matrix1.m[1][0] * matrix2.m[0][0] + matrix1.m[1][1] * matrix2.m[1][0] + matrix1.m[1][2] * matrix2.m[2][0];
	matrixMultply.m[1][1] = matrix1.m[1][0] * matrix2.m[0][1] + matrix1.m[1][1] * matrix2.m[1][1] + matrix1.m[1][2] * matrix2.m[2][1];
	matrixMultply.m[1][2] = matrix1.m[1][0] * matrix2.m[0][2] + matrix1.m[1][1] * matrix2.m[1][2] + matrix1.m[1][2] * matrix2.m[2][2];

	matrixMultply.m[2][0] = matrix1.m[2][0] * matrix2.m[0][0] + matrix1.m[2][1] * matrix2.m[1][0] + matrix1.m[2][2] * matrix2.m[2][0];
	matrixMultply.m[2][1] = matrix1.m[2][0] * matrix2.m[0][1] + matrix1.m[2][1] * matrix2.m[1][1] + matrix1.m[2][2] * matrix2.m[2][1];
	matrixMultply.m[2][2] = matrix1.m[2][0] * matrix2.m[0][2] + matrix1.m[2][1] * matrix2.m[1][2] + matrix1.m[2][2] * matrix2.m[2][2];

	return matrixMultply;
};

Matrix3x3 MakeCameraMatrix(Vector2 scale, Vector2 position) {
	Matrix3x3 cameraWorldMatrix;
	cameraWorldMatrix.m[0][0] = scale.x;
	cameraWorldMatrix.m[0][1] = 0;
	cameraWorldMatrix.m[0][2] = 0;
	cameraWorldMatrix.m[1][0] = 0;
	cameraWorldMatrix.m[1][1] = scale.y;
	cameraWorldMatrix.m[1][2] = 0;
	cameraWorldMatrix.m[2][0] = position.x;
	cameraWorldMatrix.m[2][1] = position.y;
	cameraWorldMatrix.m[2][2] = 1;

	return cameraWorldMatrix;
}

Matrix3x3 InverseMatrix(Matrix3x3 matrix) {
	float reciprocal = 1.0f / ((matrix.m[0][0] * matrix.m[1][1] * matrix.m[2][2]) + (matrix.m[0][1] * matrix.m[1][2] * matrix.m[2][0]) + (matrix.m[0][2] * matrix.m[1][0] * matrix.m[2][1]) -
	                           (matrix.m[0][2] * matrix.m[1][1] * matrix.m[2][0]) - (matrix.m[0][1] * matrix.m[1][0] * matrix.m[2][2]) - (matrix.m[0][0] * matrix.m[1][2] * matrix.m[2][1]));
	Matrix3x3 InverseMatrix;
	InverseMatrix.m[0][0] = reciprocal * (matrix.m[1][1] * matrix.m[2][2] - matrix.m[1][2] * matrix.m[2][1]);
	InverseMatrix.m[0][1] = reciprocal * -(matrix.m[0][1] * matrix.m[2][2] - matrix.m[0][2] * matrix.m[2][1]);
	InverseMatrix.m[0][2] = reciprocal * (matrix.m[0][1] * matrix.m[1][2] - matrix.m[0][2] * matrix.m[1][1]);

	InverseMatrix.m[1][0] = reciprocal * -(matrix.m[1][0] * matrix.m[2][2] - matrix.m[1][2] * matrix.m[2][0]);
	InverseMatrix.m[1][1] = reciprocal * (matrix.m[0][0] * matrix.m[2][2] - matrix.m[0][2] * matrix.m[2][0]);
	InverseMatrix.m[1][2] = reciprocal * -(matrix.m[0][0] * matrix.m[1][2] - matrix.m[0][2] * matrix.m[1][0]);

	InverseMatrix.m[2][0] = reciprocal * (matrix.m[1][0] * matrix.m[2][1] - matrix.m[1][1] * matrix.m[2][0]);
	InverseMatrix.m[2][1] = reciprocal * -(matrix.m[0][0] * matrix.m[2][1] - matrix.m[0][1] * matrix.m[2][0]);
	InverseMatrix.m[2][2] = reciprocal * (matrix.m[0][0] * matrix.m[1][1] - matrix.m[0][1] * matrix.m[1][0]);

	return InverseMatrix;
};

Matrix3x3 MakeOrthographicMatrix(float left, float top, float right, float bottom) {
	Matrix3x3 orthographicMatrix = {0};
	orthographicMatrix.m[0][0] = 2.0f / (right - left);
	orthographicMatrix.m[0][1] = 0;
	orthographicMatrix.m[0][2] = 0;

	orthographicMatrix.m[1][0] = 0;
	orthographicMatrix.m[1][1] = 2.0f / (top - bottom);
	orthographicMatrix.m[1][2] = 0;

	orthographicMatrix.m[2][0] = -(left + right) / (right - left);
	orthographicMatrix.m[2][1] = -(top + bottom) / (top - bottom);
	orthographicMatrix.m[2][2] = 1.0f;

	return orthographicMatrix;
}

Matrix3x3 MakeViewportMatrix(float left, float top, float width, float height) {
	Matrix3x3 viewPortMatrix = {0};
	viewPortMatrix.m[0][0] = (width / 2.0f);
	viewPortMatrix.m[0][1] = 0;
	viewPortMatrix.m[0][2] = 0;

	viewPortMatrix.m[1][0] = 0;
	viewPortMatrix.m[1][1] = -(height / 2.0f);
	viewPortMatrix.m[1][2] = 0;

	viewPortMatrix.m[2][0] = left + (width / 2.0f);
	viewPortMatrix.m[2][1] = top + (height / 2.0f);
	viewPortMatrix.m[2][2] = 1;

	return viewPortMatrix;
}

Vector2 ScreenPoint(Vector2 scale, float rotate, Vector2 position, Matrix3x3 cameraMatrix, int kWindowsWidih, int kWindowsHeight) {
	float theta = rotate * 3.1415f;
	Vector2 size = {0, 0};

	Matrix3x3 viewMatrix = InverseMatrix(cameraMatrix);
	Matrix3x3 orthoMatrix = MakeOrthographicMatrix(-float(kWindowsWidih / 2), float(kWindowsHeight / 2), float(kWindowsWidih / 2), -float(kWindowsHeight / 2));
	Matrix3x3 viewPortMatrix = MakeViewportMatrix(0, 0, float(kWindowsWidih), float(kWindowsHeight));

	Matrix3x3 worldMatrix = MakeAffineMatrix(scale, theta, position);
	Matrix3x3 wvpVpMatrix = Multply(worldMatrix, viewMatrix);
	wvpVpMatrix = Multply(wvpVpMatrix, orthoMatrix);
	wvpVpMatrix = Multply(wvpVpMatrix, viewPortMatrix);

	Vector2 screenPosition = Transform(size, wvpVpMatrix);
	return screenPosition;
}

void DrawShaft(Matrix3x3 cameraMatrix, int kWindowsWidih, int kWindowsHeight) {
	Matrix3x3 lineMatrix = {0};
	Vector2 position = {0};
	Vector2 scale = {1, 1};

	Vector2 top = {0, 10000};
	Vector2 bottom = {0, -10000};
	Vector2 right = {12000, 0};
	Vector2 left = {-12000, 0};

	lineMatrix = MakeAffineMatrix(scale, 0.0f, position);
	Matrix3x3 viewMatrix = InverseMatrix(cameraMatrix);
	Matrix3x3 orthoMatrix = MakeOrthographicMatrix(-float(kWindowsWidih / 2), float(kWindowsHeight / 2), float(kWindowsWidih / 2), -float(kWindowsHeight / 2));
	Matrix3x3 viewPortMatrix = MakeViewportMatrix(0, 0, float(kWindowsWidih), float(kWindowsHeight));
	Matrix3x3 wvpVpLineMatrix = Multply(lineMatrix, viewMatrix);
	wvpVpLineMatrix = Multply(wvpVpLineMatrix, orthoMatrix);
	wvpVpLineMatrix = Multply(wvpVpLineMatrix, viewPortMatrix);
	Vector2 lineTop = Transform(top, wvpVpLineMatrix);
	Vector2 lineRight = Transform(right, wvpVpLineMatrix);
	Vector2 lineLeft = Transform(left, wvpVpLineMatrix);
	Vector2 lineBottom = Transform(bottom, wvpVpLineMatrix);

}
