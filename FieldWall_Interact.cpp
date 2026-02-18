#include "FieldWall_Interact.h"
#include "Camera.h"
#include "shader.h"
#include "direct3d.h"
#include "debug.h"

#include <vector>

//=========================================================================================================
// Static / globals
//=========================================================================================================
static std::vector<WallData> g_Walls;
static MODEL* g_WallModel = nullptr;
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;
static bool g_WallDebugDraw = true;

//=========================================================================================================
// Debug draw helpers (same pattern as FieldManhole / FieldSeesaw)
//=========================================================================================================
static ImVec2 WallWorldToScreen(const XMFLOAT3& p, bool* valid)
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

static void WallDrawLine3D(const XMFLOAT3& a, const XMFLOAT3& b, ImU32 col, float thick = 1.0f)
{
    bool va, vb;
    ImVec2 sa = WallWorldToScreen(a, &va);
    ImVec2 sb = WallWorldToScreen(b, &vb);
    if (va && vb)
        ImGui::GetBackgroundDrawList()->AddLine(sa, sb, col, thick);
}

static void WallDrawOBB(const XMFLOAT3& center, const XMFLOAT3& half,
    const XMFLOAT3& rotDeg, ImU32 col)
{
    XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(
        XMConvertToRadians(rotDeg.x),
        XMConvertToRadians(rotDeg.y),
        XMConvertToRadians(rotDeg.z)
    );

    XMFLOAT3 localCorners[8] = {
        {-half.x, -half.y, -half.z}, {+half.x, -half.y, -half.z},
        {+half.x, +half.y, -half.z}, {-half.x, +half.y, -half.z},
        {-half.x, -half.y, +half.z}, {+half.x, -half.y, +half.z},
        {+half.x, +half.y, +half.z}, {-half.x, +half.y, +half.z},
    };

    XMFLOAT3 worldCorners[8];
    for (int i = 0; i < 8; i++)
    {
        XMVECTOR vLocal = XMLoadFloat3(&localCorners[i]);
        XMVECTOR vWorld = XMVector3TransformNormal(vLocal, rotMat);
        XMFLOAT3 rotated;
        XMStoreFloat3(&rotated, vWorld);
        worldCorners[i] = XMFLOAT3(
            center.x + rotated.x,
            center.y + rotated.y,
            center.z + rotated.z
        );
    }

    const int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0},
        {4,5},{5,6},{6,7},{7,4},
        {0,4},{1,5},{2,6},{3,7}
    };

    for (int i = 0; i < 12; i++)
        WallDrawLine3D(worldCorners[edges[i][0]], worldCorners[edges[i][1]], col, 2.0f);
}

//=========================================================================================================
// Lifecycle
//=========================================================================================================
void Wall_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    g_pDevice = pDevice;
    g_pContext = pContext;
    g_Walls.clear();

    if (!g_WallModel)
    {
        g_WallModel = ModelLoad("asset\\model\\Kabe.fbx");
    }
}

void Wall_Finalize()
{
    g_Walls.clear();

    if (g_WallModel)
    {
        ModelRelease(g_WallModel);
        g_WallModel = nullptr;
    }
}

int Wall_Create(float x, float y, float z, const WallParams& params, std::vector<MAPDATA>& mapData)
{
    WallData wall;
    wall.pos = XMFLOAT3(x, y, z);
    wall.scale = params.scale;
    wall.rotate = params.rotate;
    wall.colliderHalf = params.colliderHalf;
    wall.useCustomCollider = params.useCustomCollider;

    MAPDATA data;
    data.pos = wall.pos;
    data.no = FIELD_OBJ_1;  // Å© skip in field_Draw; Wall_Draw handles rendering
    data.scale = wall.scale;
    data.rotate = wall.rotate;

    // Set correct collider size from the start
    if (params.useCustomCollider)
    {
        data.colliderHalf = params.colliderHalf;
        data.useCustomCollider = true;
    }
    else
    {
        data.colliderHalf = XMFLOAT3(
            BOX_RADIUS * params.scale.x,
            BOX_RADIUS * params.scale.y,
            BOX_RADIUS * params.scale.z
        );
        data.useCustomCollider = false;
    }

    wall.mapIndex = (int)mapData.size();
    mapData.push_back(data);

    int index = (int)g_Walls.size();
    g_Walls.push_back(wall);
    return index;
}

