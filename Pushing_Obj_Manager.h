//Pushing_Obj_Manager.h
#pragma once
#include <DirectXMath.h>
#include "field.h"


// Push state enumeration
enum PUSH_STATE
{
    PUSH_STATE_NONE = 0,     // Not pushing anything
    PUSH_STATE_PUSHING,      // Currently pushing an object
};

// Push target information
struct PUSH_TARGET
{
    int fieldIndex = -1;     // Index of the field object being pushed
    DirectX::XMFLOAT3 pushDirection = { 0, 0, 0 }; // Direction of push
};

// Function declarations
void PlayerPushManager_Init();
void PlayerPushManager_Finalize();
void PlayerPushManager_Update();
void PlayerPushManager_Draw();

PUSH_STATE PlayerPushManager_GetState();
bool PlayerPushManager_IsPushing();
const PUSH_TARGET* PlayerPushManager_GetCurrentTarget();
bool PlayerPushManager_ShouldShowBillBoard();
DirectX::XMFLOAT3 PlayerPushManager_GetBillBoardPosition();