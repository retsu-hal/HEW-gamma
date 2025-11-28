/*==============================================================================

   ポリゴン描画 [polygon.cpp]
--------------------------------------------------------------------------------

==============================================================================*/
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "direct3d.h"
#include "shader.h"
#include "debug_ostream.h"
#include "sprite.h"//スプライト機能を追加

#include	"keyboard.h"//<<<<<<<<<<<<<<<<<<<追加


static	ID3D11ShaderResourceView* g_Texture = NULL;	//テクスチャ１枚を表すオブジェクト

// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;


void Polygon_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// デバイスとデバイスコンテキストのチェック
	if (!pDevice || !pContext) {
		hal::dout << "Polygon_Initialize() : 与えられたデバイスかコンテキストが不正です" << std::endl;
		return;
	}

	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;

	//テクスチャ画像読み込み
	TexMetadata		metadata;
	ScratchImage	image;
//	LoadFromWICFile(L"asset\\texture\\shadow.png", WIC_FLAGS_NONE, &metadata, image);
	LoadFromWICFile(L"asset\\texture\\runningman003.png", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_Texture);
	assert(g_Texture);//読み込み失敗時にダイアログを表示

}

void Polygon_Finalize(void)
{
	g_Texture->Release();

}
void Polygon_Draw(void)
{
	// 画面サイズ取得
	const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth();
	const float SCREEN_HEIGHT = (float)Direct3D_GetBackBufferHeight();


	XMFLOAT4 col = { 1.0f, 1.0f, 1.0f, 1.0f };	//スプライトの色
	XMFLOAT2 size = { 200, 300 };


	static XMFLOAT2 pos = { SCREEN_WIDTH / 2, SCREEN_HEIGHT - 150 };
	if (Keyboard_IsKeyDown(KK_W))//Wキー
	{
		//キーが押された場合
		pos.y += -2.0f;
	}
	//トリガーキーの場合
	if (Keyboard_IsKeyDown(KK_S))//S矢印キー
	{
		//キーが押された場合
		pos.y += 2.0f;
	}
	if (Keyboard_IsKeyDown(KK_A))//Aキー
	{
		//キーが押された場合
		pos.x += -2.0f;
	}
	//トリガーキーの場合
	if (Keyboard_IsKeyDown(KK_D))//D矢印キー
	{
		//キーが押された場合
		pos.x += 2.0f;
	}

	//スプライト回転角(度)
	static float deg = 0.0f;
	//キーボードの入力チェック
	if (Keyboard_IsKeyDown(KK_LEFT))//左矢印キー
	{
		//キーが押された場合
		deg += 2.0f;
	}
	//トリガーキーの場合
	if (Keyboard_IsKeyDown(KK_RIGHT))//右矢印キー
	{
		//キーが押された場合
		deg += -2.0f;
	}
	//スケーリング
	static	float	scl = 1.0f;
	if (Keyboard_IsKeyDown(KK_UP))//上矢印キー
	{
		//キーが押された場合
		scl += 0.1f;
		if (scl > 5.0f) scl = 5.0f;
	}
	//トリガーキーの場合
	if (Keyboard_IsKeyDown(KK_DOWN))//↓矢印キー
	{
		//キーが押された場合
		scl += -0.1f;
		if (scl < 1.0f) scl = 1.0f;
	}




	// シェーダーを描画パイプラインに設定
	Shader_Begin();


	// 頂点シェーダーに変換行列を設定

	XMMATRIX	Projection = XMMatrixOrthographicOffCenterLH(
								0.0f,
								SCREEN_WIDTH,
								SCREEN_HEIGHT,
								0.0f,
								0.0f,
								1.0f);

	XMMATRIX	Translation  = XMMatrixTranslation(pos.x,pos.y, 0.0f);
	XMMATRIX	Rotation = XMMatrixRotationZ(XMConvertToRadians(deg));
	XMMATRIX	Scaling = XMMatrixScaling(scl, scl, scl);
	XMMATRIX	World = Scaling * Rotation * Translation;

	XMMATRIX	mat = XMMatrixIdentity();
	mat = mat * World * Projection;

	Shader_SetMatrix(mat);

	SetBlendState(BLENDSTATE_ALFA);//ブレンド無し

	//スプライト描画テスト
	g_pContext->PSSetShaderResources(0, 1, &g_Texture);//g_Textureを使うように設定する



	static float bno = 0;	//ブロックの番号
	DrawSprite(size, col, (int)bno, 5, 2);

	bno += 10.0f/60.0f;	//適当に数字を増やす
	if (bno >= 10.0f)
	{
		bno = 0.0f;
	}

}
