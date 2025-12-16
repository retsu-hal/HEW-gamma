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
//=========================================================================================================
// プロトタイプ宣言
//=========================================================================================================

int Player3DField_Collision();

void Collision_DebugDraw();

bool Collision_RayToField(
	const XMFLOAT3& start,
	const XMFLOAT3& dir,
	float maxDist,
	float* hitY
);
