//fade.cpp
#include	"fade.h"
#include	"shader.h"

//=========================================================================================================
//構造体宣言
//=========================================================================================================
FadeObject	g_Fade;		

//=========================================================================================================
//グローバル変数
//=========================================================================================================
static	ID3D11ShaderResourceView* g_Texture = NULL;	//テクスチャ１枚を表すオブジェクト
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;


//=========================================================================================================
//初期化処理
//=========================================================================================================
void Fade_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	//テクスチャ読み込み
	TexMetadata		metadata;
	ScratchImage	image;
	LoadFromWICFile(L"asset\\texture\\fade.bmp", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_Texture);
	assert(g_Texture);//読み込み失敗時にダイアログを表示

	g_Fade.fadecolor.x = 0.0f;
	g_Fade.fadecolor.y = 0.0f;
	g_Fade.fadecolor.z = 0.0f;
	g_Fade.fadecolor.w = 1.0f;
	g_Fade.frame = 60.0f;	//60フレームでフェード完了
	g_Fade.state = FADE_STATE::FADE_NONE;

}

//=========================================================================================================
//終了処理
//=========================================================================================================
void Fade_Finalize()
{
	if (g_Texture != NULL)
	{
		g_Texture->Release();
		g_Texture = NULL;
	}
}

//=========================================================================================================
//更新処理
//=========================================================================================================
void Fade_Update()
{
}

//=========================================================================================================
//描画処理
//=========================================================================================================
void Fade_Draw()
{ 
	//現在の状況
	switch (g_Fade.state)
	{
		case FADE_STATE::FADE_NONE:
			return;
		case FADE_STATE::FADE_IN:
			if (g_Fade.fadecolor.w < 0.0)
			{//フェードイン終了
				g_Fade.fadecolor.w = 0.0f;
				g_Fade.state = FADE_STATE::FADE_NONE;
			}
			break;
		case FADE_STATE::FADE_OUT:
			if (g_Fade.fadecolor.w > 1.0f)
			{//フェードアウト終了
				g_Fade.fadecolor.w = 1.0;
				//フェードイン初期化
				SetFade(g_Fade.frame, g_Fade.fadecolor, FADE_STATE::FADE_IN, g_Fade.scene);
				//シーン切り替え
				SetScene(g_Fade.scene);
			}
			break;
	}

	//スプライト表示

//---------------------------------------------------
	// シェーダーを描画パイプラインに設定
	Shader_Begin();

	// 画面サイズ取得
	const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth();
	const float SCREEN_HEIGHT = (float)Direct3D_GetBackBufferHeight();

	// 頂点シェーダーに変換行列を設定
	Shader_SetMatrix(XMMatrixOrthographicOffCenterLH(
		0.0f,
		SCREEN_WIDTH,
		SCREEN_HEIGHT,
		0.0f,
		0.0f,
		1.0f));
	//---------------------------------------------------

		//テクスチャをセット
	g_pContext->PSSetShaderResources(0, 1, &g_Texture);

	//スプライト描画
	SetBlendState(BLENDSTATE_ALFA);//αブレンド
	XMFLOAT2 pos = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };
	XMFLOAT2 size = { SCREEN_WIDTH, SCREEN_HEIGHT };
	DrawSprite(pos, size, g_Fade.fadecolor);

	//フェード処理
	switch (g_Fade.state)
	{
		case FADE_STATE::FADE_IN:
			g_Fade.fadecolor.w -= (1.0f / g_Fade.frame);//透明にしていく
			break;
		case FADE_STATE::FADE_OUT:
			g_Fade.fadecolor.w += (1.0f / g_Fade.frame);//不透明にしていく
			break;
	}


}

//=========================================================================================================
//フェード状態セット処理
//=========================================================================================================
void	SetFade(int fadeframe, XMFLOAT4 color, FADE_STATE state, SCENE scene)
{ 
	g_Fade.frame = fadeframe;
	g_Fade.fadecolor = color;
	g_Fade.state = state;
	g_Fade.scene = scene;

	if (g_Fade.state == FADE_IN)
	{
		g_Fade.fadecolor.w = 1.0f;	//不透明にする
	}
	else
	{
		g_Fade.fadecolor.w = 0.0f;	//透明にする
	}


}

void	SetFade(int fadeframe, XMFLOAT4 color, FADE_STATE state)
{
	g_Fade.frame = fadeframe;
	g_Fade.fadecolor = color;
	g_Fade.state = state;

	if (g_Fade.state == FADE_IN)
	{
		g_Fade.fadecolor.w = 1.0f;	//不透明にする
	}
	else
	{
		g_Fade.fadecolor.w = 0.0f;	//透明にする
	}


}

//=========================================================================================================
//フェード状態取得処理
//=========================================================================================================
FADE_STATE	GetFadeState()
{
	return	g_Fade.state;	//現在の状態
}


