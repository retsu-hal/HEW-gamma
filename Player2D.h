#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "direct3D.h"
#include "model.h"

//=========================================================================================================
// マクロ定義
//=========================================================================================================
#define PLAYER2D_SOLID_HALF_X (0.5f)
#define PLAYER2D_SOLID_HALF_Y (1.0f)
#define PLAYER2D_SOLID_HALF_Z (0.07f)
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
	XMFLOAT3 Acceleration;		
	PLAYER2D_STATE state;		//状態

	XMVECTOR Quaternion;

};

//=========================================================================================================
// プロトタイプ宣言
//=========================================================================================================
void Player2D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Player2D_Finalize(void);
void Player2D_Update();
void Player2D_Draw(void);

XMFLOAT3 GetPlayer2DPosition();

void Player2D_Gravity();
void Player2D_Move();


void Player2D_Jump();
void Player2D_Change();
void Player2D_Reset();
void Player2D_Respawn();

PLAYER2D* GetPlayer2D();
XMFLOAT3 Player2D_GetSolidHalfSize();


void Player2D_InitAt(const XMFLOAT3& pos, const XMFLOAT3& rot);
void Player2D_Uninit();
void Player2D_SetActive(bool active);

