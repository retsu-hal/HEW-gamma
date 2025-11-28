#pragma once

#include "direct3d.h"

//=========================================================================================================
// プロトタイプ宣言
//=========================================================================================================
void Result_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Result_Finalize();
void Result_Update();
void Result_Draw();

