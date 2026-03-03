// FieldPortal.cpp

#include "FieldPortal.h"

#include "field.h"          // GetFieldMap(), Field_GetCollisionHalfSize()
#include "UtilDebug.h"      // DebugDrawAABB, DrawPoint3D, DrawLine3D

using namespace DirectX;

//==============================================================================
// 内部状態
//
//   マップ読込中に K と J の mapIndex を収集し、
//   読込完了後に読み込み順で 1:1 にペアリングする。
//
//   記号の意味（マップ文字）
//     K : ポータル入口（トリガー）
//     J : ポータル出口（トリガー）
//==============================================================================
static std::vector<int>        g_EntranceIndices;   // K（入口）の mapIndex 一覧（読込順）
static std::vector<int>        g_ExitIndices;        // J（出口）の mapIndex 一覧（読込順）
static std::vector<PortalData> g_Portals;            // K-J ペアテーブル

//==============================================================================
// 初期化 / 終了処理
//==============================================================================

void Portal_Initialize()
{
    Portal_ClearAll();
}

void Portal_Finalize()
{
    Portal_ClearAll();
}

void Portal_ClearAll()
{
    g_EntranceIndices.clear();
    g_ExitIndices.clear();
    g_Portals.clear();
}

//==============================================================================
// マップロード中の登録
//==============================================================================

void Portal_RegisterEntranceMapIndex(int mapIndex)
{
    if (mapIndex < 0) return;
    g_EntranceIndices.push_back(mapIndex);
}

void Portal_RegisterExitMapIndex(int mapIndex)
{
    if (mapIndex < 0) return;
    g_ExitIndices.push_back(mapIndex);
}

//==============================================================================
// ペア構築（マップロード完了後に呼ぶ）
//   K と J を読み込み順で 1:1 にペアリングする。
//   数が合わない場合は少ない方に合わせる（余りは無視）。
//==============================================================================

void Portal_BuildPairs()
{
    g_Portals.clear();

    const int kCount = (int)g_EntranceIndices.size();
    const int jCount = (int)g_ExitIndices.size();
    const int pairCount = (kCount < jCount) ? kCount : jCount;

    g_Portals.reserve(pairCount);
    for (int i = 0; i < pairCount; ++i)
    {
        PortalData p;
        p.kMapIndex = g_EntranceIndices[i];
        p.jMapIndex = g_ExitIndices[i];
        p.activated = false;
        g_Portals.push_back(p);
    }
}

//==============================================================================
// 転送クエリ
//
//   プレイヤーが触れたトリガーの mapIndex を受け取り、
//   転送先の座標を outDest に書き込む。
//
//   K に触れた場合 → J の座標を返す（常に可能）
//   J に触れた場合 → activated == true なら K の座標を返す
//                     activated == false なら転送不可（false を返す）
//==============================================================================

bool Portal_GetDestForTrigger(int mapIndex, XMFLOAT3* outDest)
{
    if (!outDest) return false;

    std::vector<MAPDATA>& mapData = GetFieldMap();

    const int n = (int)g_Portals.size();
    for (int i = 0; i < n; ++i)
    {
        PortalData& p = g_Portals[i];

        // K に触れた → J へ転送（常に可能）
        if (p.kMapIndex == mapIndex)
        {
            if (p.jMapIndex < 0 || p.jMapIndex >= (int)mapData.size())
                return false;

            *outDest = mapData[p.jMapIndex].pos;
            outDest->y += 0.1f;    // 地面めり込み防止
            return true;
        }

        // J に触れた → activated なら K へ転送
        if (p.jMapIndex == mapIndex)
        {
            if (!p.activated)
                return false;       // 未激活：転送不可

            if (p.kMapIndex < 0 || p.kMapIndex >= (int)mapData.size())
                return false;

            *outDest = mapData[p.kMapIndex].pos;
            outDest->y += 0.1f;    // 地面めり込み防止
            return true;
        }
    }

    return false;
}

//==============================================================================
// ペア激活（K→J 転送実行後に呼ぶ）
//==============================================================================

void Portal_ActivatePair(int mapIndex)
{
    const int n = (int)g_Portals.size();
    for (int i = 0; i < n; ++i)
    {
        if (g_Portals[i].kMapIndex == mapIndex)
        {
            g_Portals[i].activated = true;
            return;
        }
    }
}

//==============================================================================
// 内部データ参照
//==============================================================================

const std::vector<PortalData>& Portal_GetAll()
{
    return g_Portals;
}

//==============================================================================
// デバッグ描画
//==============================================================================

void Portal_DebugDraw()
{
    if (g_Portals.empty()) return;

    std::vector<MAPDATA>& mapData = GetFieldMap();

    for (size_t i = 0; i < g_Portals.size(); ++i)
    {
        const PortalData& p = g_Portals[i];

        // --- K（入口）の描画 ---
        if (p.kMapIndex >= 0 && p.kMapIndex < (int)mapData.size())
        {
            const MAPDATA& kData = mapData[p.kMapIndex];
            const XMFLOAT3 kPos = kData.pos;

            // K のトリガー AABB
            XMFLOAT3 half = Field_GetCollisionHalfSize(kData);
            DebugDrawAABB(kPos, half, IM_COL32(255, 200, 50, 255));

            // K の位置（黄色）
            DrawPoint3D(kPos, IM_COL32(255, 255, 0, 255), 4.0f);
        }

        // --- J（出口）の描画 ---
        if (p.jMapIndex >= 0 && p.jMapIndex < (int)mapData.size())
        {
            const MAPDATA& jData = mapData[p.jMapIndex];
            const XMFLOAT3 jPos = jData.pos;

            // J のトリガー AABB
            XMFLOAT3 half = Field_GetCollisionHalfSize(jData);
            DebugDrawAABB(jPos, half, IM_COL32(50, 200, 255, 255));

            // J の位置（シアン）
            DrawPoint3D(jPos, IM_COL32(0, 255, 255, 255), 4.0f);
        }

        // --- K → J の接続線 ---
        if (p.kMapIndex >= 0 && p.kMapIndex < (int)mapData.size() &&
            p.jMapIndex >= 0 && p.jMapIndex < (int)mapData.size())
        {
            const XMFLOAT3 kPos = mapData[p.kMapIndex].pos;
            const XMFLOAT3 jPos = mapData[p.jMapIndex].pos;

            if (p.activated)
            {
                // 激活済み（双方向）：緑線
                DrawLine3D(kPos, jPos, IM_COL32(0, 255, 0, 200), 2.0f);
            }
            else
            {
                // 未激活（K→J 一方通行）：白線
                DrawLine3D(kPos, jPos, IM_COL32(255, 255, 255, 150), 1.0f);
            }
        }
    }
}