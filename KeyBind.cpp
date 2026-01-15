//KeyBind.cpp
#include "KeyBind.h"
#include "Controller.h" // 必要な定義をインクルード

InputKey UpKey =		{ KK_W,     XINPUT_GAMEPAD_DPAD_UP };	//前進
InputKey RightKey =		{ KK_D,     XINPUT_GAMEPAD_DPAD_RIGHT };//右移動
InputKey DownKey =		{ KK_S,     XINPUT_GAMEPAD_DPAD_DOWN };	//後退
InputKey LeftKey =		{ KK_A,     XINPUT_GAMEPAD_DPAD_LEFT };	//左移動

InputKey JumpKey =		{ KK_SPACE, XINPUT_GAMEPAD_A };			//ジャンプ
InputKey ActionKey =	{ KK_F,     XINPUT_GAMEPAD_B };			//アクション
InputKey ChangeKey =	{ KK_F,     XINPUT_GAMEPAD_B };			//影変身

InputKey ResetKey =		{ KK_R,     XINPUT_GAMEPAD_BACK };		//リセット
InputKey MenuKey =		{ KK_ESCAPE,XINPUT_GAMEPAD_START };		//メニュー

// 関数本体もここに置く（以前ヘッダーに書いていた場合は移動）
bool IsInputDown(const InputKey& key, const Controller& pad)
{
	// キーボード
	if (key.keyboard != KK_NONE)
	{
		if (Keyboard_IsKeyDown(key.keyboard))
			return true;
	}

	// コントローラー
	if (key.gamepad != 0 && pad.IsConnected())
	{
		if (pad.IsButtonPressed(key.gamepad))
			return true;
	}

	return false;
}