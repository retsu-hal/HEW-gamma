#include "camera.h"
#include"keyboard.h"

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
		if (CameraObject.Fov <5.0f)
		{
			CameraObject.Fov = 5.0f;
		}
	}

	
}

//=========================================================================================================
// 描画処理
//=========================================================================================================
void Camera_Draw()
{
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
