#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include "field.h"


enum SEESAW_TILT_AXIS
{
    SEESAW_TILT_X = 0,
    SEESAW_TILT_Z = 1,
};

struct SeesawParams
{
    float tiltAngle;
    float maxTiltAngle;
    float tiltSpeed;
    float returnSpeed;
    float boardLength;
    bool autoReturn;
    SEESAW_TILT_AXIS tiltAxis;

    SeesawParams()
        : tiltAngle(0.0f)
        , maxTiltAngle(12.0f)
        , tiltSpeed(60.0f)
        , returnSpeed(0.0f)
        , boardLength(2.5f)
        , autoReturn(false)
        , tiltAxis(SEESAW_TILT_Z)
    {}
};

class SeesawData
{
public:
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT3 rotate;
    std::vector<int> partIndices;
    SeesawParams params;

    SeesawData()
        : pos{ 0, 0, 0 }
        , rotate{ 0, 0, 0 }
    {}
};

void Seesaw_Initialize();
void Seesaw_Finalize();

int Seesaw_Create(float x, float y, float z, std::vector<MAPDATA>& mapData);

void Seesaw_UpdateAll(float deltaTime);

std::vector<SeesawData>& Seesaw_GetAll();
SeesawData* Seesaw_Get(int index);
int Seesaw_GetCount();

bool Seesaw_IsPlayerOnBoard(int seesawIndex, const DirectX::XMFLOAT3& playerPos);
float Seesaw_GetPlayerPosition(int seesawIndex, const DirectX::XMFLOAT3& playerPos);

int Seesaw_PlayerCollision();

DirectX::XMFLOAT3 Seesaw_GetBoardColliderHalf(int seesawIndex);

void Seesaw_ClearAll();

void Seesaw_DebugDraw();