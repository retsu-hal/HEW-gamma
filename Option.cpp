// Option.cpp
#include "Option.h"
#include "sprite.h"
#include "keyboard.h"
#include "shader.h"
#include "Input.h"
#include "debug.h"
#include "fade.h"
#include "mouse.h"
#include "Audio.h"
#include "camera.h"
#include "Collision.h"
#include "newKeyBind.h"

static bool debugMode = TRUE;

static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

// テクスチャ
static ID3D11ShaderResourceView* g_BgTexture = nullptr;
static ID3D11ShaderResourceView* g_BackTitleTexture = nullptr;
static ID3D11ShaderResourceView* g_ExplanTexture = nullptr;
static ID3D11ShaderResourceView* g_ResetTexture = nullptr;
static ID3D11ShaderResourceView* g_BarTexture[3] = { nullptr };
static ID3D11ShaderResourceView* g_PadTexture = nullptr ;
static ID3D11ShaderResourceView* g_BlackTexture = nullptr;



static ID3D11ShaderResourceView* g_SelectBackTitleTexture = nullptr;
static ID3D11ShaderResourceView* g_SelectExplanTexture = nullptr;
static ID3D11ShaderResourceView* g_SelectResetTexture = nullptr;
static ID3D11ShaderResourceView* g_SelectBarTexture[3] = { nullptr };



//選択されているオプション
static int g_SelectedOption = OPTION_SELECT_NONE;


static float g_BarPos[3] = { 0.0f };

static float g_MasterVolume = 0.0f;
static float g_BgmVolume = 0.0f;

static float g_MouseSensYaw = 0.0f;
static float g_MouseSensPitch = 0.0f;

float g_mouseX = 0;
float g_mouseY = 0;

static bool g_ShowExplan = false;

OptionRect g_OptionRect[OPTION_SELECT_MAX] = {};
XMFLOAT2 g_OptionRectPos[OPTION_SELECT_MAX] ;



