//Trail.h
#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include"direct3d.h"
#include"sprite.h"
#include"shader.h"
using namespace DirectX;
//=========================================================================================================
// プロトタイプ宣言
//=========================================================================================================
void Trail_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Trail_Finalize();
void Trail_Update();
void Trail_Draw();
void SetTrailPosition(XMFLOAT3 pos);