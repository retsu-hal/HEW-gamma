#include <d3d11.h>
#include <DirectXMath.h>
#include "direct3d.h"
#include "shader.h"
#include "sprite.h"
#include "keyboard.h"
#include "Block.h"
#include "player.h"
#include "Effect.h"
#include "score.h"
using namespace DirectX;

//=========================================================================================================
// グローバル変数
//=========================================================================================================
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;
static ID3D11ShaderResourceView* g_Texture[4]{};
static BLOCK g_Block[BLOCK_ROWS][BLOCK_COLS]{};//横に6個　縦に13個
static BLOCK_STATE g_BlockState{};
static int g_BlockStateCount{};
static	XMFLOAT2	ScrollOffset = XMFLOAT2(POSITION_OFFSET_X, POSITION_OFFSET_Y);		//スクロール値の初期化

//=========================================================================================================
// 初期化処理
//=========================================================================================================
void Block_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;

	// テクスチャ読み込み
	TexMetadata metadata;
	ScratchImage image;

	LoadFromWICFile(L"Asset\\Texture\\Spade.png", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_Texture[0]);
	assert(g_Texture[0]);

	LoadFromWICFile(L"Asset\\Texture\\Clover.png", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_Texture[1]);
	assert(g_Texture[1]);

	LoadFromWICFile(L"Asset\\Texture\\Diamond.png", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_Texture[2]);
	assert(g_Texture[2]);

	LoadFromWICFile(L"Asset\\Texture\\Heart.png", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_Texture[3]);
	assert(g_Texture[3]);



	//ブロック初期化
	for (int y = 0; y < BLOCK_ROWS; y++)
	{
		for (int x = 0; x < BLOCK_COLS; x++)
		{
			//g_Block[y][x].Enable = true;	//デバッグ用
			g_Block[y][x].Enable = false;
			g_Block[y][x].Type = 0;

		}

	}

	g_BlockState = BLOCK_STATE_IDLE;
	g_BlockStateCount = 0;
}

//=========================================================================================================
// 終了処理
//=========================================================================================================
void Block_Finalize()
{
	for (int i = 0; i < 4; i++)
	{
		g_Texture[i]->Release();
	}
}

//=========================================================================================================
// 更新処理
//=========================================================================================================
void Block_Update()
{

	switch (g_BlockState)
	{
	case BLOCK_STATE_IDLE:	//ひま
		break;

	case BLOCK_STATE_ERASE_IDLE://消滅中
		g_BlockStateCount++;	//切り替えウェイトインクリメント
		if (g_BlockStateCount >= 30)//適当に待つ
		{
			Block_StackBlock();	//落下処理
		}
		break;

	case BLOCK_STATE_STACK_IDLE://落下中
		g_BlockStateCount++;
		if (g_BlockStateCount >= 30)
		{
			Block_EraseBlock();//消滅チェック
		}
		break;

	default:
		break;
	}
}

//=========================================================================================================
// 描画処理
//=========================================================================================================
void Block_Draw()
{

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


	for (int y = 0; y < BLOCK_ROWS; y++)
	{
		for (int x = 0; x < BLOCK_COLS; x++)
		{
			if (g_Block[y][x].Enable)//ブロックがあれば
			{	//配列の形に表示する

				g_pContext->PSSetShaderResources(0, 1, &g_Texture[g_Block[y][x].Type]);


				XMFLOAT2 pos = XMFLOAT2(x*BLOCK_WIDTH+(BLOCK_WIDTH*0.5f), y* BLOCK_HEIGHT+(BLOCK_HEIGHT*0.5f));
				XMFLOAT2 size = XMFLOAT2(BLOCK_WIDTH, BLOCK_HEIGHT);
				XMFLOAT4 col = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

				//平行移動 表示座標
				XMMATRIX	Translation =
					XMMatrixTranslation(pos.x,pos.y, 0.0f);
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
				DrawSprite(size, col, 0, 1, 1);
			}
		}
	}
}

//=========================================================================================================
// ブロックセット処理
//=========================================================================================================
void Block_SetBlock(int x, int y, int Type)
{
	//配列の範囲内か？
	if (x < 0 || x >= BLOCK_COLS || y < 0 || y >= BLOCK_ROWS)
	{
		return;
	}

	g_Block[y][x].Enable = true;
	g_Block[y][x].Erase = false;
	g_Block[y][x].Type = Type;

}

//=========================================================================================================
// ブロック取得処理
//=========================================================================================================
BLOCK Block_GetBlock(int x, int y)
{
	BLOCK dummy =	//ダミーブロック
	{
		false,
		false,
		0,
	};

	if (x < 0 || x >= BLOCK_COLS  ||  y < 0 || y >= BLOCK_ROWS)
	{
		return dummy;//配列範囲外の場合はダミーブロックを返す
	}

	return g_Block[y][x];
}


