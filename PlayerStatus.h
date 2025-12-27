#pragma once

#include "Keyboard.h"
#include "controller.h"
#include "Input.h"
#include "direct3d.h"

// コントローラー
extern Controller gPad;

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


// 初期位置
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
float maxFallSpeed = -0.5f;			//最大落下速度
float  dampingXZ = 0.925f;			//摩擦係数
//float gravityPower = 1.0f;		//重力加速度（もしかしたら使う予定）
float jumpPower = 0.175f;			//ジャンプ力
bool isGround = false;				//接地判定

float FirstMaxMoveSpeed = maxMoveSpeed;

inline void DefineInputKeys()
{
	if (!gPad.IsConnected())
	{ // キーボード定義
		// 移動
		static const auto UpKey = KK_W;		//前進
		static const auto RightKey = KK_D;	//右移動
		static const auto DownKey = KK_S;	//後退
		static const auto LeftKey = KK_A;	//左移動
		// 行動
		static auto JumpKey = KK_SPACE;		//ジャンプ
		static auto ActionKey = KK_F;		//アクション
		static auto ChangeKey = KK_F;		//影変身
		// その他
		static auto ResetKey = KK_R;		//リセット
		static auto MenuKey = KK_ESCAPE;	//ポーズメニュー
	}
	else
	{ // コントローラー定義
		// 移動
		//スティック操作はcppに記載
		static const auto UpKey = XINPUT_GAMEPAD_DPAD_UP;		//前進
		static const auto RightKey = XINPUT_GAMEPAD_DPAD_DOWN;	//右移動
		static const auto DownKey = XINPUT_GAMEPAD_DPAD_LEFT;	//後退
		static const auto LeftKey = XINPUT_GAMEPAD_DPAD_RIGHT;	//左移動
		// 行動
		static auto JumpKey = XINPUT_GAMEPAD_A;					//ジャンプ
		static auto ActionKey = XINPUT_GAMEPAD_B;				//アクション
		static auto ChangeKey = XINPUT_GAMEPAD_B;				//影変身
		// その他
		static auto ResetKey = XINPUT_GAMEPAD_BACK;				//リセット
		static auto MenuKey = XINPUT_GAMEPAD_START;				//ポーズメニュー
	}
}

//デバッグモード
static bool debugMode = TRUE;

// プレイヤー当たり判定サイズ
static XMFLOAT3 g_DetectHalfSize = XMFLOAT3(
	PLAYER_DETECT_HALF_X,
	PLAYER_DETECT_HALF_Y,
	PLAYER_DETECT_HALF_Z
);