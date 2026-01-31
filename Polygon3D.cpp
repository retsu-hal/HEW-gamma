//Polygon3D.cpp
#include"d3d11.h"
#include"DirectXMath.h"
#include"direct3d.h"
#include"shader.h"
#include"keyboard.h"
#include"sprite.h"
#include "mouse.h"
#include"camera.h"

using namespace DirectX;
//=========================================================================================================
// マクロ定義
//=========================================================================================================
#define NUM_VERTEX (100)

//=========================================================================================================
//構造体定義・定義
//=========================================================================================================
static Vertex vdata[NUM_VERTEX]
{
	//==================
	// 正面（+Z面）
	//==================
	//頂点０　左上
	{
		XMFLOAT3(-0.5f,0.5f,-0.5f),			//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,0.0f)					    //テクスチャ座標
	},
	//頂点1　右上
	{
		XMFLOAT3(0.5f,0.5f,-0.5f),			//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(1.0f,0.0f)					    //テクスチャ座標
	},
	//頂点2　左下
	{
		XMFLOAT3(-0.5f,-0.5f,-0.5f),		//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,1.0f)					    //テクスチャ座標
	},
	/*
	//頂点3　左下
	{
		XMFLOAT3(-0.5f,-0.5f,-0.5f),		//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,1.0f)					    //テクスチャ座標
	},
	//頂点4　右上
	{
		XMFLOAT3(0.5f,0.5f,-0.5f),			//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(1.0f,0.0f)					    //テクスチャ座標
	},
	*/
	//頂点3　右下
	{
		XMFLOAT3(0.5f,-0.5f,-0.5f),			//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(1.0f,1.0f)					    //テクスチャ座標
	},

	//==================
	// 右面（+X面）
	//==================
	// 頂点4　左上
	{
		XMFLOAT3(0.5f,0.5f,-0.5f),			//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,0.0f)					    //テクスチャ座標
	},
	// 頂点5　右上
	{
		XMFLOAT3(0.5f,0.5f,0.5f),				//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(1.0f,0.0f)					    //テクスチャ座標
	},
	//頂点6　左下
	{
		XMFLOAT3(0.5f,-0.5f,-0.5f),			//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,1.0f)					    //テクスチャ座標
	},
	/*
	//頂点9　左下
	{
		XMFLOAT3(0.5f,-0.5f,-0.5f),			//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,1.0f)					    //テクスチャ座標
	},
	// 頂点10　右上
	{
		XMFLOAT3(0.5f,0.5f,0.5f),				//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(1.0f,0.0f)					    //テクスチャ座標
	},
	*/
	// 頂点7　右下
	{
		XMFLOAT3(0.5f,-0.5f,0.5f),			//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(1.0f,1.0f)					    //テクスチャ座標
	},


	//==================
	// 裏面（-Z面）
	//==================
	// 頂点8　左上
	{
		XMFLOAT3(0.5f,0.5f,0.5f),				//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(1.0f,0.0f)					    //テクスチャ座標
	},
	// 頂点9　右上
	{
		XMFLOAT3(-0.5f,0.5f,0.5f),			//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,0.0f)					    //テクスチャ座標
	},
	// 頂点10　左下
	{
		XMFLOAT3(0.5f,-0.5f,0.5f),			//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(1.0f,1.0f)					    //テクスチャ座標
	},
	/*
	//頂点15　左下
	{
		XMFLOAT3(0.5f,-0.5f,0.5f),			//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(1.0f,1.0f)					    //テクスチャ座標
	},
	// 頂点16　右上
	{
		XMFLOAT3(-0.5f,0.5f,0.5f),			//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,0.0f)					    //テクスチャ座標
	},
	*/
	// 頂点11　右下
	{
		XMFLOAT3(-0.5f,-0.5f,0.5f),			//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,1.0f)					    //テクスチャ座標
	},

	//==================
	// 左面（-X面）
	//==================
	// 頂点12　左上
	{
		XMFLOAT3(-0.5f,0.5f,0.5f),			//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,0.0f)					    //テクスチャ座標
	},
	// 頂点13　右上
	{
		XMFLOAT3(-0.5f,0.5f,-0.5f),			//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(1.0f,0.0f)					    //テクスチャ座標
	},
	// 頂点14　左下
	{
		XMFLOAT3(-0.5f,-0.5f,0.5f),			//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,1.0f)					    //テクスチャ座標
	},
	/*
	// 頂点21　左下
	{
		XMFLOAT3(-0.5f,-0.5f,0.5f),			//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,1.0f)					    //テクスチャ座標
	},
	// 頂点22　右上
	{
		XMFLOAT3(-0.5f,0.5f,-0.5f),			//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(1.0f,0.0f)					    //テクスチャ座標
	},
	*/
	// 頂点15　右下
	{
		XMFLOAT3(-0.5f,-0.5f,-0.5f),			//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(1.0f,1.0f)					    //テクスチャ座標
	},

	//==================
	// 天面（+Y面）
	//==================
	//頂点16　左上
	{
		XMFLOAT3(-0.5f, 0.5f, 0.5f),			//頂点座標
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	//カラー
		XMFLOAT2(0.0f, 0.0f)					    //テクスチャ座標
	},
	//頂点17　右上
	{
		XMFLOAT3(0.5f, 0.5f, 0.5f),				//頂点座標
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	//カラー
		XMFLOAT2(1.0f, 0.0f)					    //テクスチャ座標
	},
	//頂点18　左下
	{
		XMFLOAT3(-0.5f,0.5f,-0.5f),			//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,1.0f)					    //テクスチャ座標
	},
	/*
	//頂点26　左下
	{
		XMFLOAT3(-0.5f,0.5f,-0.5f),			//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,1.0f)					    //テクスチャ座標
	},
	//頂点27　右上
	{
		XMFLOAT3(0.5f, 0.5f, 0.5f),				//頂点座標
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	//カラー
		XMFLOAT2(1.0f, 0.0f)					    //テクスチャ座標
	},
	*/
	//頂点19　右下
	{
		XMFLOAT3(0.5f,0.5f,-0.5f),			//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(1.0f,1.0f)					    //テクスチャ座標
	},

	//==================
	// 底面（-Ｙ面）
	//==================
	//頂点20　左上
	{
		XMFLOAT3(-0.5f,-0.5f,-0.5f),		//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,0.0f)					    //テクスチャ座標
	},
	//頂点21　右上
	{
		XMFLOAT3(0.5f,-0.5f,-0.5f),			//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(1.0f,0.0f)					    //テクスチャ座標
	},
	//頂点22　左下
	{
		XMFLOAT3(-0.5f,-0.5f,0.5f),			//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,1.0f)					    //テクスチャ座標
	},
	/*
	//頂点32　左下
	{
		XMFLOAT3(-0.5f,-0.5f,0.5f),			//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,1.0f)					    //テクスチャ座標
	},
		//頂点33　右上
	{
		XMFLOAT3(0.5f,-0.5f,-0.5f),			//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(1.0f,0.0f)					    //テクスチャ座標
	},
	*/
	//頂点23　右下
	{
		XMFLOAT3(0.5f,-0.5f,0.5f),			//頂点座標
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(1.0f,1.0f)					    //テクスチャ座標
	},
};
static UINT idxdata[6*6]=
{
	0,1,2,2,1,3,			//正面
	4,5,6,6,5,7,			//右面
	8,9,10,10,9,11,		//裏面
	12,13,14,14,13,15,	//左面
	16,17,18,18,17,19,	//天面
	20,21,22,22,21,23	//底面
	
};
static Mouse_State ms;

