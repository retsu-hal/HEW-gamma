#pragma once
#include "d3d11.h"

//=========================================================================================================
// プロトタイプ宣言
//=========================================================================================================
void Polygon3D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Polygon3D_Finalize();
void Polygon3D_Update();
void Polygon3D_Draw();