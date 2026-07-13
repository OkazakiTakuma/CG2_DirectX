#pragma once
#include <math.h>
#include "Vector.h"


/// <summary>
/// </summary>
struct Matrix3x3 {

	float m[3][3];
};

/// <summary>
/// </summary>
/// <param name="scale">蛟咲紫</param>
/// <returns></returns>
Matrix3x3 MakeAffineMatrix(Vector2 scale, float rotate, Vector2 translate);

/// <summary>
/// </summary>
/// <returns></returns>
Vector2 Transformation(Vector2 translate, Matrix3x3 matrix);

/// <summary>
/// </summary>
/// <returns></returns>
Matrix3x3 InverseMatrix(Matrix3x3 matrix);

/// <summary>
/// </summary>
/// <param name="matrix1"></param>
/// <param name="matrix2"></param>
/// <returns></returns>
Matrix3x3 Multply(Matrix3x3 matrix1, Matrix3x3 matrix2);

/// <summary>
/// </summary>
/// <returns></returns>
Matrix3x3 MakeCameraMatrix(Vector2 scale, Vector2 position);

/// <summary>
/// </summary>
/// <param name="left">left に使用する値を指定します。</param>
/// <param name="top">top に使用する値を指定します。</param>
/// <param name="right">right に使用する値を指定します。</param>
/// <param name="bottom">bottom に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
Matrix3x3 MakeOrthographicMatrix(float left, float top, float right, float bottom);

/// <summary>
/// </summary>
/// <returns></returns>
Matrix3x3 MakeViewportMatrix(float left, float top, float width, float height);

/// <summary>
/// </summary>
/// <param name="scale">蛟咲紫</param>
/// <returns></returns>
Vector2 ScreenPoint(Vector2 scale, float rotate, Vector2 position, Matrix3x3 cameraMatrix, const int kWindowsWidih, const int kWindowsHeight);
/// <summary>
/// </summary>
/// <param name="cameraMatrix">cameraMatrix に使用する値を指定します。</param>
/// <param name="kWindowsWidih">kWindowsWidih に使用する値を指定します。</param>
/// <param name="kWindowsHeight">kWindowsHeight に使用する値を指定します。</param>
void DrawShaft(Matrix3x3 cameraMatrix, int kWindowsWidih, int kWindowsHeight);
