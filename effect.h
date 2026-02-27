//effect.h
#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "direct3d.h"


//=========================================================================================================
//構造体
//=========================================================================================================
class EFFECT
{
	public:
		bool		Enable;
		DirectX::XMFLOAT3	Position;
		int			FrameCount;	//アニメーションカウンター

};

//=========================================================================================================
//プロトタイプ宣言
//=========================================================================================================
//メイン処理関数
void Effect_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Effect_Finalize();
void Effect_Update();
void Effect_Draw();

void CreateEffect(DirectX::XMFLOAT2 Position);	//エフェクト作成

