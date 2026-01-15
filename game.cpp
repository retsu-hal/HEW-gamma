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

#include "PlayerModeSwitchManager.h"

#include "debug.h"
#include "ShadowColliderBox.h"

static bool debugMode = TRUE;

static	int		g_BgmID = NULL;
LIGHTOBJECT g_BallLight;

static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static ShadowPrism g_ShadowPrism;
static ShadowBuildConfig g_ShadowConfig;

void Game_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	PlayerModeSwitchManager_Init();


	Player3D_Initialize(pDevice, pContext);
	Player2D_Initialize(pDevice, pContext);

	field_Initialize(pDevice, pContext);
	Light_Initialize(pDevice, pContext);
	Camera_Initialize();
	

	// Initialize the ball's light source
	g_BallLight.SetEnable(false);
	g_BallLight.SetAmbient(XMFLOAT4(0.08f, 0.08f, 0.1f, 1.0f));
	g_BallLight.SetDiffuse(XMFLOAT4(1.0f, 0.95f, 0.85f, 1.0f));

	g_ShadowConfig.edgeSamples = 4;
	g_ShadowConfig.thickness = 0.15f;
	g_ShadowConfig.maxCastDist = 100.0f;

	ShadowDebugOptions debugOpts;
	debugOpts.drawPrism = true;
	debugOpts.drawNormal = true;
	debugOpts.drawVertices = true;
	debugOpts.drawAABB = false;
	debugOpts.prismColor = IM_COL32(255, 50, 50, 220);
	debugOpts.normalColor = IM_COL32(255, 255, 0, 255);
	debugOpts.vertexColor = IM_COL32(0, 255, 0, 255);
	Collision_SetShadowDebugOptions(debugOpts);
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
	Collision_DebugClearExtraBoxes();


	Light_Update();
	field_Update();


	PlayerModeSwitchManager_Update();

	XMFLOAT3 LightPos = GetLight_Position();
	//g_BallLight.SetEnable(true);
	g_BallLight.SetDirection(XMFLOAT4(LightPos.x, LightPos.y, LightPos.z, 1.0f));
	g_ShadowLightPos = LightPos;

	float shadowIntensity = 1.0f;
	Shader_SetShadowLightData(LightPos, g_ShadowRadius, shadowIntensity);

	auto& map = GetFieldMap();

	int casterIdx = -1;
	for (int i = 0; i < (int)map.size(); ++i)
		if (map[i].no == FIELD_OBJ_2 ) { casterIdx = i; break; }


	bool hasShadow = false;
	if (casterIdx >= 0)
	{
		if (Shadow_NeedsRebuild(g_ShadowPrism, LightPos, map[casterIdx], 0.01f))
		{
			hasShadow = Shadow_Build(
				g_ShadowPrism,
				map[casterIdx],
				LightPos,
				map,
				g_ShadowConfig);
		}
		else
		{
			hasShadow = g_ShadowPrism.isValid;
		}
	}
	else
	{
		Shadow_Clear(g_ShadowPrism);
	}

	Collision_SetShadowPrism(hasShadow ? &g_ShadowPrism : nullptr);



	if (PlayerModeSwitchManager_GetMode() == MODE_3D)
	{
		Player3D_Update();
		//Player3DCamera_Update();

	}
	else
	{
		Player2D_Update();
		//Player2DCamera_Update();
	}
	Player3DCamera_Update();

}


void Game_Draw()
{ 

	Camera_Draw();
	Light_Draw();

	g_BallLight.SetEnable(TRUE);
	Shader_SetLight(g_BallLight.Light);
	SetDepthTest(TRUE);

	if (debugMode)
	{

	}

	XMFLOAT3 lightPos = GetLight_Position();
	float lightRadius = g_ShadowRadius;


	for (int face = 0; face < 6; face++)
	{
		Direct3D_BeginShadowPass(face);
		Shader_Begin();
		Shader_SetShadowLightData(lightPos, lightRadius, 1.0f, 1.0f);

		XMMATRIX lightViewProj = Direct3D_GetCubemapFaceViewProj(face, lightPos, lightRadius);
		Shader_SetShadowMatrix(lightViewProj);


		{
			XMMATRIX world = Light_GetWorldMatrix();
			Light_DrawRaw(world, world * lightViewProj);
		}


		{
			std::vector<MAPDATA>& Map = GetFieldMap();
			float maxShadowDist = g_ShadowRadius;

			for (size_t i = 0; i < Map.size(); ++i)
			{
				XMVECTOR v = XMLoadFloat3(&Map[i].pos) - XMLoadFloat3(&lightPos);
				if (XMVectorGetX(XMVector3LengthSq(v)) > maxShadowDist * maxShadowDist)
					continue;

				XMMATRIX world = Field_GetWorldMatrix((int)i);
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
		field_Draw();
	}
	{
		Light_Draw();
	}

	if (PlayerModeSwitchManager_GetMode() == MODE_3D)
	{
		Player3D_Draw();
	}
	else
	{
		Player2D_Draw();
	}

	Collision_DebugDraw();

	g_BallLight.SetEnable(FALSE);
	Shader_SetLight(g_BallLight.Light);
	SetDepthTest(FALSE);
}

