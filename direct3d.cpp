/*==============================================================================

   Direct3Dの初期化関連 [direct3d.cpp]
--------------------------------------------------------------------------------

==============================================================================*/
#include <d3d11.h>
#include "direct3d.h"
#include "debug_ostream.h"
#include <float.h>

#pragma comment(lib, "d3d11.lib")//DirectXのプログラムを追加する
// #pragma comment(lib, "dxgi.lib")

/* 各種インターフェース */
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;

/* バックバッファ関連 */
static ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
static ID3D11Texture2D* g_pDepthStencilBuffer = nullptr;
static ID3D11DepthStencilView* g_pDepthStencilView = nullptr;
static D3D11_TEXTURE2D_DESC g_BackBufferDesc{};
static D3D11_VIEWPORT g_Viewport{};////////////////追加

static bool configureBackBuffer(); // バックバッファの設定・生成
static void releaseBackBuffer(); // バックバッファの解放


static float	bFactor[4] = { 0.0f,0.0f,0.0f,0.0f };
static ID3D11BlendState* bState[BLENDSTATE_MAX];
static ID3D11DepthStencilState* g_DepthStateEnable;
static ID3D11DepthStencilState* g_DepthStateDisable;

ID3D11Texture2D* g_pShadowCubemapTex = nullptr;
ID3D11RenderTargetView* g_pShadowCubemapRTV[6] = {};
ID3D11ShaderResourceView* g_pShadowCubemapSRV = nullptr;
ID3D11SamplerState* g_pShadowSamplerState = nullptr;
ID3D11RasterizerState* g_pShadowRasterizer = nullptr;

static ID3D11Texture2D* g_pShadowDepthTex = nullptr;
static ID3D11DepthStencilView* g_pShadowDepthDSV = nullptr;

XMFLOAT3 g_ShadowLightPos = XMFLOAT3(0.0f, 1.0f, 10.0f);//ライトの座標
float g_ShadowLightRadius = 50.0f;//ライトの半径

