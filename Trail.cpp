//Trail.cpp
#include "Trail.h"
#include"camera.h"
//=========================================================================================================
// マクロ定義
//=========================================================================================================
#define TRAIL_LENGTH (20)		//軌跡の長さ

//=========================================================================================================
// グローバル変数
//=========================================================================================================
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;
static ID3D11Buffer* g_VertexBuffer = NULL;		// 頂点バッファ
static ID3D11ShaderResourceView* g_Texture;	//テクスチャ変数
static XMFLOAT3 g_Position[TRAIL_LENGTH];	//頂点配列

//=========================================================================================================
// 初期化処理
//=========================================================================================================
void Trail_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;
	
	// テクスチャ読み込み
	TexMetadata metadata;
	ScratchImage image;
	LoadFromWICFile(L"Asset\\Texture\\Trail.png", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(pDevice, image.GetImages(),
		image.GetImageCount(), metadata, &g_Texture);
	assert(g_Texture);

	//頂点バッファ作成
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));	//0でクリア
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(Vertex3D) * TRAIL_LENGTH * 2;	//格納できる頂点数*頂点サイズ
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	g_pDevice->CreateBuffer(&bd, NULL, &g_VertexBuffer);
}

//=========================================================================================================
// 終了処理
//=========================================================================================================
void Trail_Finalize()
{
	SAFE_RELEASE(g_VertexBuffer);
	SAFE_RELEASE(g_Texture);
}

//=========================================================================================================
// 更新処理
//=========================================================================================================
void Trail_Update()
{
}

//=========================================================================================================
// 描画処理
//=========================================================================================================
void Trail_Draw()
{
	//頂点データの書き換え
	//頂点データを頂点バッファへコピーする
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	Vertex3D* vertex = (Vertex3D*)msr.pData;

	//カメラの座標を取得
	XMFLOAT3 CameraPos = GetCameraPosition();
	float alpha = 0.4f;
	float texv = 1.0f;

	//ポリゴンの頂点を作る
	for (int i = 0; i < TRAIL_LENGTH - 1; i++)
	{
		XMFLOAT3 cameraDir, trailDir, cross, pos;

		//視線ベクトル作成
		cameraDir.x = g_Position[i].x - CameraPos.x;
		cameraDir.y = g_Position[i].y - CameraPos.y;
		cameraDir.z = g_Position[i].z - CameraPos.z;
		//トレイルの方向ベクトル
		trailDir.x = g_Position[i].x - g_Position[i + 1].x;
		trailDir.y = g_Position[i].y - g_Position[i + 1].y;
		trailDir.z = g_Position[i].z - g_Position[i + 1].z;
		//外積計算
		XMVECTOR v1 = XMLoadFloat3(&cameraDir);
		XMVECTOR v2 = XMLoadFloat3(&trailDir);
		XMVECTOR c = XMVector3Cross(v1, v2);
		XMStoreFloat3(&cross, c);
		//正規化
		float len = sqrtf(cross.x * cross.x + cross.y * cross.y + cross.z * cross.z);
		if (len != 0.0f)
		{
			cross.x /= len;
			cross.y /= len;
			cross.z /= len;
		}

		//ポリゴンの頂点座標を2つずつ作成
		pos.x = g_Position[i].x + cross.x * 0.2f;		//0.2fは幅
		pos.y = g_Position[i].y + cross.y * 0.2f;		//0.2fは幅
		pos.z = g_Position[i].z + cross.z * 0.2f;		//0.2fは幅
		
		vertex[i * 2 + 1].position = pos;
		vertex[i * 2 + 1].color = XMFLOAT4(0.0f, 1.0f, 1.0f, alpha);
		vertex[i * 2 + 1].texCoord = XMFLOAT2(0.0f, texv);
		vertex[i * 2 + 1].normal = XMFLOAT3(0.0f, 1.0f, 0.0f);

		alpha += 0.2f / (TRAIL_LENGTH);
		texv -= 1.0f / (TRAIL_LENGTH * 1.5f);
	}

	vertex[TRAIL_LENGTH * 2 - 2].position = g_Position[TRAIL_LENGTH - 1];
	vertex[TRAIL_LENGTH * 2 - 2].color = XMFLOAT4(0.0f, 1.0f, 0.9f, 1.0f);
	vertex[TRAIL_LENGTH * 2 - 2].texCoord = XMFLOAT2(0.5f, 0.0f);
	vertex[TRAIL_LENGTH * 2 - 2].normal = XMFLOAT3(0.0f, 1.0f, 0.0f);

	vertex[TRAIL_LENGTH * 2 - 1].position = g_Position[TRAIL_LENGTH - 1];
	vertex[TRAIL_LENGTH * 2 - 1].color = XMFLOAT4(0.0f, 1.0f, 0.9f, 1.0f);
	vertex[TRAIL_LENGTH * 2 - 1].texCoord = XMFLOAT2(0.5f, 0.0f);
	vertex[TRAIL_LENGTH * 2 - 1].normal = XMFLOAT3(0.0f, 1.0f, 0.0f);

	//頂点バッファへの書き込み終了
	Direct3D_GetDeviceContext()->Unmap(g_VertexBuffer, 0);

	//行列作成
	XMMATRIX Projection = GetProjectionMatrix();
	XMMATRIX View = GetViewMatrix();
	XMMATRIX World = XMMatrixIdentity();
	XMMATRIX WVP = World * View * Projection;
	Shader_SetMatrix(WVP);

	//テクスチャーセット
	g_pContext->PSSetShaderResources(0, 1, &g_Texture);
	// 頂点バッファをセット
	UINT stride = sizeof(Vertex3D); //1頂点あたりのデータサイズ
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_VertexBuffer, &stride, &offset);
	//描画するポリゴンの種類をセット
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//描画
	SetBlendState(BLENDSTATE::BLENDSTATE_ADD);
	g_pContext->Draw(TRAIL_LENGTH * 2, 0);
	SetBlendState(BLENDSTATE::BLENDSTATE_ALFA);
}

void SetTrailPosition(XMFLOAT3 pos)
{
	for (int i = 0; i < TRAIL_LENGTH - 1; i++)
	{
		g_Position[i] = g_Position[i + 1];
	}
	g_Position[TRAIL_LENGTH - 1] = pos;
}

