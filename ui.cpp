// ui.cpp
// Simple UI system: Title Menu + Pause Menu.
// Uses existing Sprite + Shader pipeline.

#include "ui.h"
#include "shader.h"
#include "sprite.h"
#include "direct3d.h"
#include "keyboard.h"
#include "DirectXTex.h"

using namespace DirectX;

//--------------------------------------------------------------------------------------
// Internal types
//--------------------------------------------------------------------------------------
struct UiTexture
{
	ID3D11ShaderResourceView* srv = nullptr;
	int wc = 1;
	int hc = 1;
};

static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static UiTexture g_texBg;        // fullscreen background
static UiTexture g_texTitleLogo; // title logo
static UiTexture g_texButton;    // button (3-state atlas: normal/hover/pressed)
static UiTexture g_texCursor;    // cursor (arrow)
static UiTexture g_texPanel;     // panel for pause/options
static UiTexture g_texTitleLabels; // labels for title menu (wc=1 hc=3)
static UiTexture g_texPauseLabels; // labels for pause menu (wc=1 hc=3)

// Title state
static int g_titleIndex = 0;
static UI_TITLE_RESULT g_titleResult = UI_TITLE_NONE;

// Pause state
static bool g_pauseOpen = false;
static int g_pauseIndex = 0;
static UI_PAUSE_RESULT g_pauseResult = UI_PAUSE_NONE;

//--------------------------------------------------------------------------------------
// Helpers
//--------------------------------------------------------------------------------------
static void SafeRelease(ID3D11ShaderResourceView*& p)
{
	if (p)
	{
		p->Release();
		p = nullptr;
	}
}

static UiTexture LoadTexture(const wchar_t* path, int wc = 1, int hc = 1)
{
	UiTexture t{};
	t.wc = wc;
	t.hc = hc;

	TexMetadata metadata{};
	ScratchImage image{};
	HRESULT hr = LoadFromWICFile(path, WIC_FLAGS_NONE, &metadata, image);
	if (FAILED(hr))
	{
		//assert(false && "UI texture load failed. Put png files under asset\\texture or Asset\\Texture");
		//return t;
	}

	hr = CreateShaderResourceView(g_pDevice, image.GetImages(), image.GetImageCount(), metadata, &t.srv);
	assert(SUCCEEDED(hr) && t.srv);
	return t;
}

static void Begin2D()
{
	Shader_Begin();
	const float w = (float)Direct3D_GetBackBufferWidth();
	const float h = (float)Direct3D_GetBackBufferHeight();
	Shader_SetMatrix(XMMatrixOrthographicOffCenterLH(0.0f, w, h, 0.0f, 0.0f, 1.0f));
}

static void DrawTexFull(const UiTexture& t, const XMFLOAT4& col)
{
	const float w = (float)Direct3D_GetBackBufferWidth();
	const float h = (float)Direct3D_GetBackBufferHeight();
	XMFLOAT2 pos{ w * 0.5f, h * 0.5f };
	XMFLOAT2 size{ w, h };
	g_pContext->PSSetShaderResources(0, 1, &t.srv);
	SetBlendState(BLENDSTATE_ALFA);
	DrawSprite(pos, size, col);
}

static void DrawButton(const UiTexture& t, XMFLOAT2 pos, XMFLOAT2 size, XMFLOAT4 col, bool selected)
{
	// button atlas: wc=1 hc=3 (0 normal, 1 selected)
	g_pContext->PSSetShaderResources(0, 1, &t.srv);
	SetBlendState(BLENDSTATE_ALFA);

	const int bno = selected ? 1 : 0;
	DrawSpriteEx(pos, size, col, bno, t.wc, t.hc);
}

static void DrawCursor(const UiTexture& t, XMFLOAT2 pos, XMFLOAT2 size, XMFLOAT4 col)
{
	g_pContext->PSSetShaderResources(0, 1, &t.srv);
	SetBlendState(BLENDSTATE_ALFA);
	DrawSpriteExRotation(pos, size, col, 0, t.wc, t.hc, 0.0f);
}

