//メモ用
/*
* 
* XInputによるコントローラー入力メモ
* 基本はXBox360コントローラーを基準にされている
* 
* Switchプロコントローラーの場合
* 
* Switch		XInput
* 
*	A			XINPUT_GAMEPAD_B
*	B			XINPUT_GAMEPAD_A
*	X			XINPUT_GAMEPAD_Y
*	Y			XINPUT_GAMEPAD_X
*
*	L			XINPUT_GAMEPAD_LEFT_SHOULDER
*	R			XINPUT_GAMEPAD_RIGHT_SHOULDER
*	ZL			GetLeftTrigger()
*	ZR			GetRightTrigger()
* 
*	＋			XINPUT_GAMEPAD_START
*	－			XINPUT_GAMEPAD_BACK
* 
* Lスティック押し込み	XINPUT_GAMEPAD_LEFT_THUMB
* Rスティック押し込み	XINPUT_GAMEPAD_RIGHT_THUMB
* 
* 十字キー 上	XINPUT_GAMEPAD_DPAD_UP
* 十字キー 下	XINPUT_GAMEPAD_DPAD_DOWN
* 十字キー 左	XINPUT_GAMEPAD_DPAD_LEFT
* 十字キー 右	XINPUT_GAMEPAD_DPAD_RIGHT
* 
*/


/*
 animation 利用方法

 ModelDraw(Model)　→　の前に
 {
　ModelUpdateAnimation(Model, 10.0f / 600.0f);   // モデル　、deltatime
　Shader_SetBones(Model);	// upload bones
 }

　ModelDraw(g_Player3D.Model);

*/