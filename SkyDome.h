#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include "direct3d.h"
#include "model.h"

using namespace DirectX;

void SkyDome_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void SkyDome_Finalize(void);
void SkyDome_Update(void);
void SkyDome_Draw(void);

class SkyDome
{
public:
	XMFLOAT3 m_position;
	XMFLOAT3 m_rotation;
	XMFLOAT3 m_scaling;
	float m_rotation_speed;


	MODEL* Model;
};

SkyDome* GetSkyDome();