//--------------------------------------------------------------------------------------
// Public API
//--------------------------------------------------------------------------------------
void UI_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	// NOTE:
	//  - asset\\texture\\ui_bg.png
	//  - asset\\texture\\ui_title_logo.png
	//  - asset\\texture\\ui_button_atlas.png  (wc=1, hc=3)
	//  - asset\\texture\\ui_cursor.png
	//  - asset\\texture\\ui_panel.png
	//  - asset\\texture\\ui_title_labels.png (wc=1 hc=3)
	//  - asset\\texture\\ui_pause_labels.png (wc=1 hc=3)


 	g_texBg = LoadTexture(L"asset\\texture\\ui_bg.png", 1, 1);
	g_texTitleLogo = LoadTexture(L"asset\\texture\\ui_title_logo.png", 1, 1);
	g_texButton = LoadTexture(L"asset\\texture\\ui_button_atlas.png", 1, 3);
	g_texCursor = LoadTexture(L"asset\\texture\\ui_cursor.png", 1, 1);
	g_texPanel = LoadTexture(L"asset\\texture\\ui_panel.png", 1, 1);
	g_texTitleLabels = LoadTexture(L"asset\\texture\\ui_title_labels.png", 1, 3);
	g_texPauseLabels = LoadTexture(L"asset\\texture\\ui_pause_labels.png", 1, 3);

	UI_Title_Reset();
	UI_Pause_Reset();

	D3D11_DEPTH_STENCIL_DESC ds{};
	ds.DepthEnable = FALSE;
	ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	ds.DepthFunc = D3D11_COMPARISON_ALWAYS;
	ds.StencilEnable = FALSE;

	//HRESULT hr = g_pDevice->CreateDepthStencilState(&ds, g_dsOff);
	//assert(SUCCEEDED(hr));

}

void UI_Finalize()
{
	SafeRelease(g_texBg.srv);
	SafeRelease(g_texTitleLogo.srv);
	SafeRelease(g_texButton.srv);
	SafeRelease(g_texCursor.srv);
	SafeRelease(g_texPanel.srv);
	SafeRelease(g_texTitleLabels.srv);
	SafeRelease(g_texPauseLabels.srv);
	//if (g_dsOff) 
	//{ 
	//	g_dsOff->Release(); 
	//	g_dsOff = nullptr;
	//}

}

//---------------- Title ----------------
void UI_Title_Reset()
{
	g_titleIndex = 0;
	g_titleResult = UI_TITLE_NONE;
}

void UI_Title_Update()
{
	if (g_titleResult != UI_TITLE_NONE)
		return;

	// Move selection
	if (Keyboard_IsKeyDownTrigger(KK_UP) || Keyboard_IsKeyDownTrigger(KK_W))
	{
		g_titleIndex = (g_titleIndex + 3 - 1) % 3;
	}
	if (Keyboard_IsKeyDownTrigger(KK_DOWN) || Keyboard_IsKeyDownTrigger(KK_S))
	{
		g_titleIndex = (g_titleIndex + 1) % 3;
	}

	// Decide
	if (Keyboard_IsKeyDownTrigger(KK_ENTER) || Keyboard_IsKeyDownTrigger(KK_SPACE))
	{
		switch (g_titleIndex)
		{
		case 0: g_titleResult = UI_TITLE_START; break;
		case 1: g_titleResult = UI_TITLE_OPTIONS; break;
		case 2: g_titleResult = UI_TITLE_EXIT; break;
		default: break;
		}
	}
}

void UI_Title_Draw()
{
	Begin2D();

	const float sw = (float)Direct3D_GetBackBufferWidth();
	const float sh = (float)Direct3D_GetBackBufferHeight();

	// Background
	DrawTexFull(g_texBg, XMFLOAT4(1, 1, 1, 1));

	// Logo
	{
		g_pContext->PSSetShaderResources(0, 1, &g_texTitleLogo.srv);
		SetBlendState(BLENDSTATE_ALFA);
		XMFLOAT2 pos{ sw * 0.5f, sh * 0.23f };
		XMFLOAT2 size{ sw * 0.60f, sh * 0.25f };
		DrawSprite(pos, size, XMFLOAT4(1, 6, 1, 1));
	}

	// Buttons
	const XMFLOAT2 btnSize{ sw * 0.40f, sh * 0.090f };
	const float baseY = sh * 0.64f;
	const float gapY = sh * 0.12f;

	for (int i = 0; i < 3; ++i)
	{
		XMFLOAT2 pos{ sw * 0.5f, baseY + gapY * i };
		const bool selected = (i == g_titleIndex);
		DrawButton(g_texButton, pos, btnSize, XMFLOAT4(1, 1, 1, 1), selected);

		// Label (baked text in a 1x3 atlas)
		g_pContext->PSSetShaderResources(0, 1, &g_texTitleLabels.srv);
		SetBlendState(BLENDSTATE_ALFA);

		//文字のサイズ少し大きくする
		XMFLOAT2 lableSize(btnSize.x * 0.92f, btnSize.y * 0.72f);

		//影　(右下にずらして黒い)
		XMFLOAT2 shadowPos(pos.x + 4.0f, pos.y + 4.0f);
		DrawSpriteEx(shadowPos, lableSize, 
			XMFLOAT4(0, 0, 0, 0.75f), i, g_texPauseLabels.wc, g_texTitleLabels.hc);

		//本体=白
		DrawSpriteEx(pos, lableSize,
			XMFLOAT4(1,1,1,1), i, g_texPauseLabels.wc, g_texTitleLabels.hc);
			


		DrawSpriteEx(pos, XMFLOAT2(btnSize.x * 0.86f, btnSize.y * 0.65f), XMFLOAT4(1, 1, 1, 1), i, g_texTitleLabels.wc, g_texTitleLabels.hc);

		// Cursor
		if (selected)
		{
			XMFLOAT2 cpos{ pos.x - btnSize.x * 0.62f, pos.y };
			XMFLOAT2 csize{ btnSize.y * 0.55f, btnSize.y * 0.55f };
			DrawCursor(g_texCursor, cpos, csize, XMFLOAT4(1, 1, 1, 1));
		}
	}
}

