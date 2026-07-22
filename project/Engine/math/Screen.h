#pragma once
#include <math.h>
#include "Vector.h"


struct Matrix3x3 {

	float m[3][3];
};

/// <param name="scale">蛟咲紫</param>
/// <returns></returns>
Matrix3x3 MakeAffineMatrix(Vector2 scale, float rotate, Vector2 translate);

/// <returns></returns>
Vector2 Transformation(Vector2 translate, Matrix3x3 matrix);

/// <returns></returns>
Matrix3x3 InverseMatrix(Matrix3x3 matrix);

/// <param name="matrix1"></param>
/// <param name="matrix2"></param>
/// <returns></returns>
Matrix3x3 Multply(Matrix3x3 matrix1, Matrix3x3 matrix2);

/// <returns></returns>
Matrix3x3 MakeCameraMatrix(Vector2 scale, Vector2 position);

Matrix3x3 MakeOrthographicMatrix(float left, float top, float right, float bottom);

/// <returns></returns>
Matrix3x3 MakeViewportMatrix(float left, float top, float width, float height);

/// <param name="scale">蛟咲紫</param>
/// <returns></returns>
Vector2 ScreenPoint(Vector2 scale, float rotate, Vector2 position, Matrix3x3 cameraMatrix, int windowWidth, int windowHeight);
/// <param name="windowWidth">描画先の幅を指定します。</param>
void DrawShaft(Matrix3x3 cameraMatrix, int windowWidth, int windowHeight);
