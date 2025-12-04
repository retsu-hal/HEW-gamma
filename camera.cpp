#include "camera.h"
#include"keyboard.h"
#include "mouse.h"

#include "debug.h"
static bool debugMode = TRUE;


Mouse_State ms{};
float cSize = 1.0f;

static bool   gCamAnglesInit = false;
static float  gYawDeg = 0.0f;
static float  gPitchDeg = 0.0f;
static const float kPitchMin = -85.0f;
static const float kPitchMax = 85.0f;

//=========================================================================================================
// グローバル変数
//=========================================================================================================
static CAMERA CameraObject;
XMFLOAT3 g_BallPosOld;

//=========================================================================================================
// 初期化処理
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

	//g_BallPosOld = GetBallPositon();
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
void Camera_Update()
{
	static bool CamMode = false;

	if (Keyboard_IsKeyDown(KK_C)) {
		CamMode = !CamMode;
	}

	if (CamMode)
	{
		Mose();
	}
	else
	{
		Keyb();
	}
}

//=========================================================================================================
// 描画処理
//=========================================================================================================
void Camera_Draw()
{
	if (debugMode)
	{
		ImGui::Begin("Debug - CHEN");
		if (ImGui::TreeNode("canera.cpp"))
		{
			ImGui::Text("PosX: %.2f", CameraObject.Position.x);
			ImGui::Text("PosY: %.2f", CameraObject.Position.y);
			ImGui::Text("PosZ: %.2f", CameraObject.Position.z);
			ImGui::TreePop();
		}
		ImGui::End();
	}

	//プロジェクション行列作成
	CameraObject.Projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(CameraObject.Fov),CameraObject.Aspect,CameraObject.NearClip,CameraObject.FarClip);

	//ビュー行列作成
	XMVECTOR vPos = XMVectorSet(CameraObject.Position.x,CameraObject.Position.y,CameraObject.Position.z,0.0f);
	XMVECTOR vAt = XMVectorSet(CameraObject.AtPosition.x, CameraObject.AtPosition.y, CameraObject.AtPosition.z, 0.0f);
	XMVECTOR vUp = XMVectorSet(CameraObject.UpVector.x, CameraObject.UpVector.y, CameraObject.UpVector.z, 0.0f);
	CameraObject.View = XMMatrixLookAtLH(vPos, vAt, vUp);

	return;
}

//=========================================================================================================
// 視野角の設定
//=========================================================================================================
void SetCameraFov(float fov)
{
	CameraObject.Fov = fov;
}

//=========================================================================================================
// アスペクト比の設定
//=========================================================================================================
void SetCameraAspect(float asp)
{
	CameraObject.Aspect = asp;
}

//=========================================================================================================
// クリップ距離の設定
//=========================================================================================================
void SetCameraClip(float n, float f)
{
	CameraObject.NearClip = n;
	CameraObject.FarClip = f;
}

//=========================================================================================================
// 位置の設定
//=========================================================================================================
void SetCameraPosition(XMFLOAT3 pos)
{
	CameraObject.Position = pos;
}

//=========================================================================================================
// 注視点の設定
//=========================================================================================================
void SetCameraAtPosition(XMFLOAT3 atpos )
{
	CameraObject.AtPosition = atpos;
}

//=========================================================================================================
// 上方向ベクトルの設定
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
// プロジェクション行列の取得
//=========================================================================================================
XMMATRIX GetProjectionMatrix()
{
	return CameraObject.Projection;
}

//=========================================================================================================
// カメラの注視点を取得
//=========================================================================================================
XMFLOAT3 GetCameraAtPosition()
{
	return CameraObject.AtPosition;
}

//=========================================================================================================
// カメラの位置を取得
//=========================================================================================================
XMFLOAT3 GetCameraPosition()
{
	return CameraObject.Position;
}

