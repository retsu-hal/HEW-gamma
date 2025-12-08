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
// 列挙
//=========================================================================================================
enum PLAYER3D_STATE
{
	PLAYER3D_IDLE=0,
	PLAYER3D_MOVE,					//移動
	PLAYER3D_RESPAWN,				//復活

	PLAYER3D_MAX,

};

//=========================================================================================================
// 構造体宣言
//=========================================================================================================
class PLAYER3D
{
public:
	XMFLOAT3 Position;			//座標
	XMFLOAT3 Rotation;			//回転
	XMFLOAT3 Scaling;			//サイズ
	XMFLOAT3 Velocity;			//方向
	XMFLOAT3 Acceleration;	//
	PLAYER3D_STATE state;			//状態
	MODEL* Model;					//モデルデータ
	XMVECTOR Quaternion;	//
	XMVECTOR Axis;				//
	float Speed;						//
};

//=========================================================================================================
//プロトタイプ宣言
//=========================================================================================================
void Player3D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Player3D_Finalize();
void Player3D_Update();
void Player3D_Draw();
XMFLOAT3 GetPlayer3DPositon();
void Player3D_Idle();
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

XMFLOAT3 Player3D_GetDetectHalfSize();
void Player3D_Respawn();
PLAYER3D* GetPlayer3D();

