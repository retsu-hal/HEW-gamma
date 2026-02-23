//camera.cpp
#include "camera.h"
#include"keyboard.h"
#include "mouse.h"
#include "player3D.h"
#include "debug.h"
#include "Player2D.h"
#include <iostream>
#include "field.h"
#include "MathUtil.h"

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
static float gDistance = 8.0f;//カメラと注視点の距離
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

// ファイル先頭付近の既存のカメラ関連のstatic変数定義の近くに追記
static float g_MouseSensYaw = 1.0f;
static float g_MouseSensPitch = 1.0f;

static XMFLOAT3 Lerp3(const XMFLOAT3& a, const XMFLOAT3& b, float t)
{// 3Dベクトルの線形補間
    return {
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    };
}

// Ray vs AABB intersection
static bool RaycastAABB(
	const XMFLOAT3& rayO,
	const XMFLOAT3& rayD_in,
	const XMFLOAT3& boxC,
	const XMFLOAT3& boxHalf,
	float maxDist,
	XMFLOAT3* outNormal,
	float* outT)
{
	XMFLOAT3 rayD = mu::Normalize(rayD_in);

	float tmin = 0.0f;
	float tmax = maxDist;
	XMFLOAT3 hitN = { 0, 0, 0 };

	auto slab = [&](float ro, float rd, float c, float h, XMFLOAT3 nNeg, XMFLOAT3 nPos) -> bool
		{
			if (fabsf(rd) < 1e-6f)
			{
				return (ro >= c - h && ro <= c + h);
			}

			float inv = 1.0f / rd;
			float t1 = (c - h - ro) * inv;
			float t2 = (c + h - ro) * inv;

			XMFLOAT3 n1 = nNeg;
			XMFLOAT3 n2 = nPos;

			if (t1 > t2) { std::swap(t1, t2); std::swap(n1, n2); }

			if (t1 > tmin) { tmin = t1; hitN = n1; }
			if (t2 < tmax) { tmax = t2; }

			return tmin <= tmax;
		};

	if (!slab(rayO.x, rayD.x, boxC.x, boxHalf.x, { -1, 0, 0 }, { 1, 0, 0 })) return false;
	if (!slab(rayO.y, rayD.y, boxC.y, boxHalf.y, { 0, -1, 0 }, { 0, 1, 0 })) return false;
	if (!slab(rayO.z, rayD.z, boxC.z, boxHalf.z, { 0, 0, -1 }, { 0, 0, 1 })) return false;

	if (tmin < 0.0f || tmin > maxDist) return false;

	if (outNormal) *outNormal = hitN;
	if (outT) *outT = tmin;
	return true;
}