UI_TITLE_RESULT UI_Title_GetResult()
{
	return g_titleResult;
}

void UI_Title_ConsumeResult()
{
	g_titleResult = UI_TITLE_NONE;
}

//---------------- Pause ----------------
void UI_Pause_Reset()
{
	g_pauseOpen = false;
	g_pauseIndex = 0;
	g_pauseResult = UI_PAUSE_NONE;
}

void UI_Pause_Open()
{
	g_pauseOpen = true;
	g_pauseIndex = 0;
	g_pauseResult = UI_PAUSE_NONE;
}

void UI_Pause_Close()
{
	g_pauseOpen = false;
	g_pauseResult = UI_PAUSE_NONE;
}

bool UI_Pause_IsOpen()
{
	return g_pauseOpen;
}

void UI_Pause_Update()
{
	if (!g_pauseOpen)
	{
		// Open by ESC
		if (Keyboard_IsKeyDownTrigger(KK_ESCAPE))
			UI_Pause_Open();
		return;
	}

	// Close by ESC
	if (Keyboard_IsKeyDownTrigger(KK_ESCAPE))
	{
		g_pauseResult = UI_PAUSE_RESUME;
		return;
	}

	// Move selection
	if (Keyboard_IsKeyDownTrigger(KK_UP) || Keyboard_IsKeyDownTrigger(KK_W))
	{
		g_pauseIndex = (g_pauseIndex + 3 - 1) % 3;
	}
	if (Keyboard_IsKeyDownTrigger(KK_DOWN) || Keyboard_IsKeyDownTrigger(KK_S))
	{
		g_pauseIndex = (g_pauseIndex + 1) % 3;
	}

	// Decide
	if (Keyboard_IsKeyDownTrigger(KK_ENTER) || Keyboard_IsKeyDownTrigger(KK_SPACE))
	{
		switch (g_pauseIndex)
		{
		case 0: g_pauseResult = UI_PAUSE_RESUME; break;
		case 1: g_pauseResult = UI_PAUSE_RESTART; break;
		case 2: g_pauseResult = UI_PAUSE_TO_TITLE; break;
		default: break;
		}
	}
}

