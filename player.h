#pragma once

#include <d3d11.h>
//=========================================================================================================
// 構造体
//=========================================================================================================
//プレイヤーステート
enum PIECE_STATE
{
	PIECE_STATE_IDLE = 0,	//何もしない
	PIECE_STATE_MOVE,		//移動中
	PIECE_STATE_GROUND_IDLE,	//着地中
	PIECE_STATE_MISS_IDLE,		//ミス発生

};

//落下ブロック
class PIECE
{
	public:
		XMFLOAT2		Position;	//表示座標
		int				Type[3];	//色種別
		PIECE_STATE		State;		//状態

		int				StateCount;	//ステート切り替わりウェイト

};

//=========================================================================================================
// プロトタイプ宣言
//=========================================================================================================
void Player_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Player_Finalize(void);
void Player_Update();
void Player_Draw(void);
void Player_Create();	//新しいブロックを作る
void Player_Move();		//移動処理

