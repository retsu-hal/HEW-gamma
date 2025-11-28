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
    Controller(int index = 0);  // コントローラー番号 (0〜3)
    void Update();              // 入力状態の更新

    // 接続状態
    bool IsConnected() const;

    // ボタン
    bool IsButtonPressed(WORD button) const;

    // スティック入力（-1.0〜1.0）
    float GetLeftStickX() const;
    float GetLeftStickY() const;
    float GetRightStickX() const;
    float GetRightStickY() const;

    // トリガー入力（0.0〜1.0）
    float GetLeftTrigger() const;
    float GetRightTrigger() const;

private:
    int m_index;
    XINPUT_STATE m_state;
    bool m_connected;

    // ← ここを const に変更
    float ApplyDeadZone(SHORT value, SHORT deadZone) const;
};
