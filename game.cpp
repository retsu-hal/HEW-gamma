#include	"Manager.h"
#include	"sprite.h"
#include	"Game.h"
#include	"keyboard.h"
#include "Polygon3D.h"
#include	"Player3D.h"
#include	"Block.h"
#include	"field.h"
#include	"Effect.h"
#include	"score.h"
#include	"Audio.h"
#include "camera.h"
#include"direct3d.h"

//=========================================================================================================
//グローバル変数
//=========================================================================================================
static	int		g_BgmID = NULL;	//サウンド管理ID
LIGHTOBJECT Light;

//=========================================================================================================
//初期化処理
//=========================================================================================================
void Game_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
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

	//ライト初期化
	XMFLOAT4 para;
	para = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);		//環境光の色
	Light.SetAmbient(para);
	para = XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f);		//光の色
	Light.SetDiffuse(para);
	para = XMFLOAT4(0.5f, -1.0f, 0.0f, 1.0f);		//光の方向
	float len = sqrtf(para.x * para.x + para.y * para.y + para.z * para.z);
	para.x /= len;
	para.y /= len;
	para.z /= len;
	Light.SetDirection(para);		//光の方向（正規化済）
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
	//３D描画
	Light.SetEnable(TRUE);					//ライティングON
	Shader_SetLight(Light.Light);		//ライト構造体をシェーダーにセット
	SetDepthTest(TRUE);

	Camera_Draw();		//最初に呼ぶ！


	Player3D_Draw();
	field_Draw();

	//２D描画
	Light.SetEnable(FALSE);					//ライティングOFF
	Shader_SetLight(Light.Light);		//ライト構造体をシェーダーにセット
	SetDepthTest(FALSE);

	


	/*
	//Block_Draw();
	
	//Effect_Draw();
	//Score_Draw();
	//Polygon3D_Draw();
	*/
}

