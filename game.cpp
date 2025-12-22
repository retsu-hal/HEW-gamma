#include	"Manager.h"
#include	"sprite.h"
#include	"Game.h"
#include	"keyboard.h"
#include	"Polygon3D.h"
#include	"Player3D.h"
#include "Player2D.h"
#include	"LightSource.h"
#include	"field.h"
#include	"Effect.h"
#include	"score.h"
#include	"Audio.h"
#include	"camera.h"
#include	"direct3d.h"
#include "Collision.h"

#include "debug.h"

static bool debugMode = TRUE;

static	int		g_BgmID = NULL;
LIGHTOBJECT g_BallLight;

static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;


void Game_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	Player3D_Initialize(pDevice, pContext);
	Player2D_Initialize(pDevice, pContext);

	field_Initialize(pDevice, pContext);
	Light_Initialize(pDevice, pContext);
	Camera_Initialize();
	

	// Initialize the ball's light source
	g_BallLight.SetEnable(false);
	XMFLOAT4 para;
	para = XMFLOAT4(0.08f, 0.08f, 0.1f, 1.0f);
	g_BallLight.SetAmbient(para);
	para = XMFLOAT4(1.0f, 0.95f, 0.85f, 1.0f);
	g_BallLight.SetDiffuse(para);
}


void Game_Finalize()
{
	field_Finalize();
	Polygon3D_Finalize();
	Light_Finalize();
	Camera_Finalize();
	
	Player3D_Finalize();
	Player2D_Finalize();

}


void Game_Update()
{
	Light_Update();


	XMFLOAT3 LightPos = GetLight_Position();
	//g_BallLight.SetEnable(true);
	g_BallLight.SetDirection(XMFLOAT4(0, 0, 0, 0));
	g_BallLight.Light.Direction = XMFLOAT4(LightPos.x, LightPos.y, LightPos.z, 1.0f);

	g_ShadowLightPos = LightPos;
	float shadowIntensity = 1.0f;
	Shader_SetShadowLightData(LightPos, g_ShadowLightRadius, shadowIntensity);

	Player3D_Update();
	Player2D_Update();
	Camera_Update();
	field_Update();



}


void Game_Draw()
{ 

	Camera_Draw();
	Light_Draw();

	g_BallLight.SetEnable(TRUE);
	Shader_SetLight(g_BallLight.Light);
	SetDepthTest(TRUE);


	XMFLOAT3 lightPos = GetLight_Position();
	if (debugMode)
	{
		ImGui::Begin("Debug - han");
		ImGui::Text("Pos: %.2f,%.2f,%.2f", lightPos.x, lightPos.y, lightPos.z);
		ImGui::End();
	}

	float lightRadius = g_ShadowLightRadius;



	for (int face = 0; face < 6; face++)
	{
		Direct3D_BeginShadowPass(face);

		Shader_Begin();

		Shader_SetShadowLightData(lightPos, lightRadius, 1.0f, 1.0f);

		XMMATRIX lightViewProj = Direct3D_GetCubemapFaceViewProj(face, lightPos, lightRadius);
		Shader_SetShadowMatrix(lightViewProj);


		{
			XMMATRIX world = Light_GetWorldMatrix();
			Shader_SetWorldMatrix(world);
			Shader_SetMatrix(world * lightViewProj);
			Light_DrawRaw(world, world * lightViewProj);
		}


		{
			std::vector<MAPDATA>& Map = GetFieldMap();
			float maxShadowDist = g_ShadowLightRadius;

			for (size_t i = 0; i < Map.size(); ++i)
			{
				XMFLOAT3 objPos = Map[i].pos;

				XMVECTOR v = XMLoadFloat3(&objPos) - XMLoadFloat3(&lightPos);
				float distSq = XMVectorGetX(XMVector3LengthSq(v));

				if (distSq > maxShadowDist * maxShadowDist)
					continue; // skip shadow draw

				XMMATRIX world = Field_GetWorldMatrix((int)i);
				//Shader_SetWorldMatrix(world);
				//Shader_SetMatrix(world * lightViewProj);
				Field_DrawShadowMap(world, world * lightViewProj, (int)i);
			}
		}
	}

	Direct3D_EndShadowPass();
	

	Shader_Begin();

	Shader_SetLight(g_BallLight.Light);


	Shader_SetShadowMap(g_pShadowCubemapSRV);
	Shader_SetShadowSampler(g_pShadowSamplerState);
	Shader_SetShadowLightData(lightPos, lightRadius, 1.0f, 0.0f);


	{
		Player3D_Draw();
	}
	{
		Player2D_Draw();
	}
	{
		field_Draw();
	}
	{
		Light_Draw();
	}

	Collision_DebugDraw();

	g_BallLight.SetEnable(FALSE);
	Shader_SetLight(g_BallLight.Light);
	SetDepthTest(FALSE);
}