bool Direct3D_Initialize(HWND hWnd)
{
	/* デバイス、スワップチェーン、コンテキスト生成 */
	DXGI_SWAP_CHAIN_DESC swap_chain_desc{};
	swap_chain_desc.Windowed = TRUE;
	swap_chain_desc.BufferCount = 2;
	// swap_chain_desc.BufferDesc.Width = 0;
	// swap_chain_desc.BufferDesc.Height = 0;
	// ⇒ ウィンドウサイズに合わせて自動的に設定される
	swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.SampleDesc.Count = 1;
	swap_chain_desc.SampleDesc.Quality = 0;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;//0にしてみる
	swap_chain_desc.OutputWindow = hWnd;

	/*
	IDXGIFactory1* pFactory;
	CreateDXGIFactory1(IID_PPV_ARGS(&pFactory));
	IDXGIAdapter1* pAdapter;
	pFactory->EnumAdapters1(1, &pAdapter); // セカンダリアダプタを取得
	pFactory->Release();
	DXGI_ADAPTER_DESC1 desc;
	pAdapter->GetDesc1(&desc); // アダプタの情報を取得して確認したい場合
	pAdapter->Release(); // D3D11CreateDeviceAndSwapChain()の第１引数に渡して利用し終わったら解放する
	*/

	UINT device_flags = 0;

#if defined(DEBUG) || defined(_DEBUG)
	//device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

	D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		device_flags,
		levels,
		ARRAYSIZE(levels),
		D3D11_SDK_VERSION,
		&swap_chain_desc,
		&g_pSwapChain,
		&g_pDevice,
		&feature_level,
		&g_pDeviceContext);

	if (FAILED(hr)) {
		MessageBox(hWnd, "Direct3Dの初期化に失敗しました", "エラー", MB_OK);
		return false;
	}

	if (!configureBackBuffer()) {
		MessageBox(hWnd, "バックバッファの設定に失敗しました", "エラー", MB_OK);
		return false;
	}

	// サンプラーステート設定
	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(samplerDesc));
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;//ちょっといいフィルターにする
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;//横の座標範囲外は画像繰り返し
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;//縦の座標範囲外は画像繰り返し
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;//未使用
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	ID3D11SamplerState* samplerState = NULL;
	g_pDevice->CreateSamplerState(&samplerDesc, &samplerState);
	//サンプラーをシェーダーへセット
	g_pDeviceContext->PSSetSamplers(0, 1, &samplerState);



	// ブレンドステート設定
	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(blendDesc));
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	//ブレンド無効
	blendDesc.RenderTarget[0].BlendEnable = FALSE;	//ブレンド無効
	g_pDevice->CreateBlendState(&blendDesc, &bState[BLENDSTATE_NONE]);

	//αブレンド
	blendDesc.RenderTarget[0].BlendEnable = TRUE;	//ブレンド有効
	g_pDevice->CreateBlendState(&blendDesc, &bState[BLENDSTATE_ALFA]);//<<ALPHA！

	//加算合成
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	g_pDevice->CreateBlendState(&blendDesc, &bState[BLENDSTATE_ADD]);

	//減算合成
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_SUBTRACT;//<<<<表示色 = 背景 - ポリゴン
	//	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_REV_SUBTRACT;//<<<<表示色 = 背景 - ポリゴン
	g_pDevice->CreateBlendState(&blendDesc, &bState[BLENDSTATE_SUB]);

	SetBlendState(BLENDSTATE_ALFA);//デフォルト設定


	// 深度ステンシルステート設定
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilDesc.StencilEnable = FALSE;
	g_pDevice->CreateDepthStencilState(&depthStencilDesc, &g_DepthStateEnable);//深度有効ステート
	depthStencilDesc.DepthEnable = FALSE;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	g_pDevice->CreateDepthStencilState(&depthStencilDesc, &g_DepthStateDisable);//深度無効ステート

	g_pDeviceContext->OMSetDepthStencilState(g_DepthStateDisable, NULL); //デフォルト　深度無効

	Direct3D_InitializeShadowMap();

	return true;
}

void	SetDepthTest(bool flg)
{
	if (flg == true)
	{
		g_pDeviceContext->OMSetDepthStencilState(g_DepthStateEnable, NULL); //デフォルト　深度無効
	}
	else
	{
		g_pDeviceContext->OMSetDepthStencilState(g_DepthStateDisable, NULL); //デフォルト　深度無効
	}


}

void Direct3D_Finalize()
{
	releaseBackBuffer();

	if (g_pSwapChain) {
		g_pSwapChain->Release();
		g_pSwapChain = nullptr;
	}

	if (g_pDeviceContext) {
		g_pDeviceContext->Release();
		g_pDeviceContext = nullptr;
	}

	if (g_pDevice) {
		g_pDevice->Release();
		g_pDevice = nullptr;
	}

	SAFE_RELEASE(g_pShadowCubemapTex);
	for (int i = 0; i < 6; i++) {
		SAFE_RELEASE(g_pShadowCubemapRTV[i]);
	}
	SAFE_RELEASE(g_pShadowCubemapSRV);
	SAFE_RELEASE(g_pShadowSamplerState);
	SAFE_RELEASE(g_pShadowRasterizer);

	SAFE_RELEASE(g_pShadowDepthTex);
	SAFE_RELEASE(g_pShadowDepthDSV);
}

void Direct3D_Clear()
{
	float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	g_pDeviceContext->ClearRenderTargetView(g_pRenderTargetView, clear_color);
	g_pDeviceContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// レンダーターゲットビューとデプスステンシルビューの設定/////////////追加
	g_pDeviceContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);


}

void Direct3D_Present()
{
	// スワップチェーンの表示
	g_pSwapChain->Present(1, 0);
}

