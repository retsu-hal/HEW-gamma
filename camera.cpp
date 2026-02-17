//camera.cpp
#include "camera.h"
#include"keyboard.h"
#include "mouse.h"
#include "player3D.h"
#include "debug.h"
#include "Player2D.h"
#include <iostream>

//=========================================================================================================
// グローバル変数
//=========================================================================================================
static CAMERA CameraObject;
XMFLOAT3 g_PlayerPosOld;


//マウス操作用変数
Mouse_State ms{};
float cSize = 1.0f;//カメラの感度調整用

//カメラ操作用変数
static bool   gCamAnglesInit = false;//カメラ角度初期化フラグ
static XMFLOAT3 gCamTarget = { 0, 0, 0 };//カメラ注視点
static XMFLOAT3 gCamPos = { 0, 0, 0 };//カメラ位置
static float gYawDeg = 180.0f;//カメラの水平回転角度
static float gPitchDeg = 15.0f;//カメラの上下回転角度
static float gDistance = 6.0f;//カメラと注視点の距離
static const float kPitchMin = -75.0f;//カメラの上下限度角度
static const float kPitchMax = 75.0f;
static XMFLOAT3 gTargetOffset = { 0.0f, 1.2f, 0.0f };//カメラ注視点オフセット
static float gFollowLerp = 0.15f;//カメラ追従の速さ


static const float kCam2D_Distance = 8.0f;
static const float kCam2D_HeightOffset = 1.5f;
static const float kCam2D_LookAtYOfs = 1.0f;
static const float kCam2D_FollowLerp = 0.12f;

static bool  g_Cam2D_Initialized = false;
static float g_Cam2D_YawDeg = 0.0f;

static XMFLOAT3 Lerp3(const XMFLOAT3& a, const XMFLOAT3& b, float t)
{// 3Dベクトルの線形補間
    return {
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    };
}

//=========================================================================================================
// 初期化
//=========================================================================================================
void Camera_Initialize()
{
    CameraObject.Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
    CameraObject.AtPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);
    CameraObject.UpVector = XMFLOAT3(0.0f, 1.0f, 0.0f);

    CameraObject.Fov = 45.0f;
    float width = (float)Direct3D_GetBackBufferWidth();
    float height = (float)Direct3D_GetBackBufferHeight();
    CameraObject.Aspect = width / height;
    CameraObject.NearClip = 0.5f;
    CameraObject.FarClip = 1000.0f;

    g_PlayerPosOld = GetPlayer3DPosition();
    
    Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
}


//=========================================================================================================
// 終了処理
//=========================================================================================================
void Camera_Finalize()
{
    return;
}


//=========================================================================================================
// 更新処理
//=========================================================================================================
void Player3DCamera_Update()
{
    Mouse_State ms{};
    Mouse_GetState(&ms);

    static bool relativeMode = true;
    bool suppressDelta = false;
    {
        if (Keyboard_IsKeyDownTrigger(KK_ESCAPE)) {
            relativeMode = !relativeMode;
            Mouse_SetMode(relativeMode ? MOUSE_POSITION_MODE_RELATIVE
                : MOUSE_POSITION_MODE_ABSOLUTE);
        }
    }

    if (ms.positionMode == MOUSE_POSITION_MODE_RELATIVE)
    {
        const float sensYaw = 1.0f;
        const float sensPitch = 1.0f;
        gYawDeg += ms.x * sensYaw;
        gPitchDeg -= ms.y * sensPitch;

        if (gPitchDeg < kPitchMin) gPitchDeg = kPitchMin;
        if (gPitchDeg > kPitchMax) gPitchDeg = kPitchMax;
    }

    XMFLOAT3 playerPos = GetPlayer3DPosition();
    XMFLOAT3 desiredTarget = {
        playerPos.x + gTargetOffset.x,
        playerPos.y + gTargetOffset.y,
        playerPos.z + gTargetOffset.z
    };

    float yaw = XMConvertToRadians(gYawDeg);
    float pitch = XMConvertToRadians(gPitchDeg);

    float cp = cosf(pitch), sp = sinf(pitch);
    float cy = cosf(yaw), sy = sinf(yaw);

    XMFLOAT3 back = { sy * cp, sp, cy * cp };
    XMFLOAT3 desiredPos = {
        desiredTarget.x - back.x * gDistance,
        desiredTarget.y - back.y * gDistance,
        desiredTarget.z - back.z * gDistance
    };

    gCamTarget = Lerp3(gCamTarget, desiredTarget, gFollowLerp);
    gCamPos = Lerp3(gCamPos, desiredPos, gFollowLerp);

    CameraObject.AtPosition = gCamTarget;
    CameraObject.Position = gCamPos;
    CameraObject.UpVector = { 0, 1, 0 };

}

