#include <d3d11.h>
#include <DirectXMath.h>
#include "direct3d.h"
#include "shader.h"
#include "debug_ostream.h"
#include "sprite.h"
#include "keyboard.h"
#include "fade.h"
#include "player.h"
#include "Block.h"
using namespace DirectX;

//=========================================================================================================
//グローバル変数
//=========================================================================================================
// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

//プレイヤー関連変数
static	ID3D11ShaderResourceView* g_Texture[4];	//テクスチャ４枚分
static	PIECE	g_Piece;						//落下ブロックオブジェクト
XMFLOAT4 col = { 1.0f, 1.0f, 1.0f, 1.0f };	//スプライトの色
XMFLOAT2 size = { BLOCK_WIDTH, BLOCK_HEIGHT };//表示サイズ
static	XMFLOAT2	ScrollOffset = XMFLOAT2(POSITION_OFFSET_X, POSITION_OFFSET_Y);	//スクロール値の初期化


//=========================================================================================================
//初期化処理
//=========================================================================================================
void Player_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
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


	LoadFromWICFile(L"asset\\texture\\Spade.png", WIC_FLAGS_NONE, &metadata, image);//テクスチャは変更可
	CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_Texture[0]);
	assert(g_Texture[0]);//読み込み失敗時にダイアログを表示

	LoadFromWICFile(L"asset\\texture\\Clover.png", WIC_FLAGS_NONE, &metadata, image);//テクスチャは変更可
	CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_Texture[1]);
	assert(g_Texture[1]);//読み込み失敗時にダイアログを表示

	LoadFromWICFile(L"asset\\texture\\Diamond.png", WIC_FLAGS_NONE, &metadata, image);//テクスチャは変更可
	CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_Texture[2]);
	assert(g_Texture[2]);//読み込み失敗時にダイアログを表示

	LoadFromWICFile(L"asset\\texture\\Heart.png", WIC_FLAGS_NONE, &metadata, image);//テクスチャは変更可
	CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_Texture[3]);
	assert(g_Texture[3]);//読み込み失敗時にダイアログを表示


	//ブロックの作成
	Player_Create();

	g_Piece.State = PIECE_STATE_MOVE;
	g_Piece.StateCount = 0;

}

//=========================================================================================================
//終了処理
//=========================================================================================================
void Player_Finalize(void)
{
	//テクスチャの解放
	for(int i=0;i<4;i++)
	{
		g_Texture[i]->Release();
	}

}

//=========================================================================================================
//更新処理
//=========================================================================================================
void Player_Update()
{
	//プレイヤーステートによる処理の分岐
	switch (g_Piece.State)
	{
	case	PIECE_STATE::PIECE_STATE_IDLE:
		break;
	case	PIECE_STATE::PIECE_STATE_MOVE://落下・移動処理
		Player_Move();
		break;
	case	PIECE_STATE::PIECE_STATE_GROUND_IDLE:
		g_Piece.StateCount++;	//カウンターインクリメント
		if (g_Piece.StateCount >= 60)	//適当に待つ
		{
			g_Piece.State = PIECE_STATE::PIECE_STATE_IDLE;
			g_Piece.StateCount = 0;
		
			//ブロック消去チェック
			Block_EraseBlock();
		}
		break;
	case	PIECE_STATE::PIECE_STATE_MISS_IDLE:
		g_Piece.StateCount++;	//カウンターインクリメント
		if (g_Piece.StateCount >= 40)	//適当に待つ
		{
			if (GetFadeState() == FADE_NONE)
			{
				//フェードアウトさせてシーンを切り替える
				XMFLOAT4	color(0.0f, 0.0f, 0.0f, 1.0f);
				SetFade(40.0f, color, FADE_OUT, SCENE_RESULT);
			}
		}
		break;

	}

}

//=========================================================================================================
//描画処理
//=========================================================================================================
void Player_Draw(void)
{

	if (g_Piece.State != PIECE_STATE_MOVE)
	{	//落下中でなければ表示はしない
		return;
	}


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


	/*
			■　＜＝＝本体　g_Type[0]
			◇              g_Type[1]
			〇			    g_Type[2]

	*/

	for(int i = 0; i < 3; i++)
	{
		//平行移動 表示座標
		XMMATRIX	Translation = 
			XMMatrixTranslation(g_Piece.Position.x, 
								g_Piece.Position.y + (i * BLOCK_HEIGHT),
								0.0f);
		//回転
		XMMATRIX	Rotation = XMMatrixRotationZ(XMConvertToRadians(0.0f));
		//拡大率（0はだめ）
		XMMATRIX	Scaling = XMMatrixScaling(1.0f, 1.0f, 1.0f);

		//ワールド行列
		XMMATRIX	World = Scaling * Rotation * Translation;
		//頂点変換行列

		//スクロール用行列作成
//		XMMATRIX	mat = XMMatrixIdentity();//行列の初期化（単位行列）
		XMMATRIX	mat = XMMatrixTranslation(ScrollOffset.x, ScrollOffset.y, 0.0f);
		mat = World * mat * Projection;

		//シェーダーへ行列をセット
		Shader_SetMatrix(mat);
		//テクスチャの設定
		g_pContext->PSSetShaderResources(0, 1, &g_Texture[g_Piece.Type[i]]);
		//ブレンド無し
		SetBlendState(BLENDSTATE_ALFA);
		//スプライト描画
		DrawSprite(size, col, 0, 1, 1);
	}

}

