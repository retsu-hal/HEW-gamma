/*==============================================================================

   シェーダー [shader.h]
														 Author : Youhei Sato
														 Date   : 2025/05/15
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef SHADER_H
#define	SHADER_H

#include <d3d11.h>
#include <DirectXMath.h>
#include "direct3d.h"

#include "model.h"
#define MAX_BONES 100

bool Shader_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Shader_Finalize();

void Shader_SetMatrix(const DirectX::XMMATRIX& matrix);

void Shader_SetWorldMatrix(const DirectX::XMMATRIX& matrix);
void Shader_SetLight(LIGHT light);

void Shader_SetShadowMatrix(const DirectX::XMMATRIX& mat); // Set at b3
void Shader_SetShadowMap(ID3D11ShaderResourceView* shadowSRV); // t1
void Shader_SetShadowSampler(ID3D11SamplerState* sampler); // s1
void Shader_SetShadowLightData(const DirectX::XMFLOAT3& pos, float radius, float intensity, float shadowPassMode = 0.0f);

void Shader_SetBones(MODEL* model);

void Shader_Begin();

#endif // SHADER_H
