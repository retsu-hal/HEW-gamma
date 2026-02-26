//camera.h
#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "direct3d.h"


class CAMERA
{
public:
	XMFLOAT3 Position;			// カメラの位置
	XMFLOAT3 AtPosition;		// カメラの注視点
	XMFLOAT3 UpVector;		// カメラの上方向ベクトル	
	
	XMMATRIX View;				//ビュー行列
	XMMATRIX Projection;		//プロジェックション行列	

	float Fov;							//視野角
	float Aspect;						//アスペクト比
	float NearClip;					//近面クリップ
	float FarClip;						//遠面クリップ
};

//=========================================================================================================
// プロトタイプ宣言
//=========================================================================================================
void Camera_Initialize();
void Camera_Finalize();
void Player3DCamera_Update();
void Player2DCamera_Update();
void Title_Camera_Update();
void Camera_Draw();
void SetCameraFov(float);
void SetCameraAspect(float);
void SetCameraClip(float, float);
void SetCameraPosition(XMFLOAT3);
void SetCameraAtPosition(XMFLOAT3);
void SetCameraUpVector(XMFLOAT3);
XMMATRIX GetViewMatrix();
XMMATRIX GetProjectionMatrix();
XMFLOAT3 GetCameraAtPosition();
XMFLOAT3 GetCameraPosition();

void Camera_Reset2DState();

void Camera_CheckCollision(XMFLOAT3 targetPos, XMFLOAT3 desiredCamPos, XMFLOAT3& outCamPos);

void Player2DCamera_DebugUpdate();
// 既存宣言の末尾付近に追記
void SetCameraMouseSensitivity(float yaw, float pitch);
float GetMouseSensYaw();
float GetMouseSensPitch();