void Option_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    g_pDevice = pDevice;
    g_pContext = pContext;

    g_MasterVolume = 50.0f;
    g_BgmVolume = 50.0f;

    g_SelectedOption = OPTION_SELECT_NONE;


    g_MouseSensYaw = GetMouseSensYaw();
    g_MouseSensPitch = GetMouseSensPitch();

	// オプションの位置を設定(左上)
    g_OptionRectPos[OPTION_SELECT_VOLUME] = XMFLOAT2(580.0f, 330.0f);
    g_OptionRectPos[OPTION_SELECT_BGM] = XMFLOAT2(580.0f, 480.0f);
    g_OptionRectPos[OPTION_SELECT_MOUSE] = XMFLOAT2(580.0f, 630.0f);
    g_OptionRectPos[OPTION_SELECT_EXPLAN] = XMFLOAT2(830.0f, 930.0f);
    g_OptionRectPos[OPTION_SELECT_RESET] = XMFLOAT2(95.0f, 930.0f);
    g_OptionRectPos[OPTION_SELECT_BACK] = XMFLOAT2(1280.0f, 930.0f);

    // オプションの矩形を設定
    g_OptionRect[OPTION_SELECT_VOLUME] = { g_OptionRectPos[OPTION_SELECT_VOLUME].x,g_OptionRectPos[OPTION_SELECT_VOLUME].y,760.0f,110.0f };
    g_OptionRect[OPTION_SELECT_BGM] = { g_OptionRectPos[OPTION_SELECT_BGM].x,g_OptionRectPos[OPTION_SELECT_BGM].y, 760.0f,110.0f };
    g_OptionRect[OPTION_SELECT_MOUSE] = { g_OptionRectPos[OPTION_SELECT_MOUSE].x,g_OptionRectPos[OPTION_SELECT_MOUSE].y, 760.0f,110.0f };
    g_OptionRect[OPTION_SELECT_EXPLAN] = { g_OptionRectPos[OPTION_SELECT_EXPLAN].x,g_OptionRectPos[OPTION_SELECT_EXPLAN].y, 230.0f,1340.0f };
    g_OptionRect[OPTION_SELECT_RESET] = { g_OptionRectPos[OPTION_SELECT_RESET].x,g_OptionRectPos[OPTION_SELECT_RESET].y, 540.0f,1340.0f };
    g_OptionRect[OPTION_SELECT_BACK] = { g_OptionRectPos[OPTION_SELECT_BACK].x,g_OptionRectPos[OPTION_SELECT_BACK].y,540.0f,1340.0 };


    //オプションの初期値を画面中央に設定+
    for (int i = 0; i < 3; i++)
    {
        g_BarPos[i] = (float)Direct3D_GetBackBufferWidth() / 2;
    }

    //テクスチャ読み込みなど
    TexMetadata		metadata;
    ScratchImage	image;
    LoadFromWICFile(L"asset\\texture\\UI\\Option\\Option.png", WIC_FLAGS_NONE, &metadata, image);
    CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_BgTexture);
    assert(g_BgTexture);//読み込み失敗時にダイアログを表示

    LoadFromWICFile(L"asset\\texture\\UI\\Option\\setumei.png", WIC_FLAGS_NONE, &metadata, image);
    CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_ExplanTexture);
    assert(g_ExplanTexture);//読み込み失敗時にダイアログを表示

    LoadFromWICFile(L"asset\\texture\\UI\\Option\\Selectsetumei.png", WIC_FLAGS_NONE, &metadata, image);
    CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_SelectExplanTexture);
    assert(g_SelectExplanTexture);//読み込み失敗時にダイアログを表示

    LoadFromWICFile(L"asset\\texture\\UI\\Option\\BackTitle.png", WIC_FLAGS_NONE, &metadata, image);
    CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_BackTitleTexture);
    assert(g_BackTitleTexture);//読み込み失敗時にダイアログを表示

    LoadFromWICFile(L"asset\\texture\\UI\\Option\\Default.png", WIC_FLAGS_NONE, &metadata, image);
    CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_ResetTexture);
    assert(g_ResetTexture);//読み込み失敗時にダイアログを表示

    LoadFromWICFile(L"asset\\texture\\UI\\Option\\pad.png", WIC_FLAGS_NONE, &metadata, image);
    CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_PadTexture);
    assert(g_ResetTexture);//読み込み失敗時にダイアログを表示

    LoadFromWICFile(L"asset\\texture\\UI\\Option\\Bg.png", WIC_FLAGS_NONE, &metadata, image);
    CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_BlackTexture);
    assert(g_ResetTexture);//読み込み失敗時にダイアログを表示

    for (int i = 0; i < 3; i++)
    {
        LoadFromWICFile(L"asset\\texture\\UI\\Option\\Bar.png", WIC_FLAGS_NONE, &metadata, image);
        CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_BarTexture[i]);
        assert(g_BarTexture[i]);//読み込み失敗時にダイアログを表示
    }

    for (int i = 0; i < 3; i++)
    {
        LoadFromWICFile(L"asset\\texture\\UI\\Option\\SelectBar.png", WIC_FLAGS_NONE, &metadata, image);
        CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_SelectBarTexture[i]);
        assert(g_SelectBarTexture[i]);//読み込み失敗時にダイアログを表示
    }

   

    LoadFromWICFile(L"asset\\texture\\UI\\Option\\SelectDefault.png", WIC_FLAGS_NONE, &metadata, image);
    CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_SelectResetTexture);
    assert(g_SelectResetTexture);//読み込み失敗時にダイアログを表示

    LoadFromWICFile(L"asset\\texture\\UI\\Option\\SelectBackTitle.png", WIC_FLAGS_NONE, &metadata, image);
    CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_SelectBackTitleTexture);
    assert(g_SelectBackTitleTexture);//読み込み失敗時にダイアログを表示

    // マウスはオプション画面では絶対座標モードにする
    Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
}
    

