//Audio.cpp
#include <d3d11.h>
#include <DirectXMath.h>
#include "direct3d.h"
#include "shader.h"
#include "sprite.h"
#include "keyboard.h"
#include "audio.h"
using namespace DirectX;

//=========================================================================================================
// 構造体定義
//=========================================================================================================
struct AUDIO
{
	IXAudio2SourceVoice* SourceVoice{};
	BYTE* SoundData{};

	int						Length{};
	int						PlayLength{};

	// フェード用パラメータ
	bool					IsFading{};
	bool					IsFadeIn{};
	float					CurrentVolume{};
	float					TargetVolume{};
	float					FadeSpeed{};
};

//=========================================================================================================
// マクロ定義
//=========================================================================================================
#define AUDIO_MAX (100)

//=========================================================================================================
// グローバル変数
//=========================================================================================================
static IXAudio2* g_Xaudio{};
static IXAudio2MasteringVoice* g_MasteringVoice{};
static AUDIO g_Audio[AUDIO_MAX]{};

//=========================================================================================================
// 初期化処理
//=========================================================================================================
void InitAudio()
{
	// XAudio生成
	XAudio2Create(&g_Xaudio, 0);

	// マスタリングボイス生成
	g_Xaudio->CreateMasteringVoice(&g_MasteringVoice);
}

//=========================================================================================================
// 終了処理
//=========================================================================================================
void UninitAudio()
{
	g_MasteringVoice->DestroyVoice();
	g_Xaudio->Release();
}

//=========================================================================================================
// 更新処理
//=========================================================================================================
void UpdateAudio()
{
	for (int i = 0; i < AUDIO_MAX; i++)
	{
		if (g_Audio[i].IsFading && g_Audio[i].SourceVoice)
		{
			if (g_Audio[i].IsFadeIn)
			{
				// フェードイン処理
				g_Audio[i].CurrentVolume += g_Audio[i].FadeSpeed;

				if (g_Audio[i].CurrentVolume >= g_Audio[i].TargetVolume)
				{
					g_Audio[i].CurrentVolume = g_Audio[i].TargetVolume;
					g_Audio[i].IsFading = false;
				}
			}
			else
			{
				// フェードアウト処理
				g_Audio[i].CurrentVolume -= g_Audio[i].FadeSpeed;

				// 停止フラグ（TargetVolume < 0）の場合は0まで下げて停止
				bool shouldStop = (g_Audio[i].TargetVolume < 0.0f);
				float actualTarget = shouldStop ? 0.0f : g_Audio[i].TargetVolume;

				if (g_Audio[i].CurrentVolume <= actualTarget)
				{
					g_Audio[i].CurrentVolume = actualTarget;
					g_Audio[i].IsFading = false;

					// 停止フラグが立っている場合のみ停止
					if (shouldStop)
					{
						g_Audio[i].SourceVoice->Stop();
					}
				}
			}

			// ボリューム適用
			g_Audio[i].SourceVoice->SetVolume(g_Audio[i].CurrentVolume);
		}
	}
}

//=========================================================================================================
// 音楽読み込み
//=========================================================================================================
int LoadAudio(const char* FileName)
{
	int index = -1;

	for (int i = 0; i < AUDIO_MAX; i++)
	{
		if (g_Audio[i].SourceVoice == nullptr)
		{
			index = i;
			break;
		}
	}

	if (index == -1)
		return -1;




	// サウンドデータ読込
	WAVEFORMATEX wfx = { 0 };

	{
		HMMIO hmmio = NULL;
		MMIOINFO mmioinfo = { 0 };
		MMCKINFO riffchunkinfo = { 0 };
		MMCKINFO datachunkinfo = { 0 };
		MMCKINFO mmckinfo = { 0 };
		UINT32 buflen;
		LONG readlen;


		hmmio = mmioOpen((LPSTR)FileName, &mmioinfo, MMIO_READ);
		assert(hmmio);

		riffchunkinfo.fccType = mmioFOURCC('W', 'A', 'V', 'E');
		mmioDescend(hmmio, &riffchunkinfo, NULL, MMIO_FINDRIFF);

		mmckinfo.ckid = mmioFOURCC('f', 'm', 't', ' ');
		mmioDescend(hmmio, &mmckinfo, &riffchunkinfo, MMIO_FINDCHUNK);

		if (mmckinfo.cksize >= sizeof(WAVEFORMATEX))
		{
			mmioRead(hmmio, (HPSTR)&wfx, sizeof(wfx));
		}
		else
		{
			PCMWAVEFORMAT pcmwf = { 0 };
			mmioRead(hmmio, (HPSTR)&pcmwf, sizeof(pcmwf));
			memset(&wfx, 0x00, sizeof(wfx));
			memcpy(&wfx, &pcmwf, sizeof(pcmwf));
			wfx.cbSize = 0;
		}
		mmioAscend(hmmio, &mmckinfo, 0);

		datachunkinfo.ckid = mmioFOURCC('d', 'a', 't', 'a');
		mmioDescend(hmmio, &datachunkinfo, &riffchunkinfo, MMIO_FINDCHUNK);



		buflen = datachunkinfo.cksize;
		g_Audio[index].SoundData = new unsigned char[buflen];
		readlen = mmioRead(hmmio, (HPSTR)g_Audio[index].SoundData, buflen);


		g_Audio[index].Length = readlen;
		g_Audio[index].PlayLength = readlen / wfx.nBlockAlign;


		mmioClose(hmmio, 0);
	}


	// サウンドソース生成
	g_Xaudio->CreateSourceVoice(&g_Audio[index].SourceVoice, &wfx);
	assert(g_Audio[index].SourceVoice);

	// フェードパラメータ初期化
	g_Audio[index].IsFading = false;
	g_Audio[index].IsFadeIn = false;
	g_Audio[index].CurrentVolume = 1.0f;
	g_Audio[index].TargetVolume = 1.0f;
	g_Audio[index].FadeSpeed = 0.0f;

	return index;
}

