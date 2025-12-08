#include "field.h"
#include "camera.h"
#include "model.h"

//=========================================================================================================
// マクロ定義
//=========================================================================================================
#define BOX_NUM_VERTEX (24)

//=========================================================================================================
//構造体定義・定義
//=========================================================================================================
static Vertex3D Box_vdata[BOX_NUM_VERTEX]
{
	//==================
	// 正面（+Z面）
	//==================
	//頂点０　左上
	{
		XMFLOAT3(-0.5f,0.5f,-0.5f),			//頂点座標
		XMFLOAT3(0.5f,0.5f,0.5f),
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,0.0f)					    //テクスチャ座標
	},
	//頂点1　右上
	{
		XMFLOAT3(0.5f,0.5f,-0.5f),			//頂点座標
		XMFLOAT3(0.5f,0.5f,0.5f),
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(1.0f,0.0f)					    //テクスチャ座標
	},
	//頂点2　左下
	{
		XMFLOAT3(-0.5f,-0.5f,-0.5f),		//頂点座標
		XMFLOAT3(0.5f,0.5f,0.5f),
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,1.0f)					    //テクスチャ座標
	},

	//頂点3　右下
	{
		XMFLOAT3(0.5f,-0.5f,-0.5f),			//頂点座標
		XMFLOAT3(0.5f,0.5f,0.5f),
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(1.0f,1.0f)					    //テクスチャ座標
	},

	//==================
	// 右面（+X面）
	//==================
	// 頂点4　左上
	{
		XMFLOAT3(0.5f,0.5f,-0.5f),			//頂点座標
		XMFLOAT3(0.5f,0.5f,0.5f),
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,0.0f)					    //テクスチャ座標
	},
	// 頂点5　右上
	{
		XMFLOAT3(0.5f,0.5f,0.5f),				//頂点座標
		XMFLOAT3(0.5f,0.5f,0.5f),
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(1.0f,0.0f)					    //テクスチャ座標
	},
	//頂点6　左下
	{
		XMFLOAT3(0.5f,-0.5f,-0.5f),			//頂点座標
		XMFLOAT3(0.5f,0.5f,0.5f),
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,1.0f)					    //テクスチャ座標
	},
	// 頂点7　右下
	{
		XMFLOAT3(0.5f,-0.5f,0.5f),			//頂点座標
		XMFLOAT3(0.5f,0.5f,0.5f),
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(1.0f,1.0f)					    //テクスチャ座標
	},

	//==================
	// 裏面（-Z面）
	//==================
	// 頂点8　左上
	{
		XMFLOAT3(0.5f,0.5f,0.5f),				//頂点座標7
		XMFLOAT3(0.5f,0.5f,0.5f),
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(1.0f,0.0f)					    //テクスチャ座標
	},
	// 頂点9　右上
	{
		XMFLOAT3(-0.5f,0.5f,0.5f),			//頂点座標
		XMFLOAT3(0.5f,0.5f,0.5f),
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,0.0f)					    //テクスチャ座標
	},
	// 頂点10　左下
	{
		XMFLOAT3(0.5f,-0.5f,0.5f),			//頂点座標
		XMFLOAT3(0.5f,0.5f,0.5f),
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(1.0f,1.0f)					    //テクスチャ座標
	},
	// 頂点11　右下
	{
		XMFLOAT3(-0.5f,-0.5f,0.5f),			//頂点座標
		XMFLOAT3(0.5f,0.5f,0.5f),
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,1.0f)					    //テクスチャ座標
	},

	//==================
	// 左面（-X面）
	//==================
	// 頂点12　左上
	{
		XMFLOAT3(-0.5f,0.5f,0.5f),			//頂点座標
		XMFLOAT3(0.5f,0.5f,0.5f),
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,0.0f)					    //テクスチャ座標
	},
	// 頂点13　右上
	{
		XMFLOAT3(-0.5f,0.5f,-0.5f),			//頂点座標
		XMFLOAT3(0.5f,0.5f,0.5f),
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(1.0f,0.0f)					    //テクスチャ座標
	},
	// 頂点14　左下
	{
		XMFLOAT3(-0.5f,-0.5f,0.5f),			//頂点座標
		XMFLOAT3(0.5f,0.5f,0.5f),
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,1.0f)					    //テクスチャ座標
	},
	// 頂点15　右下
	{
		XMFLOAT3(-0.5f,-0.5f,-0.5f),		//頂点座標
		XMFLOAT3(0.5f,0.5f,0.5f),
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(1.0f,1.0f)					    //テクスチャ座標
	},

	//==================
	// 天面（+Y面）
	//==================
	//頂点16　左上
	{
		XMFLOAT3(-0.5f, 0.5f, 0.5f),			//頂点座標
		XMFLOAT3(0.0f,1.0f,0.0f),
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	//カラー
		XMFLOAT2(0.0f, 0.0f)					    //テクスチャ座標
	},
	//頂点17　右上
	{
		XMFLOAT3(0.5f, 0.5f, 0.5f),			//頂点座標
		XMFLOAT3(0.0f,1.0f,0.0f),
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	//カラー
		XMFLOAT2(1.0f, 0.0f)					    //テクスチャ座標
	},
	//頂点18　左下
	{
		XMFLOAT3(-0.5f,0.5f,-0.5f),			//頂点座標
		XMFLOAT3(0.0f,1.0f,0.0f),
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,0.25f)					    //テクスチャ座標
	},
	//頂点19　右下
	{
		XMFLOAT3(0.5f,0.5f,-0.5f),			//頂点座標
		XMFLOAT3(0.0f,1.0f,0.0f),
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(1.0f,0.25f)					    //テクスチャ座標
	},

	//==================
	// 底面（-Ｙ面）
	//==================
	//頂点20　左上
	{
		XMFLOAT3(-0.5f,-0.5f,-0.5f),		//頂点座標
		XMFLOAT3(0.0f,1.0f,0.0f),
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,0.75f)					    //テクスチャ座標
	},
	//頂点21　右上
	{
		XMFLOAT3(0.5f,-0.5f,-0.5f),			//頂点座標
		XMFLOAT3(0.0f,1.0f,0.0f),
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(1.0f,0.75f)					    //テクスチャ座標
	},
	//頂点22　左下
	{
		XMFLOAT3(-0.5f,-0.5f,0.5f),			//頂点座標
		XMFLOAT3(0.0f,1.0f,0.0f),
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(0.0f,1.0f)					    //テクスチャ座標
	},
	//頂点23　右下
	{
		XMFLOAT3(0.5f,-0.5f,0.5f),			//頂点座標
		XMFLOAT3(0.0f,1.0f,0.0f),
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),	//カラー
		XMFLOAT2(1.0f,1.0f)					    //テクスチャ座標
	},
};
static UINT Box_idxdata[6 * 6] =
{
	0,1,2,2,1,3,			//正面
	4,5,6,6,5,7,			//右面
	8,9,10,10,9,11,		//裏面
	12,13,14,14,13,15,	//左面
	16,17,18,18,17,19,	//天面
	20,21,22,22,21,23	//底面
};

