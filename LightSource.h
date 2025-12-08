#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include "direct3d.h"
#include "model.h"

using namespace DirectX;

#define BALL_SPEEDMAX (1.0f)
#define BALL_RADIUS (0.2f)

void Ball_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Ball_Finalize(void);
void Ball_Update(void);
void Ball_Draw(void);

XMFLOAT3 GetBall_Position();

class LightSource
{
public:
	XMFLOAT3 m_position;
	XMFLOAT3 m_rotation;
	XMFLOAT3 m_scaling;
	XMFLOAT3 m_velo;
	XMFLOAT3 m_acceleration;

	MODEL* Model;

	XMVECTOR Quaternion;
	XMVECTOR Axis;
	float qSpeed;
};

LightSource* GetBall();
XMMATRIX Ball_GetWorldMatrix();
void Ball_DrawRaw(const XMMATRIX& world, const XMMATRIX& matrix);