void Player2DCamera_Update()
{
    PLAYER* p2 = GetPlayer2D();
    if (!p2) return;

    if (!g_Cam2D_Initialized)
    {
        g_Cam2D_YawDeg = p2->Rotation.y;
        g_Cam2D_Initialized = true;
    }

    XMFLOAT3 targetAt = {
        p2->Position.x,
        p2->Position.y + kCam2D_LookAtYOfs,
        p2->Position.z
    };

    float yawRad = XMConvertToRadians(g_Cam2D_YawDeg);

    float fwdX = sinf(yawRad);
    float fwdZ = cosf(yawRad);

    XMFLOAT3 targetPos = {
        p2->Position.x - fwdX * kCam2D_Distance,
        p2->Position.y + kCam2D_HeightOffset,
        p2->Position.z - fwdZ * kCam2D_Distance
    };


    CameraObject.AtPosition = Lerp3(CameraObject.AtPosition, targetAt, kCam2D_FollowLerp);
    CameraObject.Position = Lerp3(CameraObject.Position, targetPos, kCam2D_FollowLerp);
    CameraObject.UpVector = { 0.0f, 1.0f, 0.0f };

}

void Title_Camera_Update()
{
    static bool relativeMode = true;
    if (Keyboard_IsKeyDownTrigger(KK_ESCAPE)) {
            relativeMode = !relativeMode;
            Mouse_SetMode(relativeMode ? MOUSE_POSITION_MODE_RELATIVE
                : MOUSE_POSITION_MODE_ABSOLUTE);
        }

    //XMFLOAT3 playerPos = GetPlayer3DPosition();

    //// Follow only X
    //CameraObject.Position.x = playerPos.x;

    //// Much lower height (closer to ground)
    //CameraObject.Position.y = 3.0f;

    //// Closer depth (near the map)
    //CameraObject.Position.z = playerPos.z - 6.0f;

    //// Look at player (slightly up for nicer framing)
    //CameraObject.AtPosition.x = playerPos.x;
    //CameraObject.AtPosition.y = playerPos.y + 2.0f;
    //CameraObject.AtPosition.z = playerPos.z;

    // Fixed cinematic camera
    CameraObject.Position = XMFLOAT3(4.0f, 3.0f, -5.0f);
    CameraObject.AtPosition = XMFLOAT3(4.0f, 0.0f, 0.0f);
    CameraObject.UpVector = XMFLOAT3(0.0f, 1.0f, 0.0f);

}

//=========================================================================================================
// 描画処理
//=========================================================================================================
void Camera_Draw()
{

    DEBUG_IMGUI_BEGIN({
        ImGui::Begin("Debug - han");
                if (ImGui::TreeNode("camera.cpp"))
                {
                    ImGui::Text("PosX: %.2f", CameraObject.Position.x);
                    ImGui::Text("PosY: %.2f", CameraObject.Position.y);
                    ImGui::Text("PosZ: %.2f", CameraObject.Position.z);
                    ImGui::TreePop();
                }
                ImGui::End();

        });


    float w = (float)Direct3D_GetBackBufferWidth();
    float h = (float)Direct3D_GetBackBufferHeight();
    if (h > 1e-6f) CameraObject.Aspect = w / h;

    CameraObject.Projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(CameraObject.Fov),CameraObject.Aspect,CameraObject.NearClip,CameraObject.FarClip);


    XMVECTOR vPos = XMVectorSet(CameraObject.Position.x,CameraObject.Position.y,CameraObject.Position.z,0.0f);
    XMVECTOR vAt = XMVectorSet(CameraObject.AtPosition.x, CameraObject.AtPosition.y, CameraObject.AtPosition.z, 0.0f);
    XMVECTOR vUp = XMVectorSet(CameraObject.UpVector.x, CameraObject.UpVector.y, CameraObject.UpVector.z, 0.0f);
    CameraObject.View = XMMatrixLookAtLH(vPos, vAt, vUp);

    return;
}

//=========================================================================================================
// 視野角
//=========================================================================================================
void SetCameraFov(float fov)
{
    CameraObject.Fov = fov;
}


//=========================================================================================================
// アスペクト比
//=========================================================================================================
void SetCameraAspect(float asp)
{
    CameraObject.Aspect = asp;
}

//=========================================================================================================
// クリップ距離
//=========================================================================================================
void SetCameraClip(float n, float f)
{
    CameraObject.NearClip = n;
    CameraObject.FarClip = f;
}

//=========================================================================================================
// カメラ位置
//=========================================================================================================
void SetCameraPosition(XMFLOAT3 pos)
{
    CameraObject.Position = pos;
}

//=========================================================================================================
// カメラ注視点
//=========================================================================================================
void SetCameraAtPosition(XMFLOAT3 atpos )
{
    CameraObject.AtPosition = atpos;
}

//=========================================================================================================
// カメラ上方向ベクトル
//=========================================================================================================
void SetCameraUpVector(XMFLOAT3 up)
{
    CameraObject.UpVector = up;
}

//=========================================================================================================
// ビュー行列取得
//=========================================================================================================
XMMATRIX GetViewMatrix()
{
    return CameraObject.View;
}

//=========================================================================================================
// プロジェクション行列取得
//=========================================================================================================
XMMATRIX GetProjectionMatrix()
{
    return CameraObject.Projection;
}

//=========================================================================================================
// カメラ注視点取得
//=========================================================================================================
XMFLOAT3 GetCameraAtPosition()
{
    return CameraObject.AtPosition;
}

//=========================================================================================================
// カメラ位置取得
//=========================================================================================================
XMFLOAT3 GetCameraPosition()
{
    return CameraObject.Position;
}

void Camera_Reset2DState()
{
    g_Cam2D_Initialized = false;
    g_Cam2D_YawDeg = 0.0f;
}