//mapデータ配列
MAPDATA map[]=
{
	{XMFLOAT3(0.0f,0.0f,0.0f),FIELD_BOX},

	{XMFLOAT3(2.0f,0.0f,1.0f),FIELD_BOX},
	{XMFLOAT3(1.0f,0.0f,1.0f),FIELD_BOX},
	{XMFLOAT3(0.0f,0.0f,1.0f),FIELD_BOX},
	{XMFLOAT3(-1.0f,0.0f,1.0f),FIELD_BOX},
	{XMFLOAT3(-2.0f,0.0f,1.0f),FIELD_BOX},
	
	{XMFLOAT3(2.0f,0.0f,2.0f),FIELD_BOX },
	{XMFLOAT3(1.0f,0.0f,2.0f),FIELD_BOX},
	{XMFLOAT3(1.0f,1.0f,2.0f),FIELD_BOX},
	{XMFLOAT3(0.0f,0.0f,2.0f),FIELD_BOX},
	{XMFLOAT3(-1.0f,0.0f,2.0f),FIELD_BOX},
	{XMFLOAT3(-1.0f,1.0f,2.0f),FIELD_BOX},
	{XMFLOAT3(-2.0f,0.0f,2.0f),FIELD_BOX},
	
	{XMFLOAT3(2.0f,0.0f,3.0f),FIELD_BOX},
	{XMFLOAT3(1.0f,0.0f,3.0f),FIELD_BOX},
	{XMFLOAT3(0.0f,0.0f,3.0f),FIELD_BOX},
	{XMFLOAT3(-1.0f,0.0f,3.0f),FIELD_BOX},
	{XMFLOAT3(-2.0f,0.0f,3.0f),FIELD_BOX},

	{XMFLOAT3(2.0f,0.0f,4.0f),FIELD_BOX},
	{XMFLOAT3(0.0f,0.0f,4.0f),FIELD_BOX},
	{XMFLOAT3(-2.0f,0.0f,4.0f),FIELD_BOX},
	
	{XMFLOAT3(2.0f,0.0f,5.0f),FIELD_BOX},
	{XMFLOAT3(1.0f,0.0f,5.0f),FIELD_BOX},
	{XMFLOAT3(0.0f,0.0f,5.0f),FIELD_BOX},
	{XMFLOAT3(-1.0f,0.0f,5.0f),FIELD_BOX},
	{XMFLOAT3(-2.0f,0.0f,5.0f),FIELD_BOX},

	{XMFLOAT3(2.0f,0.0f,6.0f),FIELD_BOX},
	{XMFLOAT3(2.0f,1.0f,6.0f),FIELD_BOX},
	{XMFLOAT3(1.0f,0.0f,6.0f),FIELD_BOX},
	{XMFLOAT3(0.0f,0.0f,6.0f),FIELD_BOX},
	{XMFLOAT3(-1.0f,0.0f,6.0f),FIELD_BOX},
	{XMFLOAT3(-2.0f,0.0f,6.0f),FIELD_BOX},
	{XMFLOAT3(-2.0f,1.0f,6.0f),FIELD_BOX},

	{XMFLOAT3(2.0f,1.0f,7.0f),FIELD_BOX},
	{XMFLOAT3(2.0f,0.0f,7.0f),FIELD_BOX},
	{XMFLOAT3(1.0f,0.0f,7.0f),FIELD_BOX},
	{XMFLOAT3(0.0f,0.0f,7.0f),FIELD_BOX},
	{XMFLOAT3(-1.0f,0.0f,7.0f),FIELD_BOX},
	{XMFLOAT3(-2.0f,0.0f,7.0f),FIELD_BOX},
	{XMFLOAT3(-2.0f,1.0f,7.0f),FIELD_BOX},

	{XMFLOAT3(2.0f,0.0f,8.0f),FIELD_BOX},
	{XMFLOAT3(1.0f,0.0f,8.0f),FIELD_BOX},
	{XMFLOAT3(-1.0f,0.0f,8.0f),FIELD_BOX},
	{XMFLOAT3(-2.0f,0.0f,8.0f),FIELD_BOX},

	{XMFLOAT3(2.0f,0.0f,9.0f),FIELD_BOX},
	{XMFLOAT3(1.0f,0.0f,9.0f),FIELD_BOX},
	{XMFLOAT3(0.0f,0.0f,9.0f),FIELD_BOX},
	{XMFLOAT3(-1.0f,0.0f,9.0f),FIELD_BOX},
	{XMFLOAT3(-2.0f,0.0f,9.0f),FIELD_BOX},

	{XMFLOAT3(2.0f,0.0f,10.0f),FIELD_BOX},
	{XMFLOAT3(1.0f,0.0f,10.0f),FIELD_BOX},
	{XMFLOAT3(0.0f,0.0f,10.0f),FIELD_BOX},
	{XMFLOAT3(-1.0f,0.0f,10.0f),FIELD_BOX},
	{XMFLOAT3(-2.0f,0.0f,10.0f),FIELD_BOX},

	{XMFLOAT3(1.0f,0.0f,11.0f),FIELD_BOX},
	{XMFLOAT3(-1.0f,0.0f,11.0f),FIELD_BOX},


	{XMFLOAT3(2.0f,0.0f,12.0f),FIELD_BOX},
	{XMFLOAT3(1.0f,0.0f,12.0f),FIELD_BOX},
	{XMFLOAT3(0.0f,0.0f,12.0f),FIELD_BOX},
	{XMFLOAT3(-1.0f,0.0f,12.0f),FIELD_BOX},
	{XMFLOAT3(-2.0f,0.0f,12.0f),FIELD_BOX},

	{XMFLOAT3(2.0f,0.0f,13.0f),FIELD_BOX},
	{XMFLOAT3(1.0f,0.0f,13.0f),FIELD_BOX},
	{XMFLOAT3(0.0f,0.0f,13.0f),FIELD_BOX},
	{XMFLOAT3(0.0f,1.0f,13.0f),FIELD_BOX},
	{XMFLOAT3(-1.0f,0.0f,13.0f),FIELD_BOX},
	{XMFLOAT3(-2.0f,0.0f,13.0f),FIELD_BOX},

	{XMFLOAT3(2.0f,0.0f,14.0f),FIELD_BOX},
	{XMFLOAT3(1.0f,0.0f,14.0f),FIELD_BOX},
	{XMFLOAT3(-1.0f,0.0f,14.0f),FIELD_BOX},
	{XMFLOAT3(-2.0f,0.0f,14.0f),FIELD_BOX},

	{XMFLOAT3(2.0f,0.0f,15.0f),FIELD_BOX},
	{XMFLOAT3(1.0f,0.0f,15.0f),FIELD_BOX},
	{XMFLOAT3(0.0f,0.0f,15.0f),FIELD_BOX},
	{XMFLOAT3(-1.0f,0.0f,15.0f),FIELD_BOX},
	{XMFLOAT3(-2.0f,0.0f,15.0f),FIELD_BOX},

	{XMFLOAT3(0.0f,0.0f,16.0f),FIELD_BOX},

		{XMFLOAT3(0.0f,-1.0f,-3.0f),FIELD_MAX},	//データ終了
};

