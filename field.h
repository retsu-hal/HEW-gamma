//field.h
#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "direct3d.h"
#include "sprite.h"
#include "shader.h"
#include <vector>


//=========================================================================================================
// マクロ
//=========================================================================================================
#define BOX_RADIUS (0.5f)
//=========================================================================================================
// 構造体
//=========================================================================================================
enum FIELD
{
    FIELD_NONE = 0,
    FIELD_GROUND ,
    FIELD_WALL,
    FIELD_OBJ_BOX,
    FIELD_EMPTY_BOX,
    FIELD_OBJ_1,
    FIELD_OBJ_2,
    FIELD_OBJ_3,
    FIELD_BENCH,
    FIELD_DUSTBOX,
    FIELD_FOUNTAIN,
    FIELD_POLE,
    FIELD_FENCE,
    FIELD_STAGE_1,
    FIELD_STAGE_2,
    FIELD_STAGE_3,
    FIELD_GOAL,

    FIELD_SEESAW_1,
	FIELD_SEESAW_2,

    FIELD_MANHOLE,


    FIELD_MAX,
};

enum GAME_STAGE
{
    STAGE_NONE = 0,
    STAGE_SELECT,
    STAGE_1,
    STAGE_2,
    STAGE_3,

    STAGE_MAX,
};

struct MAPDATA
{
public:
    XMFLOAT3 pos;    //位置
    FIELD no;        //種類
	XMFLOAT3 scale;
	XMFLOAT3 rotate;

    XMFLOAT3 colliderHalf;
    bool useCustomCollider;

    MAPDATA()
        : pos{ 0, 0, 0 }
        , no(FIELD_GROUND)
        , scale{ 1, 1, 1 }
        , rotate{ 0, 0, 0 }
        , colliderHalf{ 0.5f, 0.5f, 0.5f }
        , useCustomCollider(false)
    {}
};

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

// 追加: マップで見つけたプレイヤー初期位置を取得
XMFLOAT3 Field_GetPlayerStartPosition();

void SetCurrentStage(GAME_STAGE stage);
GAME_STAGE GetCurrentStage();

XMFLOAT3 Field_GetCollisionHalfSize(const MAPDATA& m);