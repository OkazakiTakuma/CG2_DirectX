#pragma once

/// <summary>
/// 浮動小数点の幾何計算で共有する許容誤差を定義します。
/// </summary>
namespace MathConstants {

inline constexpr float kPi = 3.14159265358979323846f;
inline constexpr float kNormalizationEpsilon = 0.00001f;
inline constexpr float kDirectionEpsilon = 0.0001f;

} // namespace MathConstants
