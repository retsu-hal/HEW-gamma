//LightSource.h
#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include "direct3d.h"
#include "model.h"


#define LIGHT_SPEEDMAX (1.0f)
#define LIGHT_RADIUS (0.2f)

void Light_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Light_Finalize(void);
void Light_Update(void);
void Light_Draw(void);

XMFLOAT3 GetLight_Position();

class LightSource
{
public:
	XMFLOAT3 m_position;
	XMFLOAT3 m_rotation;
	XMFLOAT3 m_scaling;
	XMFLOAT3 m_velo;
	XMFLOAT3 m_acceleration;

	MODEL* Model;
};

LightSource* GetLight();
XMMATRIX Light_GetWorldMatrix();
void Light_DrawRaw(const XMMATRIX& world, const XMMATRIX& matrix);