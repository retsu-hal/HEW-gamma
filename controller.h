//controller.h
#pragma once
#include <Windows.h>
#include <Xinput.h>
#pragma comment(lib, "xinput.lib")

//----------------------------------------------
// Controller クラス
// XInput を使ってゲームパッド入力を管理にょ
//----------------------------------------------
class Controller
{
public:
    Controller(int index = 0);  // コントローラー番号 (0?3)
    void Update();              // 入力状態の更新

    // 接続状態
    bool IsConnected() const;

    // ボタン判定（互換性のため既存関数名は残す）
    bool IsButtonPressed(WORD button) const; // 現在押されているか
    bool IsButtonTrigger(WORD button) const;  // 今フレーム押された瞬間
    bool IsButtonReleased(WORD button) const; // 今フレーム離された瞬間


    // スティック入力（-1.0?1.0）
    float GetLeftStickX() const;
    float GetLeftStickY() const;
    float GetRightStickX() const;
    float GetRightStickY() const;

    // トリガー入力（0.0?1.0）
    float GetLeftTrigger() const;
    float GetRightTrigger() const;

private:
    int m_index;
    XINPUT_STATE m_state;
    XINPUT_STATE m_prevState; // 前フレームの状態を保持
    bool m_connected;

    float ApplyDeadZone(SHORT value, SHORT deadZone) const;
};
