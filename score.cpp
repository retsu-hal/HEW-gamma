//score.cpp
#include	"shader.h"
#include	"sprite.h"
#include	"score.h"

//=========================================================================================================
//マクロ定義
//=========================================================================================================
#define		SCORE_MAX	(5)		//表示桁数

//=========================================================================================================
//グローバル変数
//=========================================================================================================
// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;
static ID3D11ShaderResourceView* g_Texture = NULL;
static	float	Score;			//スコア値
static	float	ScoreBuffer;	//スコア値バッファ

//=========================================================================================================
//初期化処理
//=========================================================================================================
void Score_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{ 
	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;

	//
	Score = 0.0f;
	ScoreBuffer = 0.0f;

	//
	//テクスチャ画像読み込み
	TexMetadata		metadata;
	ScratchImage	image;
	LoadFromWICFile(L"asset\\texture\\number000.png", 
		WIC_FLAGS_NONE, &metadata, image);//テクスチャは変更可
	CreateShaderResourceView(pDevice, image.GetImages(),
		image.GetImageCount(), metadata, &g_Texture);
	assert(g_Texture);//読み込み失敗時にダイアログを表示

}
//=========================================================================================================
//終了処理
//=========================================================================================================
void Score_Finalize(void)
{ 
	if (g_Texture != NULL)
	{
		g_Texture->Release();//テクスチャを解放
		g_Texture = NULL;
	}

}

//=========================================================================================================
//更新処理
//=========================================================================================================
void Score_Update()
{ 
	if (ScoreBuffer > 0.0f)//バッファにスコアが溜まっている
	{
		Score += 0.5f;			//バッファの中身をScoreへ少しずつ
		ScoreBuffer -= 0.5f;	//移していく
	}
	else
	{
		ScoreBuffer = 0.0f;
	}

}

//=========================================================================================================
//描画処理
//=========================================================================================================
void Score_Draw(void)
{ 
	//桁分解
	int		PatNo[5] = { 0,0,0,0,0 };//分解した結果を格納

	int		temp = (int)Score;
	PatNo[0] = temp / 10000;			//１万の位
	PatNo[1] = (temp % 10000) / 1000;	//千の位
	PatNo[2] = (temp % 1000) / 100;		//百の位
	PatNo[3] = (temp % 100) / 10;		//十の位
	PatNo[4] = (temp % 10);				//１の位

	XMFLOAT3	position = XMFLOAT3(100.0f, 100.0f, 0.0f);
	XMFLOAT2	size = XMFLOAT2(50.0f, 80.0f);
	XMFLOAT4	color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	//テクスチャのセット
	g_pContext->PSSetShaderResources(0, 1, &g_Texture);

	//画面サイズ取得
	const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth();
	const float SCREEN_HEIGHT = (float)Direct3D_GetBackBufferHeight();

	//シェーダーのセット
	Shader_Begin();

	//シェーダーに２D描画の設定をする
	XMMATRIX	Projection = XMMatrixOrthographicOffCenterLH(
		0.0f,
		SCREEN_WIDTH,
		SCREEN_HEIGHT,
		0.0f,
		0.0f,
		1.0f
	);

	//スコア表示（５桁分）
	for (int i = 0; i < SCORE_MAX; i++)
	{

		//平行移動 表示座標
		XMMATRIX	Translation =
			XMMatrixTranslation(position.x, position.y, 0.0f);
		//回転
		XMMATRIX	Rotation = XMMatrixRotationZ(XMConvertToRadians(0.0f));
		//拡大率（0はだめ）
		XMMATRIX	Scaling = XMMatrixScaling(1.0f, 1.0f, 1.0f);
		//ワールド行列
		XMMATRIX	World = Scaling * Rotation * Translation;
		//スクロール用行列作成
		XMMATRIX	mat = XMMatrixTranslation(0.0f, 0.0f, 0.0f);

		mat = World * mat * Projection;

		//シェーダーへ行列をセット
		Shader_SetMatrix(mat);

		//ブレンド無し
		SetBlendState(BLENDSTATE_ALFA);

		//スプライト描画
		DrawSprite(size, color, PatNo[i], 10, 1);

		position.x += size.x;//表示座標を１桁分ずらす

	}

}

//=========================================================================================================
//スコア加算処理
//=========================================================================================================
void	AddScore(int sc)	//スコアに加算する
{ 
	ScoreBuffer += sc;
}

//=========================================================================================================
//スコア値取得処理
//=========================================================================================================
float	GetScore()			//スコア値取得
{ 
	return Score;
}

