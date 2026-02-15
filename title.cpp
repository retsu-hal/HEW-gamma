//title.cpp
#include	"Manager.h"
#include	"sprite.h"
#include	"keyboard.h"
#include	"Audio.h"
#include	"Title.h"
#include	"fade.h"
#include	"shader.h"
#include	"title_manager.h"

//=========================================================================================================
// 構造体宣言
//=========================================================================================================


//=========================================================================================================
//グローバル変数
//=========================================================================================================
static	ID3D11ShaderResourceView* g_Texture = NULL;	//テクスチャ１枚を表すオブジェクト

static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static	int	g_BgmID = NULL;
 
//=========================================================================================================
//初期化処理
//=========================================================================================================
void Title_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	Title_Manager_Initialize(pDevice, pContext);


	g_BgmID = LoadAudio("asset\\Audio\\title.wav");	//サウンドロード
	SetAudioVolume(g_BgmID, 0.05f);
	PlayAudio(g_BgmID, true);	//再生開始（ループあり）

	//テクスチャ読み込みなど
	TexMetadata		metadata;
	ScratchImage	image;
	LoadFromWICFile(L"asset\\texture\\logo.png", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_Texture);
	assert(g_Texture);//読み込み失敗時にダイアログを表示


	const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth();
	const float SCREEN_HEIGHT = (float)Direct3D_GetBackBufferHeight();



	//フェードインのセット
	XMFLOAT4	color = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	SetFade(60.0f, color, FADE_IN, SCENE_TITLE);

}

//=========================================================================================================
//終了処理
//=========================================================================================================
void Title_Finalize()
{
	//テクスチャの解放など
	SAFE_RELEASE(g_Texture);

	UnloadAudio(g_BgmID);//サウンドの解放

	Title_Manager_Finalize();
}

//=========================================================================================================
//更新処理
//=========================================================================================================
void Title_Update()
{ 
	Title_Manager_Update();

}
//=========================================================================================================
//描画処理
//=========================================================================================================
void Title_Draw()
{
	Title_Manager_Draw();
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
	g_pContext->PSSetShaderResources(0, 1, &g_Texture);//g_Textureを使うように設定する

	//スプライト描画
	SetBlendState(BLENDSTATE_NONE);//ブレンド無し
	XMFLOAT4 col = { 1.0f, 1.0f, 1.0f, 1.0f };	//スプライトの色
	//XMFLOAT2 pos = { SCREEN_WIDTH / 2, (SCREEN_HEIGHT / 2) / 2 };
	//XMFLOAT2 size = { SCREEN_WIDTH / 5, SCREEN_HEIGHT / 5};

	XMFLOAT2 pos = { SCREEN_WIDTH / 2, (SCREEN_HEIGHT / 2 - 150)};
	XMFLOAT2 size = { SCREEN_WIDTH /4, SCREEN_HEIGHT /4};
	DrawSprite(pos, size, col);//1枚絵を表示

}
