// CollisionDebug.cpp
// 衝突システムのデバッグ描画と追加デバッグボックス管理

#include <cstdio>
#include "Collision.h"
#include "player3D.h"
#include "Player2D.h"

using namespace DirectX;
using namespace mu;

// 前方宣言
extern XMFLOAT3 GetPlayerSolidCollider();
extern XMFLOAT3 GetPlayerTriggerCollider();
extern bool      Field_IsSolid(FIELD t);
extern bool      Field_IsTrigger(FIELD t);
bool OBB_Intersect_Yaw(
    const XMFLOAT3& cA, const XMFLOAT3& hA, float yawA,
    const XMFLOAT3& cB, const XMFLOAT3& hB, float yawB);

// デバッグオプション（CollisionState.cpp から取得）
extern const ShadowDebugOptions& Collision_GetShadowDebugOpts();

//=========================================================================================================
// 追加デバッグボックス
//=========================================================================================================

struct ExtraDebugBox
{
    bool     isOBB  = false;
    XMFLOAT3 center{};
    XMFLOAT3 half{};
    XMFLOAT3 rotDeg{};
    ImU32    color = 0;
};

static std::vector<ExtraDebugBox> g_ExtraBoxes;

static ImU32 MakeColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    return ((ImU32)a << 24) | ((ImU32)b << 16) | ((ImU32)g << 8) | (ImU32)r;
}

void Collision_DebugClearExtraBoxes()
{
    g_ExtraBoxes.clear();
}

void Collision_DebugAddExtraAABB(const XMFLOAT3& c, const XMFLOAT3& h,
    unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    ExtraDebugBox box;
    box.isOBB  = false;
    box.center = c;
    box.half   = h;
    box.color  = MakeColor(r, g, b, a);
    g_ExtraBoxes.push_back(box);
}

void Collision_DebugAddExtraOBB(const XMFLOAT3& c, const XMFLOAT3& h, const XMFLOAT3& rot,
    unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    ExtraDebugBox box;
    box.isOBB  = true;
    box.center = c;
    box.half   = h;
    box.rotDeg = rot;
    box.color  = MakeColor(r, g, b, a);
    g_ExtraBoxes.push_back(box);
}

//=========================================================================================================
// デバッグ描画メイン関数
//=========================================================================================================

void Collision_DebugDraw()
{
    const ShadowDebugOptions& debugOpts = Collision_GetShadowDebugOpts();
    const ShadowContactState& sc = GetShadowContactState();

    // --- 3Dプレイヤーのコライダー ---
    PLAYER* player3D = GetPlayer3D();
    if (player3D)
    {
        XMFLOAT3 pC = GetPlayerSolidCollider();
        XMFLOAT3 pH = Player3D_GetSolidHalfSize();
        DebugDrawEllipsoid(pC, pH, IM_COL32(0, 255, 0, 255));

        XMFLOAT3 tC = GetPlayerTriggerCollider();
        XMFLOAT3 tH = Player3D_GetTriggerHalfSize();
        DebugDrawOBB_Yaw(tC, tH, player3D->Rotation.y, IM_COL32(255, 255, 255, 255));
    }

    // --- 2Dプレイヤーのコライダー + 影斜面デバッグ ---
    PLAYER* player2D = GetPlayer2D();
    if (player2D && player2D->Active)
    {
        Capsule2D capsule = Player2D_GetCapsule();
        DebugDrawCapsule2D(capsule, IM_COL32(0, 255, 0, 255));

        if (debugOpts.drawStandSegments && sc.standSegValid)
        {
            ImU32 colSeg = sc.standSegWalkable ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 80, 80, 255);
            DrawLine3D(sc.standSegA, sc.standSegB, colSeg, 6.0f);
            DrawPoint3D(sc.standClosestQ, IM_COL32(255, 255, 255, 255), 5.0f);

            XMFLOAT3 p = capsule.center;
            DrawLine3D(p, p + sc.dbgMoveInXZ,  IM_COL32(80, 80, 255, 255), 2.0f);
            DrawLine3D(p, p + sc.dbgMoveOutXZ,  IM_COL32(80, 255, 255, 255), 2.0f);
            DrawLine3D(p, p + sc.dbgMoveSlope, IM_COL32(255, 255, 80, 255), 3.0f);

            float slopeTan = fabsf(sc.standSegAB.y) / (sc.standLenXZ + 1e-6f);
            char buf[160];
            std::snprintf(buf, sizeof(buf), "StandPrism=%d  walkable=%d  slopeTan=%.3f  dY=%.3f",
                sc.lastStandingPrismIndex, (int)sc.standSegWalkable, slopeTan, sc.frameSlopeDeltaY);

            ScreenPt sp = WorldToScreen(sc.standClosestQ);
            if (sp.valid)
                ImGui::GetBackgroundDrawList()->AddText(sp.pos, IM_COL32(255, 255, 255, 255), buf);
        }
    }

    // --- シーンのトリガー当たり判定 ---
    std::vector<MAPDATA>& map = GetFieldMap();
    if (player3D && !map.empty())
    {
        XMFLOAT3 tC = GetPlayerTriggerCollider();
        XMFLOAT3 tH = Player3D_GetTriggerHalfSize();

        for (size_t i = 0; i < map.size(); ++i)
        {
            if (!Field_IsTrigger(map[i].no)) continue;

            XMFLOAT3 boxH = Field_GetCollisionHalfSize(map[i]);
            bool hit = OBB_Intersect_Yaw(tC, tH, player3D->Rotation.y,
                                         map[i].pos, boxH, map[i].rotate.y);
            ImU32 col = hit ? IM_COL32(255, 0, 0, 255) : IM_COL32(0, 255, 255, 255);
            DebugDrawOBB_Yaw(map[i].pos, boxH, map[i].rotate.y, col);
        }
    }

    // --- 影プリズム ---
    const auto& shadowPrisms = Collision_GetShadowPrisms();
    if (!shadowPrisms.empty())
    {
        static const ImU32 prismColors[] = {
            IM_COL32(255, 50,  50,  220),
            IM_COL32(50,  255, 50,  220),
            IM_COL32(50,  50,  255, 220),
            IM_COL32(255, 255, 50,  220),
            IM_COL32(255, 50,  255, 220),
            IM_COL32(50,  255, 255, 220),
        };
        const int colorCount = sizeof(prismColors) / sizeof(prismColors[0]);

        for (size_t i = 0; i < shadowPrisms.size(); ++i)
        {
            if (shadowPrisms[i] && shadowPrisms[i]->isValid)
            {
                ShadowDebugOptions opts = debugOpts;
                opts.prismColor = prismColors[i % colorCount];
                DebugDrawShadowPrism(*shadowPrisms[i], opts);
            }
        }
    }

    // --- 追加デバッグボックス ---
    for (const auto& box : g_ExtraBoxes)
    {
        if (box.isOBB)
            DebugDrawOBB_Yaw(box.center, box.half, box.rotDeg.y, box.color);
        else
            DebugDrawAABB(box.center, box.half, box.color);
    }
}