#pragma once

#include "struct.h"

/// <summary>
/// Node階層からのSkeleton構築と、Animationによる姿勢・行列計算を提供します。
/// 描画状態やGPUリソースには依存しません。
/// </summary>
namespace SkeletonAnimationUtility {

Skeleton CreateSkeleton(const Node& rootNode);
void ApplyAnimation(Skeleton& skeleton, const Animation& animation, float time, float blendWeight);
void UpdateMatrices(Skeleton& skeleton);

} // namespace SkeletonAnimationUtility
