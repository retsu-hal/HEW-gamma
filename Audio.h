//Audio.h
#pragma once
#include<xaudio2.h>

// 初期化・終了処理
void InitAudio();
void UninitAudio();
void UpdateAudio();

// 音楽ファイル操作
int LoadAudio(const char* FileName);
void UnloadAudio(int Index);

// 再生制御
void PlayAudio(int Index, bool Loop = false);

// ボリューム制御
void SetMasterVolume(float Volume);
void SetAudioVolume(int Index, float Volume);

// フェード制御
void FadeInAudio(int Index, float Duration, float TargetVolume = 1.0f);
void FadeOutAudio(int Index, float Duration);
void FadeOutAndStopAudio(int Index, float Duration);  // フェードアウト後に停止
void PlayAudioWithFadeIn(int Index, bool Loop, float Duration, float TargetVolume = 1.0f);