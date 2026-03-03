//game.cpp
#include	"Manager.h"
#include	"sprite.h"
#include	"Game.h"
#include	"keyboard.h"
#include	"Polygon3D.h"
#include	"Player3D.h"
#include	"Player2D.h"
#include	"LightSource.h"
#include	"field.h"
#include	"Effect.h"
#include	"score.h"
#include	"Audio.h"
#include	"camera.h"
#include	"direct3d.h"
#include	"Collision.h"
#include	"Bill_Board.h"
#include	"SkyDome.h"

#include	"PlayerModeSwitchManager.h"
#include	"Pushing_Obj_Manager.h"

#include	"debug.h"
#include	"ShadowColliderBox.h"

#include	 <map>


static bool debugMode;
static bool g_OptionMenu = false;

//static	int	g_BgmID = NULL;
LIGHTOBJECT g_BallLight;
static XMFLOAT3 LightPos;

// BGM邂｡逅・
static int g_Bgm3D = -1;  // 3D繝｢繝ｼ繝峨・BGM
static int g_Bgm2D = -1;  // 2D繝｢繝ｼ繝峨・BGM
static PLAYER_MODE g_PrevMode = MODE_3D;  // 蜑阪ヵ繝ｬ繝ｼ繝縺ｮ繝｢繝ｼ繝・
static const float CROSSFADE_DURATION = 2.0f;  // 繧ｯ繝ｭ繧ｹ繝輔ぉ繝ｼ繝画凾髢難ｼ育ｧ抵ｼ・


static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static ID3D11ShaderResourceView* g_Goal_1_Texture = nullptr;
static ID3D11ShaderResourceView* g_Goal_2_Texture = nullptr;
static ID3D11ShaderResourceView* g_Goal_3_Texture = nullptr;

static ShadowBuildConfig g_ShadowConfig;

static std::vector<const ShadowPrism*> g_ActiveShadowPrisms;

static bool g_2DPlayerDebugMode = false;

