#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "direct3d.h"
using namespace DirectX;

enum  INPUT_DEVICE
{
	INPUT_DEVICE_KEYBOARD = 0,
	INPUT_DEVICE_PAD,
	INPUT_DEVICE_MAX,
};

enum OPTION_SELECT
{
	OPTION_SELECT_NONE = -1,
	OPTION_SELECT_VOLUME,
	OPTION_SELECT_BGM,
	OPTION_SELECT_MOUSE,
	OPTION_SELECT_INPUT,
	OPTION_SELECT_RESET,
	OPTION_SELECT_BACK,
	OPTION_SELECT_MAX,
};

struct OPTION_SETTING
{
	float bgmVolume;
	float seVolume;
	float mouseSensitivity;
	INPUT_DEVICE inputDevice = INPUT_DEVICE_PAD;
};
//=========================================================================================================
// プロトタイプ宣言
//=========================================================================================================
void Option_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Option_Finalize(void);
void Option_Update(void);
void Option_Draw(void);

float Option_GetBGMVolume();

void Option_Reset();
