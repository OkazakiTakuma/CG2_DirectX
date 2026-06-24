#pragma once
#include "Matrix.h"
#include "Screen.h"
#include "Vector.h"

struct Spring {

	Vector3 anchor;           //!< スプリングのアンカー位置
	float naturalLength;      //!< スプリングの自然長
	float stiffness;          //!< スプリングのばね定数
	float dampingCiefficient; //!< スプリングの減衰係数
};

struct Ball {
	Vector3 position;     //!< ボールの位置
	Vector3 velocity;     //!< ボールの速度
	Vector3 acceleration; //!< ボールの加速度
	float mass;           //!< ボールの質量
	float radius;         //!< ボールの半径
	unsigned int color;   //!< ボールの色
};

struct Pendulum {
	Vector3 anchor;           //!< 振り子のアンカー位置
	float length;             //!< 振り子の長さ
	float angle;              //!< 振り子の角度
	float angularVelocity;    //!< 振り子の角速度
	float angularAcceleration; //!< 振り子の角加速度
};

struct  ConicalPendulum {
	Vector3 anchor;           //!< 円錐振り子のアンカー位置
	float length;             //!< 円錐振り子の長さ
	float harfApexAngle;      //!< 円錐振り子の頂角の半分
	float angle;              //!< 円錐振り子の角度
	float angularVelocity;    //!< 円錐振り子の角速度
};

Vector3 PendulumMove(const Pendulum& pendulum);

Vector3 CirecleMove(const Vector3& center, float radius, float angle);

Vector3 SpringForce(const Spring& spring, Vector3 boundPosition, Vector3 velocity, float mass, Vector3 acceleration, float deltaTIme);
