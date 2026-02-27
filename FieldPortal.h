// FieldPortal.h
#pragma once

#include <DirectXMath.h>
#include <vector>

struct PortalData
{
    int                 entranceMapIndex; // GetFieldMap() 内の K の MAPDATA インデックス
    DirectX::XMFLOAT3   destPos;          // 対応する J の座標（不足時は R）
    bool                hasExplicitJ;     // true: J が存在 / false: R へフォールバック

    PortalData()
        : entranceMapIndex(-1)
        , destPos(0.0f, 0.0f, 0.0f)
        , hasExplicitJ(false)
    {
    }
};

// 初期化/解放
void Portal_Initialize();
void Portal_Finalize();

// ステージ切り替え/マップ再読み込み時に呼ぶ
void Portal_ClearAll();

// マップロード中に登録する（K は MapData に入れた後に mapIndex を渡す）
void Portal_RegisterEntranceMapIndex(int mapIndex);

// マップロード中に登録する（J は座標のみ）
void Portal_RegisterExitMarker(const DirectX::XMFLOAT3& pos);

// マップロード完了後に呼ぶ：K と J を「出現順」でペアリング
//  - J が不足した場合は fallbackR（=R の座標）を dest に入れる
void Portal_BuildPairs(const DirectX::XMFLOAT3& fallbackR);

// K の mapIndex から転送先を取得
bool Portal_GetDestByEntranceMapIndex(int entranceMapIndex, DirectX::XMFLOAT3* outDest);

// デバッグ描画（J の位置 / K->J の線など）
void Portal_DebugDraw();

// 内部データ参照（必要なら）
const std::vector<PortalData>& Portal_GetAll();