//=========================================================================================================
// ブロック消去処理
//=========================================================================================================
void Block_EraseBlock()
{
	bool	erase = false;		//消滅発生フラグ

	int		type = -1;		//ブロックの種類
	int		count = 0;		//同じ色が並んでいる数

	//横方向チェック
	for (int y = 0; y < BLOCK_ROWS; y++)
	{
		for (int x = 0 ; x < BLOCK_COLS; x++)
		{
			if (g_Block[y][x].Enable == true)//ブロックが存在する
			{
				if (g_Block[y][x].Type == type)//typeと同じブロック
				{
					count++;	//並んでいる数＋１

					if (count >= 2)//色が3つ以上並んでいる
					{
						for (int i = x; i > x - 3; i--)
						{	//手前に3つ分遡って消去フラグをtrueにしていく
							g_Block[y][i].Erase = true;
						}
						erase = true;	//消滅発生フラグON
						AddScore(10);	//スコアに１００点加算する
					}
				}
				else
				{//typeと異なるブロックだった場合
					type = g_Block[y][x].Type;	//チェックする種類を更新
					count = 0;					//並んでいる数リセット
				}
			}
			else
			{//ブロックが無い場合
				type = -1;
				count = 0;
			}

		}
		type = -1;
		count = 0;

	}

	//縦チェック
	type = -1;
	count = 0;

	for (int x = 0; x < BLOCK_COLS; x++)
	{
		for (int y = 0; y < BLOCK_ROWS; y++)
		{

			if (g_Block[y][x].Enable == true)//ブロックが存在する
			{
				if (g_Block[y][x].Type == type)//typeと同じブロック
				{
					count++;	//並んでいる数＋１

					if (count >= 2)//色が3つ以上並んでいる
					{
						for (int i = y; i > y - 3; i--)
						{	//手前に3つ分遡って消去フラグをtrueにしていく
							g_Block[i][x].Erase = true;
						}
						erase = true;	//消滅発生フラグON
						AddScore(10);
					}
				}
				else
				{//typeと異なるブロックだった場合
					type = g_Block[y][x].Type;	//チェックする種類を更新
					count = 0;					//並んでいる数リセット
				}
			}
			else
			{//ブロックが無い場合
				type = -1;
				count = 0;
			}
		}
		type = -1;
		count = 0;

	}


	//消滅状態のブロックの削除＆エフェクト発生
	for (int y = 0; y < BLOCK_ROWS; y++)
	{
		for (int x = 0; x < BLOCK_COLS; x++)
		{
			if (g_Block[y][x].Erase)//消滅する予定のブロック
			{
				g_Block[y][x].Enable = false;
				g_Block[y][x].Erase = false;

				//エフェクト発生予定
				XMFLOAT2	position;
				position = XMFLOAT2(x * BLOCK_WIDTH + (BLOCK_WIDTH * 0.5f), 
									y * BLOCK_HEIGHT + (BLOCK_HEIGHT * 0.5f));

				CreateEffect(position);

			}

		}
	}

	if (erase == true)
	{//消滅が発生した
		g_BlockState = BLOCK_STATE::BLOCK_STATE_ERASE_IDLE;
		g_BlockStateCount = 0;
	}
	else
	{//消滅は無かった
		Player_Create();		//新しいプレイヤー発生
		g_BlockState = BLOCK_STATE::BLOCK_STATE_IDLE;
		g_BlockStateCount = 0;
	}

}

//=========================================================================================================
// ブロック落下処理
//=========================================================================================================
void Block_StackBlock()
{
	bool stack = false;	//落下処理OFF

	for (int y = BLOCK_ROWS - 1; y > 0; y--)//下から上に見ていく
	{
		for (int x = 0; x < BLOCK_COLS; x++)
		{
			if (g_Block[y][x].Enable == false)//ブロックが無い
			{
				for (int ys = y - 1; ys >= 0; ys--)//1っ個上から最上部まで
				{
					if (g_Block[ys][x].Enable == true)
					{
						g_Block[y][x] = g_Block[ys][x];	//1つ上の構造体を下へコピー
						g_Block[ys][x].Enable = false;	//コピーしたら空になる
						stack = true;					//落下処理発生
						break;
					}
				}
			}
		}
	}

	if (stack == true)
	{	//落下処理があった場合
		g_BlockState = BLOCK_STATE::BLOCK_STATE_STACK_IDLE;
		g_BlockStateCount = 0;
	}
	else
	{	//落下処理が無かった場合
		Player_Create();
		g_BlockState = BLOCK_STATE::BLOCK_STATE_IDLE;
		g_BlockStateCount = 0;
	}

}