MODEL* Model[FIELD_MAX] = {NULL};
//=========================================================================================================
// グローバル変数
//=========================================================================================================
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;
static ID3D11ShaderResourceView* g_Texture;		//テクスチャ変数
static ID3D11Buffer* g_VertexBuffer = NULL;		// 頂点バッファ
static ID3D11Buffer* g_IndexBuffer = NULL;		// インデックスバッファ

//=========================================================================================================
// 初期化処理
//=========================================================================================================
void field_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	//Test = ModelLoad("asset\\model\\test.fbx");		//test
	
	g_pDevice = pDevice;
	g_pContext = pContext;

	// テクスチャ読み込み
	TexMetadata metadata;
	ScratchImage image;
	LoadFromWICFile(L"Asset\\Texture\\block_field.png", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(pDevice, image.GetImages(),
	image.GetImageCount(), metadata, &g_Texture);
	assert(g_Texture);

	for (int i = 0; i < FIELD_MAX; i++)
	{
		switch (i)
		{
		case FIELD_BOX:
			CreateBox();
			break;
		case FIELD_OBT_0:
			Model[FIELD_OBT_0]= ModelLoad("asset\\model\\tree.fbx");
		}
	}
}

//=========================================================================================================
// 終了処理
//=========================================================================================================
void field_Finalize(void)
{
	for (int i = 0; i < FIELD_MAX; i++)
	{
		if (Model[i] != NULL)
		{
			ModelRelease(Model[i]);
			Model[i] = NULL;
		}
	}
	
	SAFE_RELEASE(g_VertexBuffer);
	SAFE_RELEASE(g_IndexBuffer);
	SAFE_RELEASE(g_Texture);
}
//=========================================================================================================
// 更新処理
//=========================================================================================================
void field_Update(void)
{
}

