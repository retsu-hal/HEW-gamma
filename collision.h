//Collision.h
#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;
#include"direct3d.h"
#include"sprite.h"
#include"shader.h"
#include"field.h"
#include"player3D.h"
#include "ShadowColliderBox.h"
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
	TRIGGER_SIDE_TOP,    
	TRIGGER_SIDE_BOTTOM, 
};

struct TRIGGER_HIT//トリガーヒット情報
{
	bool hit = false;
	size_t mapIndex = 0;// 当たったマップデータのインデックス
	FIELD type = FIELD_MAX;
	TRIGGER_SIDE side = TRIGGER_SIDE_NONE;
};

//=========================================================================================================
// プロトタイプ宣言
//=========================================================================================================

int Player3DField_Collision();

bool Collision_PlayerTrigger(TRIGGER_HIT* outHit, float extraRange = 0.0f);

int Player2DField_Collision();
bool Collision_Player2DTrigger(TRIGGER_HIT* outHit, float extraRange = 0.0f);


bool Resolve_OBB_OBB_ZY(
	const XMFLOAT3& posA, const XMFLOAT3& halfA, float rotZRadA,
	const XMFLOAT3& posB, const XMFLOAT3& halfB, float rotYDegB,
	XMFLOAT3* outPush, XMFLOAT3* outNorm);

bool OBB_Intersect_ZY(
	const XMFLOAT3& posA, const XMFLOAT3& halfA, float rotZRadA,
	const XMFLOAT3& posB, const XMFLOAT3& halfB, float rotYDegB);


void Collision_SetShadowPrisms(const std::vector<const ShadowPrism*>& prisms);
const std::vector<const ShadowPrism*>& Collision_GetShadowPrisms();


void Collision_SetShadowPrism(const ShadowPrism* prism);
const ShadowPrism* Collision_GetShadowPrism();

void Collision_DebugClearExtraBoxes();


void Collision_DebugAddExtraAABB(const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& half,
	unsigned char r = 255, unsigned char g = 255, unsigned char b = 0, unsigned char a = 255);

void Collision_DebugAddExtraOBB(const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& half, const DirectX::XMFLOAT3& rotDeg,
	unsigned char r = 255, unsigned char g = 255, unsigned char b = 0, unsigned char a = 255);


void Collision_DebugDraw();


struct ShadowDebugOptions
{
	bool drawPrism = true; 
	bool drawAABB = false; 
	bool drawNormal = true;   
	bool drawVertices = false;

	unsigned int prismColor = 0xFF0000FF; 
	unsigned int aabbColor = 0x80FFFF00;
	unsigned int normalColor = 0xFFFFFF00;
	unsigned int vertexColor = 0xFF00FF00;
};

void Collision_SetShadowDebugOptions(const ShadowDebugOptions& options);

bool Player2DShadow_BlockMoveAtContact(float skin = 0.02f);
bool Player2DShadow_Collision();
bool Player2DShadow_TopContact();






