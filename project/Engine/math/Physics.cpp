#include "Physics.h"
#include <cmath>

Vector3 PendulumMove(const Pendulum& pendulum) { 
	return Vector3{pendulum.anchor.x + std::sin(pendulum.angle) * pendulum.length, 
					pendulum.anchor.y - std::cos(pendulum.angle) * pendulum.length, 
					pendulum.anchor.z}; // 振り子の位置を計算して返す
	
}

Vector3 CirecleMove(const Vector3& center, float radius, float angle) { return Vector3{center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius, center.z}; }

Vector3 SpringForce(const Spring& spring, Vector3 boundPosition,  Vector3 velocity, float mass, Vector3 acceleration, float deltaTIme) { 
	//Vector3 diff = boundPosition - spring.anchor;                                // スプリングのアンカー位置からボールの位置へのベクトル
	//float length = Length(diff);                                                 // ベクトルの長さを計算
	//if (length != 0.0f) {                                                        // ゼロ除算を避けるためのチェック
	//	Vector3 direction = Normalize(diff);                                     // ベクトルを正規化
	//	Vector3 restPosition = spring.anchor + direction * spring.naturalLength; // スプリングの自然長に基づく位置
	//	Vector3 displacement = length * (boundPosition - restPosition);          // ボールの位置と自然長位置の差分
	//	Vector3 restoringForce = -spring.stiffness * displacement;               // スプリングの復元力
	//	Vector3 dumpingForce = -spring.dampingCiefficient * velocity;       // 減衰力
	//	Vector3 force = restoringForce + dumpingForce;                           // 合力を計算
	//	acceleration = force / mass;                                   // ボールの加速度を計算
	//}
	//velocity += acceleration * deltaTIme; // ボールの速度を更新
	//boundPosition += velocity * deltaTIme; // ボールの位置を更新
	return boundPosition;                  // 更新された位置を返す
}