//=========================================================================================================
// 描画処理
//=========================================================================================================
void field_Draw(void)
{
	Shader_Begin();
	//プロジェクション行列作成
	XMMATRIX Projection = GetProjectionMatrix();
	//ビュー行列作成
	XMMATRIX View = GetViewMatrix();
	XMMATRIX VP = View * Projection;

	//mapの表示
	int i = 0;
	static float rot = 0.0f;
	rot -= 0.3f;
	while (map[i].no != FIELD_MAX)
	{
		// ワールド行列の作成
		//スケーリング行列の作成
		XMMATRIX ScalingMatrix = XMMatrixScaling(1.0f,1.0f,1.0f);
		//平行移動行列の作成
		XMMATRIX TranslationMatrix = XMMatrixTranslation(map[i].pos.x, map[i].pos.y, map[i].pos.z);
		//回転行列の作成
		XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(XMConvertToRadians(0.0f), XMConvertToRadians(0.0f), XMConvertToRadians(0.0f));
		//計算の順番「スケール*回転*平行移動」
		XMMATRIX WorldMatrix = ScalingMatrix * RotationMatrix * TranslationMatrix;
		//最終的な変換行列を作成	順番に注意！！
		XMMATRIX WVP = WorldMatrix * VP;
		//DirectXへ行列をセット
		Shader_SetWorldMatrix(WorldMatrix);
		Shader_SetMatrix(WVP);
		
		
		//テクスチャーセット
		g_pContext->PSSetShaderResources(0, 1, &g_Texture);
		
		//頂点バッファをセット
		UINT stride = sizeof(Vertex3D); //1頂点あたりのデータサイズ
		UINT offset = 0;
		g_pContext->IASetVertexBuffers(0, 1, &g_VertexBuffer, &stride, &offset);

		//インデックスバッファをセット
		g_pContext->IASetIndexBuffer(g_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		//描画するポリゴンの種類をセット
		g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		if (map[i].no == FIELD_BOX)
		{
			//描画リクエスト
			g_pContext->DrawIndexed(6 * 6, 0, 0);

		}
		else
		{
			//描画リクエスト
			ModelDraw(Model[map[i].no]);
		}
		//ModelDraw(Test);
		i++;
	}
}
//=========================================================================================================
// Box作成
//=========================================================================================================
void CreateBox()
{
	{
		//頂点バッファ作成
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));	//0でクリア
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof(Vertex3D) * BOX_NUM_VERTEX;	//格納できる頂点数*頂点サイズ
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		g_pDevice->CreateBuffer(&bd, NULL, &g_VertexBuffer);

		//頂点データを頂点バッファへコピーする
		D3D11_MAPPED_SUBRESOURCE msr;
		g_pContext->Map(g_VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
		Vertex3D* vertex = (Vertex3D*)msr.pData;
		//頂点データコピー
		CopyMemory(&vertex[0], &Box_vdata[0], sizeof(Vertex3D) * BOX_NUM_VERTEX);
		g_pContext->Unmap(g_VertexBuffer, 0);
	}
	
	//インデックスバッファ作成
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));	//0でクリア
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof(UINT) * 6 * 6;	//格納できる頂点数*頂点サイズ
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		g_pDevice->CreateBuffer(&bd, NULL, &g_IndexBuffer);

		//インデックスバッファへ書き込み
		D3D11_MAPPED_SUBRESOURCE msr;
		g_pContext->Map(g_IndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
		UINT* index = (UINT*)msr.pData;

		//インデックスデータをバッファへコピー
		CopyMemory(&index[0], &Box_idxdata[0], sizeof(UINT) * 6 * 6);
		g_pContext->Unmap(g_IndexBuffer, 0);

	}
}

