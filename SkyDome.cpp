#include "SkyDome.h"
#include "Camera.h"
#include "shader.h"

#define ROTATION_SPEED (2.0f/60.0f)

//グローバル変数
static SkyDome g_Sky;
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;

void SkyDome_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	g_Sky.Model = ModelLoad("asset\\model\\sky.fbx");
	g_Sky.m_position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	g_Sky.m_rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	g_Sky.m_scaling = XMFLOAT3(100.0f, 60.0f, 100.0f);

	g_Sky.m_rotation_speed = ROTATION_SPEED;
}

void SkyDome_Finalize(void)
{
	ModelRelease(g_Sky.Model);
}

void SkyDome_Update(void)
{
	g_Sky.m_position = GetCameraPosition();
	g_Sky.m_rotation.y += g_Sky.m_rotation_speed;
}

void SkyDome_Draw(void)
{
	//シェーダーを描画パイプライン設定
	Shader_Begin();

	XMMATRIX Projection = GetProjectionMatrix();

	XMMATRIX View = GetViewMatrix();

	//変換行列の作成
	XMMATRIX VP = View * Projection;

	XMMATRIX ScalingMatrix = XMMatrixScaling(
		g_Sky.m_scaling.x, g_Sky.m_scaling.y, g_Sky.m_scaling.z);

	//平行移動行列の作成
	XMMATRIX TranslationMAtrix = XMMatrixTranslation(
		g_Sky.m_position.x, g_Sky.m_position.y, g_Sky.m_position.z);

	//回転行列の作成
	XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(g_Sky.m_rotation.x),
		XMConvertToRadians(g_Sky.m_rotation.y),
		XMConvertToRadians(g_Sky.m_rotation.z));

	XMMATRIX WorldMatrix = ScalingMatrix * RotationMatrix * TranslationMAtrix;
	XMMATRIX ModelFix = XMMatrixRotationX(XM_PIDIV2);
	XMMATRIX FixedWorld = ModelFix * WorldMatrix;
	XMMATRIX WVP = FixedWorld * VP;

	//変換行列を頂点シェーダーへセット
	Shader_SetWorldMatrix(FixedWorld);
	Shader_SetMatrix(WVP);


	ModelDraw(g_Sky.Model);
}

SkyDome* GetSkyDome()
{
	return &g_Sky;
}