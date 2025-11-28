#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "direct3d.h"
#include "model.h"

//=========================================================================================================
// マクロ定義
//=========================================================================================================
#define PLAYER2D_RADIUS (0.2f)			//半径

//=========================================================================================================
// 構造体
//=========================================================================================================
//プレイヤーステート
enum PLAYER2D_STATE
{
	PLAYER2D_STATE_IDLE = 0,	//何もしない
	PLAYER2D_STATE_MOVE,		//移動中
	PLAYER2D_STATE_FALL,		//落下中
	PLAYER2D_STATE_UP,			//上昇中

	PLAYER2D_STATE_MAX,
};

class PLAYER2D
{
public:
	XMFLOAT3 Position;			//座標
	XMFLOAT3 Rotation;			//回転
	XMFLOAT3 Scaling;			//サイズ
	XMFLOAT3 Velocity;			//方向
	XMFLOAT3 Acceleration;		//加速度
	PLAYER2D_STATE state;		//状態
	MODEL* Model;				//モデルデータ
	XMVECTOR Quaternion;		//4次元回転
};

//=========================================================================================================
// プロトタイプ宣言
//=========================================================================================================
void Player2D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Player2D_Finalize(void);
void Player2D_Update();
void Player2D_Draw(void);

XMFLOAT3 GetPlayer2DPositon();

void Player2D_Move();
void Player2D_Jump();
void Player2D_Change();

PLAYER2D* GetPlayer2D();
