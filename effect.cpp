//effect.cpp
#include "effect.h"
#include "sprite.h"
#include "shader.h"

//=========================================================================================================
// マクロ定義
//=========================================================================================================
#define	EFFECT_MAX	(100)	//エフェクトオブジェクト

//=========================================================================================================
// グローバル変数
//=========================================================================================================
// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;
static ID3D11ShaderResourceView* g_Texture;
static	EFFECT	g_Effect[EFFECT_MAX];
static	XMFLOAT2 ScrollOffset = XMFLOAT2(POSITION_OFFSET_X, POSITION_OFFSET_Y);

//=========================================================================================================
// 初期化処理
//=========================================================================================================
void Effect_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	//オブジェクトの初期化
	for (int i = 0; i < EFFECT_MAX; i++)
	{
		g_Effect[i].Enable = false;
	}

	// テクスチャ読み込み
	TexMetadata metadata;
	ScratchImage image;
	LoadFromWICFile(L"Asset\\Texture\\Explosion.png", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(pDevice, image.GetImages(), 
	image.GetImageCount(), metadata, &g_Texture);
	assert(g_Texture);

}

//=========================================================================================================
// 終了処理
//=========================================================================================================
void Effect_Finalize()
{
	if (g_Texture != NULL)
	{
		g_Texture->Release();
	}
}

//=========================================================================================================
// 更新処理
//=========================================================================================================
void Effect_Update()
{
	//アニメーション処理
	for (int i = 0; i < EFFECT_MAX; i++)
	{
		if (g_Effect[i].Enable == true)
		{
			g_Effect[i].FrameCount++;	//カウンターインクリメント
			if (g_Effect[i].FrameCount > 30)
			{
				g_Effect[i].Enable = false;
			}
		}

	}

}

//=========================================================================================================
// 描画処理
//=========================================================================================================
void Effect_Draw()
{
	//テクスチャのセット
	g_pContext->PSSetShaderResources(0, 1, &g_Texture);

	// 画面サイズ取得
	const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth();
	const float SCREEN_HEIGHT = (float)Direct3D_GetBackBufferHeight();

	// シェーダーを描画パイプラインに設定
	Shader_Begin();

	// 頂点シェーダーに2D変換行列を設定
	XMMATRIX	Projection = XMMatrixOrthographicOffCenterLH(
		0.0f,
		SCREEN_WIDTH,
		SCREEN_HEIGHT,
		0.0f,
		0.0f,
		1.0f);

	//オブジェクトの表示
	for (int i = 0; i < EFFECT_MAX; i++)
	{
		if (g_Effect[i].Enable == true)
		{
			int		PatNo = (int)(g_Effect[i].FrameCount / 30.0f * 16.0f);
			XMFLOAT4	col(1.0f, 1.0f, 1.0f, 1.0f);	//白色
			XMFLOAT2	size(BLOCK_WIDTH * 1.6f, BLOCK_HEIGHT * 1.6f);

			//平行移動 表示座標
			XMMATRIX	Translation =
				XMMatrixTranslation(g_Effect[i].Position.x, g_Effect[i].Position.y, 0.0f);
			//回転
			XMMATRIX	Rotation = XMMatrixRotationZ(XMConvertToRadians(0.0f));
			//拡大率（0はだめ）
			XMMATRIX	Scaling = XMMatrixScaling(1.0f, 1.0f, 1.0f);
			//ワールド行列
			XMMATRIX	World = Scaling * Rotation * Translation;
	
			//スクロール用行列作成
			//XMMATRIX	mat = XMMatrixIdentity();//行列の初期化（単位行列）
			XMMATRIX	mat = XMMatrixTranslation(ScrollOffset.x, ScrollOffset.y, 0.0f);
			//頂点変換行列
			mat = World * mat * Projection;

			//シェーダーへ行列をセット
			Shader_SetMatrix(mat);

			SetBlendState(BLENDSTATE_ALFA);
			DrawSprite(size, col, PatNo, 4, 4);

		}

	}

}

//=========================================================================================================
// エフェクト作成
//=========================================================================================================
void CreateEffect(XMFLOAT2 Position)
{
	//0718ここから
	for (int i = 0; i < EFFECT_MAX; i++)
	{
		if (g_Effect[i].Enable == false)
		{
			g_Effect[i].Enable = true;
			g_Effect[i].Position = XMFLOAT3(Position.x, Position.y, 0.0f);
			g_Effect[i].FrameCount = 0;

			break;//1個作ったら終了
		}

	}

}

