#include "Bill_Board.h"
#include "Camera.h"
#include "shader.h"

static constexpr int NUM_VERTEX = 6;
static ID3D11Buffer* g_pVertexBuffer = nullptr;
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;
static ID3D11ShaderResourceView* g_BillBoardTexture = nullptr;

void InitializeBillBoard()
{
    g_pDevice = Direct3D_GetDevice();
    g_pContext = Direct3D_GetDeviceContext();

    // Create vertex buffer
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(Vertex3D) * NUM_VERTEX;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    g_pDevice->CreateBuffer(&bd, NULL, &g_pVertexBuffer);

    // Load billboard texture (e.g., an interaction icon)
    TexMetadata metadata;
    ScratchImage image;
    LoadFromWICFile(L"asset\\texture\\Title.png", WIC_FLAGS_NONE, &metadata, image);
    CreateShaderResourceView(g_pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_BillBoardTexture);
}

void FinalizeBillBoard()
{
    if (g_pVertexBuffer) g_pVertexBuffer->Release();
    if (g_BillBoardTexture) g_BillBoardTexture->Release();
}

void DrawBillBoard(XMFLOAT3 pos, XMFLOAT2 size, XMFLOAT4 col, int bno, int wc, int hc)
{
    g_pDevice = Direct3D_GetDevice();
    g_pContext = Direct3D_GetDeviceContext();

    // Lock vertex buffer
    D3D11_MAPPED_SUBRESOURCE msr;
    g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

    Vertex3D* v = (Vertex3D*)msr.pData;

    // Calculate texture coordinates
    float w = 1.0f / wc;
    float h = 1.0f / hc;
    float x = (bno % wc) * w;
    float y = (bno / wc) * h;

    // Set up quad vertices
    v[0].position = { -(size.x / 2), (size.y / 2), 0.0f };
    v[0].normal = { 0.0f, 0.0f, -1.0f };
    v[0].color = col;
    v[0].texCoord = { x, y };

    v[1].position = { (size.x / 2), (size.y / 2), 0.0f };
    v[1].normal = { 0.0f, 0.0f, -1.0f };
    v[1].color = col;
    v[1].texCoord = { x + w, y };

    v[2].position = { -(size.x / 2), -(size.y / 2), 0.0f };
    v[2].normal = { 0.0f, 0.0f, -1.0f };
    v[2].color = col;
    v[2].texCoord = { x, y + h };

    v[3].position = { (size.x / 2), -(size.y / 2), 0.0f };
    v[3].normal = { 0.0f, 0.0f, -1.0f };
    v[3].color = col;
    v[3].texCoord = { x + w, y + h };

    g_pContext->Unmap(g_pVertexBuffer, 0);

    // Set vertex buffer
    UINT stride = sizeof(Vertex3D);
    UINT offset = 0;
    g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
    g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // Use your project's camera functions (fixed naming)
    XMMATRIX Projection = GetProjectionMatrix();
    XMMATRIX View = GetViewMatrix();

    // Create billboard matrix (always faces camera)
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

    XMMATRIX MVP = vm * View * Projection;
    Shader_SetMatrix(MVP);

    // Set texture and draw
    g_pContext->PSSetShaderResources(0, 1, &g_BillBoardTexture);
    g_pContext->Draw(4, 0);
}

ID3D11ShaderResourceView* GetBillBoardTexture()
{
    return g_BillBoardTexture;
}