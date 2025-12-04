#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "direct3d.h"
#include "model.h"

//=========================================================================================================
// マクロ定義
//=========================================================================================================
#define PLAYER3D_RADIUS (0.2f)			//半径

//=========================================================================================================
// 構造体
//=========================================================================================================
//プレイヤーステート
enum PLAYER3D_STATE
{
	PLAYER3D_STATE_IDLE = 0,	//何もしない
	PLAYER3D_STATE_MOVE,		//移動中
	PLAYER3D_STATE_FALL,		//落下中
	PLAYER3D_STATE_UP,			//上昇中
	PLAYER3D_STATE_ACTION,		//アクション中

	PLAYER3D_STATE_MAX,
};

class PLAYER3D
{
public:
	XMFLOAT3 Position;			//座標
	XMFLOAT3 Rotation;			//回転
	XMFLOAT3 Scaling;			//サイズ
	XMFLOAT3 Velocity;			//方向
	XMFLOAT3 Acceleration;		//
	PLAYER3D_STATE state;		//状態
	MODEL* Model;				//モデルデータ
	XMVECTOR Quaternion;		//

};

//=========================================================================================================
// プロトタイプ宣言
//=========================================================================================================
void Player3D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Player3D_Finalize(void);
void Player3D_Update();
void Player3D_Draw(void);

XMFLOAT3 GetPlayer3DPositon();

void Player3D_Gravity();
void Player3D_Move();
void Player3D_Jump();
void Player3D_Change();
void Player3D_Action();
void Player3D_Reset();
void Player3D_Respown();

PLAYER3D* GetPlayer3D();



#define PLAYER3D_DETECT_HALF_X (1.5f)
#define PLAYER3D_DETECT_HALF_Y (1.0f)
#define PLAYER3D_DETECT_HALF_Z (1.5f)

bool     Player3D_IsNearPoint(const XMFLOAT3& point);
XMFLOAT3 Player3D_GetDetectHalfSize();