ID3D11Device* Direct3D_GetDevice()
{
	return g_pDevice;
}

ID3D11DeviceContext* Direct3D_GetDeviceContext()
{
	return g_pDeviceContext;
}

unsigned int Direct3D_GetBackBufferWidth()
{
	return g_BackBufferDesc.Width;
}

unsigned int Direct3D_GetBackBufferHeight()
{
	return g_BackBufferDesc.Height;
}

bool configureBackBuffer()
{
	HRESULT hr;

	ID3D11Texture2D* back_buffer_pointer = nullptr;

	// バックバッファの取得
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back_buffer_pointer);

	if (FAILED(hr)) {
		hal::dout << "バックバッファの取得に失敗しました" << std::endl;
		return false;
	}

	// バックバッファのレンダーターゲットビューの生成
	hr = g_pDevice->CreateRenderTargetView(back_buffer_pointer, nullptr, &g_pRenderTargetView);

	if (FAILED(hr)) {
		back_buffer_pointer->Release();
		hal::dout << "バックバッファのレンダーターゲットビューの生成に失敗しました" << std::endl;
		return false;
	}

	// バックバッファの状態（情報）を取得
	back_buffer_pointer->GetDesc(&g_BackBufferDesc);

	back_buffer_pointer->Release(); // バックバッファのポインタは不要なので解放

	// デプスステンシルバッファの生成
	D3D11_TEXTURE2D_DESC depth_stencil_desc{};
	depth_stencil_desc.Width = g_BackBufferDesc.Width;
	depth_stencil_desc.Height = g_BackBufferDesc.Height;
	depth_stencil_desc.MipLevels = 1;
	depth_stencil_desc.ArraySize = 1;
	depth_stencil_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depth_stencil_desc.SampleDesc.Count = 1;
	depth_stencil_desc.SampleDesc.Quality = 0;
	depth_stencil_desc.Usage = D3D11_USAGE_DEFAULT;
	depth_stencil_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depth_stencil_desc.CPUAccessFlags = 0;
	depth_stencil_desc.MiscFlags = 0;
	hr = g_pDevice->CreateTexture2D(&depth_stencil_desc, nullptr, &g_pDepthStencilBuffer);

	if (FAILED(hr)) {
		hal::dout << "デプスステンシルバッファの生成に失敗しました" << std::endl;
		return false;
	}

	// デプスステンシルビューの生成
	D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc{};
	depth_stencil_view_desc.Format = depth_stencil_desc.Format;
	depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depth_stencil_view_desc.Texture2D.MipSlice = 0;
	depth_stencil_view_desc.Flags = 0;
	hr = g_pDevice->CreateDepthStencilView(g_pDepthStencilBuffer, &depth_stencil_view_desc, &g_pDepthStencilView);

	if (FAILED(hr)) {
		hal::dout << "デプスステンシルビューの生成に失敗しました" << std::endl;
		return false;
	}


	// ビューポートの設定/////////////////////追加
	g_Viewport.TopLeftX = 0.0f;
	g_Viewport.TopLeftY = 0.0f;
	g_Viewport.Width = static_cast<FLOAT>(g_BackBufferDesc.Width);
	g_Viewport.Height = static_cast<FLOAT>(g_BackBufferDesc.Height);
	g_Viewport.MinDepth = 0.0f;
	g_Viewport.MaxDepth = 1.0f;
	g_pDeviceContext->RSSetViewports(1, &g_Viewport); // ビューポートの設定
	////////////////////////////////////////////追加


	return true;
}

void releaseBackBuffer()
{
	if (g_pRenderTargetView) {
		g_pRenderTargetView->Release();
		g_pRenderTargetView = nullptr;
	}

	if (g_pDepthStencilBuffer) {
		g_pDepthStencilBuffer->Release();
		g_pDepthStencilBuffer = nullptr;
	}

	if (g_pDepthStencilView) {
		g_pDepthStencilView->Release();
		g_pDepthStencilView = nullptr;
	}
}


