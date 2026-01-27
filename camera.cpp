//camera.cpp
#include "camera.h"
#include"keyboard.h"
#include "mouse.h"
#include "player3D.h"
#include "debug.h"
#include "Player2D.h"

//=========================================================================================================
// グローバル変数
//=========================================================================================================
static CAMERA CameraObject;
XMFLOAT3 g_PlayerPosOld;
static bool debugMode = TRUE;


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
	CameraObject.Position = XMFLOAT3(0.0f, 5.0f, -5.0f);
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
		const float sensYaw = 0.25f;
		const float sensPitch = 0.25f;
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
	XMFLOAT3 pos = g_PlayerPosOld;
	PLAYER* player2D = GetPlayer2D();

	pos.x = g_PlayerPosOld.x - pos.x;
	pos.y = g_PlayerPosOld.y - pos.y;
	pos.z = g_PlayerPosOld.z - pos.z;

	CameraObject.Position.x += pos.x;
	CameraObject.Position.y += pos.y;
	CameraObject.Position.z += pos.z;


	CameraObject.AtPosition.x = g_PlayerPosOld.x;
	CameraObject.AtPosition.y = g_PlayerPosOld.y;
	CameraObject.AtPosition.z = g_PlayerPosOld.z;

	CameraObject.Position.x += CameraObject.AtPosition.x;
	CameraObject.Position.z += CameraObject.AtPosition.z;

}

void Title_Camera_Update()
{
	static bool relativeMode = true;
	if (Keyboard_IsKeyDownTrigger(KK_ESCAPE)) {
			relativeMode = !relativeMode;
			Mouse_SetMode(relativeMode ? MOUSE_POSITION_MODE_RELATIVE
				: MOUSE_POSITION_MODE_ABSOLUTE);
		}

	XMFLOAT3 playerPos = GetPlayer3DPosition();

	// Follow only X
	CameraObject.Position.x = playerPos.x;

	// Much lower height (closer to ground)
	CameraObject.Position.y = 6.0f;

	// Closer depth (near the map)
	CameraObject.Position.z = playerPos.z - 6.0f;

	// Look at player (slightly up for nicer framing)
	CameraObject.AtPosition.x = playerPos.x;
	CameraObject.AtPosition.y = playerPos.y + 2.0f;
	CameraObject.AtPosition.z = playerPos.z;
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
			ImGui::TreePop();
		}
		ImGui::End();
	}


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

//=========================================================================================================
// マウス処理
//=========================================================================================================

