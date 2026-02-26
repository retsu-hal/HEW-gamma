// Collision2D.cpp
// 2Dプレイヤー vs シーン衝突 + 2Dトリガー + 影側壁衝突 + 影上面接触 + 移動統合

#include <cfloat>
#include <cmath>
#include <algorithm>
#include "Collision.h"
#include "Player2D.h"

using namespace DirectX;
using namespace mu;

// 前方宣言
extern XMFLOAT3 GetPlayer2DSolidCollider();
extern bool      Field_IsSolid(FIELD t);
extern bool      Field_IsTrigger(FIELD t);

//=========================================================================================================
// トリガー方向計算（2D）
//=========================================================================================================

static TRIGGER_SIDE CalcTriggerSide2D(const XMFLOAT3& playerPos, float playerRotZ, const XMFLOAT3& triggerPos)
{
    float dx = triggerPos.x - playerPos.x;
    float dy = triggerPos.y - playerPos.y;

    float radZ = XMConvertToRadians(playerRotZ);
    float cosZ = cosf(-radZ);
    float sinZ = sinf(-radZ);

    float localX = dx * cosZ - dy * sinZ;
    float localY = dx * sinZ + dy * cosZ;

    if (fabsf(localX) > fabsf(localY))
        return (localX > 0) ? TRIGGER_SIDE_RIGHT : TRIGGER_SIDE_LEFT;
    else
        return (localY > 0) ? TRIGGER_SIDE_TOP : TRIGGER_SIDE_BOTTOM;
}

//=========================================================================================================
// XZ平面の内積（影衝突用）
//=========================================================================================================

static float DotXZ(const XMFLOAT3& a, const XMFLOAT3& b)
{
    return a.x * b.x + a.z * b.z;
}

static float ProjectRadiusOnAxis_Yaw(const XMFLOAT3& axis, const XMFLOAT3& half, float yawDeg)
{
    const float yaw   = XMConvertToRadians(yawDeg);
    const XMFLOAT3 right = { cosf(yaw), 0.0f, -sinf(yaw) };
    const XMFLOAT3 fwd   = { sinf(yaw), 0.0f,  cosf(yaw) };
    const XMFLOAT3 up    = { 0.0f, 1.0f, 0.0f };

    return fabsf(Dot(axis, right)) * half.x
         + fabsf(Dot(axis, up))    * half.y
         + fabsf(Dot(axis, fwd))   * half.z;
}

//=========================================================================================================
// 2Dプレイヤー vs シーン（Solid）衝突
//=========================================================================================================

