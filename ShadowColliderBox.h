#pragma once
#include <vector>
#include <cstdint>
#include <DirectXMath.h>
#include "field.h"

// ============================================================
// 影生成用プリズムデータ
// ============================================================
struct ShadowEdgeSegment
{
    DirectX::XMFLOAT3 a{};
    DirectX::XMFLOAT3 b{};
};

struct ShadowPrism
{
	// プリズムの基準情報
    DirectX::XMFLOAT3 origin{};
	DirectX::XMFLOAT3 n{};// 法線（単位ベクトル）
    DirectX::XMFLOAT3 u{}, v{};// 接線（単位ベクトル）
    float thickness = 0.15f;
    
    DirectX::XMFLOAT3 lightDir{};

	// プリズムの断面ポリゴン（ローカル2D座標系）
    std::vector<DirectX::XMFLOAT2> poly;
    std::vector<DirectX::XMFLOAT3> baseWorld;
    std::vector<DirectX::XMFLOAT3> topWorld;

	// AABB（ワールド座標系）
    DirectX::XMFLOAT3 aabbMin{};
    DirectX::XMFLOAT3 aabbMax{};

    float groundMaxY = 0.0f;
    float groundBandY = 0.25f;
    std::vector<DirectX::XMFLOAT2> groundPoly;
    std::vector<ShadowEdgeSegment> standSegments;

	// キャッシュ情報
    bool isValid = false;
    int casterIndex = -1;
    DirectX::XMFLOAT3 cachedLightPos{};
    DirectX::XMFLOAT3 cachedCasterPos{};
    DirectX::XMFLOAT3 cachedCasterRotate{};
};

// ============================================================
// 影生成設定
// ============================================================
struct ShadowBuildConfig
{
    int edgeSamples = 4;       
    float maxCastDist = 100.0f;
    float samePlaneDot = 0.95f;
    float thickness = 0.15f;   
    float mergeEpsilon = 0.01f;
    float groundBandY = 0.25f;
    float rebuildThreshold = 0.01f;
};

// ============================================================
// 影生成関数群
// ============================================================

// シャドウプリズムの構築
bool Shadow_Build(
    ShadowPrism& out,
    const MAPDATA& caster,
    const DirectX::XMFLOAT3& lightPos,
    const std::vector<MAPDATA>& receivers,
    const ShadowBuildConfig& config = ShadowBuildConfig());

// シャドウプリズムの再構築（キャスターOBBから）
bool Shadow_NeedsRebuild(
    const ShadowPrism& prism,
    const DirectX::XMFLOAT3& lightPos,
    const MAPDATA& caster,
    float threshold = 0.01f);

// シャドウプリズムのクリア
void Shadow_Clear(ShadowPrism& prism);

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
