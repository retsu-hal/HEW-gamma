//camera.h
#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "direct3d.h"


class CAMERA
{
public:
	DirectX::XMFLOAT3 Position;			// カメラの位置
	DirectX::XMFLOAT3 AtPosition;		// カメラの注視点
	DirectX::XMFLOAT3 UpVector;		// カメラの上方向ベクトル	
	
	DirectX::XMMATRIX View;				//ビュー行列
	DirectX::XMMATRIX Projection;		//プロジェックション行列	

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
void SetCameraPosition(DirectX::XMFLOAT3);
void SetCameraAtPosition(DirectX::XMFLOAT3);
void SetCameraUpVector(DirectX::XMFLOAT3);
DirectX::XMMATRIX GetViewMatrix();
DirectX::XMMATRIX GetProjectionMatrix();
DirectX::XMFLOAT3 GetCameraAtPosition();
DirectX::XMFLOAT3 GetCameraPosition();

void Camera_Reset2DState();

void Camera_CheckCollision(DirectX::XMFLOAT3 targetPos,
	DirectX::XMFLOAT3 desiredCamPos, DirectX::XMFLOAT3& outCamPos);

void Player2DCamera_DebugUpdate();
// 既存宣言の末尾付近に追記
void SetCameraMouseSensitivity(float yaw, float pitch);
float GetMouseSensYaw();
float GetMouseSensPitch();
