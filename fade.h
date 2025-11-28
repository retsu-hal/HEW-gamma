#pragma once

#include "direct3d.h"
#include "sprite.h"
#include "Manager.h"

//=========================================================================================================
// 構造体定義
//=========================================================================================================
enum FADE_STATE
{
	FADE_NONE = 0,
	FADE_IN,
	FADE_OUT,

};

struct FadeObject
{
	FADE_STATE	state;			//フェード処理状態
	float		count;			//カウンター
	float		frame;			//フェード処理時間
	XMFLOAT4	fadecolor;		//フェード色
	SCENE		scene;			//次に切り替わるシーン
};

//=========================================================================================================
// プロトタイプ宣言
//=========================================================================================================
void Fade_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Fade_Finalize();
void Fade_Update();
void Fade_Draw();
void	SetFade(int fadeframe, XMFLOAT4 color, FADE_STATE state, SCENE scene);
FADE_STATE	GetFadeState();



