// KeyBind.h
#pragma once
#include "PlayerStatus.h"

extern InputKey UpKey;
extern InputKey RightKey;
extern InputKey DownKey;
extern InputKey LeftKey;
extern InputKey JumpKey;
extern InputKey ActionKey;
extern InputKey ChangeKey;
extern InputKey ResetKey;
extern InputKey MenuKey;

bool IsInputDown(const InputKey& key, const Controller& controller);