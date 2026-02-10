//Player3D.h
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
#define PLAYER3D_SOLID_HALF_X (0.45f)
#define PLAYER3D_SOLID_HALF_Y (0.9f)
#define PLAYER3D_SOLID_HALF_Z (0.45f)

// トリガー当たり判定サイズ
#define PLAYER3D_TRIGGER_HALF_X (1.0f)
#define PLAYER3D_TRIGGER_HALF_Y (1.0f)
#define PLAYER3D_TRIGGER_HALF_Z (1.0f)

//=========================================================================================================
// プロトタイプ宣言
//=========================================================================================================
void Player3D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Player3D_Finalize(void);
void Player3D_Update();
void Player3D_Draw(void);

XMFLOAT3 GetPlayer3DPosition();

void Player3D_Idle();
void Player3D_Gravity();
void Player3D_Move();
void Player3D_Jump();
void Player3D_Dash();
void Player3D_Change();
void Player3D_Action();
void Player3D_Respawn();
void Player3D_CheckGoal();

PLAYER* GetPlayer3D();

XMFLOAT3 Player3D_GetSolidHalfSize();
XMFLOAT3 Player3D_GetTriggerHalfSize();

XMFLOAT3 Player3D_GetForward();
void     Player3D_InitAt(const XMFLOAT3& pos, const XMFLOAT3& rot);
void     Player3D_SetActive(bool active);
void	 Player3D_setposition(XMFLOAT3 pos);

void Player3D_StartAutoWalk(float targetYawDeg);  // タイトル画面用自動歩行開始
void Player3D_AutoWalk();                         // タイトル画面用自動歩行更新
bool Player3D_IsAutoWalking();                    // 自動歩行中かチェック