#pragma once
#include <d3d11.h>
#include "direct3d.h"
#include <DirectXMath.h>
using namespace DirectX;

void InitializeBillBoard();
void FinalizeBillBoard();
void DrawBillBoard(XMFLOAT3 pos, XMFLOAT2 size, XMFLOAT4 col, int bno, int wc, int hc);
ID3D11ShaderResourceView* GetBillBoardTexture();

