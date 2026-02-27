// Collision.h
#pragma once

#include <DirectXMath.h>
#include <vector>

#include "field.h"
#include "ShadowColliderBox.h"
#include "Player2DCapsule.h"
#include "UtilDebug.h"
#include "UtilMath.h"

using namespace DirectX;

//=========================================================================================================
// 列挙型 & 構造体
//=========================================================================================================

enum COLLISION_HIT
{
    HIT_NONE,
    HIT_GROUND,
    HIT_WALL_NegZ,
    HIT_WALL_PlusX,
    HIT_WALL_PlusZ,
    HIT_WALL_NegX,
    HIT_MAX,
};

enum TRIGGER_SIDE
{
    TRIGGER_SIDE_NONE = 0,
    TRIGGER_SIDE_FRONT,
    TRIGGER_SIDE_BACK,
    TRIGGER_SIDE_LEFT,
    TRIGGER_SIDE_RIGHT,
    TRIGGER_SIDE_TOP,
    TRIGGER_SIDE_BOTTOM,
};

struct TRIGGER_HIT
{
    bool         hit = false;
    size_t       mapIndex = 0;
    FIELD        type = FIELD_MAX;
    TRIGGER_SIDE side = TRIGGER_SIDE_NONE;
};

struct OptionRect
{
    float x, y, width, height;
    bool contains(float mousex, float mousey) const
    {
        return mousex >= x && mousex <= x + width &&
            mousey >= y && mousey <= y + height;
    }
};

//=========================================================================================================
// 影コリジョン状態（複数モジュールで共有）
//=========================================================================================================

struct ShadowContactState
{
    // 現在立っているプリズムのインデックス
    int       lastStandingPrismIndex = -1;
    XMFLOAT3  lastShadowTopPos = { 0, 0, 0 };
    int       graceFrames = 0;

    // 立ち方向（XZ平面への投影）
    XMFLOAT3  lastStandDirXZ = { 1, 0, 0 };
    bool      hasStandDir = false;

    // 立ち判定用線分情報
    bool      standSegValid = false;
    bool      standSegWalkable = false;
    XMFLOAT3  standSegA = { 0, 0, 0 };
    XMFLOAT3  standSegB = { 0, 0, 0 };
    XMFLOAT3  standSegAB = { 1, 0, 0 };
    XMFLOAT3  standDirXZ = { 1, 0, 0 };
    float     standLenXZ = 1.0f;
    float     frameSlopeDeltaY = 0.0f;
    XMFLOAT3  standClosestQ = { 0, 0, 0 };

    // デバッグ用移動ベクトル
    XMFLOAT3  dbgMoveInXZ = { 0, 0, 0 };
    XMFLOAT3  dbgMoveOutXZ = { 0, 0, 0 };
    XMFLOAT3  dbgMoveSlope = { 0, 0, 0 };

    void Reset()
    {
        lastStandingPrismIndex = -1;
        lastShadowTopPos = { 0, 0, 0 };
        graceFrames = 0;
        standSegValid = false;
        standSegWalkable = false;
    }
};

ShadowContactState& GetShadowContactState();

//=========================================================================================================
// 影プリズム管理
//=========================================================================================================

void                                    Collision_SetShadowPrisms(const std::vector<const ShadowPrism*>& prisms);
const std::vector<const ShadowPrism*>& Collision_GetShadowPrisms();
void                                    Collision_SetShadowPrism(const ShadowPrism* prism);
const ShadowPrism* Collision_GetShadowPrism();
void                                    Collision_ResetShadowContactState();

//=========================================================================================================
// 3Dプレイヤー衝突
//=========================================================================================================

int  Player3DField_Collision();
bool Collision_PlayerTrigger(TRIGGER_HIT* outHit, float extraRange = 0.0f);

//=========================================================================================================
// 2Dプレイヤー衝突
//=========================================================================================================

int  Collision_Player2D_MoveAndCollision();
int  Player2DField_Collision();
bool Collision_Player2DTrigger(TRIGGER_HIT* outHit, float extraRange = 0.0f);
bool Player2DShadow_Collision();
bool Player2DShadow_TopContact();

//=========================================================================================================
// 幾何ユーティリティ（OBB / カプセル）
//=========================================================================================================

bool Resolve_OBB_OBB_ZY(
    const XMFLOAT3& posA, const XMFLOAT3& halfA, float rotZRadA,
    const XMFLOAT3& posB, const XMFLOAT3& halfB, float rotYDegB,
    XMFLOAT3* outPush, XMFLOAT3* outNorm);

bool OBB_Intersect_ZY(
    const XMFLOAT3& posA, const XMFLOAT3& halfA, float rotZRadA,
    const XMFLOAT3& posB, const XMFLOAT3& halfB, float rotYDegB);

bool Resolve_Capsule2D_OBB(
    const Capsule2D& capsule,
    const XMFLOAT3& boxCenter, const XMFLOAT3& boxHalf, float boxYawDeg,
    XMFLOAT3* outPush, XMFLOAT3* outNormal);

bool Capsule2D_Intersect_OBB(
    const Capsule2D& capsule,
    const XMFLOAT3& boxCenter, const XMFLOAT3& boxHalf, float boxYawDeg);

//=========================================================================================================
// デバッグ描画
//=========================================================================================================

void Collision_SetShadowDebugOptions(const ShadowDebugOptions& options);
void Collision_DebugClearExtraBoxes();
void Collision_DebugAddExtraAABB(const XMFLOAT3& center, const XMFLOAT3& half,
    unsigned char r = 255, unsigned char g = 255, unsigned char b = 0, unsigned char a = 255);
void Collision_DebugAddExtraOBB(const XMFLOAT3& center, const XMFLOAT3& half, const XMFLOAT3& rotDeg,
    unsigned char r = 255, unsigned char g = 255, unsigned char b = 0, unsigned char a = 255);
void Collision_DebugDraw();