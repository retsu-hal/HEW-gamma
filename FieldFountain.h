// FieldFountain.h
#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include "field.h"

//=========================================================================================================
// 噴水パラメータ
//=========================================================================================================
struct FountainParams
{
    float boostMultiplier;   // ジャンプ力倍率（通常ジャンプに対する倍率）
    float triggerHalfX;      // トリガー判定の半サイズ X
    float triggerHalfY;      // トリガー判定の半サイズ Y
    float triggerHalfZ;      // トリガー判定の半サイズ Z

    // 空中ブースト（ジャンプ後に追加の上向き加速度を与える）
    int   boostFrames;       // ブースト持続フレーム数
    float boostAccelY;       // 毎フレームの追加上向き加速度

    FountainParams()
        : boostMultiplier(1.5f)
        , triggerHalfX(1.0f)
        , triggerHalfY(1.0f)
        , triggerHalfZ(1.0f)
        , boostFrames(15)
        , boostAccelY(0.0026f)
    {
    }
};

//=========================================================================================================
// 噴水データ
//=========================================================================================================
class FountainData
{
public:
    DirectX::XMFLOAT3 basePos;      // 基準位置（マップ座標）
    int mapIndex;                    // GetFieldMap() 内の該当 MAPDATA インデックス
    FountainParams params;

    FountainData()
        : basePos{ 0, 0, 0 }
        , mapIndex(-1)
    {
    }
};

//=========================================================================================================
// API
//=========================================================================================================

// 初期化 / 終了
void Fountain_Initialize();
void Fountain_Finalize();
void Fountain_ClearAll();

// マップロード中に呼ぶ：MAPDATA を生成して mapData に追加し、内部リストに登録
int  Fountain_Create(float x, float y, float z, std::vector<MAPDATA>& mapData);

// 更新（現状はブースト管理のみ、将来的にアニメ等拡張可能）
void Fountain_UpdateAll(float deltaTime);

// プレイヤーとの噴水インタラクション（Player3D_Gravity から呼ぶ）
//   - 噴水の判定盒内でジャンプ入力時に強化ジャンプを発動
//   - 空中ブースト中は追加加速度を適用
void Fountain_PlayerInteraction();

// 参照
std::vector<FountainData>& Fountain_GetAll();
FountainData* Fountain_Get(int index);
int  Fountain_GetCount();

// デバッグ描画
void Fountain_DebugDraw();