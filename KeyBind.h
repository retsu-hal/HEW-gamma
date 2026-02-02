// KeyBind.h
#pragma once
#include <Windows.h>
#include <Xinput.h> 

#pragma comment(lib, "xinput.lib")
#include "Keyboard.h"

class Controller;

struct InputKey
{
	Keyboard_Keys keyboard;
	WORD          gamepad;
};

extern InputKey UpKey;
extern InputKey RightKey;
extern InputKey DownKey;
extern InputKey LeftKey;
extern InputKey JumpKey;
extern InputKey DashKey;
extern InputKey ActionKey;
extern InputKey ChangeKey;
extern InputKey ResetKey;
extern InputKey MenuKey;

bool IsInputTrigger(const InputKey& key, const Controller& controller);	//‰Ÿ‚³‚ê‚½Žž

bool IsInputPress(const InputKey& key, const Controller& controller);	//‰Ÿ‚³‚ê‚Ä‚¢‚éŠÔ

bool IsInputUp(const InputKey& key, const Controller& controller);		//—£‚³‚ê‚½Žž