void Game_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	

	g_pDevice = pDevice;
	g_pContext = pContext;

	PlayerModeSwitchManager_Init();
	PlayerPushManager_Init();


	Player3D_Initialize(pDevice, pContext);
	Player2D_Initialize(pDevice, pContext);
	{
		XMFLOAT3 start = Field_GetPlayerStartPosition();

		if (PLAYER* p3 = GetPlayer3D())
		{
			p3->Position = start;
			p3->Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
		}
		if (PLAYER* p2 = GetPlayer2D())
		{
			p2->Position = start;
			p2->Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
		}
	}

	field_Initialize(pDevice, pContext);
	SkyDome_Initialize(pDevice, pContext);
	Light_Initialize(pDevice, pContext);
	Camera_Initialize();
	InitializeBillBoard();

	// Initialize the ball's light source
	g_BallLight.SetEnable(false);
	g_BallLight.SetAmbient(XMFLOAT4(0.08f, 0.08f, 0.1f, 1.0f));
	g_BallLight.SetDiffuse(XMFLOAT4(1.0f, 0.95f, 0.85f, 1.0f));

	g_ShadowConfig.edgeSamples = 4;
	g_ShadowConfig.thickness = 1.0f;
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

	GAME_STAGE currentStage = GetCurrentStage();

	if (currentStage == STAGE_1)
	{
		LoadMapFromFile("asset\\MapData\\stage1.txt");
	}
	else if (currentStage == STAGE_2)
	{
		LoadMapFromFile("asset\\MapData\\stage2.txt");
	}
	else if (currentStage == STAGE_3)
	{
		LoadMapFromFile("asset\\MapData\\stage3.txt");
	}
	else
	{
		// Default: load stage select
		LoadMapFromFile("asset\\MapData\\stage_select.txt");
	}

	// BGM蛻晄悄蛹・
	g_Bgm3D = LoadAudio("asset/Audio/stage.wav");  // 3D繝｢繝ｼ繝峨・BGM繝輔ぃ繧､繝ｫ蜷阪ｒ謖・ｮ・
	g_Bgm2D = LoadAudio("asset/Audio/stageR.wav");  // 2D繝｢繝ｼ繝峨・BGM繝輔ぃ繧､繝ｫ蜷阪ｒ謖・ｮ・

	// 蛻晄悄迥ｶ諷九・3D繝｢繝ｼ繝峨↑縺ｮ縺ｧBGM3D繧貞・逕・
	if (g_Bgm3D >= 0)
	{
		PlayAudio(g_Bgm3D, true);  // 繝ｫ繝ｼ繝怜・逕・
		SetAudioVolume(g_Bgm3D, 1.0f);
	}

	// 2D繝｢繝ｼ繝峨・BGM縺ｯ辟｡髻ｳ縺ｧ蠕・ｩ・
	if (g_Bgm2D >= 0)
	{
		PlayAudio(g_Bgm2D, true);  // 繝ｫ繝ｼ繝怜・逕・
		SetAudioVolume(g_Bgm2D, 0.0f);  // 辟｡髻ｳ
	}

	g_PrevMode = MODE_3D;

	GetShadowManager()->Initialize();

	TexMetadata metadata;
	ScratchImage image;
	LoadFromWICFile(L"asset\\texture\\UI\\easy.png", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(g_pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_Goal_1_Texture);

	LoadFromWICFile(L"asset\\texture\\UI\\mid.png", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(g_pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_Goal_2_Texture);

	LoadFromWICFile(L"asset\\texture\\UI\\hard.png", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(g_pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_Goal_3_Texture);

	Collision_ResetShadowContactState();
}


void Game_Finalize()
{
	Collision_ResetShadowContactState();

	SAFE_RELEASE(g_Goal_1_Texture);
	SAFE_RELEASE(g_Goal_2_Texture);
	SAFE_RELEASE(g_Goal_3_Texture);

	// BGM隗｣謾ｾ
	if (g_Bgm3D >= 0)
	{
		UnloadAudio(g_Bgm3D);
		g_Bgm3D = -1;
	}
	if (g_Bgm2D >= 0)
	{
		UnloadAudio(g_Bgm2D);
		g_Bgm2D = -1;
	}

	GetShadowManager()->Finalize();

	field_Finalize();
	SkyDome_Finalize();
	Polygon3D_Finalize();
	Light_Finalize();
	Camera_Finalize();

	PlayerPushManager_Finalize();
	PlayerModeSwitchManager_Finalize();
	Player3D_Finalize();
	Player2D_Finalize();
	FinalizeBillBoard();

}


void Game_Update()
{
	// BGM譖ｴ譁ｰ・医ヵ繧ｧ繝ｼ繝牙・逅・ｼ・
	UpdateAudio();

	// 繝｢繝ｼ繝牙・繧頑崛縺域､懷・縺ｨ繧ｯ繝ｭ繧ｹ繝輔ぉ繝ｼ繝・
	PLAYER_MODE currentMode = PlayerModeSwitchManager_GetMode();

	if (currentMode != g_PrevMode)
	{
		// 繝｢繝ｼ繝峨′蛻・ｊ譖ｿ繧上▲縺・
		if (currentMode == MODE_3D)
		{
			// 2D竊・D蛻・ｊ譖ｿ縺茨ｼ・D縺ｮBGM繧偵ヵ繧ｧ繝ｼ繝峨う繝ｳ縲・D縺ｮBGM繧偵ヵ繧ｧ繝ｼ繝峨い繧ｦ繝・
			if (g_Bgm3D >= 0)
			{
				FadeInAudio(g_Bgm3D, CROSSFADE_DURATION, 1.0f);
			}
			if (g_Bgm2D >= 0)
			{
				FadeOutAudio(g_Bgm2D, CROSSFADE_DURATION);
			}
		}
		else if (currentMode == MODE_2D)
		{
			// 3D竊・D蛻・ｊ譖ｿ縺茨ｼ・D縺ｮBGM繧偵ヵ繧ｧ繝ｼ繝峨う繝ｳ縲・D縺ｮBGM繧偵ヵ繧ｧ繝ｼ繝峨い繧ｦ繝・
			if (g_Bgm2D >= 0)
			{
				FadeInAudio(g_Bgm2D, CROSSFADE_DURATION, 1.0f);
			}
			if (g_Bgm3D >= 0)
			{
				FadeOutAudio(g_Bgm3D, CROSSFADE_DURATION);
			}
		}

		g_PrevMode = currentMode;
	}

	Collision_DebugClearExtraBoxes();


	Light_Update();
	field_Update();
	SkyDome_Update();

	// 繝｢繝ｼ繝牙・繧頑崛縺医・譖ｴ譁ｰ縺ｯ蟶ｸ縺ｫ陦後≧
	PlayerModeSwitchManager_Update();
	if (PlayerModeSwitchManager_GetMode() == MODE_3D)
	{
		PlayerPushManager_Update();
	}

	//XMFLOAT3 LightPos = GetLight_Position();
	//g_BallLight.SetEnable(true);
	//g_BallLight.SetDirection(XMFLOAT4(LightPos.x, LightPos.y, LightPos.z, 1.0f));
	//g_ShadowLightPos = LightPos;

	for (const auto& mapData : GetFieldMap())
	{
		if (mapData.no == FIELD_OBJ_3)
		{
			// Use the first FIELD_OBJ_3 found for the light
			g_BallLight.SetEnable(true);
			//LightPos = mapData.pos;  // Optional, if shadows depend on OBJ_3
			LightPos = { mapData.pos.x ,mapData.pos.y + 0.25f,mapData.pos.z };
			break;
		}
	}

	float shadowIntensity = 1.0f;
	Shader_SetShadowLightData(LightPos, g_ShadowRadius, shadowIntensity);

	auto& map = GetFieldMap();

	GetShadowManager()->UpdateAllShadows(LightPos, map, g_ShadowConfig);

	g_ActiveShadowPrisms.clear();
	const auto& shadows = GetShadowManager()->GetShadows();
	for (const auto& shadow : shadows)
	{
		if (shadow.isValid)
		{
			g_ActiveShadowPrisms.push_back(&shadow);
		}
	}


	Collision_SetShadowPrisms(g_ActiveShadowPrisms);
	

#ifdef _DEBUG
	if (Keyboard_IsKeyDownTrigger(KK_LEFTALT))// F1キーでデバッグモードのオンオフ切り替え
	{
		g_2DPlayerDebugMode = !g_2DPlayerDebugMode;
	}
#endif // _DEBUG



	if (PlayerModeSwitchManager_GetMode() == MODE_3D)
	{// 3D繝｢繝ｼ繝峨・譖ｴ譁ｰ
		Player3D_Update();
		Player3DCamera_Update();

	}
	else
	{// 2D繝｢繝ｼ繝峨・譖ｴ譁ｰ
		Player2D_Update();
		Player2DCamera_Update();
		

#ifdef _DEBUG
		if (g_2DPlayerDebugMode)
		{
			Player2DCamera_DebugUpdate();
		}
#endif // _DEBUG
	}
	//Player3DCamera_Update();

}


void Game_Draw()
{ 
	Camera_Draw();

	g_BallLight.SetEnable(TRUE);
	Shader_SetLight(g_BallLight.Light);
	SetDepthTest(TRUE);

	XMFLOAT3 lightPos = GetLight_Position();
	float lightRadius = g_ShadowRadius;


	for (int face = 0; face < 6; face++)
	{
		Direct3D_BeginShadowPass(face);
		Shader_Begin();
		Shader_SetShadowLightData(LightPos, lightRadius, 1.0f, 1.0f);

		XMMATRIX lightViewProj = Direct3D_GetCubemapFaceViewProj(face, LightPos, lightRadius);
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
				XMVECTOR v = XMLoadFloat3(&Map[i].pos) - XMLoadFloat3(&LightPos);
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
	Shader_SetShadowLightData(LightPos, lightRadius, 1.0f, 0.0f);

	{
		SkyDome_Draw();
	}
	{
		field_Draw();
	}

	if (PlayerModeSwitchManager_GetMode() == MODE_3D)
	{
		g_BallLight.SetEnable(FALSE);
		Shader_SetLight(g_BallLight.Light);


		Player3D_Draw();
		PlayerPushManager_Draw();
		PlayerModeSwitchManager_Draw();
		g_BallLight.SetEnable(TRUE);
		Shader_SetLight(g_BallLight.Light);
	}
	else
	{
		
		Player2D_Draw();
	
	}

	// ===== UNBIND shadow map and DISABLE light before SkyDome =====

	g_BallLight.SetEnable(FALSE);
	Shader_SetLight(g_BallLight.Light);
	

	DEBUG_IMGUI_BEGIN({
		Collision_DebugDraw();
		});

	

	for (const auto& mapData : GetFieldMap())
	{

		if (mapData.no == FIELD_STAGE_1)
		{
			g_pContext->PSSetShaderResources(0, 1, &g_Goal_1_Texture);

			XMFLOAT2 size = { 1.0f, 1.0f };  // Billboard size
			XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
			DrawBillBoard({ mapData.pos.x ,mapData.pos.y + 2.0f,mapData.pos.z }, size, color, 0, 1, 1);
		}
		if (mapData.no == FIELD_STAGE_2)
		{
			g_pContext->PSSetShaderResources(0, 1, &g_Goal_2_Texture);

			XMFLOAT2 size = { 1.0f, 1.0f };  // Billboard size
			XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
			DrawBillBoard({ mapData.pos.x ,mapData.pos.y + 2.0f,mapData.pos.z }, size, color, 0, 1, 1);
		}
		if (mapData.no == FIELD_STAGE_3)
		{
			g_pContext->PSSetShaderResources(0, 1, &g_Goal_3_Texture);

			XMFLOAT2 size = { 1.0f, 1.0f };  // Billboard size
			XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
			DrawBillBoard({ mapData.pos.x ,mapData.pos.y + 2.0f,mapData.pos.z }, size, color, 0, 1, 1);
		}
	}

	

	SetDepthTest(FALSE);
}