void Option_Finalize()
{
    SAFE_RELEASE(g_BgTexture);
	SAFE_RELEASE(g_ExplanTexture);
	SAFE_RELEASE(g_BackTitleTexture);
	SAFE_RELEASE(g_ResetTexture);
	SAFE_RELEASE(g_PadTexture);
	SAFE_RELEASE(g_BlackTexture);

    if (!g_BarTexture)
    {
        for (int i = 0; i < 3; i++)
        {
            SAFE_RELEASE(g_BarTexture[i]);
		}
    }

    if (!g_SelectBarTexture)
    {
        for (int i = 0; i < 3; i++)
        {
            SAFE_RELEASE(g_SelectBarTexture[i]);
        }
    }

	SAFE_RELEASE(g_SelectExplanTexture);
    SAFE_RELEASE(g_SelectResetTexture);
	SAFE_RELEASE(g_SelectBackTitleTexture);
}

void Option_Update()
{
    Mouse_State ms{};
    Mouse_GetState(&ms);

    g_mouseX = ms.x;
    g_mouseY = ms.y;
    int debug = 0;

    const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth();
    const float SCREEN_HEIGHT = (float)Direct3D_GetBackBufferHeight();

    // 音量バーのX座標を中央に初期化
    const float minX = 580.0f;
    const float maxX = 1340.0f;
    
    static bool s_prevLeft = false;
    bool leftPressed = (ms.leftButton && !s_prevLeft);
    
    
    
    // 説明画面表示中：どこでも左クリックで戻る
    if (g_ShowExplan)
    {
        if (leftPressed|| IsInputTrigger(JumpKey,gPad))
        {
            g_ShowExplan = false;
        }

        s_prevLeft = ms.leftButton;
        return;
    }

    // マウスカーソルと重なったオプションを選択状態にする

    for (int i = 0; i < OPTION_SELECT_MAX; ++i)
    {
        if (g_OptionRect[i].contains(g_mouseX, g_mouseY))
        {
            g_SelectedOption = i;
            debug = i;
            if (ms.leftButton)
            {
                switch (g_SelectedOption)
                {
                case OPTION_SELECT_VOLUME:
                    // 押し続けている間、バー位置を更新
                    g_BarPos[g_SelectedOption] = g_mouseX;

                    if (g_BarPos[g_SelectedOption] < minX)
                    {
                        g_BarPos[g_SelectedOption] = minX;
                    }
                    else if (g_BarPos[g_SelectedOption] > maxX)
                    {
                        g_BarPos[g_SelectedOption] = maxX;
                    }
                    g_MasterVolume = (g_BarPos[g_SelectedOption] - minX) / (maxX - minX)*100.0f;

					SetMasterVolume(g_MasterVolume);
                    break;
                case OPTION_SELECT_BGM:
                    g_BarPos[g_SelectedOption] = g_mouseX;

                    if (g_BarPos[g_SelectedOption] < minX)
                    {
                        g_BarPos[g_SelectedOption] = minX;
                    }
                    else if (g_BarPos[g_SelectedOption] > maxX)
                    {
                        g_BarPos[g_SelectedOption] = maxX;
                    }

					g_BgmVolume = (g_BarPos[g_SelectedOption] - minX) / (maxX - minX) * 100.0f;

					SetAudioVolume(0, g_BgmVolume / 100.0f); // BGMの音量を設定
                    break;
                case OPTION_SELECT_MOUSE:
                    g_BarPos[g_SelectedOption] = g_mouseX;

                    if (g_BarPos[g_SelectedOption] < minX)
                    {
                        g_BarPos[g_SelectedOption] = minX;
                    }
                    else if (g_BarPos[g_SelectedOption] > maxX)
                    {
                        g_BarPos[g_SelectedOption] = maxX;
                    }

                    g_MouseSensYaw = (g_BarPos[g_SelectedOption] - minX) / (maxX - minX);
					g_MouseSensPitch = (g_BarPos[g_SelectedOption] - minX) / (maxX - minX);
                    
					SetCameraMouseSensitivity(g_MouseSensYaw, g_MouseSensPitch);
                    break;
                case OPTION_SELECT_EXPLAN:
                    g_ShowExplan = true;
                    break;
                case OPTION_SELECT_RESET:
                    Option_Initialize(g_pDevice, g_pContext);
                    break;
                case OPTION_SELECT_BACK:
                    XMFLOAT4 color(0.0f, 0.0f, 0.0f, 1.0f);
                    SetFade(40.0f, color, FADE_OUT, SCENE_TITLE);
                    break;
                }
            }
        }

        
		s_prevLeft = ms.leftButton;
    }
}
void Option_Draw()
{
    if(g_ShowExplan)
    {
        Explan_draw();
		return; // Option画面を描かない（上書き防止）
    }

    Shader_Begin();

    const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth();
    const float SCREEN_HEIGHT = (float)Direct3D_GetBackBufferHeight();

    Shader_SetMatrix(XMMatrixOrthographicOffCenterLH(
        0.0f,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        0.0f,
        0.0f,
        1.0f));
    
    // 背景描画
    {
        g_pContext->PSSetShaderResources(0, 1, &g_BgTexture);
        SetBlendState(BLENDSTATE_ALFA);

        XMFLOAT2 Bgpos = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };
        XMFLOAT2 Bgsize = { SCREEN_WIDTH, SCREEN_HEIGHT };
        XMFLOAT4 Bgcol = { 1.0f, 1.0f, 1.0f, 1.0f };
        DrawSprite(Bgpos, Bgsize, Bgcol);
    }
    
    // KeyPad描画
    {
        if (g_SelectedOption == OPTION_SELECT_EXPLAN || g_OptionRect[OPTION_SELECT_EXPLAN].contains(g_mouseX, g_mouseY))
        {
            g_pContext->PSSetShaderResources(0, 1, &g_SelectExplanTexture);
            SetBlendState(BLENDSTATE_ALFA);

            XMFLOAT2 KeyPadpos = { SCREEN_WIDTH / 2, (SCREEN_HEIGHT / 2 + 450.0f) };
            XMFLOAT2 KeyPadsize = { SCREEN_WIDTH, SCREEN_HEIGHT };
            XMFLOAT4 KeyPadcol = { 1.0f, 1.0f, 1.0f, 1.0f };
            DrawSprite(KeyPadpos, KeyPadsize, KeyPadcol);
        }
        else
        {
            g_pContext->PSSetShaderResources(0, 1, &g_ExplanTexture);
            SetBlendState(BLENDSTATE_ALFA);

            XMFLOAT2 KeyPadpos = { SCREEN_WIDTH / 2, (SCREEN_HEIGHT / 2 + 450.0f) };
            XMFLOAT2 KeyPadsize = { SCREEN_WIDTH, SCREEN_HEIGHT };
            XMFLOAT4 KeyPadcol = { 1.0f, 1.0f, 1.0f, 1.0f };
            DrawSprite(KeyPadpos, KeyPadsize, KeyPadcol);
        }  
    }

    //デフォルト描画
    {
        if (g_SelectedOption == OPTION_SELECT_RESET || g_OptionRect[OPTION_SELECT_RESET].contains(g_mouseX, g_mouseY))
        {
            g_pContext->PSSetShaderResources(0, 1, &g_SelectResetTexture);
            SetBlendState(BLENDSTATE_ALFA);

            XMFLOAT2 Explainpos = { (SCREEN_WIDTH / 2 - 600), (SCREEN_HEIGHT / 2 + 450) };
            XMFLOAT2 Explainsize = { SCREEN_WIDTH, SCREEN_HEIGHT };
            XMFLOAT4 Explaincol = { 1.0f, 1.0f, 1.0f, 1.0f };
            DrawSprite(Explainpos, Explainsize, Explaincol);
        }
        else
        {
            g_pContext->PSSetShaderResources(0, 1, &g_ResetTexture);
            SetBlendState(BLENDSTATE_ALFA);

            XMFLOAT2 Explainpos = { (SCREEN_WIDTH / 2 - 600), (SCREEN_HEIGHT / 2 + 450) };
            XMFLOAT2 Explainsize = { SCREEN_WIDTH, SCREEN_HEIGHT };
            XMFLOAT4 Explaincol = { 1.0f, 1.0f, 1.0f, 1.0f };
            DrawSprite(Explainpos, Explainsize, Explaincol);
        }
    }
        
    // BackTitle描画
    {
        if (g_SelectedOption == OPTION_SELECT_BACK || g_OptionRect[OPTION_SELECT_BACK].contains(g_mouseX, g_mouseY))
        {
            g_pContext->PSSetShaderResources(0, 1, &g_SelectBackTitleTexture);
            SetBlendState(BLENDSTATE_ALFA);
            XMFLOAT2 BackTitlepos = { (SCREEN_WIDTH / 2 + 600), (SCREEN_HEIGHT / 2 + 450) };
            XMFLOAT2 BackTitlesize = { SCREEN_WIDTH, SCREEN_HEIGHT };
            XMFLOAT4 BackTitlecol = { 1.0f, 1.0f, 1.0f, 1.0f };
            DrawSprite(BackTitlepos, BackTitlesize, BackTitlecol);
        }
        else
        {
            g_pContext->PSSetShaderResources(0, 1, &g_BackTitleTexture);
            SetBlendState(BLENDSTATE_ALFA);

            XMFLOAT2 BackTitlepos = { (SCREEN_WIDTH / 2 + 600), (SCREEN_HEIGHT / 2 + 450) };
            XMFLOAT2 BackTitlesize = { SCREEN_WIDTH, SCREEN_HEIGHT };
            XMFLOAT4 BackTitlecol = { 1.0f, 1.0f, 1.0f, 1.0f };
            DrawSprite(BackTitlepos, BackTitlesize, BackTitlecol);
        }
    }
    
    // オプションバー描画(マスターボリューム)
    {
        if (g_SelectedOption == OPTION_SELECT_VOLUME || g_OptionRect[OPTION_SELECT_VOLUME].contains(g_mouseX, g_mouseY))
        {
            g_pContext->PSSetShaderResources(0, 1, &g_SelectBarTexture[OPTION_SELECT_VOLUME]);
            SetBlendState(BLENDSTATE_ALFA);

            XMFLOAT2 Barpos = { g_BarPos[OPTION_SELECT_VOLUME] , (SCREEN_HEIGHT / 2 - 150) };
            XMFLOAT2 Barsize = { SCREEN_WIDTH, SCREEN_HEIGHT };
            XMFLOAT4 Barcol = { 1.0f, 1.0f, 1.0f, 1.0f };
            DrawSprite(Barpos, Barsize, Barcol);
        }
        else
        {
            g_pContext->PSSetShaderResources(0, 1, &g_BarTexture[OPTION_SELECT_VOLUME]);
            SetBlendState(BLENDSTATE_ALFA);
            XMFLOAT2 Barpos = { g_BarPos[OPTION_SELECT_VOLUME], (SCREEN_HEIGHT / 2 - 150) };
            XMFLOAT2 Barsize = { SCREEN_WIDTH, SCREEN_HEIGHT };
            XMFLOAT4 Barcol = { 1.0f, 1.0f, 1.0f, 1.0f };
            DrawSprite(Barpos, Barsize, Barcol);
        }
    }
        
	// オプションバー描画(BGMボリューム)
    {
        if (g_SelectedOption == OPTION_SELECT_BGM || g_OptionRect[OPTION_SELECT_BGM].contains(g_mouseX, g_mouseY))
        {
            g_pContext->PSSetShaderResources(0, 1, &g_SelectBarTexture[OPTION_SELECT_BGM]);
            SetBlendState(BLENDSTATE_ALFA);
            XMFLOAT2 Barpos = { g_BarPos[OPTION_SELECT_BGM], (SCREEN_HEIGHT / 2) };
            XMFLOAT2 Barsize = { SCREEN_WIDTH, SCREEN_HEIGHT };
            XMFLOAT4 Barcol = { 1.0f, 1.0f, 1.0f, 1.0f };
            DrawSprite(Barpos, Barsize, Barcol);
        }
        else
        {
            g_pContext->PSSetShaderResources(0, 1, &g_BarTexture[OPTION_SELECT_BGM]);
            SetBlendState(BLENDSTATE_ALFA);
            XMFLOAT2 Barpos = { g_BarPos[OPTION_SELECT_BGM], (SCREEN_HEIGHT / 2) };
            XMFLOAT2 Barsize = { SCREEN_WIDTH, SCREEN_HEIGHT };
            XMFLOAT4 Barcol = { 1.0f, 1.0f, 1.0f, 1.0f };
            DrawSprite(Barpos, Barsize, Barcol);
        }
    }

	// オプションバー描画(マウス感度)
    {
        if (g_SelectedOption == OPTION_SELECT_MOUSE || g_OptionRect[OPTION_SELECT_MOUSE].contains(g_mouseX, g_mouseY))
        {
            g_pContext->PSSetShaderResources(0, 1, &g_SelectBarTexture[OPTION_SELECT_MOUSE]);
            SetBlendState(BLENDSTATE_ALFA);
            XMFLOAT2 Barpos = { g_BarPos[OPTION_SELECT_MOUSE], (SCREEN_HEIGHT / 2 + 150) };
            XMFLOAT2 Barsize = { SCREEN_WIDTH, SCREEN_HEIGHT };
            XMFLOAT4 Barcol = { 1.0f, 1.0f, 1.0f, 1.0f };
            DrawSprite(Barpos, Barsize, Barcol);
        }
        else
        {
            g_pContext->PSSetShaderResources(0, 1, &g_BarTexture[OPTION_SELECT_MOUSE]);
            SetBlendState(BLENDSTATE_ALFA);
            XMFLOAT2 Barpos = { g_BarPos[OPTION_SELECT_MOUSE], (SCREEN_HEIGHT / 2 + 150) };
            XMFLOAT2 Barsize = { SCREEN_WIDTH, SCREEN_HEIGHT };
            XMFLOAT4 Barcol = { 1.0f, 1.0f, 1.0f, 1.0f };
            DrawSprite(Barpos, Barsize, Barcol);
        }

    }

}    

