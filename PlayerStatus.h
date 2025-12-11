#pragma once

#include "Keyboard.h"
#include "direct3d.h"

// プレイヤー変身フラグ
//  true: 2D状態
//  false:3D状態
bool isChange = false;

// プレイヤーステート
enum PLAYER_STATE
{
	PLAYER_STATE_IDLE = 0,		//何もしない
	PLAYER_STATE_MOVE,			//移動中
	PLAYER_STATE_FALL,			//落下中
	PLAYER_STATE_UP,			//上昇中
	PLAYER_STATE_ACTION,		//アクション中(3D限定)

	PLAYER_STATE_MAX,
};

// プレイヤー当たり判定サイズ
#define PLAYER_DETECT_HALF_X (1.5f)
#define PLAYER_DETECT_HALF_Y (1.0f)
#define PLAYER_DETECT_HALF_Z (1.5f)


// 入力ベクトル
XMFLOAT3 inputDir(0.0f, 0.0f, 0.0f);


// リセット用
XMFLOAT3		Firstposition;
XMFLOAT3		FirstRotation;
XMFLOAT3		FirstScaling;
XMFLOAT3		FirstVelocity;
XMFLOAT3		FirstAcceleration;
PLAYER_STATE	FirstState;
float			FirstStopTime;
XMVECTOR		FirstQuaternion;


// プレイヤーステータス
float moveSpeed = 0.005f;			//移動速度
float maxMoveSpeed = 1.0f;			//最大移動速度
float maxGravity = -0.25f;			//最大落下速度
float jumpPower = 0.175f;			//ジャンプ力
bool isGround = false;				//接地判定


// キーボード定義
// 移動
static const auto UpKey = KK_W;			//前進
static const auto RightKey = KK_D;		//右移動
static const auto DownKey = KK_S;		//後退
static const auto LeftKey = KK_A;		//左移動
// 行動
static const auto JumpKey = KK_SPACE;	//ジャンプ
static const auto ActionKey = KK_F;		//アクション
static const auto ChangeKey = KK_F;		//影変身
// その他
static const auto ResetKey = KK_R;		//リセット
static const auto MenuKey = KK_ESCAPE;	//終了


//デバッグモード
static bool debugMode = TRUE;

// プレイヤー当たり判定サイズ
static XMFLOAT3 g_DetectHalfSize = XMFLOAT3(
	PLAYER_DETECT_HALF_X,
	PLAYER_DETECT_HALF_Y,
	PLAYER_DETECT_HALF_Z
);