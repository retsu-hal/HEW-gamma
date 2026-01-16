#pragma once
#include <xaudio2.h>

//=========================================================================================================
//プロトタイプ宣言
//=========================================================================================================
void InitAudio();		//サウンドの初期化
void UninitAudio();		//サウンドの終了
int LoadAudio(const char* FileName);		//サウンドデータ読み込み
void UnloadAudio(int Index);				//サウンドデータ解放（停止）
void PlayAudio(int Index, bool Loop = false);//サウンドデータ再生

void SetAudioVolume(int Index, float Volume);   // Individual sound volume
void SetMasterVolume(float Volume);

/*
//以下の関数はプログラムの最初と最後に1回ずつ呼び出せばOK
void InitAudio();		//サウンドの初期化
void UninitAudio();		//サウンドの終了


//以下はシーンごとの処理

//グローバル変数
static int g_BgmID = NULL;//ロードするデータの数だけ変数が必要

//初期化時
g_BgmID = LoadAudio("asset\\Audio\\title.wav");	//サウンドデータ読み込み
PlayAudio(g_BgmID, true);	//サウンドデータ再生(ループあり)

//終了時
UnloadAudio(g_BgmID);		//サウンドデータ解放（停止）


*/




