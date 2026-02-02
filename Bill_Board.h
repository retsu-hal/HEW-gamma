#pragma once
#include <d3d11.h>
#include "direct3d.h"
#include <DirectXMath.h>
using namespace DirectX;

//プロトタイプ宣言
void		InitializeBillBoard();
void		FinalizeBillBoard();


void		DrawBillBoard(XMFLOAT3 pos, XMFLOAT2 size, XMFLOAT4 col, int bno, int wc, int hc);//行列使用版


// 頂点構造体
struct Vertex_BillBoard
{
	XMFLOAT3 position; // 頂点座標  //XMFLOAT3へ変更
	XMFLOAT4 color;		//頂点カラー（R,G,B,A）
	XMFLOAT2 texCoord;	//テクスチャ座標
};
