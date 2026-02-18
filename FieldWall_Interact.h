#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include "field.h"
#include "model.h"

using namespace DirectX;

struct WallParams
{
    XMFLOAT3 scale;
    XMFLOAT3 rotate;
    XMFLOAT3 colliderHalf;
    bool useCustomCollider;

    WallParams()
        : scale{ 1.0f, 1.0f, 1.0f }
        , rotate{ 0.0f, 0.0f, 0.0f }
        , colliderHalf{ 0.5f, 0.5f, 0.5f }
        , useCustomCollider(false)
    {
    }
};

class WallData
{
public:
    XMFLOAT3 pos;
    XMFLOAT3 scale;
    XMFLOAT3 rotate;
    XMFLOAT3 colliderHalf;
    bool useCustomCollider;
    int mapIndex;  // Index into field map data (for collision compatibility)

    WallData()
        : pos{ 0, 0, 0 }
        , scale{ 1, 1, 1 }
        , rotate{ 0, 0, 0 }
        , colliderHalf{ 0.5f, 0.5f, 0.5f }
        , useCustomCollider(false)
        , mapIndex(-1)
    {
    }
};

// Lifecycle
void Wall_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Wall_Finalize();

// Creation / clearing
int  Wall_Create(float x, float y, float z, const WallParams& params, std::vector<MAPDATA>& mapData);
void Wall_ClearAll();

// Per-frame
void Wall_Update();
void Wall_Draw();

// Accessors
std::vector<WallData>& Wall_GetAll();
WallData* Wall_Get(int index);
int Wall_GetCount();

// Debug
void Wall_DebugDraw();
void Wall_SetDebugDraw(bool enable);