void Option_Reset()
{
    g_MasterVolume = 1.0f;
    g_BgmVolume = 1.0f;

    g_MouseSensYaw = GetMouseSensYaw();
    g_MouseSensPitch = GetMouseSensPitch();
}

void Explan_draw()
{
    Shader_Begin();

    const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth();
    const float SCREEN_HEIGHT = (float)Direct3D_GetBackBufferHeight();

    Shader_SetMatrix(XMMatrixOrthographicOffCenterLH(
        0.0f,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        0.0f,
        0.0f,
        1.0f));

    g_pContext->PSSetShaderResources(0, 1, &g_BlackTexture);
    SetBlendState(BLENDSTATE_ALFA);

    XMFLOAT2 Bgpos = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };
    XMFLOAT2 Bglainsize = { SCREEN_WIDTH, SCREEN_HEIGHT };
    XMFLOAT4 Bglaincol = { 1.0f, 1.0f, 1.0f, 1.0f };
    DrawSprite(Bgpos, Bglainsize, Bglaincol);

    g_pContext->PSSetShaderResources(0, 1, &g_PadTexture);
    SetBlendState(BLENDSTATE_ALFA);
   
    XMFLOAT2 Explainpos = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2  };
    XMFLOAT2 Explainsize = { SCREEN_WIDTH, SCREEN_HEIGHT };
    XMFLOAT4 Explaincol = { 1.0f, 1.0f, 1.0f, 1.0f };
    DrawSprite(Explainpos, Explainsize, Explaincol);
}