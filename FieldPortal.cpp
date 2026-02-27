// FieldPortal.cpp

#include "FieldPortal.h"

#include "field.h"          // GetFieldMap(), Field_GetCollisionHalfSize()
#include "UtilDebug.h"      // DebugDrawAABB, DrawPoint3D, DrawLine3D

using namespace DirectX;

//==============================================================================
// 内部状態
//==============================================================================

static std::vector<int>        g_PortalEntranceMapIndices; // K の mapIndex（出現順）
static std::vector<XMFLOAT3>   g_PortalExitMarkers;        // J の座標（出現順）
static std::vector<PortalData> g_Portals;                  // K->J（または R）確定後
static XMFLOAT3                g_FallbackR = { 0.0f, 0.0f, 0.0f };    // デバッグ表示用

//==============================================================================
// API
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
    g_PortalEntranceMapIndices.clear();
    g_PortalExitMarkers.clear();
    g_Portals.clear();
    g_FallbackR = { 0.0f, 0.0f, 0.0f };
}

void Portal_RegisterEntranceMapIndex(int mapIndex)
{
    if (mapIndex < 0) return;
    g_PortalEntranceMapIndices.push_back(mapIndex);
}

void Portal_RegisterExitMarker(const XMFLOAT3& pos)
{
    g_PortalExitMarkers.push_back(pos);
}

void Portal_BuildPairs(const XMFLOAT3& fallbackR)
{
    g_Portals.clear();
    g_FallbackR = fallbackR;

    // K の数だけ PortalData を作る（J が足りなければ R へ）
    const int kCount = (int)g_PortalEntranceMapIndices.size();
    const int jCount = (int)g_PortalExitMarkers.size();

    g_Portals.reserve(kCount);
    for (int i = 0; i < kCount; ++i)
    {
        PortalData p;
        p.entranceMapIndex = g_PortalEntranceMapIndices[i];
        if (i < jCount)
        {
            p.destPos = g_PortalExitMarkers[i];
            p.hasExplicitJ = true;
        }
        else
        {
            // J 不足：R へフォールバック
            p.destPos = fallbackR;
            p.hasExplicitJ = false;
        }
        g_Portals.push_back(p);
    }
}

bool Portal_GetDestByEntranceMapIndex(int entranceMapIndex, XMFLOAT3* outDest)
{
    if (!outDest) return false;
    const int n = (int)g_Portals.size();
    for (int i = 0; i < n; ++i)
    {
        if (g_Portals[i].entranceMapIndex == entranceMapIndex)
        {
            *outDest = g_Portals[i].destPos;
            return true;
        }
    }
    return false;
}

const std::vector<PortalData>& Portal_GetAll()
{
    return g_Portals;
}

//==============================================================================
// デバッグ描画
//==============================================================================

void Portal_DebugDraw()
{
    // J マーカー（全て）
    for (size_t i = 0; i < g_PortalExitMarkers.size(); ++i)
    {
        const XMFLOAT3& jpos = g_PortalExitMarkers[i];
        DrawPoint3D(jpos, IM_COL32(0, 255, 120, 255), 5.0f);
    }

    // R（フォールバック）
    DrawPoint3D(g_FallbackR, IM_COL32(255, 255, 255, 255), 6.0f);

    if (g_Portals.empty()) return;

    std::vector<MAPDATA>& mapData = GetFieldMap();

    for (size_t i = 0; i < g_Portals.size(); ++i)
    {
        const PortalData& p = g_Portals[i];
        if (p.entranceMapIndex < 0 || p.entranceMapIndex >= (int)mapData.size())
            continue;

        const MAPDATA& k = mapData[p.entranceMapIndex];
        const XMFLOAT3 kPos = k.pos;

        // K の Trigger 領域（AABB）
        XMFLOAT3 half = Field_GetCollisionHalfSize(k);
        DebugDrawAABB(kPos, half, IM_COL32(255, 200, 50, 255));

        // K 点（黄色）
        DrawPoint3D(kPos, IM_COL32(255, 255, 0, 255), 4.0f);

        // K->Dest（J または R）
        const XMFLOAT3 dPos = p.destPos;
        if (p.hasExplicitJ)
        {
            // 対応する J（シアン）
            DrawPoint3D(dPos, IM_COL32(0, 255, 255, 255), 5.0f);
            DrawLine3D(kPos, dPos, IM_COL32(80, 160, 255, 180), 1.0f);
        }
        else
        {
            // J 不足：R（白）へ
            DrawLine3D(kPos, dPos, IM_COL32(255, 255, 255, 180), 1.0f);
        }
    }
}
