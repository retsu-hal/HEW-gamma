#include	"Manager.h"
#include	"sprite.h"
#include	"Game.h"
#include	"keyboard.h"
#include	"Polygon3D.h"
#include	"Player3D.h"
#include	"LightSource.h"
#include	"field.h"
#include	"Effect.h"
#include	"score.h"
#include	"Audio.h"
#include	"camera.h"
#include	"direct3d.h"


#include "Collision.h"

//=========================================================================================================
//グローバル変数
//=========================================================================================================
static	int		g_BgmID = NULL;	//サウンド管理ID
LIGHTOBJECT g_BallLight;

static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

//=========================================================================================================
//初期化処理
//=========================================================================================================
void Game_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	/*
	//Player_Initialize(pDevice, pContext);
	//Block_Initialize(pDevice, pContext);
	//Effect_Initialize(pDevice, pContext);
	//Score_Initialize(pDevice, pContext);
	//Polygon3D_Initialize(pDevice, pContext);
	//g_BgmID = LoadAudio("asset\\Audio\\bgm.wav");	//サウンドロード
	//PlayAudio(g_BgmID, true);	//再生開始（ループあり）
	//PlayAudio(g_BgmID);			//再生開始（ループなし）
	//PlayAudio(g_BgmID, false);	//再生開始（ループなし）
	*/
	Player3D_Initialize(pDevice, pContext);

	field_Initialize(pDevice, pContext);
	Camera_Initialize();

	// Initialize the ball's light source
	g_BallLight.SetEnable(true);
	XMFLOAT4 para;
	para = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
	g_BallLight.SetAmbient(para);
	para = XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f);
	g_BallLight.SetDiffuse(para);
}
//=========================================================================================================
//終了処理
//=========================================================================================================
void Game_Finalize()
{
	field_Finalize();
	Polygon3D_Finalize();
	Camera_Finalize();
	
	Player3D_Finalize();


	/*
	//Block_Finalize();
	
	//Effect_Finalize();
	//Score_Finalize();	
	//UnloadAudio(g_BgmID);//サウンドの解放
	*/
}

//=========================================================================================================
//更新処理
//=========================================================================================================
void Game_Update()
{
	//更新処理
	Light_Update();

	// Update ball light position
	XMFLOAT3 LightPos = GetLight_Position();
	g_BallLight.SetEnable(true);
	g_BallLight.SetDirection(XMFLOAT4(0, 0, 0, 0)); // No fixed direction (omnidirectional)
	g_BallLight.Light.Direction = XMFLOAT4(LightPos.x, LightPos.y, LightPos.z, 1.0f); // Set light position

	// Update global shadow light position to match ball
	//g_ShadowLightPos = LightPos;
	float shadowIntensity = 1.0f; // Blend value: higher means stronger light influence.
	Shader_SetShadowLightData(LightPos, g_ShadowLightRadius, shadowIntensity);

	Camera_Update();
	field_Update();

	Player3D_Update();

	/*
	//Block_Update();
	//Effect_Update();
	//Score_Update();
	//Polygon3D_Update();
	*/
}

//=========================================================================================================
//描画処理
//=========================================================================================================
void Game_Draw()
{ 
	Camera_Draw();		//最初に呼ぶ！

	//３D描画
	g_BallLight.SetEnable(TRUE);					//ライティングON
	Shader_SetLight(g_BallLight.Light);		//ライト構造体をシェーダーにセット
	SetDepthTest(TRUE);

	// Dynamic light position (can be fixed or move based on puzzle state)
	XMFLOAT3 lightPos = g_ShadowLightPos;
	float lightRadius = g_ShadowLightRadius;

<<<<<<< HEAD
	Collision_DebugDraw();

	//�QD�`��
	Light.SetEnable(FALSE);					//���C�e�B���OOFF
	Shader_SetLight(Light.Light);		//���C�g�\���̂��V�F�[�_�[�ɃZ�b�g
=======
	// ------- OMNIDIRECTIONAL SHADOW PASS (6 cubemap faces) -------

	for (int face = 0; face < 6; face++)
	{
		Direct3D_BeginShadowPass(face);

		Shader_Begin();
		// Set shadow pass mode = 1.0 (pixel shader will output linear depth)
		Shader_SetShadowLightData(lightPos, lightRadius, 1.0f, 1.0f);

		XMMATRIX lightViewProj = Direct3D_GetCubemapFaceViewProj(face, lightPos, lightRadius);
		Shader_SetShadowMatrix(lightViewProj);

		// Draw ball
		{
			XMMATRIX world = Light_GetWorldMatrix();
			Shader_SetWorldMatrix(world);
			Shader_SetMatrix(world * lightViewProj);
			Light_DrawRaw(world, world * lightViewProj);
		}

		// Draw field
		{
			Field_DrawShadowMap(lightViewProj);
		}
	}

	Direct3D_EndShadowPass();
	
	// ------- MAIN CAMERA PASS -------
	Shader_Begin();

	Shader_SetLight(g_BallLight.Light);

	// Shadow resources for pixel shader
	Shader_SetShadowMap(g_pShadowCubemapSRV);
	Shader_SetShadowSampler(g_pShadowSamplerState);
	Shader_SetShadowLightData(lightPos, lightRadius, 1.0f, 0.0f);

	//Player
	{
		Player3D_Draw();
	}
	// Draw ball (visible)
	{
		//Ball_Draw();
	}
	// Draw field (visible)
	{
		field_Draw();
	}

	//２D描画
	g_BallLight.SetEnable(FALSE);					//ライティングOFF
	Shader_SetLight(g_BallLight.Light);		//ライト構造体をシェーダーにセット
>>>>>>> 73db079adbbd2c5bc2d828804b3c84a05ea683c0
	SetDepthTest(FALSE);

	


	/*
	//Block_Draw();
	
	//Effect_Draw();
	//Score_Draw();
	//Polygon3D_Draw();
	*/
}

