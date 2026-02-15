// Manhole.h
#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include "field.h"

using namespace DirectX;

struct ManholeParams
{
    float amplitude;  
    float speed;      
    float phase;      
    float phaseOffset;

    ManholeParams()
        : amplitude(2.0f)
        , speed(1.5f)
        , phase(0.0f)
        , phaseOffset(0.0f)
    {}
};

class ManholeData
{
public:
    XMFLOAT3 basePos;       
    XMFLOAT3 currentPos;    
    int mapIndex;           
    ManholeParams params;

    ManholeData()
        : basePos{ 0, 0, 0 }
        , currentPos{ 0, 0, 0 }
        , mapIndex(-1)
    {}
};

void Manhole_Initialize();
void Manhole_Finalize();

int Manhole_Create(float x, float y, float z, std::vector<MAPDATA>& mapData);

void Manhole_UpdateAll(float deltaTime);

std::vector<ManholeData>& Manhole_GetAll();
ManholeData* Manhole_Get(int index);
int Manhole_GetCount();

void Manhole_ClearAll();

void Manhole_DebugDraw();
void Manhole_SetDebugDraw(bool enable);