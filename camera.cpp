// camera.cpp
#include "camera.h"
#include "keyboard.h"
#include "mouse.h"
#include "player3D.h"
#include "Player2D.h"
#include "field.h"
#include "UtilMath.h"
#include "LightSource.h"
#include "Switch_Light.h"

#include <iostream>

using namespace mu;
using namespace DirectX;

//=========================================================================================================
// 内部状態（本ファイル内のみ）
//=========================================================================================================

// カメラ本体
static CAMERA  g_CameraObject;

// プレイヤー位置の前フレーム保持（用途：将来の揺れ/補間などの拡張用）
XMFLOAT3 g_PlayerPosOld;

// マウス状態（デバッグ用途/既存互換のため残す）
Mouse_State ms{};
float cSize = 1.0f; // カメラの感度/サイズ調整用（現状は未使用気味）

//---------------------------------------------------------------------------------------------------------
// 3Dカメラ（追従 + マウス操作）
//---------------------------------------------------------------------------------------------------------
static bool    gCamAnglesInit = false;        // 角度初期化フラグ（現状は未使用だが互換で保持）
static XMFLOAT3 gCamTarget = { 0, 0, 0 };     // カメラ注視点（補間後）
static XMFLOAT3 gCamPos = { 0, 0, 0 };     // カメラ位置（補間後）

static float   gYawDeg = 180.0f;            // 水平回転（deg）
static float   gPitchDeg = 15.0f;             // 垂直回転（deg）
static float   gDistance = 8.0f;              // ターゲットからの距離

static const float kPitchMin = -75.0f;
static const float kPitchMax = 75.0f;

static XMFLOAT3 gTargetOffset = { 0.0f, 1.2f, 0.0f }; // プレイヤーから注視点へのオフセット
static float    gFollowLerp = 0.15f;               // 追従補間（0..1）

// マウス感度
static float g_MouseSensYaw = 1.0f;
static float g_MouseSensPitch = 1.0f;

//---------------------------------------------------------------------------------------------------------
// 2Dカメラ（固定Yawで追従）
//---------------------------------------------------------------------------------------------------------
static const float kCam2D_Distance = 8.0f;
static const float kCam2D_HeightOffset = 1.5f;
static const float kCam2D_LookAtYOfs = 1.0f;
static const float kCam2D_FollowLerp = 0.12f;

static bool  g_Cam2D_Initialized = false;
static float g_Cam2D_YawDeg = 0.0f;

//---------------------------------------------------------------------------------------------------------
// ライトカメラ
//---------------------------------------------------------------------------------------------------------
static const float kCamLight_Distance = 8.0f;
static const float kCamLight_HeightOffset = 3.0f;
static const float kCamLight_LookAtYOfs = 0.0f;
static const float kCamLight_FollowLerp = 0.12f;

static bool  g_CamLight_Initialized = false;
static float g_CamLight_YawDeg = 0.0f;

//---------------------------------------------------------------------------------------------------------
// カメラ衝突設定（レイキャスト）
//---------------------------------------------------------------------------------------------------------
static const float kCameraCollisionRadius = 0.1f;  // カメラ球半径（当たり判定の拡張量）
static const float kCameraCollisionPadding = 0.1f;  // 壁から離す追加距離
static const float kCameraMinDistance = 1.2f;  // 最低距離（近すぎ防止）

//=========================================================================================================
// ユーティリティ
//=========================================================================================================

// 3Dベクトルの線形補間
static XMFLOAT3 Lerp3(const XMFLOAT3& a, const XMFLOAT3& b, float t)
{
	return {
		a.x + (b.x - a.x) * t,
		a.y + (b.y - a.y) * t,
		a.z + (b.z - a.z) * t
	};
}

// Ray vs AABB（スラブ法）
// outNormal: 進入面の法線（ローカル）
// outT     : ヒット距離（rayO + rayD * t）
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
				// レイが軸に平行：原点がスラブ内ならOK
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

// Ray vs OBB（OBBをローカルに落としてAABBとして判定）
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