// Ray vs OBB intersection (uses boxHalf you already computed)
static bool RaycastOBB(
	const XMFLOAT3& rayO,
	const XMFLOAT3& rayD_in,
	const XMFLOAT3& boxC,
	const XMFLOAT3& boxHalf,
	const XMFLOAT3& boxRotDeg,
	float maxDist,
	XMFLOAT3* outNormalW,
	float* outT)
{
	const XMMATRIX R = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(boxRotDeg.x),
		XMConvertToRadians(boxRotDeg.y),
		XMConvertToRadians(boxRotDeg.z));

	const XMMATRIX invR = XMMatrixTranspose(R);

	XMVECTOR O = XMLoadFloat3(&rayO);
	XMVECTOR D = XMLoadFloat3(&rayD_in);
	D = XMVector3Normalize(D);

	XMVECTOR C = XMLoadFloat3(&boxC);

	XMVECTOR Orel = O - C;
	XMVECTOR OlocV = XMVector3TransformNormal(Orel, invR);
	XMVECTOR DlocV = XMVector3TransformNormal(D, invR);

	XMFLOAT3 Oloc, Dloc;
	XMStoreFloat3(&Oloc, OlocV);
	XMStoreFloat3(&Dloc, DlocV);

	XMFLOAT3 nL = { 0, 0, 0 };
	float t = 0.0f;

	if (!RaycastAABB(Oloc, Dloc, XMFLOAT3(0, 0, 0), boxHalf, maxDist, &nL, &t))
		return false;

	XMVECTOR nW = XMVector3TransformNormal(XMLoadFloat3(&nL), R);
	nW = XMVector3Normalize(nW);

	if (outNormalW) XMStoreFloat3(outNormalW, nW);
	if (outT) *outT = t;
	return true;
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

	/*static bool relativeMode = true;
	bool suppressDelta = false;
	{
		if (Keyboard_IsKeyDownTrigger(KK_ESCAPE)) {
			relativeMode = !relativeMode;
			Mouse_SetMode(relativeMode ? MOUSE_POSITION_MODE_RELATIVE
				: MOUSE_POSITION_MODE_ABSOLUTE);
		}
	}*/

	
	if (ms.positionMode == MOUSE_POSITION_MODE_RELATIVE)
	{
		const float sensYaw = 1.0f * g_MouseSensYaw;
		const float sensPitch = 1.0f * g_MouseSensPitch;
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
	/*static bool relativeMode = true;
	if (Keyboard_IsKeyDownTrigger(KK_ESCAPE)) {
			relativeMode = !relativeMode;
			Mouse_SetMode(relativeMode ? MOUSE_POSITION_MODE_RELATIVE
				: MOUSE_POSITION_MODE_ABSOLUTE);
		}*/

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

	if (debugMode)
	{
		ImGui::Begin("Debug - han");
		if (ImGui::TreeNode("camera.cpp"))
		{
			ImGui::Text("PosX: %.2f", CameraObject.Position.x);
			ImGui::Text("PosY: %.2f", CameraObject.Position.y);
			ImGui::Text("PosZ: %.2f", CameraObject.Position.z);
			ImGui::Text("Pitch: %.2f", g_MouseSensPitch);
			ImGui::Text("Yaw: %.2f", g_MouseSensYaw);
			ImGui::TreePop();
		}
		ImGui::End();
	}

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

void SetCameraMouseSensitivity(float yaw, float pitch)
{
	g_MouseSensYaw = yaw;
	g_MouseSensPitch = pitch;
}

float GetMouseSensYaw()
{
	return g_MouseSensYaw;
}

float GetMouseSensPitch()
{
	return g_MouseSensPitch;
}

// Camera collision configuration
static const float kCameraCollisionRadius = 0.1f;      // Camera collision sphere size
static const float kCameraCollisionPadding = 0.1f;    // Extra space from walls
static const float kCameraMinDistance = 1.2f;         // Minimum distance from player

static bool CameraShouldCollide(FIELD type)
{
	switch (type)
	{
	case FIELD_GROUND:
	case FIELD_WALL:
		return true;
	default:
		return false;
	}
}

static void DrawRayDebug(const XMFLOAT3& a, const XMFLOAT3& b, ImU32 col = IM_COL32(255, 0, 0, 255))
{
	auto WorldToScreen = [](const XMFLOAT3& p, ImVec2& out) -> bool
		{
			XMMATRIX vp = GetViewMatrix() * GetProjectionMatrix();
			XMVECTOR vW = XMVectorSet(p.x, p.y, p.z, 1.0f);
			XMVECTOR vC = XMVector3TransformCoord(vW, vp);

			XMFLOAT3 ndc;
			XMStoreFloat3(&ndc, vC);

			if (ndc.x < -1.5f || ndc.x > 1.5f || ndc.y < -1.5f || ndc.y > 1.5f || ndc.z < 0.0f)
				return false;

			ImGuiViewport* vp_ = ImGui::GetMainViewport();
			float w = (float)Direct3D_GetBackBufferWidth();
			float h = (float)Direct3D_GetBackBufferHeight();

			out.x = vp_->Pos.x + (ndc.x * 0.5f + 0.5f) * w;
			out.y = vp_->Pos.y + (-ndc.y * 0.5f + 0.5f) * h;
			return true;
		};

	ImVec2 sa, sb;
	if (WorldToScreen(a, sa) && WorldToScreen(b, sb))
	{
		ImGui::GetBackgroundDrawList()->AddLine(sa, sb, col, 2.0f);
	}
}

void Camera_CheckCollision(XMFLOAT3 targetPos, XMFLOAT3 desiredCamPos, XMFLOAT3& outCamPos)
{
	DrawRayDebug(targetPos, desiredCamPos, IM_COL32(255, 0, 0, 255));
	outCamPos = desiredCamPos;

	// Calculate ray from target to desired camera position
	XMFLOAT3 rayDir;
	rayDir.x = desiredCamPos.x - targetPos.x;
	rayDir.y = desiredCamPos.y - targetPos.y;
	rayDir.z = desiredCamPos.z - targetPos.z;

	float rayLength = sqrtf(rayDir.x * rayDir.x + rayDir.y * rayDir.y + rayDir.z * rayDir.z);

	if (rayLength < 0.001f)
	{
		return;
	}

	// Normalize ray direction
	rayDir.x /= rayLength;
	rayDir.y /= rayLength;
	rayDir.z /= rayLength;

	std::vector<MAPDATA>& map = GetFieldMap();

	float closestHit = rayLength;
	bool hitSomething = false;

	const float cameraRadius = kCameraCollisionRadius;

	for (size_t i = 0; i < map.size(); i++)
	{
		if (map[i].pos.y < targetPos.y )
			continue;
		if (!CameraShouldCollide(map[i].no))
			continue;

		XMFLOAT3 boxHalf;
		if (map[i].useCustomCollider)
		{
			boxHalf = map[i].colliderHalf;
		}
		else
		{
			boxHalf = XMFLOAT3(
				BOX_RADIUS * map[i].scale.x,
				BOX_RADIUS * map[i].scale.y,
				BOX_RADIUS * map[i].scale.z
			);
		}

		// Only slightly expand the box
		boxHalf.x += cameraRadius;
		boxHalf.y += cameraRadius;
		boxHalf.z += cameraRadius;

		float hitDist = 0.0f;

		XMFLOAT3 hitNormal{};
		if (RaycastOBB(targetPos, rayDir, map[i].pos, boxHalf, map[i].rotate, rayLength, &hitNormal, &hitDist))
		{
			if (map[i].no == FIELD_EMPTY_BOX)
			{
				OutputDebugStringA("Camera hit FIELD_EMPTY_BOX\n");
			}

			char buf[128];
			sprintf_s(buf, "Camera hit type=%d at (%.2f, %.2f, %.2f)\n",
				(int)map[i].no, map[i].pos.x, map[i].pos.y, map[i].pos.z);
			OutputDebugStringA(buf);

			if (hitDist < closestHit)
			{
				closestHit = hitDist;
				hitSomething = true;
			}
		}

	}

	if (hitSomething)
	{
		// Keep a comfortable distance from the obstacle
		float safeDistance = closestHit - cameraRadius - kCameraCollisionPadding;

		if (safeDistance < kCameraMinDistance)
			safeDistance = kCameraMinDistance;

		outCamPos.x = targetPos.x + rayDir.x * safeDistance;
		outCamPos.y = targetPos.y + rayDir.y * safeDistance;
		outCamPos.z = targetPos.z + rayDir.z * safeDistance;
	}

}
