//field.cpp
#include "field.h"
#include "camera.h"
#include "model.h"
#include <fstream>
#include <sstream>
#include <iostream>

//=========================================================================================================
// マクロ定義
//=========================================================================================================
#define BOX_NUM_VERTEX (36)

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
	0,  1,  2,  2,  1,  3,//-Z
	4,  5,  6,  6,  5,  7,//+Z
	8,  9, 10, 10,  9, 11,//+X
   12, 13, 14, 14, 13, 15,//-X
   16, 17, 18, 18, 17, 19,//+Y
   20, 21, 22, 22, 21, 23,//-Y

};

//=========================================================================================================
// グローバル変数
//=========================================================================================================
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;
static ID3D11ShaderResourceView* g_Texture;		//テクスチャ変数
static ID3D11Buffer* g_VertexBuffer = NULL;		// 頂点バッファ
static ID3D11Buffer* g_IndexBuffer = NULL;		// インデックスバッファ

MODEL* Model[FIELD_MAX] = { NULL };
static std::vector<MAPDATA> g_MapData;

static void EnsureBoxCreated()
{
	if (g_VertexBuffer && g_IndexBuffer) return;
	CreateBox();
}

//=========================================================================================================
// 初期化
//=========================================================================================================
void field_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	// テクスチャ
	TexMetadata metadata;
	ScratchImage image;
	LoadFromWICFile(L"asset\\Texture\\block_field.png", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_Texture);
	assert(g_Texture);

	if (!LoadMapFromFile("asset\\MapData\\stage1.txt"))
	{
		// Error:  could not load map
		MessageBox(nullptr, "Failed to load map file! Error", "エラー", MB_OK);
	}

	for (size_t i = 0; i < g_MapData.size(); ++i)
	{
		g_MapData[i].scale = XMFLOAT3(1.0f, 1.0f, 1.0f);
		g_MapData[i].rotate = XMFLOAT3(0.0f, 0.0f, 0.0f);
	}


	bool hasGround = false;
	bool hasWall = false;
	for (const auto& m : g_MapData) {
		if (m.no == FIELD_GROUND) hasGround = true;
		if (m.no == FIELD_WALL)   hasWall = true;
	}

	if (hasGround || hasWall || !Model[FIELD_OBJ_1] || !Model[FIELD_OBJ_2] || !Model[FIELD_EMPTY_BOX]) EnsureBoxCreated();

	if (!Model[FIELD_OBJ_BOX])
	{
		Model[FIELD_OBJ_BOX] = ModelLoad("asset\\model\\tree.fbx");
	}
	if (!Model[FIELD_GOAL])
	{
		Model[FIELD_GOAL] = ModelLoad("asset\\model\\test.fbx");
	}

	
}


//=========================================================================================================
// 終了
//=========================================================================================================
void field_Finalize(void)
{
	g_MapData.clear();  // Clear the vector

	for (int i = 0; i < FIELD_MAX; i++)
	{
		if (Model[i] != NULL) {
			ModelRelease(Model[i]);
			Model[i] = NULL;
		}
	}
	SAFE_RELEASE(g_VertexBuffer);
	SAFE_RELEASE(g_IndexBuffer);
	SAFE_RELEASE(g_Texture);
}

//=========================================================================================================
// 更新
//=========================================================================================================
void field_Update(void)
{
	for (size_t i = 0; i < g_MapData.size(); ++i)
	{
		if (g_MapData[i].no == FIELD_OBJ_1)
		{
			g_MapData[i].scale = XMFLOAT3(1.0f, 4.0f, 5.0f);
			g_MapData[i].rotate = XMFLOAT3(0.0f, 20.0f, 0.0f);
		}
	}
}