void Mose()
{
	Mouse_GetState(&ms);

	const float sensitivityYaw = 0.5f;//x  マウス感度調整用
	const float sensitivityPitch = 0.5f;//y  マウス感度調整用
	const float moveSpeedBase = 0.1f;


	static bool relativeMode = false;
	static bool prevRight = false;
	bool suppressDelta = false;

	if (ms.rightButton && !prevRight) {//右クリックが押された瞬間
		relativeMode = !relativeMode;
		Mouse_SetMode(relativeMode ? MOUSE_POSITION_MODE_RELATIVE
			: MOUSE_POSITION_MODE_ABSOLUTE);
		suppressDelta = true;
	}

	prevRight = ms.rightButton;

	if (Keyboard_IsKeyDown(KK_Z)) {
		CameraObject.Fov += 0.5f;
		if (CameraObject.Fov > 160.0f)
		{
			CameraObject.Fov = 160.0f;
		}
	}
	if (Keyboard_IsKeyDown(KK_X)) {
		CameraObject.Fov -= 0.5f;
		if (CameraObject.Fov < 5.0f)
		{
			CameraObject.Fov = 5.0f;
		}
	}

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


	XMFLOAT3 fwd = { at.x - pos.x,at.y - pos.y,at.z - pos.z };
	float flen = sqrtf(fwd.x * fwd.x + fwd.z * fwd.z);
	if (flen > 1e-6f) {
		fwd.x /= flen;
		fwd.y /= flen;
		fwd.z /= flen;
	}
	else
	{
		fwd = { 0.0f,0.0f,1.0f };
	}

	XMFLOAT3 right = { fwd.z,0.0f,-fwd.x };
	const XMFLOAT3 up = { 0.0f,1.0f,0.0f };




	//float speedMul = Keyboard_IsKeyDown(KK_LEFTSHIFT) ? 4.0f : 1.0f;
	float moveSpeed = moveSpeedBase * 1.0f;

	float spF = 0.0f;
	float spR = 0.0f;
	float spU = 0.0f;

	if (Keyboard_IsKeyDown(KK_W)) spF += moveSpeed;
	if (Keyboard_IsKeyDown(KK_S)) spF -= moveSpeed;

	if (Keyboard_IsKeyDown(KK_D)) spR += moveSpeed;
	if (Keyboard_IsKeyDown(KK_A)) spR -= moveSpeed;

	if (Keyboard_IsKeyDown(KK_LEFTSHIFT)) spU += moveSpeed;
	if (Keyboard_IsKeyDown(KK_LEFTCONTROL)) spU -= moveSpeed;

	XMFLOAT3 disp = {
		fwd.x * spF + right.x * spR + up.x * spU,
		fwd.y * spF + right.y * spR + up.y * spU,
		fwd.z * spF + right.z * spR + up.z * spU,
	};

	pos.x += disp.x;
	pos.y += disp.y;
	pos.z += disp.z;
	at.x += disp.x;
	at.y += disp.y;
	at.z += disp.z;

	CameraObject.Position = pos;
	CameraObject.AtPosition = at;
}

void Keyb()
{
	//ボールの座標を取得
	XMFLOAT3 pos = g_BallPosOld;
	//g_BallPosOld = GetBallPositon();

	//前回のボールと現在のボールの座標の差分
	pos.x = g_BallPosOld.x - pos.x;
	pos.y = g_BallPosOld.y - pos.y;
	pos.z = g_BallPosOld.z - pos.z;
	//カメラを移動
	CameraObject.Position.x += pos.x;
	CameraObject.Position.y += pos.y;
	CameraObject.Position.z += pos.z;

	//ボールの座標を注視点としてセット
	CameraObject.AtPosition.x = g_BallPosOld.x;
	CameraObject.AtPosition.y = g_BallPosOld.y;
	CameraObject.AtPosition.z = g_BallPosOld.z;

	//注視点を中心にカメラの回転(Y軸回転)
	float Rotation = 0.0f;
	if (Keyboard_IsKeyDown(KK_A))
	{
		Rotation = 1.0f;
	}
	if (Keyboard_IsKeyDown(KK_D))
	{
		Rotation = -1.0f;
	}

	//注視点を中心にカメラの回転
	XMFLOAT2 vec;
	vec.x = CameraObject.Position.x - CameraObject.AtPosition.x;
	vec.y = CameraObject.Position.z - CameraObject.AtPosition.z;
	//注視点からカメラへのベクトル
	float co = cosf(XMConvertToRadians(Rotation));
	float si = sinf(XMConvertToRadians(Rotation));
	CameraObject.Position.x = (vec.x * co - vec.y * si);
	CameraObject.Position.z = (vec.x * si + vec.y * co);
	CameraObject.Position.x += CameraObject.AtPosition.x;
	CameraObject.Position.z += CameraObject.AtPosition.z;
	/*
	//vecを正規化
	float len = sqrtf(vec.x * vec.x + vec.y * vec.y);
	vec.x /= len;
	vec.y /= len;

	float speed = 0.0f;
	if (Keyboard_IsKeyDown(KK_W))
	{
		speed = -0.1f;
	}
	if (Keyboard_IsKeyDown(KK_S))
	{
		speed = 0.1f;
	}
	vec.x *= speed;
	vec.y *= speed;
	CameraObject.Position.x += vec.x;
	CameraObject.Position.z += vec.y;
	CameraObject.AtPosition.x += vec.x;
	CameraObject.AtPosition.z += vec.y;
	*/

	if (Keyboard_IsKeyDown(KK_Q))
	{
		CameraObject.Fov += 0.3f;
		if (CameraObject.Fov > 160.0f)
		{
			CameraObject.Fov = 160.0f;
		}
	}
	if (Keyboard_IsKeyDown(KK_E))
	{
		CameraObject.Fov -= 0.3f;
		if (CameraObject.Fov < 5.0f)
		{
			CameraObject.Fov = 5.0f;
		}
	}
}
