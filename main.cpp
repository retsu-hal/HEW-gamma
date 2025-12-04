
//ウィンドウの表示
#include <SDKDDKVer.h>	//利用できる最も上位の Windows プラットフォームが定義される
#define WIN32_LEAN_AND_MEAN	//32bitアプリには不要な情報を抑止してコンパイル時間を短縮
#include	<windows.h>
#include	"debug_ostream.h"	//デバッグ表示
#include <algorithm>			
#include "direct3d.h"			
#include "shader.h"
#include "polygon.h"
#include "field.h"
#include "sprite.h"
#include "keyboard.h"
#include "Player3D.h"
#include "Effect.h"
#include "score.h"
#include "Manager.h"
#include "Audio.h"	
#include "Mouse.h"

#include "debug.h"

//==================================
//グローバル変数
//==================================
#ifdef _DEBUG	//デバッグビルドの時だけ変数が作られる
int		g_CountFPS;			//FPSカウンター
char	g_DebugStr[2048];	//FPS表示文字列
#endif
#pragma comment(lib, "winmm.lib")

//=================================
//マクロ定義
//=================================
#define		CLASS_NAME	"DX21 Window"
#define		WINDOW_CAPTION	"ポリゴン描画"
#define		SCREEN_WIDTH	(1280)
#define		SCREEN_HEIGHT	(720)

//===================================
//プロトタイプ宣言
//===================================

// ImGui用ウィンドウプロシージャハンドラ
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


//ウィンドウプロシージャ
//コールバック関数＝＞他人が呼び出してくれる関数
LRESULT	CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int APIENTRY WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance, LPSTR lpCmd, int nCmdShow)
{

	//乱数の初期化
	srand(timeGetTime());

	//フレームレート計測用変数
	DWORD	dwExecLastTime;
	DWORD	dwFPSLastTime;
	DWORD	dwCurrentTime;
	DWORD	dwFrameCount;

	HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);


	//ウィンドウクラスの登録（ウィンドウの仕様的な物を決めてWindowsへセットする）
	WNDCLASS	wc;	//構造体を準備
	ZeroMemory(&wc, sizeof(WNDCLASS));//内容を０で初期化
	wc.lpfnWndProc = WndProc;	//コールバック関数のポインター
	wc.lpszClassName = CLASS_NAME;	//この仕様書の名前
	wc.hInstance = hInstance;	//このアプリケーションのこと
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);//カーソルの種類
	wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);//ウィンドウの背景色は黒
	RegisterClass(&wc);	//構造体をWindowsへセット

	//クライアント領域のサイズを表す矩形 (左からleft, top, right, bottom)
	RECT window_rect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
	//ウィンドウのスタイル（ウィンドウ枠と最大化ボタンを削除）
	DWORD window_style = WS_OVERLAPPEDWINDOW;
	//DWORD window_style = WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX);
	//指定したクライアント領域を確保するために新たな矩形座標を計算
	AdjustWindowRect(&window_rect, window_style, FALSE);
	//調整された矩形の横と縦のサイズを計算
	int window_width = window_rect.right - window_rect.left;
	int window_height = window_rect.bottom - window_rect.top;

	//ウィンドウの作成
	HWND	hWnd = CreateWindow(
		CLASS_NAME,
		WINDOW_CAPTION,
		window_style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		window_width,
		window_height,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	//作成したウィンドウを表示する
	ShowWindow(hWnd, nCmdShow);//引数に従って表示、または非表示
	//ウィンドウ内部の更新要求
	UpdateWindow(hWnd);


	Direct3D_Initialize(hWnd);
	Mouse_Initialize(hWnd);
	Keyboard_Initialize();

	Shader_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext()); // シェーダの初期化
	InitializeSprite();//スプライトの初期化

	InitAudio();	//サウンドの初期化

	{// ImGui 初期化
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();

		// io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		// 日本語フォント設定
		io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/msyh.ttc", 18.0f, nullptr,
			io.Fonts->GetGlyphRangesChineseFull());


		// ImGui 用の DirectX11 と Win32 の初期化
		ImGui_ImplWin32_Init(hWnd);
		ImGui_ImplDX11_Init(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
	}

	Manager_Initialize();


	/////////////////////////////////

	//メッセージループ
	MSG	msg;
	ZeroMemory(&msg, sizeof(MSG));

	//フレームレート計測初期化
	timeBeginPeriod(1);	//タイマーの精度を設定
	dwExecLastTime = dwFPSLastTime = timeGetTime();//現在のタイマー値
	dwCurrentTime = dwFrameCount = 0;

	//終了メッセージが来るまでループする
	do {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{ // ウィンドウメッセージが来ていたら
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{ // ゲームの処理
			dwCurrentTime = timeGetTime();
			if ((dwCurrentTime - dwFPSLastTime) >= 1000)
			{
#ifdef _DEBUG
				g_CountFPS = dwFrameCount;
#endif
				dwFPSLastTime = dwCurrentTime;//現在のタイマー値保存
				dwFrameCount = 0;
			}

			if ((dwCurrentTime - dwExecLastTime) >= ((float)1000 / 60))
			{// 1/60s 経過した
				dwExecLastTime = dwCurrentTime;//現在のタイマーと保存
#ifdef _DEBUG
				//ウィンドウキャプションへ現在のFPSを表示
				wsprintf(g_DebugStr, "GAMMA ");
				wsprintf(&g_DebugStr[strlen(g_DebugStr)],
					" FPS : %d", g_CountFPS);
				SetWindowText(hWnd, g_DebugStr);
#endif

				ImGui_ImplWin32_NewFrame();
				ImGui_ImplDX11_NewFrame();
				ImGui::NewFrame();

				//更新処理
				Manager_Update();

				//描画処理
				Direct3D_Clear();
				Manager_Draw();

				ImGui::Render();
				ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

				{
					ImGuiIO& io = ImGui::GetIO();
					if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
					{
						ImGui::UpdatePlatformWindows();
						ImGui::RenderPlatformWindowsDefault();
					}
				}

				Direct3D_Present();
				keycopy();
				Mouse_ResetScrollWheelValue();

				dwFrameCount++;		//処理回数更新
			}

		}
	} while (msg.message != WM_QUIT);

	Manager_Finalize();

	Mouse_Finalize();
	UninitAudio();		//サウンドの終了

	Shader_Finalize(); // シェーダの終了処理
	FinalizeSprite();	//スプライトの終了処理
	/////////////////////////////////////////
	Direct3D_Finalize();


	//終了する
	return (int)msg.wParam;

}