void cameraMouse()
{
		Mouse_GetState(&ms);

		const float sensitivityYaw = 0.5f;
		const float sensitivityPitch = 0.5f;
		const float moveSpeedBase = 0.1f;

		
		static bool relativeMode = true;
		bool suppressDelta = false;
		{
			if (Keyboard_IsKeyDownTrigger(KK_ESCAPE)) {
				relativeMode = !relativeMode;
				Mouse_SetMode(relativeMode ? MOUSE_POSITION_MODE_RELATIVE
					: MOUSE_POSITION_MODE_ABSOLUTE);
				suppressDelta = true;
			}
		}

	   
		if (Keyboard_IsKeyDown(KK_Q)) {
			CameraObject.Fov += 0.5f;
			if (CameraObject.Fov > 160.0f) CameraObject.Fov = 160.0f;
		}
		if (Keyboard_IsKeyDown(KK_E)) {
			CameraObject.Fov -= 0.5f;
			if (CameraObject.Fov < 5.0f) CameraObject.Fov = 5.0f;
		}

		XMFLOAT3 playerDelta = g_PlayerPosOld;
		g_PlayerPosOld = GetPlayer3DPosition();
		playerDelta.x = g_PlayerPosOld.x - playerDelta.x;
		playerDelta.y = g_PlayerPosOld.y - playerDelta.y;
		playerDelta.z = g_PlayerPosOld.z - playerDelta.z;

		XMFLOAT3 pos = CameraObject.Position;
		XMFLOAT3 at = CameraObject.AtPosition;


		if (!gCamAnglesInit) {
			float rx = pos.x - at.x;
			float ry = pos.y - at.y;
			float rz = pos.z - at.z;
			float r = sqrtf(rx * rx + ry * ry + rz * rz);
			if (r < 1e-6f) r = 1e-6f;

			gYawDeg = XMConvertToDegrees(atan2f(rz, rx));
			gPitchDeg = XMConvertToDegrees(asinf(ry / r));
			XMVECTOR v = XMVectorSet(gPitchDeg, 0, 0, 0);
			XMVECTOR lo = XMVectorReplicate(kPitchMin);
			XMVECTOR hi = XMVectorReplicate(kPitchMax);
			v = XMVectorClamp(v, lo, hi);
			gPitchDeg = XMVectorGetX(v);

			gCamAnglesInit = true;
		}


		if (ms.positionMode == MOUSE_POSITION_MODE_RELATIVE && !suppressDelta) {
			gYawDeg -= ms.x * sensitivityYaw;
			gPitchDeg += ms.y * sensitivityPitch;
			XMVECTOR v = XMVectorSet(gPitchDeg, 0, 0, 0);
			XMVECTOR lo = XMVectorReplicate(kPitchMin);
			XMVECTOR hi = XMVectorReplicate(kPitchMax);
			v = XMVectorClamp(v, lo, hi);
			gPitchDeg = XMVectorGetX(v);
		}


		float relX = pos.x - at.x;
		float relY = pos.y - at.y;
		float relZ = pos.z - at.z;
		float radius = sqrtf(relX * relX + relY * relY + relZ * relZ);
		if (radius < 1e-6f) radius = 1e-6f;

		float yawRad = XMConvertToRadians(gYawDeg);
		float pitchRad = XMConvertToRadians(gPitchDeg);

		float cp = cosf(pitchRad);
		float sp = sinf(pitchRad);
		float cy = cosf(yawRad);
		float sy = sinf(yawRad);

		float rx = radius * cp * cy;
		float ry = radius * sp;
		float rz = radius * cp * sy;

		pos.x = at.x + rx;
		pos.y = at.y + ry;
		pos.z = at.z + rz;


		XMFLOAT3 fwd = { at.x - pos.x, at.y - pos.y, at.z - pos.z };
		float flen = sqrtf(fwd.x * fwd.x + fwd.z * fwd.z);
		if (flen > 1e-6f) {
			fwd.x /= flen;
			fwd.y /= flen;
			fwd.z /= flen;
		}
		else {
			fwd = { 0.0f, 0.0f, 1.0f };
		}
		XMFLOAT3 right = { fwd.z, 0.0f, -fwd.x };
		const XMFLOAT3 up = { 0.0f, 1.0f, 0.0f };


		pos.x += playerDelta.x;
		pos.y += playerDelta.y;
		pos.z += playerDelta.z;
		at.x += playerDelta.x;
		at.y += playerDelta.y;
		at.z += playerDelta.z;

		CameraObject.Position = pos;
		CameraObject.AtPosition = at;
	}

void cameraKeyb()
{

	XMFLOAT3 pos = g_PlayerPosOld;
	g_PlayerPosOld = GetPlayer3DPosition();


	pos.x = g_PlayerPosOld.x - pos.x;
	pos.y = g_PlayerPosOld.y - pos.y;
	pos.z = g_PlayerPosOld.z - pos.z;

	CameraObject.Position.x += pos.x;
	CameraObject.Position.y += pos.y;
	CameraObject.Position.z += pos.z;


	CameraObject.AtPosition.x = g_PlayerPosOld.x;
	CameraObject.AtPosition.y = g_PlayerPosOld.y;
	CameraObject.AtPosition.z = g_PlayerPosOld.z;


	float Rotation = 0.0f;


	XMFLOAT2 vec;
	vec.x = CameraObject.Position.x - CameraObject.AtPosition.x;
	vec.y = CameraObject.Position.z - CameraObject.AtPosition.z;

	float co = cosf(XMConvertToRadians(Rotation));
	float si = sinf(XMConvertToRadians(Rotation));
	CameraObject.Position.x = (vec.x * co - vec.y * si);
	CameraObject.Position.z = (vec.x * si + vec.y * co);
	CameraObject.Position.x += CameraObject.AtPosition.x;
	CameraObject.Position.z += CameraObject.AtPosition.z;

}

void cameraMouse_1()
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
		const float sensYaw = 0.25f;
		const float sensPitch = 0.25f;
		gYawDeg -= ms.x * sensYaw;
		gPitchDeg += ms.y * sensPitch;

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