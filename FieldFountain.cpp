// FieldFountain.cpp
#include "FieldFountain.h"

#include "player3D.h"
#include "PlayerStatus.h"
#include "Collision.h"
#include "UtilDebug.h"
#include "UtilMath.h"
#include "newKeyBind.h"
#include "Input.h"

#include <cmath>

using namespace DirectX;
using namespace mu;

//=========================================================================================================
// 内部設定（モデル配置の定義）
//=========================================================================================================

struct FountainPartConfig
{
    FIELD    partType;
    XMFLOAT3 offsetPos;  // 配置オフセット（マップ座標→実座標）
    XMFLOAT3 scale;      // 表示スケール
    XMFLOAT3 rotate;     // 表示回転
};

static const FountainPartConfig FOUNTAIN_CONFIG =
{
    FIELD_FOUNTAIN,
    XMFLOAT3(0.0f, -0.5f, 0.0f),           // Reasonable offset to align with ground
    XMFLOAT3(0.005f, 0.005f, 0.005f),
    XMFLOAT3(90.0f, 0.0f, 90.0f),
};

//=========================================================================================================
// 内部状態
//=========================================================================================================

static std::vector<FountainData> g_Fountains;

// 空中ブースト状態（プレイヤーは1人なのでグローバルで管理）
static bool  g_Boosting = false;
static int   g_BoostFramesLeft = 0;
static float g_BoostAccelY = 0.0f;


//=========================================================================================================
// 初期化 / 終了
//=========================================================================================================

void Fountain_Initialize()
{
    Fountain_ClearAll();
}

void Fountain_Finalize()
{
    Fountain_ClearAll();
}

void Fountain_ClearAll()
{
    g_Fountains.clear();
    g_Boosting = false;
    g_BoostFramesLeft = 0;
    g_BoostAccelY = 0.0f;
}

//=========================================================================================================
// 生成（MapData に MAPDATA を追加し mapIndex を保持）
//=========================================================================================================

int Fountain_Create(float x, float y, float z, std::vector<MAPDATA>& mapData)
{
    FountainData fountain;

    fountain.basePos = XMFLOAT3(x, y, z); // store original map position

    MAPDATA data;
    // Apply offset to the visual/collision position
    data.pos = XMFLOAT3(
        x + FOUNTAIN_CONFIG.offsetPos.x,
        y + FOUNTAIN_CONFIG.offsetPos.y,
        z + FOUNTAIN_CONFIG.offsetPos.z
    );
    data.no = FOUNTAIN_CONFIG.partType;
    data.scale = FOUNTAIN_CONFIG.scale;
    data.rotate = FOUNTAIN_CONFIG.rotate;

    data.useCustomCollider = true;
    data.colliderHalf = XMFLOAT3(
        fountain.params.triggerHalfX,
        fountain.params.triggerHalfY,
        fountain.params.triggerHalfZ
    );

    fountain.mapIndex = (int)mapData.size();
    mapData.push_back(data);

    int index = (int)g_Fountains.size();
    g_Fountains.push_back(fountain);
    return index;
}



//=========================================================================================================
// 参照
//=========================================================================================================

std::vector<FountainData>& Fountain_GetAll() { return g_Fountains; }

FountainData* Fountain_Get(int index)
{
    if (index < 0 || index >= (int)g_Fountains.size())
        return nullptr;
    return &g_Fountains[index];
}

int Fountain_GetCount()
{
    return (int)g_Fountains.size();
}

//=========================================================================================================
// 更新
//=========================================================================================================

