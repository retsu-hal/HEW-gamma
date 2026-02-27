// FieldManhole.h
#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include "field.h"

//=========================================================================================================
// マンホール（上下する足場）パラメータ
//=========================================================================================================
struct ManholeParams
{
    float amplitude;     // 上下振幅（Y方向）
    float speed;         // 進行速度（phase増加量/秒）
    float phase;         // 位相（ラジアン）
    float phaseOffset;   // 位相オフセット

    ManholeParams()
        : amplitude(2.0f)
        , speed(1.5f)
        , phase(0.0f)
        , phaseOffset(0.0f)
    {
    }
};

//=========================================================================================================
// マンホール個体データ
//=========================================================================================================
class ManholeData
{
public:
    DirectX::XMFLOAT3 basePos;     // 基準位置（振動中心）
    DirectX::XMFLOAT3 currentPos;  // 現在位置（mapに反映）
    int mapIndex;                  // GetFieldMap() 内の該当MAPDATAインデックス
    ManholeParams params;

    ManholeData()
        : basePos{ 0, 0, 0 }
        , currentPos{ 0, 0, 0 }
        , mapIndex(-1)
    {
    }
};

//=========================================================================================================
// API
//=========================================================================================================
void Manhole_Initialize();
void Manhole_Finalize();

int  Manhole_Create(float x, float y, float z, std::vector<MAPDATA>& mapData);

void Manhole_UpdateAll(float deltaTime);

std::vector<ManholeData>& Manhole_GetAll();
ManholeData* Manhole_Get(int index);
int  Manhole_GetCount();

void Manhole_ClearAll();

void Manhole_DebugDraw();