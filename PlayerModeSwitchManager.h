//PlayerModeSwitchManager.h
#pragma once
#include <DirectXMath.h>
using namespace DirectX;

// プレイヤーモード
enum PLAYER_MODE
{
	MODE_3D = 0,
	MODE_2D
};

// ボックスの面
enum BOX_FACE
{
	FACE_NONE = 0,
	FACE_POS_X, FACE_NEG_X,
	FACE_POS_Z, FACE_NEG_Z,
	FACE_POS_Y, FACE_NEG_Y
};

// モード切り替えターゲット情報
struct SWITCH_TARGET
{
	int     fieldIndex = -1;
	BOX_FACE face = FACE_NONE;
	XMFLOAT3 normal = { 0,0,0 };
};

void PlayerModeSwitchManager_Init();// 初期化
void PlayerModeSwitchManager_Update();// 更新
PLAYER_MODE PlayerModeSwitchManager_GetMode();// モード取得