MAPDATA* GetFieldMap(void)
{
	return &map[0];
}

XMMATRIX Field_GetWorldMatrix(int i)
{
	XMMATRIX ScalingMatrix = XMMatrixScaling(
		1.0f, 1.0f, 1.0f);

	//平行移動行列の作成
	XMMATRIX TranslationMAtrix = XMMatrixTranslation(
		map[i].pos.x, map[i].pos.y, map[i].pos.z);


	//回転行列の作成
	XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(
		//XMConvertToRadians(rot),
		XMConvertToRadians(0.0f),
		XMConvertToRadians(0.0f),
		XMConvertToRadians(0.0f));

	return ScalingMatrix * RotationMatrix * TranslationMAtrix;
}


void Field_DrawShadowMap(const XMMATRIX& lightViewProj)
{
	int objectsDrawn = 0;
	int i = 0;

	while (map[i].no != FIELD_MAX)
	{
		// Calculate world matrix for this specific field object
		XMMATRIX world = Field_GetWorldMatrix(i);

		// Set matrices for shadow rendering
		Shader_SetWorldMatrix(world);
		Shader_SetMatrix(world * lightViewProj);

		if (map[i].no == FIELD_BOX)
		{
			// Set vertex and index buffers for box
			UINT stride = sizeof(Vertex3D);
			UINT offset = 0;
			g_pContext->IASetVertexBuffers(0, 1, &g_VertexBuffer, &stride, &offset);
			g_pContext->IASetIndexBuffer(g_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
			g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			g_pContext->DrawIndexed(6 * 6, 0, 0);
			objectsDrawn++;
		}
		else
		{
			// Draw model for other field types
			if (Model[map[i].no] != NULL)
			{
				ModelDraw(Model[map[i].no]);
				objectsDrawn++;
			}
		}
		i++;
	}
}

