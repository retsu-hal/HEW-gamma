#pragma once
#include <d3d11.h>
#include "direct3d.h"
#include <DirectXMath.h>


void InitializeBillBoard();
void FinalizeBillBoard();

void DrawBillBoard(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT2 size, DirectX::XMFLOAT4 col, int bno, int wc, int hc);
//ID3D11ShaderResourceView* GetBillBoardTexture();

// 頂点構造体
struct Vertex_BillBoard
{
	DirectX::XMFLOAT3 position; // 頂点座標  //XMFLOAT3へ変更
	DirectX::XMFLOAT4 color;		//頂点カラー（R,G,B,A）
	DirectX::XMFLOAT2 texCoord;	//テクスチャ座標
};