// FieldManhole.cpp

#include "FieldManhole.h"
#include "player3D.h"
#include "Collision.h"
#include <cmath>
#include "camera.h"
#include "direct3d.h"

#include "debug.h"
static bool g_ManholeDebugDraw = true;

struct ManholePartConfig
{
    FIELD partType;
    XMFLOAT3 offsetPos;
    XMFLOAT3 scale;
    XMFLOAT3 rotate;
    //XMFLOAT3 colliderHalf;
    //bool useCustomCollider;
};

static const ManholePartConfig MANHOLE_CONFIG =
{
    FIELD_MANHOLE,
    XMFLOAT3(0.0f, -0.5f, 0.0f),
    XMFLOAT3(1.0f, 0.3f, 1.0f),
    XMFLOAT3(0.0f, 0.0f, 0.0f),
    //XMFLOAT3(0.5f, 0.15f, 0.5f),
    //true                        
};

static std::vector<ManholeData> g_Manholes;
static float g_TotalTime = 0.0f;

void Manhole_SetDebugDraw(bool enable)
{
    g_ManholeDebugDraw = enable;
}

void Manhole_Initialize()
{
    g_Manholes.clear();
    g_TotalTime = 0.0f;
}

void Manhole_Finalize()
{
    g_Manholes.clear();
}

void Manhole_ClearAll()
{
    g_Manholes.clear();
    g_TotalTime = 0.0f;
}

int Manhole_Create(float x, float y, float z, std::vector<MAPDATA>& mapData)
{
    ManholeData manhole;

    manhole.basePos = XMFLOAT3(
        x + MANHOLE_CONFIG.offsetPos.x,
        y + MANHOLE_CONFIG.offsetPos.y,
        z + MANHOLE_CONFIG.offsetPos.z
    );
    manhole.currentPos = XMFLOAT3(x, y, z);

    MAPDATA data;
    data.pos = XMFLOAT3(x, y, z);
    data.no = MANHOLE_CONFIG.partType;
    data.scale = MANHOLE_CONFIG.scale;
    data.rotate = MANHOLE_CONFIG.rotate;

    manhole.mapIndex = (int)mapData.size();
    mapData.push_back(data);

    int index = (int)g_Manholes.size();
    g_Manholes.push_back(manhole);
    return index;
}

std::vector<ManholeData>& Manhole_GetAll()
{
    return g_Manholes;
}

ManholeData* Manhole_Get(int index)
{
    if (index < 0 || index >= (int)g_Manholes.size())
        return nullptr;
    return &g_Manholes[index];
}

int Manhole_GetCount()
{
    return (int)g_Manholes.size();
}

static void UpdateSingleManhole(int index, float deltaTime)
{
    if (index < 0 || index >= (int)g_Manholes.size())
        return;

    ManholeData& manhole = g_Manholes[index];
    ManholeParams& params = manhole.params;

    params.phase += params.speed * deltaTime;

    const float PI2 = 6.28318530718f;
    while (params.phase > PI2)
        params.phase -= PI2;

    float offset = params.amplitude * (1.0f - cosf(params.phase + params.phaseOffset)) * 0.5f;

    manhole.currentPos.y = manhole.basePos.y + offset;

    std::vector<MAPDATA>& mapData = GetFieldMap();
    if (manhole.mapIndex >= 0 && manhole.mapIndex < (int)mapData.size())
    {
        mapData[manhole.mapIndex].pos.y = manhole.currentPos.y;
    }
}

void Manhole_UpdateAll(float deltaTime)
{
    g_TotalTime += deltaTime;

    for (int i = 0; i < (int)g_Manholes.size(); ++i)
    {
        UpdateSingleManhole(i, deltaTime);
    }
}

