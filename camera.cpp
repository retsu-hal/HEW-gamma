#include "camera.h"
#include"keyboard.h"
#include "mouse.h"
#include "player3D.h"
#include "debug.h"


//=========================================================================================================
// グローバル変数
//=========================================================================================================
static CAMERA CameraObject;
XMFLOAT3 g_PlayerPosOld;
static bool debugMode = TRUE;


Mouse_State ms{};
float cSize = 1.0f;

static bool   gCamAnglesInit = false;
static float  gYawDeg = 0.0f;
static float  gPitchDeg = 0.0f;
static const float kPitchMin = -85.0f;
static const float kPitchMax = 85.0f;

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

	g_PlayerPosOld = GetPlayer3DPositon();
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
<<<<<<< HEAD
	static bool CamMode = false;

	if (Keyboard_IsKeyDownTrigger(KK_C)) {
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


	if (debugMode)
	{
		ImGui::Begin("Debug - CHEN");
		if (CamMode) ImGui::Text("is Camera");
		else ImGui::Text("is Player");
		ImGui::End();
	}
=======
	Mose();
>>>>>>> e5c42e52dadd8f7ac1b98dbd526d411a4a6f1297
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

        const float sensitivityYaw = 0.5f;   // x  マウス感度調整用
        const float sensitivityPitch = 0.5f; // y  マウス感度調整用
        const float moveSpeedBase = 0.1f;

        // 常に相対モードにする
        Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
       
        // FOV調整
        if (Keyboard_IsKeyDown(KK_Q)) {
            CameraObject.Fov += 0.5f;
            if (CameraObject.Fov > 160.0f) CameraObject.Fov = 160.0f;
        }
        if (Keyboard_IsKeyDown(KK_E)) {
            CameraObject.Fov -= 0.5f;
            if (CameraObject.Fov < 5.0f) CameraObject.Fov = 5.0f;
        }

        // プレイヤーの移動量取得
        XMFLOAT3 playerDelta = g_PlayerPosOld;
        g_PlayerPosOld = GetPlayer3DPositon();
        playerDelta.x = g_PlayerPosOld.x - playerDelta.x;
        playerDelta.y = g_PlayerPosOld.y - playerDelta.y;
        playerDelta.z = g_PlayerPosOld.z - playerDelta.z;

        XMFLOAT3 pos = CameraObject.Position;
        XMFLOAT3 at = CameraObject.AtPosition;

        // 初期角度計算
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

        // マウスによる回転
        if (ms.positionMode == MOUSE_POSITION_MODE_RELATIVE) {
            gYawDeg -= ms.x * sensitivityYaw;
            gPitchDeg += ms.y * sensitivityPitch;
            XMVECTOR v = XMVectorSet(gPitchDeg, 0, 0, 0);
            XMVECTOR lo = XMVectorReplicate(kPitchMin);
            XMVECTOR hi = XMVectorReplicate(kPitchMax);
            v = XMVectorClamp(v, lo, hi);
            gPitchDeg = XMVectorGetX(v);
        }

        // カメラ半径と位置計算
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

        // 正規化された方向ベクトル
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

        // プレイヤー移動分をカメラに反映
        pos.x += playerDelta.x;
        pos.y += playerDelta.y;
        pos.z += playerDelta.z;
        at.x += playerDelta.x;
        at.y += playerDelta.y;
        at.z += playerDelta.z;

        CameraObject.Position = pos;
        CameraObject.AtPosition = at;
    }

void Keyb()
{
	//プレイヤーの座標を取得
	XMFLOAT3 pos = g_PlayerPosOld;
	g_PlayerPosOld = GetPlayer3DPositon();

	//前回のプレイヤーと現在のプレイヤーの座標の差分
	pos.x = g_PlayerPosOld.x - pos.x;
	pos.y = g_PlayerPosOld.y - pos.y;
	pos.z = g_PlayerPosOld.z - pos.z;
	//カメラを移動
	CameraObject.Position.x += pos.x;
	CameraObject.Position.y += pos.y;
	CameraObject.Position.z += pos.z;

	//ボールの座標を注視点としてセット
	CameraObject.AtPosition.x = g_PlayerPosOld.x;
	CameraObject.AtPosition.y = g_PlayerPosOld.y;
	CameraObject.AtPosition.z = g_PlayerPosOld.z;

	//注視点を中心にカメラの回転(Y軸回転)
	float Rotation = 0.0f;

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

}
