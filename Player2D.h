//Player2D.h
#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "direct3d.h"
#include "model.h"
#include "PlayerStatus.h"
#include "newKeyBind.h"

//=========================================================================================================
// マクロ定義
//=========================================================================================================
// プレイヤー当たり判定サイズ
#define PLAYER2D_SOLID_HALF_X (0.5f)
#define PLAYER2D_SOLID_HALF_Y (1.0f)
#define PLAYER2D_SOLID_HALF_Z (0.07f)


enum PLAYER2D_ANIM
{
    PLAYER2D_ANIM_IDLE = 0, 
    PLAYER2D_ANIM_WALK,     
    PLAYER2D_ANIM_JUMP,     
    PLAYER2D_ANIM_FALL,     

    PLAYER2D_ANIM_MAX
};

struct Player2DAnimDef
{
    const wchar_t* texturePath;
    int cols;                  
    int rows;                  
    int startFrame;            
    int frameCount;            
    float frameSpeed;          
    bool loop;                 
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


void Player2D_Reset();
void Player2D_Respawn();

PLAYER* GetPlayer2D();

XMFLOAT3 Player2D_GetSolidHalfSize();


void Player2D_InitAt(const XMFLOAT3& pos, const XMFLOAT3& rot);
void Player2D_Uninit();
void Player2D_SetActive(bool active);


void Player2D_SetAnim(PLAYER2D_ANIM anim);
PLAYER2D_ANIM Player2D_GetAnim();