void Wall_ClearAll()
{
    g_Walls.clear();
}

std::vector<WallData>& Wall_GetAll()
{
    return g_Walls;
}

WallData* Wall_Get(int index)
{
    if (index < 0 || index >= (int)g_Walls.size())
        return nullptr;
    return &g_Walls[index];
}

int Wall_GetCount()
{
    return (int)g_Walls.size();
}

void Wall_Update()
{
    std::vector<MAPDATA>& mapData = GetFieldMap();

    for (int i = 0; i < (int)g_Walls.size(); ++i)
    {
        WallData& wall = g_Walls[i];
        if (wall.mapIndex < 0 || wall.mapIndex >= (int)mapData.size())
            continue;

        mapData[wall.mapIndex].pos = wall.pos;
        mapData[wall.mapIndex].scale = wall.scale;
        mapData[wall.mapIndex].rotate = wall.rotate;

        // Always recompute colliderHalf from scale so collision matches visual
        if (wall.useCustomCollider)
        {
            mapData[wall.mapIndex].colliderHalf = wall.colliderHalf;
            mapData[wall.mapIndex].useCustomCollider = true;
        }
        else
        {
            // Derive half-extents from scale (model is unit-size, BOX_RADIUS = 0.5)
            mapData[wall.mapIndex].colliderHalf = XMFLOAT3(
                BOX_RADIUS * wall.scale.x,
                BOX_RADIUS * wall.scale.y,
                BOX_RADIUS * wall.scale.z
            );
            mapData[wall.mapIndex].useCustomCollider = false;
        }
    }
}


void Wall_Draw()
{
    if (!g_WallModel) return;

    Shader_Begin();
    XMMATRIX Projection = GetProjectionMatrix();
    XMMATRIX View = GetViewMatrix();
    XMMATRIX VP = View * Projection;

    for (int i = 0; i < (int)g_Walls.size(); ++i)
    {
        const WallData& wall = g_Walls[i];

        XMMATRIX ScalingMatrix = XMMatrixScaling(
            wall.scale.x, wall.scale.y, wall.scale.z);

        XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(
            XMConvertToRadians(wall.rotate.x),
            XMConvertToRadians(wall.rotate.y),
            XMConvertToRadians(wall.rotate.z));

        XMMATRIX TranslationMatrix = XMMatrixTranslation(
            wall.pos.x, wall.pos.y, wall.pos.z);

        XMMATRIX WorldMatrix = ScalingMatrix * RotationMatrix * TranslationMatrix;
        XMMATRIX WVP = WorldMatrix * VP;

        Shader_SetWorldMatrix(WorldMatrix);
        Shader_SetMatrix(WVP);

        ModelDraw(g_WallModel);
    }
}

void Wall_SetDebugDraw(bool enable)
{
    g_WallDebugDraw = enable;
}

void Wall_DebugDraw()
{
    if (!g_WallDebugDraw) return;
    if (g_Walls.empty()) return;

    DEBUG_IMGUI_BEGIN({
        for (int i = 0; i < (int)g_Walls.size(); ++i)
        {
            const WallData& wall = g_Walls[i];

            XMFLOAT3 half;
            if (wall.useCustomCollider)
            {
                half = wall.colliderHalf;
            }
            else
            {
                half = XMFLOAT3(
                    BOX_RADIUS * wall.scale.x,
                    BOX_RADIUS * wall.scale.y,
                    BOX_RADIUS * wall.scale.z
                );
            }

            WallDrawOBB(wall.pos, half, wall.rotate, IM_COL32(0, 150, 255, 255));
        }
        });
}