void UI_Pause_Draw()
{
	if (!g_pauseOpen)
		return;

	SetDepthTest(FALSE);
	Begin2D();

	const float sw = (float)Direct3D_GetBackBufferWidth();
	const float sh = (float)Direct3D_GetBackBufferHeight();

	// =========================
	// ① 背景：まず通常表示
	// =========================
	DrawTexFull(g_texBg, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

	// =========================
	// ② 暗くする：黒い半透明フィルター
	// （bgが暗い/見づらい問題を安定して解決）
	// =========================
	g_pContext->PSSetShaderResources(0, 1, &g_texPanel.srv);
	SetBlendState(BLENDSTATE_ALFA);

	XMFLOAT2 center{ sw * 0.5f, sh * 0.5f };
	XMFLOAT2 full{ sw, sh };
	DrawSprite(center, full, XMFLOAT4(0, 0, 0, 0.55f));

	// =========================
	// ③ パネル：メニュー枠（中央）
	// =========================
	g_pContext->PSSetShaderResources(0, 1, &g_texPanel.srv);
	SetBlendState(BLENDSTATE_ALFA);

	XMFLOAT2 panelPos{ sw * 0.5f, sh * 0.50f };
	XMFLOAT2 panelSize{ sw * 0.60f, sh * 0.55f };
	DrawSprite(panelPos, panelSize, XMFLOAT4(1, 6, 1, 1));

	// =========================
	// ④ ボタン（大きめ）
	// =========================
	const XMFLOAT2 btnSize{ sw * 0.55f, sh * 0.11f };
	const float baseY = sh * 0.43f;
	const float gapY = sh * 0.13f;

	for (int i = 0; i < 3; ++i)
	{
		XMFLOAT2 pos{ sw * 0.5f, baseY + gapY * i };
		const bool selected = (i == g_pauseIndex);

		// ボタン本体
		DrawButton(g_texButton, pos, btnSize, XMFLOAT4(1, 5, 1, 5), selected);

		// =========================
		// ⑤ 文字：加算でくっきり（暗さ対策）
		// =========================
		g_pContext->PSSetShaderResources(0, 1, &g_texPauseLabels.srv);
		SetBlendState(BLENDSTATE_ADD);

		XMFLOAT2 labelSize(btnSize.x * 0.92f, btnSize.y * 0.70f);
		XMFLOAT4 labelCol = selected
			? XMFLOAT4(1.6f, 1.6f, 1.6f, 1.0f)   // 選択中は明るく
			: XMFLOAT4(1.2f, 1.2f, 1.2f, 1.0f);  // 非選択も見やすく

		DrawSpriteEx(pos, labelSize, labelCol, i, g_texPauseLabels.wc, g_texPauseLabels.hc);

		// 戻す
		SetBlendState(BLENDSTATE_ALFA);

		// =========================
		// ⑥ カーソル：選択中だけ
		// =========================
		if (selected)
		{
			XMFLOAT2 cpos{ pos.x - btnSize.x * 0.62f, pos.y };
			XMFLOAT2 csize{ btnSize.y * 0.65f, btnSize.y * 0.65f };
			DrawCursor(g_texCursor, cpos, csize, XMFLOAT4(1, 7, 1, 1));
		}
	}
}

//void UI_Pause_Draw()
//{
//	SetDepthTest(FALSE);
//	SetBlendState(BLENDSTATE_ALFA);
//	if (!g_pauseOpen)
//		return;
//
//	Begin2D();
//
//	const float sw = (float)Direct3D_GetBackBufferWidth();
//	const float sh = (float)Direct3D_GetBackBufferHeight();
//
//	// Dim background
//	DrawTexFull(g_texBg, XMFLOAT4(1.6f, 1.6f, 1.6f, 1.0f));
//
//	// Panel
//	{
//		g_pContext->PSSetShaderResources(0, 1, &g_texPanel.srv);
//		SetBlendState(BLENDSTATE_ALFA);
//		XMFLOAT2 pos{ sw * 0.0f, sh * 0.0f };//panelbg
//		XMFLOAT2 size{ sw * 0.100f, sh * 0.50f };
//		DrawSprite(pos, size, XMFLOAT4(1, 1, 1, 1));
//	}
//
//	// Buttons
//	const XMFLOAT2 btnSize{ sw * 0.50f, sh * 0.095f };
//	const float baseY = sh * 0.45f;
//	const float gapY = sh * 0.10f;
//
//	for (int i = 0; i < 3; ++i)
//	{
//		XMFLOAT2 pos{ sw * 0.5f, baseY + gapY * i };
//		const bool selected = (i == g_pauseIndex);
//		DrawButton(g_texButton, pos, btnSize, XMFLOAT4(1, 5, 1, 5), selected);
//
//		// Label (baked text in a 1x3 atlas)
//		g_pContext->PSSetShaderResources(0, 1, &g_texPauseLabels.srv);
//		SetBlendState(BLENDSTATE_ALFA);
//		DrawSpriteEx(pos, XMFLOAT2(btnSize.x * 0.78f, btnSize.y * 0.55f), XMFLOAT4(1, 1, 1, 1), i, g_texPauseLabels.wc, g_texPauseLabels.hc);
//
//		if (selected)
//		{
//			XMFLOAT2 cpos{ pos.x - btnSize.x * 0.62f, pos.y };
//			XMFLOAT2 csize{ btnSize.y * 0.55f, btnSize.y * 0.55f };
//			DrawCursor(g_texCursor, cpos, csize, XMFLOAT4(1, 7, 1, 1));
//		}
//	}
//}

UI_PAUSE_RESULT UI_Pause_GetResult()
{
	return g_pauseResult;
}

void UI_Pause_ConsumeResult()
{
	if (g_pauseResult == UI_PAUSE_RESUME)
		UI_Pause_Close();
	else
		g_pauseResult = UI_PAUSE_NONE;
}
