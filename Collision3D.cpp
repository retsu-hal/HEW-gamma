// Collision3D.cpp
// 3Dプレイヤー vs シーン衝突 + 3Dトリガー判定

#include <cmath>
#include "Collision.h"
#include "player3D.h"

using namespace DirectX;
using namespace mu;

// 前方宣言（CollisionState.cpp / CollisionGeometry.cpp で定義）
extern XMFLOAT3 GetPlayerSolidCollider();
extern XMFLOAT3 GetPlayerTriggerCollider();
extern bool      Field_IsSolid(FIELD t);
extern bool      Field_IsTrigger(FIELD t);
bool OBB_Intersect_Yaw(
    const XMFLOAT3& cA, const XMFLOAT3& hA, float yawA,
    const XMFLOAT3& cB, const XMFLOAT3& hB, float yawB);
bool Resolve_Ellipsoid_OBB_Yaw(
    const XMFLOAT3& ellC, const XMFLOAT3& ellR,
    const XMFLOAT3& boxC, const XMFLOAT3& boxH, float boxYaw,
    XMFLOAT3* outPush, XMFLOAT3* outNormal);

//=========================================================================================================
// トリガー方向計算（3D）
//=========================================================================================================

static TRIGGER_SIDE CalcTriggerSide(const XMFLOAT3& playerC, const XMFLOAT3& targetC)
{
    PLAYER* p = GetPlayer3D();
    if (!p) return TRIGGER_SIDE_NONE;

    float yaw = XMConvertToRadians(p->Rotation.y);
    XMFLOAT3 fwd   = Normalize2D({ sinf(yaw), 0, cosf(yaw) });
    XMFLOAT3 right = { fwd.z, 0, -fwd.x };
    XMFLOAT3 to    = { targetC.x - playerC.x, 0, targetC.z - playerC.z };

    float f = Dot2D(to, fwd);
    float r = Dot2D(to, right);

    if (fabsf(f) < 1e-5f && fabsf(r) < 1e-5f) return TRIGGER_SIDE_NONE;
    if (fabsf(f) >= fabsf(r)) return (f >= 0) ? TRIGGER_SIDE_FRONT : TRIGGER_SIDE_BACK;
    return (r >= 0) ? TRIGGER_SIDE_RIGHT : TRIGGER_SIDE_LEFT;
}

//=========================================================================================================
// 3Dプレイヤー vs シーン（Solid）衝突
//=========================================================================================================

int Player3DField_Collision()
{
    int hit = HIT_NONE;

    PLAYER* player = GetPlayer3D();
    std::vector<MAPDATA>& map = GetFieldMap();
    if (!player || map.empty()) return hit;

    player->isGround = false;
    XMFLOAT3 ellR = Player3D_GetSolidHalfSize();
    XMFLOAT3 ellC = GetPlayerSolidCollider();

    for (size_t i = 0; i < map.size(); ++i)
    {
        if (!Field_IsSolid(map[i].no)) continue;
        if (map[i].no == FIELD_SEESAW_2) continue;

        float boxYaw = (map[i].no == FIELD_OBJ_1) ? map[i].rotate.y : 0.0f;

        XMFLOAT3 push, norm;
        if (!Resolve_Ellipsoid_OBB_Yaw(ellC, ellR, map[i].pos,
            Field_GetCollisionHalfSize(map[i]), boxYaw, &push, &norm))
            continue;

        ellC = ellC + push;

        float ax = fabsf(norm.x), ay = fabsf(norm.y), az = fabsf(norm.z);

        if (ay >= ax && ay >= az)
        {
            if (norm.y > 0) { player->isGround = true; player->Velocity.y = 0; hit = HIT_GROUND; }
            else if (player->Velocity.y > 0) player->Velocity.y = 0;
        }
        else if (ax >= az)
        {
            player->Velocity.x = 0;
            hit = (norm.x > 0) ? HIT_WALL_PlusX : HIT_WALL_NegX;
        }
        else
        {
            player->Velocity.z = 0;
            hit = (norm.z > 0) ? HIT_WALL_PlusZ : HIT_WALL_NegZ;
        }
    }

    player->Position.x = ellC.x;
    player->Position.y = ellC.y - ellR.y;
    player->Position.z = ellC.z;

    return hit;
}

//=========================================================================================================
// 3Dトリガー判定
//=========================================================================================================

bool Collision_PlayerTrigger(TRIGGER_HIT* outHit, float extraRange)
{
    if (outHit) *outHit = TRIGGER_HIT{};

    PLAYER* p = GetPlayer3D();
    if (!p) return false;

    auto& map = GetFieldMap();
    if (map.empty()) return false;

    XMFLOAT3 pHalf = Player3D_GetTriggerHalfSize();
    XMFLOAT3 pC    = GetPlayerTriggerCollider();
    float    pYaw  = p->Rotation.y;

    bool         found  = false;
    float        bestD2 = 1e30f;
    TRIGGER_HIT  best;

    for (size_t i = 0; i < map.size(); ++i)
    {
        if (!Field_IsTrigger(map[i].no)) continue;

        XMFLOAT3 tHalf = Field_GetCollisionHalfSize(map[i]);
        tHalf.x += extraRange; tHalf.y += extraRange; tHalf.z += extraRange;

        if (!OBB_Intersect_Yaw(pC, pHalf, pYaw, map[i].pos, tHalf, map[i].rotate.y))
            continue;

        float dx = map[i].pos.x - pC.x;
        float dz = map[i].pos.z - pC.z;
        float d2 = dx * dx + dz * dz;

        if (!found || d2 < bestD2)
        {
            found  = true;
            bestD2 = d2;
            best.hit      = true;
            best.mapIndex = i;
            best.type     = map[i].no;
            best.side     = CalcTriggerSide(pC, map[i].pos);
        }
    }

    if (!found) return false;
    if (outHit) *outHit = best;
    return true;
}