//=========================================================================================================
// 音楽停止処理
//=========================================================================================================
void UnloadAudio(int Index)
{
	g_Audio[Index].SourceVoice->Stop();
	g_Audio[Index].SourceVoice->DestroyVoice();

	delete[] g_Audio[Index].SoundData;
	g_Audio[Index].SoundData = nullptr;
	g_Audio[Index].SourceVoice = nullptr;
}




//=========================================================================================================
// 音楽再生処理
//=========================================================================================================
void PlayAudio(int Index, bool Loop)
{
	g_Audio[Index].SourceVoice->Stop();
	g_Audio[Index].SourceVoice->FlushSourceBuffers();


	// バッファ設定
	XAUDIO2_BUFFER bufinfo;

	memset(&bufinfo, 0x00, sizeof(bufinfo));
	bufinfo.AudioBytes = g_Audio[Index].Length;
	bufinfo.pAudioData = g_Audio[Index].SoundData;
	bufinfo.PlayBegin = 0;
	bufinfo.PlayLength = g_Audio[Index].PlayLength;

	// ループ設定
	if (Loop)
	{
		bufinfo.LoopBegin = 0;
		bufinfo.LoopLength = g_Audio[Index].PlayLength;
		bufinfo.LoopCount = XAUDIO2_LOOP_INFINITE;
	}

	g_Audio[Index].SourceVoice->SubmitSourceBuffer(&bufinfo, NULL);


	// 再生
	g_Audio[Index].SourceVoice->Start();

}

void SetMasterVolume(float Volume)
{
	if (g_MasteringVoice)
	{
		g_MasteringVoice->SetVolume(Volume);
	}
}

void SetAudioVolume(int Index, float Volume)
{
	if (g_Audio[Index].SourceVoice)
	{
		g_Audio[Index].SourceVoice->SetVolume(Volume);
		g_Audio[Index].CurrentVolume = Volume;
	}
}

//=========================================================================================================
// フェードイン開始
//=========================================================================================================
void FadeInAudio(int Index, float Duration, float TargetVolume)
{
	if (g_Audio[Index].SourceVoice)
	{
		g_Audio[Index].IsFading = true;
		g_Audio[Index].IsFadeIn = true;
		g_Audio[Index].CurrentVolume = 0.0f;
		g_Audio[Index].TargetVolume = TargetVolume;
		g_Audio[Index].FadeSpeed = TargetVolume / (Duration * 60.0f); // 60FPS想定

		// 初期ボリュームを0に設定
		g_Audio[Index].SourceVoice->SetVolume(0.0f);
	}
}

//=========================================================================================================
// フェードアウト開始
//=========================================================================================================
void FadeOutAudio(int Index, float Duration)
{
	if (g_Audio[Index].SourceVoice)
	{
		g_Audio[Index].IsFading = true;
		g_Audio[Index].IsFadeIn = false;
		g_Audio[Index].TargetVolume = 0.0f;
		g_Audio[Index].FadeSpeed = g_Audio[Index].CurrentVolume / (Duration * 60.0f); // 60FPS想定
	}
}

//=========================================================================================================
// フェードアウト＆停止（フェードアウト完了後に再生を停止する）
//=========================================================================================================
void FadeOutAndStopAudio(int Index, float Duration)
{
	if (g_Audio[Index].SourceVoice)
	{
		g_Audio[Index].IsFading = true;
		g_Audio[Index].IsFadeIn = false;
		g_Audio[Index].TargetVolume = -1.0f; // 負の値で停止フラグとして使用
		g_Audio[Index].FadeSpeed = g_Audio[Index].CurrentVolume / (Duration * 60.0f); // 60FPS想定
	}
}

//=========================================================================================================
// フェードイン再生（再生とフェードインを同時実行）
//=========================================================================================================
void PlayAudioWithFadeIn(int Index, bool Loop, float Duration, float TargetVolume)
{
	PlayAudio(Index, Loop);
	FadeInAudio(Index, Duration, TargetVolume);
}
