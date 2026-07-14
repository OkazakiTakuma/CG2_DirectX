#pragma once
#include "../Engine/flame/Component.h"
#include "Vector.h"
#include <string>

class Player : public Component {
public:
	/// <summary>
	/// 毎フレーム WASD 入力を見て、XZ 平面上でプレイヤーを移動します。
	/// </summary>
	void Update() override;

	/// <summary>
	/// 現在位置をスポーンポイントへ戻します。
	/// </summary>
	void ResetToSpawnPoint();

	/// <summary>
	/// スポーンポイントを設定します。
	/// </summary>
	/// <param name="spawnPoint">リロード時やリセット時に戻す位置を指定します。</param>
	void SetSpawnPoint(const Vector3& spawnPoint) { spawnPoint_ = spawnPoint; }
	const Vector3& GetSpawnPoint() const { return spawnPoint_; }

	/// <summary>
	/// 1 フレームあたりの移動速度を設定します。
	/// </summary>
	/// <param name="moveSpeed">WASD 入力時に加算する移動量を指定します。</param>
	void SetMoveSpeed(float moveSpeed) { moveSpeed_ = moveSpeed < 0.0f ? 0.0f : moveSpeed; }
	float GetMoveSpeed() const { return moveSpeed_; }

	/// <summary>
	/// プレイヤー表示に使用するモデル情報を設定します。
	/// </summary>
	/// <param name="modelFilePath">ModelManager に読み込まれているモデル名を指定します。</param>
	/// <param name="isAnimationModel">アニメーションモデルの場合 true を指定します。</param>
	void SetModelFilePath(const std::string& modelFilePath, bool isAnimationModel) {
		modelFilePath_ = modelFilePath;
		isAnimationModel_ = isAnimationModel;
	}
	const std::string& GetModelFilePath() const { return modelFilePath_; }
	bool GetIsAnimationModel() const { return isAnimationModel_; }

private:
	Vector3 spawnPoint_{0.0f, 0.0f, 0.0f};
	Vector3 currentMoveVelocity_{0.0f, 0.0f, 0.0f};
	float moveSpeed_ = 0.1f;
	std::string modelFilePath_;
	bool isAnimationModel_ = false;
};
