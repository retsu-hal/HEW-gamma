#include "Controller.h"
#include <cmath>

//----------------------------------------------
// コンストラクタ
//----------------------------------------------
Controller::Controller(int index)
    : m_index(index), m_connected(false)
{
    ZeroMemory(&m_state, sizeof(XINPUT_STATE));
}

//----------------------------------------------
// 入力状態の更新
//----------------------------------------------
void Controller::Update()
{
    ZeroMemory(&m_state, sizeof(XINPUT_STATE));
    DWORD result = XInputGetState(m_index, &m_state);
    m_connected = (result == ERROR_SUCCESS);
}

//----------------------------------------------
// 接続チェック
//----------------------------------------------
bool Controller::IsConnected() const
{
    return m_connected;
}

//----------------------------------------------
// ボタン判定
//----------------------------------------------
bool Controller::IsButtonPressed(WORD button) const
{
    if (!m_connected) return false;
    return (m_state.Gamepad.wButtons & button) != 0;
}

//----------------------------------------------
// スティック値を正規化（-1.0～1.0）
//----------------------------------------------
float Controller::ApplyDeadZone(SHORT value, SHORT deadZone) const
{
    int intVal = static_cast<int>(value);
    int intDead = static_cast<int>(deadZone);
    if (std::abs(intVal) < intDead) return 0.0f;
    float sign = (intVal >= 0) ? 1.0f : -1.0f;
    return (float)((std::abs(intVal) - intDead) / (32767.0f - intDead)) * sign;
}

float Controller::GetLeftStickX() const
{
    return ApplyDeadZone(m_state.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
}

float Controller::GetLeftStickY() const
{
    return ApplyDeadZone(m_state.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
}

float Controller::GetRightStickX() const
{
    return ApplyDeadZone(m_state.Gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
}

float Controller::GetRightStickY() const
{
    return ApplyDeadZone(m_state.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
}

//----------------------------------------------
// トリガー入力（0.0〜1.0）
//----------------------------------------------
float Controller::GetLeftTrigger() const
{
    if (!m_connected) return 0.0f;
    return m_state.Gamepad.bLeftTrigger / 255.0f;
}

float Controller::GetRightTrigger() const
{
    if (!m_connected) return 0.0f;
    return m_state.Gamepad.bRightTrigger / 255.0f;
}