static ImVec2 ManholeWorldToScreen(const XMFLOAT3& p, bool* valid)
{
    *valid = false;

    XMMATRIX vp = GetViewMatrix() * GetProjectionMatrix();
    XMVECTOR vW = XMVectorSet(p.x, p.y, p.z, 1.0f);
    XMVECTOR vV = XMVector3TransformCoord(vW, GetViewMatrix());

    if (XMVectorGetZ(vV) <= 0.01f) return ImVec2(0, 0);

    XMVECTOR vC = XMVector3TransformCoord(vW, vp);
    XMFLOAT3 ndc;
    XMStoreFloat3(&ndc, vC);

    if (ndc.x < -1.5f || ndc.x > 1.5f || ndc.y < -1.5f || ndc.y > 1.5f)
        return ImVec2(0, 0);

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float w = (float)Direct3D_GetBackBufferWidth();
    float h = (float)Direct3D_GetBackBufferHeight();

    *valid = true;
    return ImVec2(
        viewport->Pos.x + (ndc.x * 0.5f + 0.5f) * w,
        viewport->Pos.y + (-ndc.y * 0.5f + 0.5f) * h
    );
}

static void ManholeDrawLine3D(const XMFLOAT3& a, const XMFLOAT3& b, ImU32 col, float thick = 1.0f)
{
    bool va, vb;
    ImVec2 sa = ManholeWorldToScreen(a, &va);
    ImVec2 sb = ManholeWorldToScreen(b, &vb);
    if (va && vb)
        ImGui::GetBackgroundDrawList()->AddLine(sa, sb, col, thick);
}

static void ManholeDrawAABB(const XMFLOAT3& center, const XMFLOAT3& half, ImU32 col)
{
    XMFLOAT3 corners[8] = {
        {center.x - half.x, center.y - half.y, center.z - half.z},
        {center.x + half.x, center.y - half.y, center.z - half.z},
        {center.x + half.x, center.y + half.y, center.z - half.z},
        {center.x - half.x, center.y + half.y, center.z - half.z},
        {center.x - half.x, center.y - half.y, center.z + half.z},
        {center.x + half.x, center.y - half.y, center.z + half.z},
        {center.x + half.x, center.y + half.y, center.z + half.z},
        {center.x - half.x, center.y + half.y, center.z + half.z},
    };

    const int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0},
        {4,5},{5,6},{6,7},{7,4},
        {0,4},{1,5},{2,6},{3,7}
    };

    for (int i = 0; i < 12; i++)
        ManholeDrawLine3D(corners[edges[i][0]], corners[edges[i][1]], col, 2.0f);
}

static void ManholeDrawPoint3D(const XMFLOAT3& p, ImU32 col, float size = 4.0f)
{
    bool valid;
    ImVec2 sp = ManholeWorldToScreen(p, &valid);
    if (valid)
        ImGui::GetBackgroundDrawList()->AddCircleFilled(sp, size, col);
}

void Manhole_DebugDraw()
{
    if (!g_ManholeDebugDraw) return;
    if (g_Manholes.empty()) return;

    std::vector<MAPDATA>& mapData = GetFieldMap();

    for (int i = 0; i < (int)g_Manholes.size(); ++i)
    {
        const ManholeData& manhole = g_Manholes[i];

        if (manhole.mapIndex < 0 || manhole.mapIndex >= (int)mapData.size())
            continue;

        const MAPDATA& data = mapData[manhole.mapIndex];

        XMFLOAT3 half = {
            MANHOLE_CONFIG.scale.x * BOX_RADIUS,
            MANHOLE_CONFIG.scale.y * BOX_RADIUS,
            MANHOLE_CONFIG.scale.z * BOX_RADIUS
        };

        ManholeDrawAABB(data.pos, half, IM_COL32(255, 200, 50, 255));

        ManholeDrawPoint3D(manhole.basePos, IM_COL32(0, 255, 255, 255), 5.0f);

        ManholeDrawPoint3D(data.pos, IM_COL32(255, 255, 0, 255), 4.0f);

        XMFLOAT3 lowPos = manhole.basePos; 
        XMFLOAT3 highPos = manhole.basePos;
        highPos.y += manhole.params.amplitude;
        ManholeDrawLine3D(lowPos, highPos, IM_COL32(100, 100, 255, 180), 1.0f);
    }
}