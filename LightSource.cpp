#include "LightSource.h"
#include "Camera.h"
#include "sprite.h"//スプライト機能を追加
#include "shader.h"
#include "keyboard.h"

#include"Collision.h"

LightSource g_Ball;

//グローバル変数
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;


void Ball_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	g_Ball.Model = ModelLoad("asset\\model\\ball.fbx");
	g_Ball.m_position = XMFLOAT3(0.0f, 5.0f, 5.0f);
	g_Ball.m_rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	g_Ball.m_scaling = XMFLOAT3(1.0f, 1.0f, 1.0f);
	g_Ball.m_velo = XMFLOAT3(0.0f, 0.0f, 0.0f);

	g_Ball.m_acceleration = XMFLOAT3(0.0f, -9.8f / 600.0f * 0.5f, 0.0f);
}
void Ball_Finalize(void)
{
	ModelRelease(g_Ball.Model);
}
void Ball_Update(void)
{
	g_Ball.m_velo = XMFLOAT3(0, 0, 0);
	//Camera Movement
	if (Keyboard_IsKeyDown(KK_UP)) {
		g_Ball.m_velo.z = 1.0f / 60.0f;
	}
	if (Keyboard_IsKeyDown(KK_DOWN)) {
		g_Ball.m_velo.z = -1.0f / 60.0f;
	}
	if (Keyboard_IsKeyDown(KK_LEFT)) {
		g_Ball.m_velo.x = -1.0f / 60.0f;

	}
	if (Keyboard_IsKeyDown(KK_RIGHT)) {
		g_Ball.m_velo.x = 1.0f / 60.0f;
	}

	g_Ball.m_position.x += g_Ball.m_velo.x;
	g_Ball.m_position.y += g_Ball.m_velo.y;
	g_Ball.m_position.z += g_Ball.m_velo.z;

}
void Ball_Draw(void)
{
	//シェーダーを描画パイプライン設定
	Shader_Begin();

	XMMATRIX Projection = GetProjectionMatrix();

	XMMATRIX View = GetViewMatrix();

	//変換行列の作成
	XMMATRIX VP = View * Projection;

	XMMATRIX ScalingMatrix = XMMatrixScaling(
		g_Ball.m_scaling.x, g_Ball.m_scaling.y, g_Ball.m_scaling.z);

	//平行移動行列の作成
	XMMATRIX TranslationMAtrix = XMMatrixTranslation(
		g_Ball.m_position.x, g_Ball.m_position.y, g_Ball.m_position.z);

	//回転行列の作成
	XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(g_Ball.m_rotation.x),
		XMConvertToRadians(g_Ball.m_rotation.y),
		XMConvertToRadians(g_Ball.m_rotation.z));

	XMMATRIX WorldMatrix = ScalingMatrix * RotationMatrix * TranslationMAtrix;
	XMMATRIX WVP = WorldMatrix * VP;

	//変換行列を頂点シェーダーへセット
	Shader_SetWorldMatrix(WorldMatrix);
	Shader_SetMatrix(WVP);


	//ModelDraw(g_Ball.Model);
}

XMFLOAT3 GetBall_Position()
{
	return g_Ball.m_position;
}

LightSource* GetBall()
{
	return &g_Ball;
}

XMMATRIX Ball_GetWorldMatrix()
{
	// Using ball's translation, rotation, scaling
	LightSource* ball = GetBall(); // Or your own way to fetch Ball object
	XMMATRIX scale = XMMatrixScaling(ball->m_scaling.x, ball->m_scaling.y, ball->m_scaling.z);
	XMMATRIX rotation = XMMatrixRotationRollPitchYaw(ball->m_rotation.x, ball->m_rotation.y, ball->m_rotation.z);
	XMMATRIX translate = XMMatrixTranslation(ball->m_position.x, ball->m_position.y, ball->m_position.z);
	return scale * rotation * translate;
}

void Ball_DrawRaw(const XMMATRIX& world, const XMMATRIX& matrix)
{
	Shader_SetWorldMatrix(world);
	Shader_SetMatrix(matrix);
	//ModelDraw(g_Ball.Model);
}