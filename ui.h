#pragma once
#ifndef UI_H  
#define UI_H  

// ëºÇÃUIä÷òAä÷êîÇ‚íËã`Ç™Ç±Ç±Ç…ä‹Ç‹ÇÍÇÈÇ∆âºíËÇµÇ‹Ç∑  

// UI_IsMenuActiveä÷êîÇÃêÈåæ  

#endif // UI_H
// ui.h


#include <d3d11.h>
#include <DirectXMath.h>

//--------------------------------------------------------------------------------------
// UI result codes
//--------------------------------------------------------------------------------------
enum UI_TITLE_RESULT
{
    UI_TITLE_NONE = 0,
    UI_TITLE_START,
    UI_TITLE_OPTIONS,
    UI_TITLE_EXIT,
};

enum UI_PAUSE_RESULT
{
    UI_PAUSE_ISOPEN,
    UI_PAUSE_NONE = 0,
    UI_PAUSE_RESUME,
    UI_PAUSE_RESTART,
    UI_PAUSE_TO_TITLE,
};

//--------------------------------------------------------------------------------------
// Lifecycle
//--------------------------------------------------------------------------------------
void UI_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void UI_Finalize();

//--------------------------------------------------------------------------------------
// Title menu
//--------------------------------------------------------------------------------------
void UI_Title_Reset();
void UI_Title_Update();
void UI_Title_Draw();
UI_TITLE_RESULT UI_Title_GetResult();
void UI_Title_ConsumeResult();

//--------------------------------------------------------------------------------------
// Pause menu (for SCENE_GAME)
//--------------------------------------------------------------------------------------
void UI_Pause_Reset();
void UI_Pause_Open();
void UI_Pause_Close();
bool UI_Pause_IsOpen();
void UI_Pause_Update();
void UI_Pause_Draw();
UI_PAUSE_RESULT UI_Pause_GetResult();
void UI_Pause_ConsumeResult();