//=========================================
//ウィンドウプロシージャ
// メッセージループ内で呼び出される
//=========================================
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HGDIOBJ hbrWhite, hbrGray;

	HDC		hdc;	//ウィンドウ画面を表す情報（デバイスコンテキスト 入出力先）
	PAINTSTRUCT	ps;	//ウィンドウ画面の大きさなど描画関連の情報

	if (ImGui::GetCurrentContext() != nullptr) {
		if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
			return 0;
	}

	switch (uMsg)
	{
	case WM_ACTIVATEAPP:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
		Keyboard_ProcessMessage(uMsg, wParam, lParam);
		break;


	case WM_SIZING:
	{
		const float targetAspect = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;

		RECT* rc = reinterpret_cast<RECT*>(lParam);
		int w = rc->right - rc->left;
		int h = rc->bottom - rc->top;

		float currentAspect = (float)w / (float)h;

		switch (wParam)
		{
		case WMSZ_LEFT:
		case WMSZ_RIGHT:
			h = (int)(w / targetAspect + 0.5f);
			if (wParam == WMSZ_LEFT)
				rc->top = rc->bottom - h;
			else
				rc->bottom = rc->top + h;
			break;

		case WMSZ_TOP:
		case WMSZ_BOTTOM:
			w = (int)(h * targetAspect + 0.5f);
			if (wParam == WMSZ_TOP)
				rc->left = rc->right - w;
			else
				rc->right = rc->left + w;
			break;

		default:
			if (currentAspect > targetAspect)
			{
				w = (int)(h * targetAspect + 0.5f);
			}
			else
			{
				h = (int)(w / targetAspect + 0.5f);
			}

			if (wParam == WMSZ_TOPLEFT)
			{
				rc->left = rc->right - w;
				rc->top = rc->bottom - h;
			}
			else if (wParam == WMSZ_TOPRIGHT)
			{
				rc->right = rc->left + w;
				rc->top = rc->bottom - h;
			}
			else if (wParam == WMSZ_BOTTOMLEFT)
			{
				rc->left = rc->right - w;
				rc->bottom = rc->top + h;
			}
			else if (wParam == WMSZ_BOTTOMRIGHT)
			{
				rc->right = rc->left + w;
				rc->bottom = rc->top + h;
			}
			break;
		}

		return TRUE;
	}

	case WM_KEYDOWN:	//キーが押された
		//if (wParam == VK_ESCAPE)//押されたのはESCキー
		//{
		//	//ウィンドウを閉じたいリクエストをWindowsに送る
		//	SendMessage(hWnd, WM_CLOSE, 0, 0);
		//}

		Keyboard_ProcessMessage(uMsg, wParam, lParam);
		break;
	//case WM_CLOSE:	//ウィンドウを閉じなさい命令				
	//	//if (
	//	//	MessageBox(hWnd, "本当に終了してよろしいですか？",
	//	//		"確認", MB_OKCANCEL | MB_DEFBUTTON2) == IDOK
	//	//	)
	//	//{//OKが押されたとき
	//	//	DestroyWindow(hWnd);//終了する手続きをWindowsへリクエスト
	//	//}
	//	//else
	//	//{
	//	//	return 0;	//やっぱり終わらない
	//	//}

	//	//break;
	case WM_DESTROY:	//終了してOKですよ
		PostQuitMessage(0);		//自分のメッセージに０を送る
		break;

	case WM_INPUT:
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEWHEEL:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_MOUSEHOVER:
		Mouse_ProcessMessage(uMsg, wParam, lParam);
		break;
	}

	

	//必用の無いメッセージは適当に処理させて終了
	return DefWindowProc(hWnd, uMsg, wParam, lParam);

}
