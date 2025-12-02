#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include "direct3d.h"
#include "model.h"
//=========================================================================================================
// マクロ定義
//=========================================================================================================
#define BALL_RADIUS (0.2f)			//半径

//=========================================================================================================
// 列挙
//=========================================================================================================
enum BALL_STATE
{
	BALL_STATE_IDLE=0,
	BALL_STATE_MOVE,					//移動
	BALL_STATE_DIRECTION,			//方向指示
	BALL_STATE_POWER,				//威力
	BALL_STATE_RESPAWN,				//復活

	BALL_STATE_MAX,

};

//=========================================================================================================
// 構造体宣言
//=========================================================================================================
class BALL
{
public:
	XMFLOAT3 Position;			//座標
	XMFLOAT3 Rotation;			//回転
	XMFLOAT3 Scaling;			//サイズ
	XMFLOAT3 Velocity;			//方向
	XMFLOAT3 Acceleration;	//
	BALL_STATE state;			//状態
	MODEL* Model;					//モデルデータ
	XMVECTOR Quaternion;	//
	XMVECTOR Axis;				//
	float Speed;						//
};

//=========================================================================================================
//プロトタイプ宣言
//=========================================================================================================
void Ball_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Ball_Finalize();
void Ball_Update();
void Ball_Draw();
XMFLOAT3 GetBallPositon();
void Ball_Idle();
void Ball_Move();
void Ball_Power();
void Ball_Direction();
void Ball_Respawn();
BALL* GetBall();