//=========================================================================================================
// 描画
//=========================================================================================================
void field_Draw(void)
{
	Shader_Begin();
	XMMATRIX Projection = GetProjectionMatrix();
	XMMATRIX View = GetViewMatrix();
	XMMATRIX VP = View * Projection;

	for (size_t i = 0; i < g_MapData.size(); ++i)
	{
		XMMATRIX ScalingMatrix = XMMatrixScaling(
			g_MapData[i].scale.x,
			g_MapData[i].scale.y,
			g_MapData[i].scale.z);
		XMMATRIX TranslationMatrix = XMMatrixTranslation(
			g_MapData[i].pos.x,
			g_MapData[i].pos.y,
			g_MapData[i].pos.z);
		XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(
			XMConvertToRadians(g_MapData[i].rotate.x),
			XMConvertToRadians(g_MapData[i].rotate.y),
			XMConvertToRadians(g_MapData[i].rotate.z));

		XMMATRIX WorldMatrix = ScalingMatrix * RotationMatrix * TranslationMatrix;
		XMMATRIX WVP = WorldMatrix * VP;

		Shader_SetWorldMatrix(WorldMatrix);
		Shader_SetMatrix(WVP);

		

		if (Model[g_MapData[i].no])
			ModelDraw(Model[g_MapData[i].no]);
		else if (g_MapData[i].no == FIELD_EMPTY_BOX)
		{
			continue;
		}
		else {
			g_pContext->PSSetShaderResources(0, 1, &g_Texture);
			UINT stride = sizeof(Vertex3D);
			UINT offset = 0;
			g_pContext->IASetVertexBuffers(0, 1, &g_VertexBuffer, &stride, &offset);
			g_pContext->IASetIndexBuffer(g_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
			g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			g_pContext->DrawIndexed(6 * 6, 0, 0);
		}
	}
}

//=========================================================================================================
// Box作成
//=========================================================================================================
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
	{
		//頂点バッファ作成
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));//0でクリア
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof(UINT) * BOX_NUM_VERTEX;//格納できる頂点数*頂点サイズ
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		g_pDevice->CreateBuffer(&bd, NULL, &g_IndexBuffer);

		//index buffer read in 
		D3D11_MAPPED_SUBRESOURCE msr;
		g_pContext->Map(g_IndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
		UINT* index = (UINT*)msr.pData;

		//copy index buffer
		CopyMemory(&index[0], &Box_idxdata[0], sizeof(UINT) * BOX_NUM_VERTEX);
		g_pContext->Unmap(g_IndexBuffer, 0);
	}
}

XMMATRIX Field_GetWorldMatrix(int i)
{
	const auto& m = g_MapData[i];

	XMMATRIX ScalingMatrix = XMMatrixScaling(m.scale.x, m.scale.y, m.scale.z);
	XMMATRIX TranslationMatrix = XMMatrixTranslation(m.pos.x, m.pos.y, m.pos.z);
	XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(m.rotate.x),
		XMConvertToRadians(m.rotate.y),
		XMConvertToRadians(m.rotate.z));
	return ScalingMatrix * RotationMatrix * TranslationMatrix;
}

void Field_DrawShadowMap(const XMMATRIX& world, const XMMATRIX& matrix, int i)
{
	Shader_SetWorldMatrix(world);
	Shader_SetMatrix(matrix);
	// Set vertex and index buffers for box

	if (g_MapData[i].no == FIELD_GROUND)
		return; // skip shadow

	if (g_MapData[i].no == FIELD_EMPTY_BOX)
		return;
	
	if (Model[g_MapData[i].no])
	{
		ModelDraw(Model[g_MapData[i].no]);
		return;
	}

	UINT stride = sizeof(Vertex3D);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_VertexBuffer, &stride, &offset);
	g_pContext->IASetIndexBuffer(g_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	g_pContext->DrawIndexed(6 * 6, 0, 0);

}

//=========================================================================================================
// txtロード
//=========================================================================================================
std::vector<MAPDATA>& GetFieldMap()
{
	return g_MapData;
}


bool LoadMapFromFile(const char* filename)
{
	std::ifstream file(filename);
	if (!file.is_open()) return false;

	// Detect map 1 (stage1)
	bool isMap1 = false;
	if (strstr(filename, "stage1") != nullptr)
	{
		isMap1 = true;
	}

	g_MapData.clear();
	std::string line;
	int y = 0;  // height layer
	int z = 0;  // depth

	while (std::getline(file, line))
	{
		// Skip comments
		if (line.empty() || line[0] == '#')
		{
			// Check for new layer
			if (line.find("Layer") != std::string::npos) {
				y++;
				z = 0;
			}
			continue;
		}

		// Parse each character
		for (int x = 0; x < (int)line.length(); x++)
		{
			FIELD type;
			bool valid = true;

			switch (line[x])
			{
			case 'G': type = FIELD_GROUND;   break;
			case 'W':  type = FIELD_WALL; break;
			case 'B':  type = FIELD_OBJ_BOX;  break;
			case 'E':  type = FIELD_EMPTY_BOX;  break;
			case '1':  type = FIELD_GOAL;  break;
			case 'S':  type = FIELD_OBJ_1;  break;
			case '2':  type = FIELD_OBJ_2;  break;
			case '.': valid = false;      break;  // Empty
			case ' ': valid = false;      break;  // Space
			default:  valid = false;      break;
			}

			if (valid)
			{
				MAPDATA data;
				if (isMap1)
				{
					data.pos = XMFLOAT3(
						(float)(x - 2),
						(float)(y - 1),
						(float)(z - 2)
					);
				}
				else
				{
					data.pos = XMFLOAT3(
						(float)x,
						(float)y,
						(float)z
					);
				}
				data.no = type;
				g_MapData.push_back(data);
			}
		}
		z++;
	}

	file.close();
	return true;
}