//=========================================================================================================
// グローバル変数
//=========================================================================================================
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;
static ID3D11ShaderResourceView* g_Texture;		//テクスチャ変数
static ID3D11Buffer* g_VertexBuffer = NULL;		// 頂点バッファ
static ID3D11Buffer* g_IndexBuffer = NULL;		// 頂点バッファ
static XMFLOAT3 Position;		//ポリゴン座標
static XMFLOAT3 Rotation;		//ポリゴン回転角度
static XMFLOAT3 Scaling;		//ポリゴン拡大率

//=========================================================================================================
// 初期化処理
//=========================================================================================================
void Polygon3D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	Scaling = XMFLOAT3(1.0f, 1.0f, 1.0f);
	
	//頂点バッファ作成
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));	//0でクリア
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(Vertex) * NUM_VERTEX;	//格納できる頂点数*頂点サイズ
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	pDevice->CreateBuffer(&bd, NULL, &g_VertexBuffer);

	// テクスチャ読み込み
	TexMetadata metadata;
	ScratchImage image;
	LoadFromWICFile(L"Asset\\Texture\\202501429-ProjectSEKAI-AkiyamaMizuki-iPhone-Android-Wallpaper.jpg", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(pDevice, image.GetImages(),
	image.GetImageCount(), metadata, &g_Texture);
	assert(g_Texture);

	g_pDevice = pDevice;
	g_pContext = pContext;

	//インデックスバッファ作成
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));	//0でクリア
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof(UINT) * 6 * 6;	//格納できる頂点数*頂点サイズ
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		pDevice->CreateBuffer(&bd, NULL, &g_IndexBuffer);

		//インデックスバッファへ書き込み
		D3D11_MAPPED_SUBRESOURCE msr;
		pContext->Map(g_IndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
		UINT* index = (UINT*)msr.pData;

		//インデックスデータをバッファへコピー
		CopyMemory(&index[0], &idxdata[0], sizeof(UINT) *6 * 6);
		pContext->Unmap(g_IndexBuffer, 0);

	}
}

