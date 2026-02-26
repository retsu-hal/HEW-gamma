//Collision.h
#pragma once


#include <DirectXMath.h>
#include <vector>


#include"field.h"
#include "ShadowColliderBox.h"
#include "Player2DCapsule.h"
#include"UtilDebug.h"
#include "UtilMath.h"


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
void Collision_ResetShadowContactState();

int Player3DField_Collision();
bool Collision_PlayerTrigger(TRIGGER_HIT* outHit, float extraRange = 0.0f);

int Collision_Player2D_MoveAndCollision();

int Player2DField_Collision();
bool Collision_Player2DTrigger(TRIGGER_HIT* outHit, float extraRange = 0.0f);

bool Player2DShadow_Collision();
bool Player2DShadow_TopContact();


bool Resolve_OBB_OBB_ZY(
    const XMFLOAT3& posA, const XMFLOAT3& halfA, float rotZRadA,
    const XMFLOAT3& posB, const XMFLOAT3& halfB, float rotYDegB,
    XMFLOAT3* outPush, XMFLOAT3* outNorm);

bool OBB_Intersect_ZY(
    const XMFLOAT3& posA, const XMFLOAT3& halfA, float rotZRadA,
    const XMFLOAT3& posB, const XMFLOAT3& halfB, float rotYDegB);


bool Resolve_Capsule2D_OBB(
    const Capsule2D& capsule,
    const XMFLOAT3& boxCenter, const XMFLOAT3& boxHalf, float boxYawDeg,
    XMFLOAT3* outPush, XMFLOAT3* outNormal);

bool Capsule2D_Intersect_OBB(
    const Capsule2D& capsule,
    const XMFLOAT3& boxCenter, const XMFLOAT3& boxHalf, float boxYawDeg);


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


void Collision_SetShadowDebugOptions(const ShadowDebugOptions& options);




struct OptionRect
{
	float x, y, width, height;
	bool contains(float mousex, float mousey)const
	{
		return mousex >= x && mousex <= x + width && mousey >= y && mousey <= y + height;
	}
};