// カメラが衝突判定対象とするフィールド種別
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

//=========================================================================================================
// 初期化 / 終了
//=========================================================================================================

void Camera_Initialize()
{
	g_CameraObject.Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	g_CameraObject.AtPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);
	g_CameraObject.UpVector = XMFLOAT3(0.0f, 1.0f, 0.0f);

	g_CameraObject.Fov = 45.0f;

	float width = (float)Direct3D_GetBackBufferWidth();
	float height = (float)Direct3D_GetBackBufferHeight();
	g_CameraObject.Aspect = (height > 1e-6f) ? (width / height) : 1.0f;

	g_CameraObject.NearClip = 0.5f;
	g_CameraObject.FarClip = 1000.0f;

	g_PlayerPosOld = GetPlayer3DPosition();

	// 3D操作を想定し、相対モードを基本とする
	Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
}

void Camera_Finalize()
{
	// 現状は動的リソース無し
	return;
}

//=========================================================================================================
// 更新
//=========================================================================================================

void Player3DCamera_Update()
{
	Mouse_State msLocal{};
	Mouse_GetState(&msLocal);

	// マウス操作（相対モードのみ）
	if (msLocal.positionMode == MOUSE_POSITION_MODE_RELATIVE)
	{
		const float sensYaw = 1.0f * g_MouseSensYaw;
		const float sensPitch = 1.0f * g_MouseSensPitch;

		gYawDeg += msLocal.x * sensYaw;
		gPitchDeg -= msLocal.y * sensPitch;

		if (gPitchDeg < kPitchMin) gPitchDeg = kPitchMin;
		if (gPitchDeg > kPitchMax) gPitchDeg = kPitchMax;
	}

	// ホイールズーム（距離制御）
	const float zoomSpeed = 0.06f;
	const float minDistance = 2.5f;
	const float maxDistance = 12.0f;

	gDistance -= msLocal.scrollWheelValue * zoomSpeed;
	if (gDistance < minDistance) gDistance = minDistance;
	if (gDistance > maxDistance) gDistance = maxDistance;

	// ターゲット（プレイヤー位置 + オフセット）
	XMFLOAT3 playerPos = GetPlayer3DPosition();
	XMFLOAT3 desiredTarget = {
		playerPos.x + gTargetOffset.x,
		playerPos.y + gTargetOffset.y,
		playerPos.z + gTargetOffset.z
	};

	// 球面座標風に「背面方向」を算出してカメラ位置を決める
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

	// 壁がある場合は手前に詰める（衝突回避）
	XMFLOAT3 finalPos;
	Camera_CheckCollision(desiredTarget, desiredPos, finalPos);

	// 追従補間（カメラの急な揺れを抑える）
	gCamTarget = Lerp3(gCamTarget, desiredTarget, gFollowLerp);
	gCamPos = Lerp3(gCamPos, finalPos, gFollowLerp);

	g_CameraObject.AtPosition = gCamTarget;
	g_CameraObject.Position = gCamPos;
	g_CameraObject.UpVector = { 0, 1, 0 };
}

void Player2DCamera_Update()
{
	PLAYER* p2 = GetPlayer2D();
	if (!p2) return;

	// 2Dモードに入った瞬間のYawを固定（壁面に対して一定方向で追従）
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

	g_CameraObject.AtPosition = Lerp3(g_CameraObject.AtPosition, targetAt, kCam2D_FollowLerp);
	g_CameraObject.Position = Lerp3(g_CameraObject.Position, targetPos, kCam2D_FollowLerp);
	g_CameraObject.UpVector = { 0.0f, 1.0f, 0.0f };
}

