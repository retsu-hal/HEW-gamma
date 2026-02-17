#pragma once

#include "imgui.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"

extern bool g_DebugMode;

#ifdef _DEBUG
    #define DEBUG_IMGUI_BEGIN(block) \
            do { if (g_DebugMode) { block } } while(0)
#else
    #define DEBUG_IMGUI_BEGIN(block) ((void)0)
#endif


