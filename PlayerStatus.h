//PlayerStatus.h
#pragma once

#include "Keyboard.h"
#include "controller.h"
#include "Input.h"
#include "model.h"
#include "direct3d.h"

// コントローラー
extern Controller gPad;

enum PLAYER_ANIM {
	PLAYER_ANIM_IDLE = 0,
	PLAYER_ANIM_WALK,
	PLAYER_ANIM_PUSH,

	PLAYER_ANIM_MAX
};

// プレイヤーステート
enum PLAYER_STATE
{
	PLAYER_STATE_IDLE = 0,		//何もしない
	PLAYER_STATE_MOVE,			//移動中
	PLAYER_STATE_FALL,			//落下中
	PLAYER_STATE_JUMP,			//上昇中
	PLAYER_STATE_DASH,			//ダッシュ中
	PLAYER_STATE_RESPAWN,		//変身中
	PLAYER_STATE_ACTION,		//アクション中(3D限定)


	PLAYER_STATE_MAX,
};


// 初期位置
class PLAYER
{
public:
	XMFLOAT3		Firstposition;
	XMFLOAT3		FirstRotation;
	XMFLOAT3		FirstScaling;
	XMFLOAT3		FirstVelocity;
	XMFLOAT3		FirstAcceleration;
	PLAYER_STATE	FirstState;
	PLAYER_ANIM		FirstAnim;
	float			FirstStopTime;
	XMVECTOR		FirstQuaternion;

	XMFLOAT3 Position;			//座標
	XMFLOAT3 Rotation;			//回転
	XMFLOAT3 Scaling;			//サイズ
	XMFLOAT3 Velocity;			//方向
	XMFLOAT3 Acceleration;		//加速度
	PLAYER_STATE state;			//状態
	PLAYER_ANIM CurrentAnimIndex;
	MODEL* Model[PLAYER_ANIM_MAX];	//モデルデータ
	XMVECTOR Quaternion;		//クォータニオン回転

	// プレイヤーステータス
	float moveSpeed = 0.009f;			//移動速度
	float maxMoveSpeed = 0.5f;			//最大移動速度
	float maxFallSpeed = -0.5f;			//最大落下速度
	float dampingXZ = 0.925f;			//摩擦係数
	float gravityPower = -1.0f;			//重力加速度（もしかしたら使う予定）
	float jumpPower = 0.175f;			//ジャンプ力
	float dashMoveSpeed = 2.0f;			//ダッシュ移動速度倍率	
	bool isGround = false;				//接地判定
	bool isDash = false;				//ダッシュ判定
	bool Active = true;
	float FirstMaxMoveSpeed = maxMoveSpeed;

	// プレイヤー変身フラグ
	bool isChange = false;

	bool blockMovement = false; // 移動禁止フラグ

	//  true: 2D状態
	//  false:3D状態
};

struct InputKey
{
	Keyboard_Keys keyboard; // KK_*
	WORD          gamepad;  // XINPUT_GAMEPAD_*
};