//---------------------------------------------------------------------------------------------------------
// ライトカメラ更新
//---------------------------------------------------------------------------------------------------------
void LightCamera_Update()
{
	// Get the OBJ_3 position from Switch_Light
	XMFLOAT3 lightPos = SwitchLight_GetLightObjPosition();

	if (!g_CamLight_Initialized)
	{
		// Set yaw based on current stage
		GAME_STAGE stage = GetCurrentStage();
		switch (stage)
		{
		case STAGE_1:
			g_CamLight_YawDeg = 270.0f;
			break;
		case STAGE_2:
			g_CamLight_YawDeg = 90.0f;
			break;
		case STAGE_3:
			g_CamLight_YawDeg = 270.0f;
			break;
		case STAGE_SELECT:
			g_CamLight_YawDeg = 0.0f;
			break;
		default:
			g_CamLight_YawDeg = 0.0f;
			break;
		}
		g_CamLight_Initialized = true;
	}

	// Look-at target: OBJ_3 position + Y offset
	XMFLOAT3 targetAt = {
		lightPos.x,
		lightPos.y + kCamLight_LookAtYOfs,
		lightPos.z
	};

	// Camera position: behind the OBJ_3 (fixed Yaw)
	float yawRad = XMConvertToRadians(g_CamLight_YawDeg);

	float fwdX = sinf(yawRad);
	float fwdZ = cosf(yawRad);

	XMFLOAT3 targetPos = {
		lightPos.x - fwdX * kCamLight_Distance,
		lightPos.y + kCamLight_HeightOffset,
		lightPos.z - fwdZ * kCamLight_Distance
	};

	// Smooth follow (Lerp)
	g_CameraObject.AtPosition = Lerp3(g_CameraObject.AtPosition, targetAt, kCamLight_FollowLerp);
	g_CameraObject.Position = Lerp3(g_CameraObject.Position, targetPos, kCamLight_FollowLerp);
	g_CameraObject.UpVector = { 0.0f, 1.0f, 0.0f };
}

void Camera_ResetLightState()
{
	g_CamLight_Initialized = false;
	g_CamLight_YawDeg = 0.0f;
}

float GetLightCameraYaw()
{
	return g_CamLight_YawDeg;
}

void Player2DCamera_DebugUpdate()
{
	// デバッグ用：3Dと同じ操作系で2Dプレイヤーを追う
	Mouse_State msLocal{};
	Mouse_GetState(&msLocal);

	if (msLocal.positionMode == MOUSE_POSITION_MODE_RELATIVE)
	{
		gYawDeg += msLocal.x * 1.0f;
		gPitchDeg -= msLocal.y * 1.0f;
		if (gPitchDeg < kPitchMin) gPitchDeg = kPitchMin;
		if (gPitchDeg > kPitchMax) gPitchDeg = kPitchMax;
	}

	XMFLOAT3 playerPos = GetPlayer2DPosition();
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

	g_CameraObject.AtPosition = gCamTarget;
	g_CameraObject.Position = gCamPos;
	g_CameraObject.UpVector = { 0, 1, 0 };
}

void Title_Camera_Update()
{
	// タイトル：固定シネマティック
	g_CameraObject.Position = XMFLOAT3(4.0f, 3.0f, -5.0f);
	g_CameraObject.AtPosition = XMFLOAT3(4.0f, 1.0f, 0.0f);
	g_CameraObject.UpVector = XMFLOAT3(0.0f, 1.0f, 0.0f);
}

//=========================================================================================================
// 行列更新
//=========================================================================================================

void Camera_Draw()
{
	float w = (float)Direct3D_GetBackBufferWidth();
	float h = (float)Direct3D_GetBackBufferHeight();
	if (h > 1e-6f) g_CameraObject.Aspect = w / h;

	g_CameraObject.Projection = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(g_CameraObject.Fov),
		g_CameraObject.Aspect,
		g_CameraObject.NearClip,
		g_CameraObject.FarClip);

	XMVECTOR vPos = XMVectorSet(g_CameraObject.Position.x, g_CameraObject.Position.y, g_CameraObject.Position.z, 0.0f);
	XMVECTOR vAt = XMVectorSet(g_CameraObject.AtPosition.x, g_CameraObject.AtPosition.y, g_CameraObject.AtPosition.z, 0.0f);
	XMVECTOR vUp = XMVectorSet(g_CameraObject.UpVector.x, g_CameraObject.UpVector.y, g_CameraObject.UpVector.z, 0.0f);

	g_CameraObject.View = XMMatrixLookAtLH(vPos, vAt, vUp);
}

