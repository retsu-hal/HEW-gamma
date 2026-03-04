//LightSource.cpp
#include "LightSource.h"
#include "Camera.h"
#include "sprite.h"//スプライト機能を追加
#include "shader.h"
#include "keyboard.h"

#include"Collision.h"

#include "debug.h"
static bool debugMode;

LightSource g_Light;

//グローバル変数
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;


void Light_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	g_Light.Model = ModelLoad("asset\\model\\ball.fbx");
	g_Light.m_position = XMFLOAT3(0.0f, 5.0f, 5.0f);
	g_Light.m_rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	g_Light.m_scaling = XMFLOAT3(1.0f, 1.0f, 1.0f);
	g_Light.m_velo = XMFLOAT3(0.0f, 0.0f, 0.0f);

	g_Light.m_acceleration = XMFLOAT3(0.0f, -9.8f / 600.0f * 0.5f, 0.0f);
}
void Light_Finalize(void)
{
	ModelRelease(g_Light.Model);
}
void Light_Update(void)
{
}
void Light_Draw(void)
{

	//シェーダーを描画パイプライン設定
	Shader_Begin();

	XMMATRIX Projection = GetProjectionMatrix();

	XMMATRIX View = GetViewMatrix();

	//変換行列の作成
	XMMATRIX VP = View * Projection;

	XMMATRIX ScalingMatrix = XMMatrixScaling(
		g_Light.m_scaling.x, g_Light.m_scaling.y, g_Light.m_scaling.z);

	//平行移動行列の作成
	XMMATRIX TranslationMAtrix = XMMatrixTranslation(
		g_Light.m_position.x, g_Light.m_position.y, g_Light.m_position.z);

	//回転行列の作成
	XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(g_Light.m_rotation.x),
		XMConvertToRadians(g_Light.m_rotation.y),
		XMConvertToRadians(g_Light.m_rotation.z));

	XMMATRIX WorldMatrix = ScalingMatrix * RotationMatrix * TranslationMAtrix;
	XMMATRIX WVP = WorldMatrix * VP;

	//変換行列を頂点シェーダーへセット
	Shader_SetWorldMatrix(WorldMatrix);
	Shader_SetMatrix(WVP);


	ModelDraw(g_Light.Model);
}

XMFLOAT3 GetLight_Position()
{
	return g_Light.m_position;
}

LightSource* GetLight()
{
	return &g_Light;
}

XMMATRIX Light_GetWorldMatrix()
{
	// Using light's translation, rotation, scaling
	LightSource* light = GetLight(); // Or your own way to fetch Light object
	XMMATRIX scale = XMMatrixScaling(light->m_scaling.x, light->m_scaling.y, light->m_scaling.z);
	XMMATRIX rotation = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(light->m_rotation.x),
		XMConvertToRadians(light->m_rotation.y),
		XMConvertToRadians(light->m_rotation.z));
	XMMATRIX translate = XMMatrixTranslation(light->m_position.x, light->m_position.y, light->m_position.z);
	return scale * rotation * translate;
}

void Light_DrawRaw(const XMMATRIX& world, const XMMATRIX& matrix)
{
	Shader_SetWorldMatrix(world);
	Shader_SetMatrix(matrix);
	ModelDraw(g_Light.Model);
}