#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include"direct3d.h"
#include"sprite.h"
#include"shader.h"
using namespace DirectX;
//=========================================================================================================
// マクロ定義
//=========================================================================================================


//=========================================================================================================
// プロトタイプ宣言
//=========================================================================================================
void Pause_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Pause_Finalize(void);
void Pause_Update(void);
void Pause_Draw(void);
bool Pause_IsActive(); 
void Pause_Toggle(); 
