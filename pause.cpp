#include "pause.h"
#include "keyboard.h"

//=========================================================================================================
// マクロ定義
//=========================================================================================================


//=========================================================================================================
// グローバル変数
//=========================================================================================================
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;
static ID3D11ShaderResourceView* g_Texture;
static bool g_Pause= false;

void Pause_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{

	g_pContext = pContext;
	g_pDevice = pDevice;

}

void Pause_Finalize(void)
{
}

void Pause_Update(void)
{
	// ESC のトグル
	if (Keyboard_IsKeyDownTrigger(KK_ESCAPE))
	{
		g_Pause = !g_Pause;
	}
}

void Pause_Draw(void)
{
	// 画面サイズ取得
	const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth();
	const float SCREEN_HEIGHT = (float)Direct3D_GetBackBufferHeight();
	

	XMFLOAT2 pos = { 0.0f,0.0f };
	XMFLOAT2 size = { SCREEN_WIDTH,SCREEN_HEIGHT };
	XMFLOAT4 col = { 0.0f,0.0f,0.0f,0.5f };
	DrawSprite(pos, size, col);
}

bool Pause_IsActive()
{
	return ;
}

void Pause_Toggle()
{
	g_Pause = !g_Pause;
}
