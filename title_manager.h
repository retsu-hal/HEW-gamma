#pragma once

#include "direct3d.h"

void Title_Manager_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Title_Manager_Finalize();
void Title_Manager_Update();
void Title_Manager_Draw();