//=========================================================================================================
// 終了処理
//=========================================================================================================
void Polygon3D_Finalize()
{
	if (g_IndexBuffer != NULL)
	{
		g_IndexBuffer->Release();
		g_IndexBuffer = NULL;
	}

	if(g_VertexBuffer!=NULL)
	{
		g_VertexBuffer->Release();
		g_VertexBuffer = NULL;
	}

	if (g_Texture != NULL)
	{
		g_Texture->Release();
		g_Texture = NULL;
	}
}

//=========================================================================================================
// 更新処理
//=========================================================================================================
void Polygon3D_Update()
{
	/*Rotation.x += 2.0f;
	Rotation.y += 2.0f;*/
	
	Mouse_GetState(&ms);
	float sensitivity = 0.01f;	//マウス加速度
	static bool relativeMode = false;	//モード切替（true:相対/false:絶対）

	bool suppressDelta = false;

	static bool prevRight = false;
	if (ms.rightButton && !prevRight) {
		relativeMode = !relativeMode;
		Mouse_SetMode(relativeMode ? MOUSE_POSITION_MODE_RELATIVE
			: MOUSE_POSITION_MODE_ABSOLUTE);

		suppressDelta = true;
	}

	prevRight = ms.rightButton;

	if (ms.positionMode == MOUSE_POSITION_MODE_RELATIVE)
	{
		if (!suppressDelta)
		{
			Position.x += ms.x * sensitivity;
			Position.y -= ms.y * sensitivity;
		}
	}

	//移動
	if (Keyboard_IsKeyDown(KK_W))
	{
		Position.z += 0.5f;
	}
	if (Keyboard_IsKeyDown(KK_S))
	{
		Position.z -= 0.5f;
	}
	if (Keyboard_IsKeyDown(KK_A))
	{
		Rotation.y -= 0.2f;
	}
	if (Keyboard_IsKeyDown(KK_D))
	{
		Rotation.y += 0.2f;
	}
}

//=========================================================================================================
// 描画処理
//=========================================================================================================
void Polygon3D_Draw()
{

	// ワールド行列の作成
	//スケーリング行列の作成
	XMMATRIX ScalingMatrix = XMMatrixScaling(Scaling.x, Scaling.y, Scaling.z);
	//平行移動行列の作成
	XMMATRIX TranslationMatrix = XMMatrixTranslation(Position.x, Position.y, Position.z);
	//回転行列の作成
	XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(XMConvertToRadians(Rotation.x), XMConvertToRadians(Rotation.y), XMConvertToRadians(Rotation.z));

	//計算の順番「スケール*回転*平行移動」
	XMMATRIX WorldMatrix = ScalingMatrix * RotationMatrix * TranslationMatrix;

	//プロジェクション行列作成
	XMMATRIX Projection = GetProjectionMatrix();
	

	//ビュー行列作成
	XMMATRIX View = GetViewMatrix();

	//最終的な変換行列を作成	順番に注意！！
	XMMATRIX WVP = WorldMatrix * View * Projection;

	//変換行列を頂点シェーダへセット
	Shader_SetMatrix(WVP);

	//シェーダーを描画パイプラインへ設定
	Shader_Begin();

	//頂点データを頂点バッファへコピーする
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	Vertex* vertex = (Vertex*)msr.pData;
	//頂点データコピー
	CopyMemory(&vertex[0], &vdata[0], sizeof(Vertex) * NUM_VERTEX);
	g_pContext->Unmap(g_VertexBuffer, 0);

	//テクスチャーセット
	g_pContext->PSSetShaderResources(0, 1, &g_Texture);

	//頂点バッファをセット
	UINT stride = sizeof(Vertex); //1頂点あたりのデータサイズ
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_VertexBuffer, &stride, &offset);

	//インデックスバッファをセット
	g_pContext->IASetIndexBuffer(g_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	//描画するポリゴンの種類をセット
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//描画リクエスト
	//g_pContext->Draw(NUM_VERTEX, 0);

	g_pContext->DrawIndexed(6*6, 0, 0);
}