int Player2DField_Collision()
{
    int hit = HIT_NONE;

    PLAYER* player = GetPlayer2D();
    std::vector<MAPDATA>& map = GetFieldMap();
    if (!player || map.empty()) return hit;

    player->isGround = false;
    Capsule2D capsule = Player2D_GetCapsule();
    const int MAX_ITERATIONS = 4;

    for (int iter = 0; iter < MAX_ITERATIONS; ++iter)
    {
        bool hitThisIter = false;

        for (size_t i = 0; i < map.size(); ++i)
        {
            if (!Field_IsSolid(map[i].no)) continue;

            XMFLOAT3 push, norm;
            if (!Resolve_Capsule2D_OBB(capsule, map[i].pos,
                Field_GetCollisionHalfSize(map[i]), map[i].rotate.y, &push, &norm))
                continue;

            if (fabsf(push.x) > 1e-8f || fabsf(push.y) > 1e-8f || fabsf(push.z) > 1e-8f)
            {
                capsule.center = capsule.center + push;
                hitThisIter = true;
            }

            float ax = fabsf(norm.x), ay = fabsf(norm.y), az = fabsf(norm.z);

            if (ay >= ax && ay >= az)
            {
                if (norm.y > 0)
                    { player->isGround = true; player->Velocity.y = 0; hit = HIT_GROUND; }
                else if (player->Velocity.y > 0)
                    player->Velocity.y = 0;
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

        if (!hitThisIter) break;
    }

    float totalHeight = PLAYER2D_CAPSULE_HEIGHT + PLAYER2D_CAPSULE_RADIUS * 2.0f;
    player->Position.x = capsule.center.x;
    player->Position.y = capsule.center.y - totalHeight * 0.5f;
    player->Position.z = capsule.center.z;

    return hit;
}

//=========================================================================================================
// 2Dトリガー判定
//=========================================================================================================

bool Collision_Player2DTrigger(TRIGGER_HIT* outHit, float extraRange)
{
    if (outHit) *outHit = TRIGGER_HIT{};

    PLAYER* p = GetPlayer2D();
    if (!p) return false;

    auto& map = GetFieldMap();
    if (map.empty()) return false;

    Capsule2D capsule = Player2D_GetCapsule();
    Capsule2D expandedCapsule = capsule;
    expandedCapsule.radius += extraRange;

    bool        found  = false;
    float       bestD2 = 1e30f;
    TRIGGER_HIT best;

    for (size_t i = 0; i < map.size(); ++i)
    {
        if (!Field_IsTrigger(map[i].no)) continue;

        XMFLOAT3 tHalf = Field_GetCollisionHalfSize(map[i]);
        if (!Capsule2D_Intersect_OBB(expandedCapsule, map[i].pos, tHalf, map[i].rotate.y))
            continue;

        float dx = map[i].pos.x - capsule.center.x;
        float dy = map[i].pos.y - capsule.center.y;
        float d2 = dx * dx + dy * dy;

        if (!found || d2 < bestD2)
        {
            found  = true;
            bestD2 = d2;
            best.hit      = true;
            best.mapIndex = (int)i;
            best.type     = map[i].no;
            best.side     = CalcTriggerSide2D(capsule.center, p->Rotation.z, map[i].pos);
        }
    }

    if (!found) return false;
    if (outHit) *outHit = best;
    return true;
}

//=========================================================================================================
// 2D影側壁衝突（速度クリップ）
//=========================================================================================================

bool Player2DShadow_Collision()
{
    PLAYER* player = GetPlayer2D();
    if (!player) return false;

    const auto& prisms = Collision_GetShadowPrisms();
    if (prisms.empty()) return false;

    ShadowContactState& sc = GetShadowContactState();
    Capsule2D capsule = Player2D_GetCapsule();

    const float skin = 0.01f;
    const float eps  = 1e-6f;

    XMFLOAT3 dXZ = { player->Velocity.x, 0.0f, player->Velocity.z };
    bool hitAny = false;

    sc.frameSlopeDeltaY = 0.0f;
    sc.dbgMoveInXZ  = dXZ;
    sc.dbgMoveOutXZ = dXZ;
    sc.dbgMoveSlope = { 0, 0, 0 };

    // 斜面上に立っている場合、移動を立ち方向へ投影する
    if (sc.hasStandDir && sc.lastStandingPrismIndex >= 0 && player->Velocity.y <= 0.02f)
    {
        float dl = sqrtf(dXZ.x * dXZ.x + dXZ.z * dXZ.z);
        if (dl > 1e-6f)
        {
            float d = dXZ.x * sc.standDirXZ.x + dXZ.z * sc.standDirXZ.z;
            dXZ.x = sc.standDirXZ.x * d;
            dXZ.z = sc.standDirXZ.z * d;
        }
    }

    // ローカルlambdaユーティリティ
    auto Area2 = [](const std::vector<XMFLOAT2>& p) -> float {
        float a = 0.0f;
        int n = (int)p.size();
        for (int i = 0, j = n - 1; i < n; j = i++)
            a += p[j].x * p[i].y - p[i].x * p[j].y;
        return a;
    };
    auto Dot2 = [](const XMFLOAT2& a, const XMFLOAT2& b) { return a.x * b.x + a.y * b.y; };
    auto Sub2 = [](const XMFLOAT2& a, const XMFLOAT2& b) -> XMFLOAT2 { return { a.x - b.x, a.y - b.y }; };
    auto Len2 = [](const XMFLOAT2& v) { return sqrtf(v.x * v.x + v.y * v.y); };

    auto CapsuleProjection = [&](const XMFLOAT3& axis) -> float {
        XMFLOAT3 capAxis = { -sinf(capsule.rotationZ), cosf(capsule.rotationZ), 0.0f };
        return capsule.radius + fabsf(Dot(axis, capAxis)) * capsule.halfHeight;
    };

    for (int prismIdx = 0; prismIdx < (int)prisms.size(); ++prismIdx)
    {
        const ShadowPrism* prism = prisms[prismIdx];
        if (!prism || !prism->isValid) continue;
        if (prism->poly.size() < 3)    continue;

        // 現在立っているプリズムはスキップ（TopContactで処理済み）
        if (sc.standSegValid && sc.standSegWalkable &&
            prismIdx == sc.lastStandingPrismIndex && player->Velocity.y <= 0.02f)
            continue;

        // 上向きの面はスキップ（地面であり壁ではない）
        if (prism->n.y > 0.70f) continue;

        const float moveLen = sqrtf(dXZ.x * dXZ.x + dXZ.z * dXZ.z);
        const float gate    = capsule.radius + capsule.halfHeight + moveLen + 0.3f;

        // AABBによる粗判定
        if (capsule.center.x < prism->aabbMin.x - gate || capsule.center.x > prism->aabbMax.x + gate ||
            capsule.center.z < prism->aabbMin.z - gate || capsule.center.z > prism->aabbMax.z + gate)
            continue;

        std::vector<XMFLOAT2> poly = prism->poly;
        if (Area2(poly) < 0.0f) std::reverse(poly.begin(), poly.end());

        const XMFLOAT3 rel = capsule.center - prism->origin;
        const float u0 = Dot(rel, prism->u);
        const float v0 = Dot(rel, prism->v);
        const float n0 = Dot(rel, prism->n);
        const XMFLOAT2 p0 = { u0, v0 };

        const float rN = CapsuleProjection(prism->n);
        if (!(n0 >= -rN && n0 <= prism->thickness + rN)) continue;

        {
            const float bottomN = n0 - rN;
            if (prism->n.y > 0.7f && bottomN >= prism->thickness - 0.02f) continue;
        }

        if (moveLen < eps) continue;

        XMFLOAT2 dp = { DotXZ(dXZ, prism->u), DotXZ(dXZ, prism->v) };
        if (fabsf(dp.x) + fabsf(dp.y) < 1e-7f) continue;

        // カプセルが多角形の拡張領域内にあるか判定
        bool insideExp0 = true;
        for (int i = 0; i < (int)poly.size(); ++i)
        {
            const XMFLOAT2 a = poly[i];
            const XMFLOAT2 b = poly[(i + 1) % (int)poly.size()];
            const XMFLOAT2 e = { b.x - a.x, b.y - a.y };

            XMFLOAT2 nIn = { -e.y, e.x };
            float nl = Len2(nIn);
            if (nl < 1e-6f) continue;
            nIn.x /= nl; nIn.y /= nl;

            const XMFLOAT3 axisW = prism->u * nIn.x + prism->v * nIn.y;
            const float rEdge = CapsuleProjection(axisW) + skin;
            const float s0 = Dot2(nIn, Sub2(p0, a));
            if (s0 < -rEdge) { insideExp0 = false; break; }
        }

        if (insideExp0)
        {
            // 内部にいる：各辺で速度をクリップ
            for (int pass = 0; pass < 3; ++pass)
            {
                XMFLOAT2 dpNow = { DotXZ(dXZ, prism->u), DotXZ(dXZ, prism->v) };
                if (fabsf(dpNow.x) + fabsf(dpNow.y) < 1e-7f) break;

                bool changed = false;
                for (int i = 0; i < (int)poly.size(); ++i)
                {
                    const XMFLOAT2 a = poly[i];
                    const XMFLOAT2 b = poly[(i + 1) % (int)poly.size()];
                    const XMFLOAT2 e = { b.x - a.x, b.y - a.y };

                    XMFLOAT2 nIn = { -e.y, e.x };
                    float nl = Len2(nIn);
                    if (nl < 1e-6f) continue;
                    nIn.x /= nl; nIn.y /= nl;

                    const XMFLOAT3 axisW = prism->u * nIn.x + prism->v * nIn.y;
                    const float rEdge = CapsuleProjection(axisW) + skin;
                    const float s0    = Dot2(nIn, Sub2(p0, a));
                    const float slack = s0 + rEdge;

                    if (slack > 0.05f) continue;

                    const float vIn = Dot2(nIn, dpNow);
                    if (vIn <= 0.0f) continue;

                    XMFLOAT3 axisXZ = { axisW.x, 0.0f, axisW.z };
                    const float len2xz = axisXZ.x * axisXZ.x + axisXZ.z * axisXZ.z;
                    if (len2xz < 1e-6f) continue;

                    const float proj = (dXZ.x * axisXZ.x + dXZ.z * axisXZ.z) / len2xz;
                    if (proj > 0.0f)
                    {
                        float clipAmount = proj;
                        if (slack > 0.0f)
                        {
                            float blend = 1.0f - (slack / 0.05f);
                            clipAmount *= Clamp(blend, 0.0f, 1.0f);
                        }
                        dXZ.x -= axisXZ.x * clipAmount;
                        dXZ.z -= axisXZ.z * clipAmount;
                        changed = true;
                        hitAny  = true;
                    }
                }
                if (!changed) break;
            }
        }
        else
        {
            // 外部にいる：スイープテスト（侵入時刻を求める）
            float tEnter = 0.0f;
            float tExit  = 1.0f;
            bool  canEnter = true;

            for (int i = 0; i < (int)poly.size(); ++i)
            {
                const XMFLOAT2 a = poly[i];
                const XMFLOAT2 b = poly[(i + 1) % (int)poly.size()];
                const XMFLOAT2 e = { b.x - a.x, b.y - a.y };

                XMFLOAT2 nIn = { -e.y, e.x };
                float nl = Len2(nIn);
                if (nl < 1e-6f) continue;
                nIn.x /= nl; nIn.y /= nl;

                const XMFLOAT3 axisW = prism->u * nIn.x + prism->v * nIn.y;
                const float rEdge = CapsuleProjection(axisW) + skin;
                const float s0 = Dot2(nIn, Sub2(p0, a));
                const float sv = Dot2(nIn, dp);

                if (fabsf(sv) < 1e-6f)
                {
                    if (s0 < -rEdge) { canEnter = false; break; }
                    continue;
                }

                float t = (-rEdge - s0) / sv;

                if (sv > 0.0f) { if (t > tEnter) tEnter = t; }
                else           { if (t < tExit)  tExit  = t; }

                if (tEnter > tExit) { canEnter = false; break; }
            }

            if (canEnter && tEnter >= 0.0f && tEnter <= 1.0f)
            {
                const float tStop = (std::max)(0.0f, tEnter - 1e-4f);
                dXZ.x *= tStop;
                dXZ.z *= tStop;
                hitAny = true;
            }
        }
    }

    sc.dbgMoveOutXZ    = dXZ;
    sc.frameSlopeDeltaY = 0.0f;

    if (sc.standSegValid && sc.standSegWalkable && sc.hasStandDir &&
        sc.lastStandingPrismIndex >= 0 && player->Velocity.y <= 0.02f && sc.standLenXZ > 1e-6f)
    {
        float amount = dXZ.x * sc.standDirXZ.x + dXZ.z * sc.standDirXZ.z;
        sc.frameSlopeDeltaY = sc.standSegAB.y * (amount / sc.standLenXZ);
    }
    sc.dbgMoveSlope = { dXZ.x, sc.frameSlopeDeltaY, dXZ.z };

    player->Velocity.x = dXZ.x;
    player->Velocity.z = dXZ.z;

    return hitAny;
}

//=========================================================================================================
// 2D影上面接触（立ち判定 + 斜面歩行）
//=========================================================================================================

bool Player2DShadow_TopContact()
{
    PLAYER* player = GetPlayer2D();
    if (!player) return false;

    const auto& prisms = Collision_GetShadowPrisms();
    if (prisms.empty()) return false;

    ShadowContactState& sc = GetShadowContactState();
    Capsule2D capsule = Player2D_GetCapsule();

    const float kSkin          = 0.01f;
    const float kStandWidth    = capsule.radius + 0.12f;
    const float kContactUp     = 0.25f;
    const float kContactDown   = -(capsule.radius + 0.45f);
    const float kJumpEscapeVel = 0.02f;
    const bool  isRising       = (player->Velocity.y > kJumpEscapeVel);
    const float kMaxWalkSlopeTan = 57.0f;

    auto Dot3   = [](const XMFLOAT3& a, const XMFLOAT3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; };
    auto LenSqXZ = [](const XMFLOAT3& v) { return v.x * v.x + v.z * v.z; };

    auto CapsuleProjection = [&](const XMFLOAT3& axisW) -> float {
        XMFLOAT3 capAxis = { -sinf(capsule.rotationZ), cosf(capsule.rotationZ), 0.0f };
        return capsule.radius + fabsf(Dot(axisW, capAxis)) * capsule.halfHeight;
    };

    // カプセルのAABB（粗判定用）
    XMFLOAT3 capHalf = capsule.GetBoundingHalfSize();
    const float aabbPad = 0.30f;
    XMFLOAT3 capMin = capsule.center - (capHalf + XMFLOAT3(aabbPad, aabbPad, aabbPad));
    XMFLOAT3 capMax = capsule.center + (capHalf + XMFLOAT3(aabbPad, aabbPad, aabbPad));

    auto AABBOverlap = [](const XMFLOAT3& aMin, const XMFLOAT3& aMax,
                          const XMFLOAT3& bMin, const XMFLOAT3& bMax) -> bool {
        return (aMin.x <= bMax.x && aMax.x >= bMin.x) &&
               (aMin.y <= bMax.y && aMax.y >= bMin.y) &&
               (aMin.z <= bMax.z && aMax.z >= bMin.z);
    };

    const float footY     = player->Position.y;
    const float footYPrev = footY - player->Velocity.y - sc.frameSlopeDeltaY;

    int      bestIdx     = -1;
    float    bestAbs     = 1e9f;
    float    bestGroundY = 0.0f;
    XMFLOAT3 bestDirXZ   = { 1, 0, 0 };
    bool     bestHasDir  = false;
    XMFLOAT3 bestSegA{}, bestSegB{}, bestSegQ{}, bestSegAB = { 1, 0, 0 };
    float    bestLenXZ   = 1.0f;
    bool     bestWalkable = false;

    for (int prismIdx = 0; prismIdx < (int)prisms.size(); ++prismIdx)
    {
        const ShadowPrism* prism = prisms[prismIdx];
        if (!prism || !prism->isValid) continue;
        if (prism->baseWorld.size() < 2) continue;

        XMFLOAT3 pMin = prism->aabbMin - XMFLOAT3(aabbPad, aabbPad, aabbPad);
        XMFLOAT3 pMax = prism->aabbMax + XMFLOAT3(aabbPad, aabbPad, aabbPad);
        if (!AABBOverlap(capMin, capMax, pMin, pMax)) continue;

        XMFLOAT3 rel = capsule.center - prism->origin;
        float n0 = Dot3(rel, prism->n);
        float rN = CapsuleProjection(prism->n);
        const float slabTol = 0.05f;
        if (n0 < -rN - slabTol || n0 > prism->thickness + rN + slabTol) continue;

        float sliceN = Clamp(n0, 0.0f, prism->thickness);

        auto ProcessSeg = [&](XMFLOAT3 a, XMFLOAT3 b)
        {
            a = a + prism->n * sliceN;
            b = b + prism->n * sliceN;

            XMFLOAT3 e = b - a;
            if (LenSqXZ(e) < 1e-5f) return;

            XMFLOAT3 p  = capsule.center;
            XMFLOAT3 ab = b - a;
            float abxz2  = ab.x * ab.x + ab.z * ab.z;
            if (abxz2 < 1e-6f) return;

            float lenXZ   = sqrtf(abxz2);
            float slopeTan = fabsf(ab.y) / (lenXZ + 1e-6f);
            bool  walkable = (slopeTan <= kMaxWalkSlopeTan);

            float t = ((p.x - a.x) * ab.x + (p.z - a.z) * ab.z) / abxz2;
            t = Clamp(t, 0.0f, 1.0f);

            XMFLOAT3 q   = a + ab * t;
            float dx      = p.x - q.x;
            float dz      = p.z - q.z;
            float distXZ  = sqrtf(dx * dx + dz * dz);
            if (distXZ > kStandWidth) return;

            float groundY      = q.y;
            float distToGround = footY - groundY;

            if (isRising && distToGround > 0.0f) return;

            bool crossed = (footYPrev >= groundY + kSkin) && (footY <= groundY + kSkin);
            if (!crossed)
            {
                if (distToGround < kContactDown || distToGround > kContactUp) return;
            }

            float absDist = fabsf(distToGround);
            if (absDist < bestAbs)
            {
                bestAbs     = absDist;
                bestIdx     = prismIdx;
                bestGroundY = groundY;
                bestSegA    = a;
                bestSegB    = b;
                bestSegQ    = q;
                bestSegAB   = ab;
                bestLenXZ   = lenXZ;
                bestWalkable = walkable;

                XMFLOAT3 dir = { ab.x, 0.0f, ab.z };
                float dl = sqrtf(dir.x * dir.x + dir.z * dir.z);
                if (dl > 1e-6f) { dir.x /= dl; dir.z /= dl; bestDirXZ = dir; bestHasDir = true; }
                else            { bestHasDir = false; }
            }
        };

        if (!prism->standSegments.empty())
        {
            for (const auto& s : prism->standSegments)
                ProcessSeg(s.a, s.b);
        }
        else
        {
            const int nV = (int)prism->baseWorld.size();
            for (int i = 0; i < nV; ++i)
                ProcessSeg(prism->baseWorld[i], prism->baseWorld[(i + 1) % nV]);
        }
    }

    bool hitAny = false;

    if (bestIdx >= 0)
    {
        float targetY       = bestGroundY + kSkin;
        player->Position.y  = targetY;
        if (player->Velocity.y < 0.0f) player->Velocity.y = 0.0f;
        player->isGround = true;
        hitAny = true;

        sc.lastStandingPrismIndex = bestIdx;
        sc.graceFrames            = 0;
        sc.lastShadowTopPos       = { player->Position.x, targetY, player->Position.z };

        sc.standSegValid    = true;
        sc.standSegWalkable = bestWalkable;
        sc.standSegA        = bestSegA;
        sc.standSegB        = bestSegB;
        sc.standSegAB       = bestSegAB;
        sc.standClosestQ    = bestSegQ;
        sc.standLenXZ       = (bestLenXZ > 1e-6f) ? bestLenXZ : 1.0f;

        sc.hasStandDir = bestHasDir;
        if (bestHasDir)
        {
            sc.standDirXZ     = bestDirXZ;
            sc.lastStandDirXZ = bestDirXZ;
        }
    }
    else
    {
        // Coyote time
        const int GRACE = 6;
        if (!isRising && sc.lastStandingPrismIndex >= 0)
        {
            sc.graceFrames++;
            if (sc.graceFrames <= GRACE)
            {
                player->isGround = true;
                hitAny = true;
                if (player->Velocity.y < 0.0f) player->Velocity.y = 0.0f;
            }
            else
            {
                sc.lastStandingPrismIndex = -1;
                sc.graceFrames    = 0;
                sc.standSegValid  = false;
                sc.standSegWalkable = false;
            }
        }
        else
        {
            sc.lastStandingPrismIndex = -1;
            sc.graceFrames    = 0;
            sc.standSegValid  = false;
            sc.standSegWalkable = false;
        }
    }

    return hitAny;
}

//=========================================================================================================
// 2D移動統合（速度→位置→衝突→上面接触）
//=========================================================================================================

int Collision_Player2D_MoveAndCollision()
{
    PLAYER* player = GetPlayer2D();
    if (!player) return HIT_NONE;

    ShadowContactState& sc = GetShadowContactState();

    Player2DShadow_Collision();

    player->Position.x += player->Velocity.x;
    player->Position.y += player->Velocity.y;
    player->Position.z += player->Velocity.z;
    player->Position.y += sc.frameSlopeDeltaY;

    int hit = Player2DField_Collision();

    Player2DShadow_TopContact();

    return hit;
}