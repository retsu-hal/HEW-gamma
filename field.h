#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "direct3d.h"
#include "sprite.h"
#include "shader.h"
#include <vector>
using namespace DirectX;

//=========================================================================================================
// マクロ
//=========================================================================================================
#define BOX_RADIUS (0.5f)
//=========================================================================================================
// 構造体
//=========================================================================================================
enum FIELD
{
    FIELD_GROUND = 0,
    FIELD_WALL,
    FIELD_OBJ_BOX,
    FIELD_OBJ_1,
    FIELD_OBJ_2,
    FIELD_GOAL,
    FIELD_MAX,
};

class MAPDATA
{
public:
    XMFLOAT3 pos;    //位置
    FIELD no;        //種類
};

// グローバルなフィールドデータ配列
//extern std::vector<MAPDATA> g_MapData;

//=========================================================================================================
// プロトタイプ宣言
//=========================================================================================================
void field_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void field_Finalize(void);
void field_Update(void);
void field_Draw(void);
void CreateBox(void);

bool LoadMapFromFile(const char* filename);
std::vector<MAPDATA>& GetFieldMap();

XMMATRIX Field_GetWorldMatrix(int i);
void Field_DrawShadowMap(const XMMATRIX& world, const XMMATRIX& matrix, int i);

