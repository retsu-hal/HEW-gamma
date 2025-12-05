#include "direct3d.h"
#include "Manager.h"
#include "keyboard.h"
#include "Game.h"
#include "Title.h"
#include "Result.h"
#include "fade.h"

#include "debug.h"

//=========================================================================================================
// グローバル変数
//=========================================================================================================
static	SCENE	g_Scene = SCENE_NONE;	//現在のシーン番号

//=========================================================================================================
// 初期化処理
//=========================================================================================================
void	Manager_Initialize()
{ 
	Fade_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());

	////本来はtitleの初期化でフェードインをセットする
	//XMFLOAT4 color = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	//SetFade(60.0f, color, FADE_STATE::FADE_IN, SCENE_GAME);
	//SetScene(SCENE_GAME);	//最初に動かすシーンに切り替える


	//本来の形
	Fade_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
	SetScene(SCENE_TITLE);	//最初に動かすシーンに切り替える

}

//=========================================================================================================
// 終了処理
//=========================================================================================================
void	Manager_Finalize()
{ 
	Fade_Finalize();
	SetScene(SCENE_NONE);
}

//=========================================================================================================
// 更新処理
//=========================================================================================================
void	Manager_Update()
{
	switch (g_Scene)	//現在シーンのアップデート関数を呼び出す
	{
		case SCENE_NONE:
			break;
		case SCENE_TITLE:
			Title_Update();	
			break;
		case SCENE_GAME:
			Game_Update();
			break;
		case SCENE_RESULT:
			Result_Update();
			break;
		default:
			break;
	}

	Fade_Update();

}

//=========================================================================================================
// 描画処理
//=========================================================================================================
void	Manager_Draw()
{ 
	ImGui_ImplWin32_NewFrame();
	ImGui_ImplDX11_NewFrame();
	ImGui::NewFrame();

	switch (g_Scene)	//現在シーンの描画関数を呼び出す
	{
		case SCENE_NONE:
			break;
		case SCENE_TITLE:
			Title_Draw();	
			break;
		case SCENE_GAME:
			Game_Draw();
			break;
		case SCENE_RESULT:
			Result_Draw();
			break;
		default:
			break;
	}

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	Fade_Draw();
}

//=========================================================================================================
// シーン状態セット処理
//=========================================================================================================
void	SetScene(SCENE scene) //シーンを切り替える
{
	//実行中のシーンを終了させる
	switch (g_Scene)	//現在シーンの終了関数を呼び出す
	{
		case SCENE_NONE:
			break;
		case SCENE_TITLE:
			Title_Finalize();	
			break;
		case SCENE_GAME:
			Game_Finalize();
			break;
		case SCENE_RESULT:
			Result_Finalize();
			break;
		default:
			break;
	}

	g_Scene = scene;	//指定のシーンへ切り替える

	//次のシーンを初期化する
	switch (g_Scene)	//現在シーンの初期化関数を呼び出す
	{
		case SCENE_NONE:
			break;
		case SCENE_TITLE:
			Title_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
			break;
		case SCENE_GAME:
			Game_Initialize( Direct3D_GetDevice(), Direct3D_GetDeviceContext());
			break;
		case SCENE_RESULT:
			Result_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
			break;
		default:
			break;
	}

}