//以下の関数を一番下へ追加
void SetBlendState(BLENDSTATE blend)
{

	g_pDeviceContext->OMSetBlendState(bState[blend], bFactor, 0xffffffff);

}

void Direct3D_InitializeShadowMap()
{
	SAFE_RELEASE(g_pShadowCubemapTex);
	for (int i = 0; i < 6; i++) {
		SAFE_RELEASE(g_pShadowCubemapRTV[i]);
	}
	SAFE_RELEASE(g_pShadowCubemapSRV);
	SAFE_RELEASE(g_pShadowSamplerState);
	SAFE_RELEASE(g_pShadowRasterizer);
	SAFE_RELEASE(g_pShadowDepthTex);
	SAFE_RELEASE(g_pShadowDepthDSV);

	HRESULT hr;

	hal::dout << "Initializing shadow map (color cubemap)..." << std::endl;

	// ============ COLOR CUBEMAP (stores linear depth as color) ============
	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = SHADOW_MAP_SIZE;
	texDesc.Height = SHADOW_MAP_SIZE;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 6;
	texDesc.Format = DXGI_FORMAT_R32_FLOAT;  // Color format
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;  // RTV, not DSV! 
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	hr = g_pDevice->CreateTexture2D(&texDesc, nullptr, &g_pShadowCubemapTex);
	if (FAILED(hr)) {
		hal::dout << "ERROR: Failed to create shadow cubemap texture!  HRESULT: " << hr << std::endl;
		return;
	}
	hal::dout << "Shadow cubemap texture created successfully" << std::endl;

	// ============ Create RTV for each cubemap face ============
	for (int i = 0; i < 6; i++)
	{
		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.MipSlice = 0;
		rtvDesc.Texture2DArray.FirstArraySlice = i;
		rtvDesc.Texture2DArray.ArraySize = 1;

		hr = g_pDevice->CreateRenderTargetView(g_pShadowCubemapTex, &rtvDesc, &g_pShadowCubemapRTV[i]);
		if (FAILED(hr)) {
			hal::dout << "ERROR: Failed to create RTV for face " << i << "! HR: " << hr << std::endl;
		}
		else {
			hal::dout << "Created RTV for face " << i << std::endl;
		}
	}

	// ============ Separate depth buffer for shadow rendering ============
	D3D11_TEXTURE2D_DESC depthDesc = {};
	depthDesc.Width = SHADOW_MAP_SIZE;
	depthDesc.Height = SHADOW_MAP_SIZE;
	depthDesc.MipLevels = 1;
	depthDesc.ArraySize = 1;
	depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.SampleDesc.Quality = 0;
	depthDesc.Usage = D3D11_USAGE_DEFAULT;

	hr = g_pDevice->CreateTexture2D(&depthDesc, nullptr, &g_pShadowDepthTex);
	if (FAILED(hr)) {
		hal::dout << "ERROR: Failed to create shadow depth texture! HR: " << hr << std::endl;
		return;
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;

	hr = g_pDevice->CreateDepthStencilView(g_pShadowDepthTex, &dsvDesc, &g_pShadowDepthDSV);
	if (FAILED(hr)) {
		hal::dout << "ERROR: Failed to create shadow depth DSV! HR: " << hr << std::endl;
	}
	else {
		hal::dout << "Shadow depth DSV created successfully" << std::endl;
	}

	// ============ Cubemap SRV for sampling in pixel shader ============
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MipLevels = 1;
	srvDesc.TextureCube.MostDetailedMip = 0;

	hr = g_pDevice->CreateShaderResourceView(g_pShadowCubemapTex, &srvDesc, &g_pShadowCubemapSRV);
	if (FAILED(hr)) {
		hal::dout << "ERROR: Failed to create cubemap SRV!  HR: " << hr << std::endl;
	}
	else {
		hal::dout << "Cubemap SRV created successfully" << std::endl;
	}

	// ============ Regular sampler for manual depth comparison ============
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.BorderColor[0] = 1.0f;
	sampDesc.BorderColor[1] = 1.0f;
	sampDesc.BorderColor[2] = 1.0f;
	sampDesc.BorderColor[3] = 1.0f;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.MaxAnisotropy = 1;
	sampDesc.MipLODBias = 0;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	hr = g_pDevice->CreateSamplerState(&sampDesc, &g_pShadowSamplerState);
	if (FAILED(hr)) {
		hal::dout << "ERROR: Failed to create shadow sampler! HR: " << hr << std::endl;
	}
	else {
		hal::dout << "Shadow sampler created successfully" << std::endl;
	}

	//D3D11_RASTERIZER_DESC rsDesc = {};
	//rsDesc.FillMode = D3D11_FILL_SOLID;
	//rsDesc.CullMode = D3D11_CULL_NONE;
	//rsDesc.DepthClipEnable = TRUE;

	//hr = g_pDevice->CreateRasterizerState(&rsDesc, &g_pShadowRasterizer);
	//if (FAILED(hr))
	//{
	//	hal::dout << "ERROR: Failed to create shadow rasterizer state!" << std::endl;
	//}

	hal::dout << "Shadow map initialization complete!" << std::endl;
}

void Direct3D_BeginShadowPass(int faceIndex)
{
	if (faceIndex < 0 || faceIndex >= 6) return;

	// Set viewport to shadow map size
	D3D11_VIEWPORT vp = {};
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = (FLOAT)SHADOW_MAP_SIZE;
	vp.Height = (FLOAT)SHADOW_MAP_SIZE;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	g_pDeviceContext->RSSetViewports(1, &vp);
	//g_pDeviceContext->RSSetState(g_pShadowRasterizer);

	// Render to color cubemap face + depth buffer
	g_pDeviceContext->OMSetRenderTargets(1, &g_pShadowCubemapRTV[faceIndex], g_pShadowDepthDSV);


	// Clear color to 1.0 (far) and depth
	float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	g_pDeviceContext->ClearRenderTargetView(g_pShadowCubemapRTV[faceIndex], clearColor);
	g_pDeviceContext->ClearDepthStencilView(g_pShadowDepthDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void Direct3D_EndShadowPass()
{
	g_pDeviceContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);
	g_pDeviceContext->RSSetViewports(1, &g_Viewport);
	g_pDeviceContext->RSSetState(nullptr);
}

XMMATRIX Direct3D_GetCubemapFaceViewProj(int faceIndex, const XMFLOAT3& lightPos, float radius)
{
	XMVECTOR eye = XMLoadFloat3(&lightPos);

	XMVECTOR directions[6] = {
		XMVectorSet(1,  0,  0, 0), // +X
		XMVectorSet(-1,  0,  0, 0), // -X
		XMVectorSet(0,  1,  0, 0), // +Y
		XMVectorSet(0, -1,  0, 0), // -Y
		XMVectorSet(0,  0,  1, 0), // +Z
		XMVectorSet(0,  0, -1, 0)  // -Z
	};

	XMVECTOR ups[6] = {
		XMVectorSet(0, 1, 0, 0),  // +X
		XMVectorSet(0, 1, 0, 0),  // -X
		XMVectorSet(0, 0,-1, 0),  // +Y
		XMVectorSet(0, 0, 1, 0),  // -Y
		XMVectorSet(0, 1, 0, 0),  // +Z
		XMVectorSet(0, 1, 0, 0)   // -Z
	};

	XMVECTOR target = XMVectorAdd(eye, directions[faceIndex]);
	XMVECTOR up = ups[faceIndex];

	XMMATRIX view = XMMatrixLookAtLH(eye, target, up);

	float nearPlane = 0.1f;
	float farPlane = radius;
	XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.0f, nearPlane, farPlane);

	return view * proj;
}