//=========================================================================================================
// セッター / ゲッター
//=========================================================================================================

void SetCameraFov(float fov) { g_CameraObject.Fov = fov; }
void SetCameraAspect(float asp) { g_CameraObject.Aspect = asp; }
void SetCameraClip(float n, float f) { g_CameraObject.NearClip = n; g_CameraObject.FarClip = f; }
void SetCameraPosition(XMFLOAT3 pos) { g_CameraObject.Position = pos; }
void SetCameraAtPosition(XMFLOAT3 atpos) { g_CameraObject.AtPosition = atpos; }
void SetCameraUpVector(XMFLOAT3 up) { g_CameraObject.UpVector = up; }

XMMATRIX GetViewMatrix() { return g_CameraObject.View; }
XMMATRIX GetProjectionMatrix() { return g_CameraObject.Projection; }
XMFLOAT3 GetCameraAtPosition() { return g_CameraObject.AtPosition; }
XMFLOAT3 GetCameraPosition() { return g_CameraObject.Position; }

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

float GetMouseSensYaw() { return g_MouseSensYaw; }
float GetMouseSensPitch() { return g_MouseSensPitch; }

//=========================================================================================================
// カメラ衝突
//=========================================================================================================

void Camera_CheckCollision(XMFLOAT3 targetPos, XMFLOAT3 desiredCamPos, XMFLOAT3& outCamPos)
{
	outCamPos = desiredCamPos;

	// レイ（ターゲット -> 目的カメラ位置）
	XMFLOAT3 rayDir = {
		desiredCamPos.x - targetPos.x,
		desiredCamPos.y - targetPos.y,
		desiredCamPos.z - targetPos.z
	};

	float rayLength = sqrtf(rayDir.x * rayDir.x + rayDir.y * rayDir.y + rayDir.z * rayDir.z);
	if (rayLength < 0.001f) return;

	rayDir.x /= rayLength;
	rayDir.y /= rayLength;
	rayDir.z /= rayLength;

	std::vector<MAPDATA>& map = GetFieldMap();

	float closestHit = rayLength;
	bool  hitSomething = false;

	const float cameraRadius = kCameraCollisionRadius;

	for (size_t i = 0; i < map.size(); i++)
	{
		if (!CameraShouldCollide(map[i].no)) continue;

		// field.cpp は整理対象外のため、ここでは既存ロジックを保持
		XMFLOAT3 boxHalf;
		if (map[i].useCustomCollider) boxHalf = map[i].colliderHalf;
		else
		{
			boxHalf = XMFLOAT3(
				BOX_RADIUS * map[i].scale.x,
				BOX_RADIUS * map[i].scale.y,
				BOX_RADIUS * map[i].scale.z
			);
		}

		// カメラ半径分だけ当たりを太らせる
		boxHalf.x += cameraRadius;
		boxHalf.y += cameraRadius;
		boxHalf.z += cameraRadius;

		float hitDist = 0.0f;
		XMFLOAT3 hitNormal{};

		if (RaycastOBB(targetPos, rayDir, map[i].pos, boxHalf, map[i].rotate, rayLength, &hitNormal, &hitDist))
		{
			if (hitDist < closestHit)
			{
				closestHit = hitDist;
				hitSomething = true;
			}
		}
	}

	if (hitSomething)
	{
		// 壁の少し手前にカメラを寄せる
		float safeDistance = closestHit - cameraRadius - kCameraCollisionPadding;
		if (safeDistance < kCameraMinDistance) safeDistance = kCameraMinDistance;

		outCamPos.x = targetPos.x + rayDir.x * safeDistance;
		outCamPos.y = targetPos.y + rayDir.y * safeDistance;
		outCamPos.z = targetPos.z + rayDir.z * safeDistance;
	}
}