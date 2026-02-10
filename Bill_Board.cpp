#include "Bill_Board.h"
#include "Camera.h"
#include "shader.h"

static constexpr int NUM_VERTEX = 6;
static ID3D11Buffer* g_pVertexBuffer = nullptr;
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

void InitializeBillBoard()
{
    g_pDevice = Direct3D_GetDevice();

    // 頂点バッファ生成
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(Vertex3D) * NUM_VERTEX;//<<<<<<<格納する最大頂点数
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    g_pDevice->CreateBuffer(&bd, NULL, &g_pVertexBuffer);
}

void FinalizeBillBoard()
{
    g_pVertexBuffer->Release();	//頂点バッファの解放
}

void DrawBillBoard(XMFLOAT3 pos, XMFLOAT2 size, XMFLOAT4 col, int bno, int wc, int hc)
{
	g_pDevice = Direct3D_GetDevice();
	g_pContext = Direct3D_GetDeviceContext();

	// 頂点バッファをロックする
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	// 頂点バッファへの仮想ポインタを取得
	Vertex3D* v = (Vertex3D*)msr.pData;

	//ブロックの縦横サイズを計算
	float w = 1.0f / wc;
	float h = 1.0f / hc;
	//bnoの左上のテクスチャ座標を計算
	float x = (bno % wc) * w;
	float y = (bno / wc) * h;

	// 指定の位置に指定のサイズ、色の四角形を描画する /////////テクスチャ追加
	v[0].position = { -(size.x / 2), (size.y / 2), 0.0f };
	v[0].color = col;
	v[0].texCoord = { x, y };

	v[1].position = { (size.x / 2), (size.y / 2), 0.0f };
	v[1].color = col;
	v[1].texCoord = { x + w, y };

	v[2].position = { -(size.x / 2), -(size.y / 2), 0.0f };
	v[2].color = col;
	v[2].texCoord = { x, y + h };

	v[3].position = { (size.x / 2), -(size.y / 2), 0.0f };
	v[3].color = col;
	v[3].texCoord = { x + w, y + h };


	// 頂点バッファのロックを解除
	g_pContext->Unmap(g_pVertexBuffer, 0);

	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex3D);//頂点１つあたりのサイズを指定
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// プリミティブトポロジ設定　ポリゴンの描画ルール的なもの
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);


	XMMATRIX Projection = GetProjectionMatrix();

	XMMATRIX View = GetViewMatrix();

	XMMATRIX vm = GetViewMatrix();
	vm.r[3].m128_f32[0] = 0.0f;
	vm.r[3].m128_f32[1] = 0.0f;
	vm.r[3].m128_f32[2] = 0.0f;
	vm.r[3].m128_f32[3] = 1.0f;
	vm = XMMatrixTranspose(vm);
	vm.r[3].m128_f32[0] = pos.x;
	vm.r[3].m128_f32[1] = pos.y;
	vm.r[3].m128_f32[2] = pos.z;
	vm.r[3].m128_f32[3] = 1.0f;

	//変換行列の作成
	XMMATRIX MVP = vm * View * Projection;
	Shader_SetMatrix(MVP);
	// ポリゴン描画命令発行
	g_pContext->Draw(4, 0);//表示に使用する頂点数を指定
}
