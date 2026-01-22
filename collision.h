//Collision.h
#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include"direct3d.h"
#include"sprite.h"
#include"shader.h"
using namespace DirectX;
#include"field.h"
#include"player3D.h"
//=========================================================================================================
// マクロ定義
//=========================================================================================================
#define COE (0.1f)//反発係数

//=========================================================================================================
// 構造体宣言
//=========================================================================================================
enum COLLISION_HIT
{
	HIT_NONE,
	HIT_GROUND,
	HIT_WALL_NegZ,		//-Z
	HIT_WALL_PlusX,		//+x
	HIT_WALL_PlusZ,		//+Z
	HIT_WALL_NegX,		//-X

	HIT_MAX,
};

enum TRIGGER_SIDE//トリガーが当たった面
{
	TRIGGER_SIDE_NONE = 0,
	TRIGGER_SIDE_FRONT,
	TRIGGER_SIDE_BACK,
	TRIGGER_SIDE_LEFT,
	TRIGGER_SIDE_RIGHT,
};


//=========================================================================================================
// プロトタイプ宣言
//=========================================================================================================

int Player3DField_Collision();

void Collision_DebugDraw();

struct TRIGGER_HIT//トリガーヒット情報
{
	bool hit = false;
	size_t mapIndex = 0;// 当たったマップデータのインデックス
	FIELD type = FIELD_MAX;
	TRIGGER_SIDE side = TRIGGER_SIDE_NONE;
};

bool Collision_PlayerTrigger(TRIGGER_HIT* outHit, float extraRange = 0.0f);