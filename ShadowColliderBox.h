#pragma once
#include <vector>
#include <cstdint>
#include <DirectXMath.h>
#include "field.h"

//=========================================================================================================
// 影コライダー（Shadow Prism）
// ・光源と caster(遮蔽物) から receiver(受け側壁) へ投影された「影の押し出しプリズム」を構築
// ・2Dプレイヤーの「影上歩行/壁衝突」判定に使用
//=========================================================================================================

struct ShadowEdgeSegment
{
    DirectX::XMFLOAT3 a{};
    DirectX::XMFLOAT3 b{};
};

struct ShadowPrism
{
    DirectX::XMFLOAT3 origin{}; // プリズム基準点（baseWorldの重心）
    DirectX::XMFLOAT3 n{};      // プリズムの法線（受け側壁の法線、単位ベクトル）
    DirectX::XMFLOAT3 u{}, v{}; // 平面内基底（単位ベクトル、u-v 平面がプリズム面）

    float thickness = 0.15f;    // 法線方向への厚み
    DirectX::XMFLOAT3 lightDir{}; // origin -> light 方向（参考情報）

    // 平面内2Dポリゴン（origin基準で u,v 軸へ射影した点列）
    std::vector<DirectX::XMFLOAT2> poly;

    // base/top のワールド座標（polyをworldへ戻したもの）
    std::vector<DirectX::XMFLOAT3> baseWorld;
    std::vector<DirectX::XMFLOAT3> topWorld;

    // 全体AABB（ワールド）
    DirectX::XMFLOAT3 aabbMin{};
    DirectX::XMFLOAT3 aabbMax{};

    // 「歩ける縁」判定用（上側チェーンから抽出したエッジ群）
    float groundMaxY = 0.0f;
    float groundBandY = 0.4f;
    std::vector<DirectX::XMFLOAT2> groundPoly;
    std::vector<ShadowEdgeSegment> standSegments;

    // キャッシュ（再構築判定用）
    bool isValid = false;
    int  casterIndex = -1;
    DirectX::XMFLOAT3 cachedLightPos{};
    DirectX::XMFLOAT3 cachedCasterPos{};
    DirectX::XMFLOAT3 cachedCasterRotate{};
};

//=========================================================================================================
// 内部設定
//=========================================================================================================

struct ShadowBuildConfig
{
    int   edgeSamples = 4;          // OBBエッジ上の追加サンプル数
    float maxCastDist = 100.0f;     // レイ最大距離
    float samePlaneDot = 0.95f;     // 同一平面扱いの法線dot閾値
    float thickness = 0.15f;        // 押し出し厚み
    float mergeEpsilon = 0.01f;     // サンプル点の重複除去距離
    float groundBandY = 0.4f;       // 地面帯の許容幅（将来拡張用）
    float rebuildThreshold = 0.01f; // 再構築判定（位置差）
};

//=========================================================================================================
// API
//=========================================================================================================

bool Shadow_Build(
    ShadowPrism& out,
    const MAPDATA& caster,
    const DirectX::XMFLOAT3& lightPos,
    const std::vector<MAPDATA>& receivers,
    const ShadowBuildConfig& config = ShadowBuildConfig());

bool Shadow_NeedsRebuild(
    const ShadowPrism& prism,
    const DirectX::XMFLOAT3& lightPos,
    const MAPDATA& caster,
    float threshold = 0.01f);

void Shadow_Clear(ShadowPrism& prism);

//=========================================================================================================
// ShadowManager（複数casterの影を管理）
//=========================================================================================================

class ShadowManager
{
public:
    void Initialize();
    void Finalize();

    void UpdateAllShadows(
        const DirectX::XMFLOAT3& lightPos,
        const std::vector<MAPDATA>& map,
        const ShadowBuildConfig& config);

    const std::vector<ShadowPrism>& GetShadows() const { return m_Shadows; }
    size_t GetShadowCount() const { return m_Shadows.size(); }

    bool HasValidShadows() const;
    void ClearAll();

private:
    std::vector<ShadowPrism> m_Shadows;
    std::vector<int> m_CasterIndices;
};

ShadowManager* GetShadowManager();