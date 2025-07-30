#pragma once
#include <math.h>

/// <summary>
/// 3x3の行列
/// </summary>
struct Matrix3x3 {

	float m[3][3];
};
/// <summary>
/// 2次元ベクトル
/// </summary>
struct Vector2 {
	float x;
	float y;
};
/// <summary>
/// アファイン行列を作る関数
/// </summary>
/// <param name="scale">倍率</param>
/// <param name="rotate">角度</param>
/// <param name="translate">位置</param>
/// <returns></returns>
Matrix3x3 MakeAffineMatrix(Vector2 scale, float rotate, Vector2 translate);

/// <summary>
/// 行列を変位させた座標に変換する関数
/// </summary>
/// <param name="translate">変位させる値</param>
/// <param name="matrix">行列</param>
/// <returns></returns>
Vector2 Transform(Vector2 translate, Matrix3x3 matrix);

/// <summary>
/// 3x3の逆行列を求める
/// </summary>
/// <param name="matrix">3x3行列</param>
/// <returns></returns>
Matrix3x3 InverseMatrix(Matrix3x3 matrix);

/// <summary>
/// 行列の合成
/// </summary>
/// <param name="matrix1"></param>
/// <param name="matrix2"></param>
/// <returns></returns>
Matrix3x3 Multply(Matrix3x3 matrix1, Matrix3x3 matrix2);

/// <summary>
/// カメラ行列を作る関数
/// </summary>
/// <param name="scale">カメラの倍率</param>
/// <param name="position">カメラの座標</param>
/// <returns></returns>
Matrix3x3 MakeCameraMatrix(Vector2 scale, Vector2 position);

/// <summary>
/// 正射影行列を作る関数
/// </summary>
/// <param name="left">左端</param>
/// <param name="top">上端</param>
/// <param name="right">右端</param>
/// <param name="bottom"><下端/param>
/// <returns>正射影行列</returns>
Matrix3x3 MakeOrthographicMatrix(float left, float top, float right, float bottom);

/// <summary>
/// ビューポート行列を作る関数
/// </summary>
/// <param name="left">左端</param>
/// <param name="top">上端</param>
/// <param name="width">横幅</param>
/// <param name="height">縦幅</param>
/// <returns></returns>
Matrix3x3 MakeViewportMatrix(float left, float top, float width, float height);

/// <summary>
/// レンダリングパイプライン制御を使用してスクリーン座標系に変換する関数
/// </summary>
/// <param name="scale">倍率</param>
/// <param name="rotate">角度</param>
/// <param name="position">変換したい座標</param>
/// <param name="cameraMatrix">カメラ行列</param>
/// <param name="kWindowsWidih">スクリーン画面の横幅</param>
/// <param name=" kWindowsHeight">スクリーン画面の縦幅</param>
/// <returns></returns>
Vector2 ScreenPoint(Vector2 scale, float rotate, Vector2 position, Matrix3x3 cameraMatrix, const int kWindowsWidih, const int kWindowsHeight);
/// <summary>
/// x軸(赤)とy軸(緑)を描画する関数
/// </summary>
/// <param name="cameraMatrix">カメラ行列</param>
/// <param name="kWindowsWidih">スクリーン画面の横幅</param>
/// <param name=" kWindowsHeight">スクリーン画面の縦幅</param>
void DrawShaft(Matrix3x3 cameraMatrix, int kWindowsWidih, int kWindowsHeight);
