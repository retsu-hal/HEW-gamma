//Player2D.h
#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "direct3d.h"
#include "model.h"
#include "PlayerStatus.h"
#include "newKeyBind.h"
#include "Player2DCapsule.h"

//=========================================================================================================
// マクロ定義
//=========================================================================================================


// プレイヤーの当たり判定用の半サイズ
#define PLAYER2D_SOLID_HALF_X (0.5f)
#define PLAYER2D_SOLID_HALF_Y (1.0f)
#define PLAYER2D_SOLID_HALF_Z (0.07f)



enum PLAYER2D_ANIME// アニメーションの種類
{
    PLAYER2D_ANIME_IDLE = 0, 
    PLAYER2D_ANIME_WALK,     
    PLAYER2D_ANIME_JUMP,     
    PLAYER2D_ANIME_JUMP_AIR,
    PLAYER2D_ANIME_FALL,     

    PLAYER2D_ANIME_MAX
};

struct Player2DAnimeDef// アニメーション定義
{
    const wchar_t* texturePath;
    int cols;                  // 列数
    int rows;                  // 行数
    int startFrame;            // アニメ開始フレーム
    int frameCount;            // フレーム数
    float frameSpeed;          // フレーム切り替え速度（秒）
    bool loop;                 // ループするか
};


//=========================================================================================================
// プロトタイプ宣言
//=========================================================================================================
void Player2D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Player2D_Finalize(void);
void Player2D_Update();
void Player2D_Draw(void);

DirectX::XMFLOAT3 GetPlayer2DPosition();

void Player2D_Gravity();
void Player2D_Move();
void Player2D_Jump();


void Player2D_Reset();
void Player2D_Respawn();

PLAYER* GetPlayer2D();


Capsule2D Player2D_GetCapsule();
DirectX::XMFLOAT3 Player2D_GetSolidHalfSize();


void Player2D_InitAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& rot);
void Player2D_Uninit();
void Player2D_SetActive(bool active);


void Player2D_SetAnime(PLAYER2D_ANIME anime);
PLAYER2D_ANIME Player2D_GetAnime();