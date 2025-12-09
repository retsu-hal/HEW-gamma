#include "field.h"
#include "camera.h"
#include "model.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>

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

//=========================================================================================================
// グローバル変数
//=========================================================================================================
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;
static ID3D11ShaderResourceView* g_Texture;		//テクスチャ変数
static ID3D11Buffer* g_VertexBuffer = NULL;		// 頂点バッファ
static ID3D11Buffer* g_IndexBuffer = NULL;		// インデックスバッファ
std::vector<MAPDATA> g_MapData; 
MODEL* Model[FIELD_MAX] = { NULL };

// 初期化
void field_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// 必要ならここでg_MapData.clear();
	LoadMapFromCSV("Asset\\MapData\\map_testdata.csv");

	g_pDevice = pDevice;
	g_pContext = pContext;

	// テクスチャ
	TexMetadata metadata;
	ScratchImage image;
	LoadFromWICFile(L"Asset\\Texture\\block_field.png", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_Texture);
	assert(g_Texture);

	for (int i = 0; i < FIELD_MAX; i++) {
		switch (i) {
		case FIELD_BOX:
			CreateBox();
			break;
		case FIELD_OBJ_1:
			Model[FIELD_OBJ_1] = ModelLoad("asset\\model\\tree.fbx");
			break;
			// 他のOBJタイプも必要なら追加
		}
	}
}

// 終了
void field_Finalize(void)
{
	for (int i = 0; i < FIELD_MAX; i++) {
		if (Model[i] != NULL) {
			ModelRelease(Model[i]);
			Model[i] = NULL;
		}
	}
	SAFE_RELEASE(g_VertexBuffer);
	SAFE_RELEASE(g_IndexBuffer);
	SAFE_RELEASE(g_Texture);
}

// 更新
void field_Update(void)
{
}

// 描画
void field_Draw(void)
{
	Shader_Begin();
	XMMATRIX Projection = GetProjectionMatrix();
	XMMATRIX View = GetViewMatrix();
	XMMATRIX VP = View * Projection;

	for (size_t i = 0; i < g_MapData.size(); ++i)
	{
		XMMATRIX ScalingMatrix = XMMatrixScaling(1.0f, 1.0f, 1.0f);
		XMMATRIX TranslationMatrix = XMMatrixTranslation(g_MapData[i].pos.x, g_MapData[i].pos.y, g_MapData[i].pos.z);
		XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(0, 0, 0);
		XMMATRIX WorldMatrix = ScalingMatrix * RotationMatrix * TranslationMatrix;
		XMMATRIX WVP = WorldMatrix * VP;

		Shader_SetWorldMatrix(WorldMatrix);
		Shader_SetMatrix(WVP);

		g_pContext->PSSetShaderResources(0, 1, &g_Texture);
		UINT stride = sizeof(Vertex3D);
		UINT offset = 0;
		g_pContext->IASetVertexBuffers(0, 1, &g_VertexBuffer, &stride, &offset);
		g_pContext->IASetIndexBuffer(g_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		if (g_MapData[i].no == FIELD_BOX)
			g_pContext->DrawIndexed(6 * 6, 0, 0);
		else if (Model[g_MapData[i].no])
			ModelDraw(Model[g_MapData[i].no]);
	}
}

// Box作成
void CreateBox()
{
	// 頂点バッファ
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(Vertex3D) * BOX_NUM_VERTEX;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	g_pDevice->CreateBuffer(&bd, NULL, &g_VertexBuffer);

	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	Vertex3D* vertex = (Vertex3D*)msr.pData;
	CopyMemory(vertex, Box_vdata, sizeof(Vertex3D) * BOX_NUM_VERTEX);
	g_pContext->Unmap(g_VertexBuffer, 0);

	// インデックスバッファ
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(UINT) * 6 * 6;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	g_pDevice->CreateBuffer(&bd, NULL, &g_IndexBuffer);

	g_pContext->Map(g_IndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	UINT* index = (UINT*)msr.pData;
	CopyMemory(index, Box_idxdata, sizeof(UINT) * 6 * 6);
	g_pContext->Unmap(g_IndexBuffer, 0);
}

MAPDATA* GetFieldMap(void)
{
	if (g_MapData.empty()) return nullptr;
	return g_MapData.data();
}

size_t GetFieldMapSize(void)
{
	return g_MapData.size();
}

XMMATRIX Field_GetWorldMatrix(int i)
{
	XMMATRIX ScalingMatrix = XMMatrixScaling(1.0f, 1.0f, 1.0f);
	XMMATRIX TranslationMatrix = XMMatrixTranslation(g_MapData[i].pos.x, g_MapData[i].pos.y, g_MapData[i].pos.z);
	XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(0, 0, 0);
	return ScalingMatrix * RotationMatrix * TranslationMatrix;
}

void Field_DrawShadowMap(const XMMATRIX& lightViewProj)
{
	for (size_t i = 0; i < g_MapData.size(); ++i)
	{
		XMMATRIX world = Field_GetWorldMatrix(i);
		Shader_SetWorldMatrix(world);
		Shader_SetMatrix(world * lightViewProj);

		if (g_MapData[i].no == FIELD_BOX) {
			UINT stride = sizeof(Vertex3D);
			UINT offset = 0;
			g_pContext->IASetVertexBuffers(0, 1, &g_VertexBuffer, &stride, &offset);
			g_pContext->IASetIndexBuffer(g_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
			g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			g_pContext->DrawIndexed(6 * 6, 0, 0);
		}
		else if (Model[g_MapData[i].no])
			ModelDraw(Model[g_MapData[i].no]);
	}
}

// CSVロード
void LoadMapFromCSV(const char* filename)
{
	std::cout << "Loaded field count: " << g_MapData.size() << std::endl;
	std::ifstream file(filename);
	if (!file.is_open())
	{
		std::cerr << "ファイルを開けません: " << filename << std::endl;
		return;
	}

	g_MapData.clear(); // 必ずクリア

	std::string line;
	// ヘッダは捨てる
	std::getline(file, line);

	while (std::getline(file, line))
	{
		std::stringstream ss(line);
		std::string segment;
		MAPDATA data;

		try {
			std::getline(ss, segment, ',');
			if (segment.empty()) continue;
			data.pos.x = std::stof(segment);

			std::getline(ss, segment, ',');
			if (segment.empty()) continue;
			data.pos.y = std::stof(segment);

			std::getline(ss, segment, ',');
			if (segment.empty()) continue;
			data.pos.z = std::stof(segment);

			std::getline(ss, segment, ',');
			if (segment.empty()) continue;
			data.no = (FIELD)std::stoi(segment);

			// 範囲外チェック
			if ((int)data.no < 0 || data.no >= FIELD_MAX) continue;

			g_MapData.push_back(data);
		}
		catch (const std::exception&) {
			continue; // 不正行はスキップ
		}
	}
}
