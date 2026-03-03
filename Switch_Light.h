#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include "direct3d.h"

using namespace DirectX;

void SwitchLight_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void SwitchLight_Finalize(void);
void SwitchLight_Update(void);
void SwitchLight_Draw(void);

// Returns true if currently controlling the light
bool SwitchLight_IsLightMode(void);
XMFLOAT3 SwitchLight_GetLightObjPosition(void);
