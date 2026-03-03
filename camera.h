// camera.h
#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "direct3d.h"

//=========================================================================================================
// カメラ構造体（シンプルなビュー・プロジェクション管理）
// ※ 本プロジェクトは C API 風の関数群で CameraObject を操作する設計
//=========================================================================================================
class CAMERA
{
public:
	DirectX::XMFLOAT3 Position;     // カメラ位置（World）
	DirectX::XMFLOAT3 AtPosition;   // 注視点（World）
	DirectX::XMFLOAT3 UpVector;     // 上方向ベクトル（World）

	DirectX::XMMATRIX View;         // ビュー行列
	DirectX::XMMATRIX Projection;   // プロジェクション行列

	float Fov;                      // 視野角（deg）
	float Aspect;                   // アスペクト比
	float NearClip;                 // 近クリップ
	float FarClip;                  // 遠クリップ
};

//=========================================================================================================
// ライフサイクル
//=========================================================================================================
void Camera_Initialize();
void Camera_Finalize();

//=========================================================================================================
// 更新（プレイヤー視点）
//=========================================================================================================
void Player3DCamera_Update();
void Player2DCamera_Update();
void Player2DCamera_DebugUpdate();
void Title_Camera_Update();
void LightCamera_Update();

//=========================================================================================================
// 描画（行列更新）
//=========================================================================================================
void Camera_Draw();

//=========================================================================================================
// セッター / ゲッター
//=========================================================================================================
void SetCameraFov(float fov);
void SetCameraAspect(float asp);
void SetCameraClip(float nearClip, float farClip);
void SetCameraPosition(DirectX::XMFLOAT3 pos);
void SetCameraAtPosition(DirectX::XMFLOAT3 atpos);
void SetCameraUpVector(DirectX::XMFLOAT3 up);

DirectX::XMMATRIX GetViewMatrix();
DirectX::XMMATRIX GetProjectionMatrix();
DirectX::XMFLOAT3 GetCameraAtPosition();
DirectX::XMFLOAT3 GetCameraPosition();

//=========================================================================================================
// 2Dカメラ内部状態のリセット（2D->3D切り替え等で使用）
//=========================================================================================================
void Camera_Reset2DState();

//=========================================================================================================
// カメラ衝突（ターゲット->カメラのレイで壁に当たる場合、カメラ位置を手前に詰める）
//=========================================================================================================
void Camera_CheckCollision(
	DirectX::XMFLOAT3 targetPos,
	DirectX::XMFLOAT3 desiredCamPos,
	DirectX::XMFLOAT3& outCamPos
);

//=========================================================================================================
// マウス感度（Yaw/Pitch）
//=========================================================================================================
void SetCameraMouseSensitivity(float yaw, float pitch);
float GetMouseSensYaw();
float GetMouseSensPitch();

// Add alongside Camera_Reset2DState()
void Camera_ResetLightState();