// FieldSeesaw.h
#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include "field.h"

//=========================================================================================================
// シーソー傾き軸
//=========================================================================================================
enum SEESAW_TILT_AXIS
{
    SEESAW_TILT_X = 0,  // X軸回り（Z方向に傾く）
    SEESAW_TILT_Z = 1,  // Z軸回り（X方向に傾く）
};

//=========================================================================================================
// シーソー制御パラメータ
//=========================================================================================================
struct SeesawParams
{
    float tiltAngle;      // 現在の傾き角（deg）
    float maxTiltAngle;   // 最大傾き角（deg）
    float tiltSpeed;      // プレイヤー搭乗時の傾き追従速度（deg/sec相当）
    float returnSpeed;    // 非搭乗時の戻り速度（autoReturn時に使用）
    float boardLength;    // 板の半長さ（ローカル距離の正規化用）
    bool  autoReturn;     // 離れたら0度に戻すか
    SEESAW_TILT_AXIS tiltAxis;

    SeesawParams()
        : tiltAngle(0.0f)
        , maxTiltAngle(12.0f)
        , tiltSpeed(60.0f)
        , returnSpeed(0.0f)
        , boardLength(2.5f)
        , autoReturn(false)
        , tiltAxis(SEESAW_TILT_Z)
    {
    }
};

//=========================================================================================================
// シーソー個体データ
//=========================================================================================================
class SeesawData
{
public:
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT3 rotate;
    std::vector<int> partIndices; // MapData内の部品（ベース/板）のインデックス
    SeesawParams params;

    SeesawData()
        : pos{ 0, 0, 0 }
        , rotate{ 0, 0, 0 }
    {
    }
};

//=========================================================================================================
// API
//=========================================================================================================
void Seesaw_Initialize();
void Seesaw_Finalize();

int  Seesaw_Create(float x, float y, float z, std::vector<MAPDATA>& mapData);
void Seesaw_UpdateAll(float deltaTime);

std::vector<SeesawData>& Seesaw_GetAll();
SeesawData* Seesaw_Get(int index);
int  Seesaw_GetCount();

bool  Seesaw_IsPlayerOnBoard(int seesawIndex, const DirectX::XMFLOAT3& playerPos);
float Seesaw_GetPlayerPosition(int seesawIndex, const DirectX::XMFLOAT3& playerPos);

int  Seesaw_PlayerCollision();

DirectX::XMFLOAT3 Seesaw_GetBoardColliderHalf(int seesawIndex);

void Seesaw_ClearAll();

void Seesaw_DebugDraw();