//title.h
#pragma once

#include "direct3d.h"

//=========================================================================================================
// プロトタイプ宣言
//=========================================================================================================

enum TITLE_OPTION
{
	TITLE_OPTION_START = 0,
	TITLE_OPTION_HOWTO,
	TITLE_OPTION_MAX
};

enum TITLE_OPTION_PAGE
{
	PAGE_1 = 0,
	PAGE_2,
	PAGE_MAX
};

void Title_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Title_Finalize();
void Title_Update();
void Title_Draw();


