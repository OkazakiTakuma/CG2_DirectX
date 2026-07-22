#include "SkeletonAnimationUtility.h"

#include "Quaternion.h"
#include <cassert>
#include <optional>

namespace {

Matrix4x4 MakeQuaternionAffineMatrix(
    const Vector3& scale,
    const Quaternion& rotation,
    const Vector3& translation) {
	return Multiply(
	    Multiply(MakeScaleMatrix(scale), MakeRotateMatrix(rotation)),
	    MakeTranslateMatrix(translation));
}

Vector3 Interpolate(const std::vector<KeyframeVector3>& keyframes, float time) {
	// 指定時刻を挟む2キーを探し、移動・拡縮値を線形補間する。
	assert(!keyframes.empty());
	if (keyframes.size() == 1 || time <= keyframes.front().time) {
		return keyframes.front().value;
	}
	for (size_t index = 0; index + 1 < keyframes.size(); ++index) {
		const size_t nextIndex = index + 1;
		if (keyframes[index].time <= time && time <= keyframes[nextIndex].time) {
			const float duration = keyframes[nextIndex].time - keyframes[index].time;
			if (duration <= 0.0f) {
				return keyframes[nextIndex].value;
			}
			const float ratio = (time - keyframes[index].time) / duration;
			return Leap(keyframes[index].value, keyframes[nextIndex].value, ratio);
		}
	}
	return keyframes.back().value;
}

Quaternion Interpolate(const std::vector<KeyframeQuaternion>& keyframes, float time) {
	// 回転は一定角速度に近い補間となるよう球面線形補間を使用する。
	assert(!keyframes.empty());
	if (keyframes.size() == 1 || time <= keyframes.front().time) {
		return keyframes.front().value;
	}
	for (size_t index = 0; index + 1 < keyframes.size(); ++index) {
		const size_t nextIndex = index + 1;
		if (keyframes[index].time <= time && time <= keyframes[nextIndex].time) {
			const float duration = keyframes[nextIndex].time - keyframes[index].time;
			if (duration <= 0.0f) {
				return keyframes[nextIndex].value;
			}
			const float ratio = (time - keyframes[index].time) / duration;
			return Slerp(keyframes[index].value, keyframes[nextIndex].value, ratio);
		}
	}
	return keyframes.back().value;
}

int32_t CreateJoint(
    const Node& node,
    const std::optional<int32_t>& parent,
    std::vector<Joint>& joints,
    std::map<std::string, int32_t>& jointMap) {
	Joint joint;
	// ノード階層を深さ優先で平坦な配列へ変換し、親子インデックスを保存する。
	joint.transform = node.transform;
	joint.bindTransform = node.transform;
	joint.localMatrix = node.localMatrix;
	joint.bindLocalMatrix = node.localMatrix;
	joint.skeletonSpaceMatrix = MakeIdentity4x4();
	joint.name = node.name;
	joint.index = static_cast<int32_t>(joints.size());
	joint.parent = parent;

	jointMap[joint.name] = joint.index;
	joints.push_back(joint);
	for (const Node& child : node.children) {
		const int32_t childIndex = CreateJoint(child, joint.index, joints, jointMap);
		joints[joint.index].children.push_back(childIndex);
	}
	return joint.index;
}

} // namespace

namespace SkeletonAnimationUtility {

Skeleton CreateSkeleton(const Node& rootNode) {
	// モデルのルートノードから実行時スケルトンと名前検索表を構築する。
	Skeleton skeleton;
	skeleton.root = CreateJoint(rootNode, std::nullopt, skeleton.joints, skeleton.jointMap);
	return skeleton;
}

void ApplyAnimation(Skeleton& skeleton, const Animation& animation, float time, float blendWeight) {
	if (skeleton.joints.empty() || animation.duration <= 0.0f) {
		return;
	}
	// 毎フレームバインド姿勢へ戻してから、存在するチャンネルだけを上書きする。
	for (Joint& joint : skeleton.joints) {
		joint.transform = joint.bindTransform;
		joint.localMatrix = joint.bindLocalMatrix;
		QuaternionTransform animatedTransform = joint.bindTransform;
		const auto animationIterator = animation.nodeAnimations.find(joint.name);
		if (animationIterator != animation.nodeAnimations.end()) {
			const NodeAnimation& nodeAnimation = animationIterator->second;
			if (!nodeAnimation.translate.keyframes.empty()) {
				animatedTransform.translate = Interpolate(nodeAnimation.translate.keyframes, time);
			}
			if (!nodeAnimation.rotate.keyframes.empty()) {
				animatedTransform.rotate = Interpolate(nodeAnimation.rotate.keyframes, time);
			}
			if (!nodeAnimation.scale.keyframes.empty()) {
				animatedTransform.scale = Interpolate(nodeAnimation.scale.keyframes, time);
			}
		}
		if (blendWeight > 0.0f) {
			joint.transform.translate = Leap(joint.bindTransform.translate, animatedTransform.translate, blendWeight);
			joint.transform.rotate = Slerp(joint.bindTransform.rotate, animatedTransform.rotate, blendWeight);
			joint.transform.scale = Leap(joint.bindTransform.scale, animatedTransform.scale, blendWeight);
		}
	}
}

void UpdateMatrices(Skeleton& skeleton) {
	// 親が先に格納されているため、配列順に更新してスケルトン空間行列を伝播する。
	for (Joint& joint : skeleton.joints) {
		joint.localMatrix = MakeQuaternionAffineMatrix(
		    joint.transform.scale, joint.transform.rotate, joint.transform.translate);
		joint.skeletonSpaceMatrix = joint.parent
		                                ? Multiply(joint.localMatrix, skeleton.joints[*joint.parent].skeletonSpaceMatrix)
		                                : joint.localMatrix;
	}
}

} // namespace SkeletonAnimationUtility
