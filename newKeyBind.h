// KeyBind.h
#pragma once
#include "PlayerStatus.h"

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