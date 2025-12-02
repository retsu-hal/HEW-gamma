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
#define BOX_RADIUS (0.5f)		//半径

//=========================================================================================================
// 構造体定義・定義
//=========================================================================================================
enum FIELD
{
	FIELD_BOX=0,
	FIELD_OBT_0,

	FIELD_FLOOR,
	FIELD_WALL,

	FIELD_MAX,
};

class MAPDATA
{
public:
	XMFLOAT3 pos;	//座標
	FIELD no;			//種類
};

//=========================================================================================================
// プロトタイプ宣言
//=========================================================================================================
void field_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void field_Finalize(void);
void field_Update(void);
void field_Draw(void);
void CreateBox(void);
MAPDATA* GetFieldMap(void);

