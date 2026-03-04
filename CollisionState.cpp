// CollisionState.cpp
// グローバル共有状態、影プリズム管理、Field型判定

#include "Collision.h"
#include "player3D.h"
#include "Player2D.h"

using namespace DirectX;
using namespace mu;

//=========================================================================================================
// グローバル共有状態（散在していた静的変数を一元管理）
//=========================================================================================================

static std::vector<const ShadowPrism*> g_ShadowPrisms;
static ShadowDebugOptions              g_ShadowDebugOpts;
static ShadowContactState              g_ShadowContact;

ShadowContactState& GetShadowContactState()
{
    return g_ShadowContact;
}

//=========================================================================================================
// 影プリズムリスト管理
//=========================================================================================================

void Collision_SetShadowPrisms(const std::vector<const ShadowPrism*>& prisms)
{
    g_ShadowPrisms = prisms;
}

const std::vector<const ShadowPrism*>& Collision_GetShadowPrisms()
{
    return g_ShadowPrisms;
}

void Collision_SetShadowPrism(const ShadowPrism* prism)
{
    g_ShadowPrisms.clear();
    if (prism && prism->isValid)
        g_ShadowPrisms.push_back(prism);
}

const ShadowPrism* Collision_GetShadowPrism()
{
    return g_ShadowPrisms.empty() ? nullptr : g_ShadowPrisms[0];
}

void Collision_ResetShadowContactState()
{
    g_ShadowContact.Reset();
}

//=========================================================================================================
// デバッグオプション
//=========================================================================================================

void Collision_SetShadowDebugOptions(const ShadowDebugOptions& options)
{
    g_ShadowDebugOpts = options;
}

const ShadowDebugOptions& Collision_GetShadowDebugOpts()
{
    return g_ShadowDebugOpts;
}

//=========================================================================================================
// Field型判定
//=========================================================================================================

bool Field_IsSolid(FIELD t)
{
    switch (t)
    {
    case FIELD_GROUND:
    case FIELD_WALL:
    case FIELD_OBJ_BOX:
    case FIELD_EMPTY_BOX:
    case FIELD_OBJ_1:
    case FIELD_SEESAW_1:
    case FIELD_SEESAW_2:
    case FIELD_MANHOLE:
    case FIELD_OBJ_2:
    case FIELD_OBJ_3:
        return true;
    default:
        return false;
    }
}

bool Field_IsTrigger(FIELD t)
{
    switch (t)
    {
    case FIELD_GOAL:
    case FIELD_OBJ_1:
	case FIELD_WALL:
    case FIELD_STAGE_1:
    case FIELD_STAGE_2:
    case FIELD_STAGE_3:

	case FIELD_PORTAL_K:
    case FIELD_PORTAL_J:

	case FIELD_FOUNTAIN:
        return true;
    default:
        return false;
    }
}

//=========================================================================================================
// プレイヤーコライダー中心座標の取得
//=========================================================================================================

XMFLOAT3 GetPlayerSolidCollider()
{
    PLAYER* p = GetPlayer3D();
    XMFLOAT3 c = p->Position;
    c.y += Player3D_GetSolidHalfSize().y;
    return c;
}

XMFLOAT3 GetPlayer2DSolidCollider()
{
    PLAYER* p = GetPlayer2D();
    XMFLOAT3 c = p->Position;
    c.y += Player2D_GetSolidHalfSize().y + 0.1f;
    return c;
}

XMFLOAT3 GetPlayerTriggerCollider()
{
    PLAYER* p = GetPlayer3D();
    XMFLOAT3 c = p->Position;
    c.y += Player3D_GetTriggerHalfSize().y;
    return c;
}