//=========================================================================================================
//プレイヤー作成処理（核となるブロック作成）
//=========================================================================================================
void Player_Create()
{
	//出現座標
	g_Piece.Position = { BLOCK_WIDTH * 2.5f, -BLOCK_HEIGHT * 2.5f };//とりあえず

	//ブロックを３つ決める
	for (int i = 0; i < 3; i++)
	{
		g_Piece.Type[i] = rand() % 4;	//4種類から1つ選ぶ
	}

	//ゲームオーバーチェック
	//プレイヤーの一番下のブロックの表示座標を配列の位置に変換
	int x = g_Piece.Position.x / BLOCK_WIDTH;
	int y = 0;

	BLOCK	block = Block_GetBlock(x, y);
	if (block.Enable == true)//出現場所の最上段にブロックがあったら終了
	{	//ブロックがあったらミスモードへ移行	
		g_Piece.State = PIECE_STATE::PIECE_STATE_MISS_IDLE;
		g_Piece.StateCount = 0;
	}
	else
	{
		g_Piece.State = PIECE_STATE::PIECE_STATE_MOVE;	//移動モード
		g_Piece.StateCount = 0;//カウンターリセット
	}


	//g_Piece.State = PIECE_STATE::PIECE_STATE_MOVE;	//移動モード
	//g_Piece.StateCount = 0;//カウンターリセット


}

//=========================================================================================================
//移動処理
//=========================================================================================================
void Player_Move()
{

	//ブロックのローテーション
	if (Keyboard_IsKeyDownTrigger(KK_UP))
	{
		//Type[]の内容をローテーション
		int type = g_Piece.Type[0];
		for (int i = 0; i < 2; i++)
		{
			g_Piece.Type[i] = g_Piece.Type[i + 1];
		}
		g_Piece.Type[2] = type;
	}

	//表示座標を配列番号へ変換
	//Y座標については最下段のブロックの底面の座標とする
	int		x, y;
	x = (int)g_Piece.Position.x / BLOCK_WIDTH;
	y = (int)(g_Piece.Position.y + 
				(BLOCK_HEIGHT * 2.5f)) / BLOCK_HEIGHT;


	BLOCK	block;

	//左移動
	if (Keyboard_IsKeyDownTrigger(KK_LEFT))
	{
		if (x > 0)//配列の一番左より右にいる
		{
			block = Block_GetBlock(x - 1, y);
			if (block.Enable == false)//左隣にブロックが無い
			{
				//X座標を左へブロック１つ分うごかす
				g_Piece.Position.x += -BLOCK_WIDTH;
			}
		}
	}
	//右移動
	if (Keyboard_IsKeyDownTrigger(KK_RIGHT))
	{

		if (x < (BLOCK_COLS - 1))//配列の一番右よりも左にいる
		{
			block = Block_GetBlock(x + 1, y);
			if (block.Enable == false)//右隣にブロックが無い
			{
				//X座標を右へブロック１つ分うごかす
				g_Piece.Position.x += BLOCK_WIDTH;
			}
		}
	}

	//着地チェック

	block = Block_GetBlock(x, y);//プレイヤーの真下のブロック

	bool ground = false;//着地したかフラグ

	if (block.Enable == true)
	{
		ground = true;
	}
	if (y >= BLOCK_ROWS)	//配列の一番下にいる
	{
		ground = true;
	}

	//着地した時の処理
	if (ground == true)
	{
		//プレイヤーを配列へコピーする
		for (int i = 0; i < 3; i++)
		{
			x = (int)g_Piece.Position.x / BLOCK_WIDTH;
			y = (int)(g_Piece.Position.y + 
						(BLOCK_HEIGHT * i)) / BLOCK_HEIGHT;
			Block_SetBlock(x, y, g_Piece.Type[i]);
		}

		g_Piece.State = PIECE_STATE::PIECE_STATE_GROUND_IDLE;//着地した

	}

	//加速処理
	if (Keyboard_IsKeyDown(KK_DOWN))
	{
		g_Piece.Position.y += 5.0f;	//速度は適当に調節する
	}
	//通常の落下処理
	g_Piece.Position.y += 2.0f * 0.2f;	//速度は適当に調節する

}