void Fountain_UpdateAll(float deltaTime)
{
    // 空中ブースト処理
    if (g_Boosting)
    {
        PLAYER* p = GetPlayer3D();
        if (p)
        {
            if (!p->isGround && g_BoostFramesLeft > 0)
            {
                // 上向き加速度
                p->Velocity.y += g_BoostAccelY;

                // XZ方向にも若干の推進力（向いている方向）
                XMFLOAT3 fwd = Player3D_GetForward();
                float xzPush = p->moveSpeed * 0.5f;  // 空中は地上の半分
                p->Velocity.x += fwd.x * xzPush;
                p->Velocity.z += fwd.z * xzPush;

                g_BoostFramesLeft--;
            }
            else
            {
                // 着地またはフレーム切れでブースト終了
                g_Boosting = false;
                g_BoostFramesLeft = 0;
            }
        }
    }
}
//=========================================================================================================
// プレイヤーとの噴水インタラクション
//   Player3D_CheckGoal() から呼ばれる想定
//   噴水の判定盒内でジャンプキーを押した瞬間に強化ジャンプを発動
//=========================================================================================================

void Fountain_PlayerInteraction()
{
    PLAYER* p = GetPlayer3D();
    if (!p) return;

    extern Controller gPad;
    if (!IsInputTrigger(JumpKey, gPad)) return;

    if (!p->isGround) return;

    // Use the hit information already known from Player3D_CheckGoal
    // Find the nearest fountain (or use all)
    XMFLOAT3 playerPos = p->Position;
    std::vector<MAPDATA>& mapData = GetFieldMap();

    for (size_t i = 0; i < g_Fountains.size(); ++i)
    {
        const FountainData& ft = g_Fountains[i];
        if (ft.mapIndex < 0 || ft.mapIndex >= (int)mapData.size())
            continue;

        const MAPDATA& md = mapData[ft.mapIndex];
        XMFLOAT3 half = Field_GetCollisionHalfSize(md);
        XMFLOAT3 pHalf = Player3D_GetTriggerHalfSize();

        float dx = fabsf(playerPos.x - md.pos.x);
        float dy = fabsf(playerPos.y - md.pos.y);
        float dz = fabsf(playerPos.z - md.pos.z);

        if (dx > (half.x + pHalf.x) ||
            dy > (half.y + pHalf.y) ||
            dz > (half.z + pHalf.z))
            continue;

        // === Enhanced jump ===
        ModelResetAnimation(p->Model[PLAYER_ANIM_JUMP]);
        p->CurrentAnimIndex = PLAYER_ANIM_JUMP;

        p->Velocity.y = p->jumpPower * ft.params.boostMultiplier;

        XMFLOAT3 fwd = Player3D_GetForward();
        float xzBoost = p->moveSpeed * ft.params.boostMultiplier;
        p->Velocity.x += fwd.x * xzBoost;
        p->Velocity.z += fwd.z * xzBoost;

        p->isGround = false;
        // Set state to MOVE and mark that jump was consumed
        p->state = PLAYER_STATE_MOVE;
        p->fountainJumped = true;  // ← NEW FLAG: prevent normal jump override

        // Start aerial boost
        g_Boosting = true;
        g_BoostFramesLeft = ft.params.boostFrames;
        g_BoostAccelY = ft.params.boostAccelY;

        return;
    }
}//=========================================================================================================
// デバッグ描画
//=========================================================================================================

void Fountain_DebugDraw()
{
    if (g_Fountains.empty()) return;

    std::vector<MAPDATA>& mapData = GetFieldMap();

    for (size_t i = 0; i < g_Fountains.size(); ++i)
    {
        const FountainData& ft = g_Fountains[i];

        if (ft.mapIndex < 0 || ft.mapIndex >= (int)mapData.size())
            continue;

        const MAPDATA& data = mapData[ft.mapIndex];

        // トリガー判定盒を描画
        XMFLOAT3 half = Field_GetCollisionHalfSize(data);
        DebugDrawAABB(data.pos, half, IM_COL32(0, 150, 255, 200));

        // 噴水位置
        DrawPoint3D(data.pos, IM_COL32(0, 200, 255, 255), 5.0f);

        // 基準位置
        DrawPoint3D(ft.basePos, IM_COL32(100, 100, 255, 255), 3.0f);

        // ブースト状態表示
        if (g_Boosting)
        {
            XMFLOAT3 topPos = data.pos;
            topPos.y += 3.0f;
            DrawLine3D(data.pos, topPos, IM_COL32(0, 255, 100, 200), 2.0f);
        }
    }
}

