#include "ShadowColliderBox.h"
#include <algorithm>
#include <cmath>
#include "MathUtil.h"
using namespace mu;
using namespace DirectX;

static ShadowManager g_ShadowManager;

static bool Field_IsReceiver(FIELD t)// 影を受けるオブジェクトか
{
    switch (t)
    {
    case FIELD_OBJ_1:
        return true;
    default:
        return false;
    }
}

static bool Field_IsShadows(FIELD t)// 影を落とすオブジェクトか
{
    switch (t)
    {
    case FIELD_OBJ_2:
    case FIELD_SEESAW_1:
    case FIELD_SEESAW_2:
    case FIELD_MANHOLE:
        return true;
    default:
        return false;
    }
}


ShadowManager* GetShadowManager()
{
    return &g_ShadowManager;
}

// フィールドの半分のサイズを取得
static XMFLOAT3 Field_GetHalfSize(const MAPDATA& m)
{
    if (m.useCustomCollider)
    {
        return m.colliderHalf;
    }
	return { BOX_RADIUS * m.scale.x, BOX_RADIUS * m.scale.y, BOX_RADIUS * m.scale.z };
}

static XMFLOAT3 Field_GetColliderCenter(const MAPDATA& m)
{
    XMFLOAT3 offset = { 0, 0, 0 };

    if (m.no == FIELD_SEESAW_2)
    {
        offset.y = 0.5f;
    }

    if (offset.x == 0 && offset.y == 0 && offset.z == 0)
    {
        return m.pos;
    }

    XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(
        XMConvertToRadians(m.rotate.x),
        XMConvertToRadians(m.rotate.y),
        XMConvertToRadians(m.rotate.z)
    );

    XMVECTOR vOffset = XMLoadFloat3(&offset);
    XMVECTOR vRotatedOffset = XMVector3TransformNormal(vOffset, rotMat);
    XMFLOAT3 rotatedOffset;
    XMStoreFloat3(&rotatedOffset, vRotatedOffset);

    return XMFLOAT3{
        m.pos.x + rotatedOffset.x,
        m.pos.y + rotatedOffset.y,
        m.pos.z + rotatedOffset.z
    };
}

// レイの当たり情報
struct RayHit
{
	bool hit = false;
	float t = 0.0f;
	XMFLOAT3 point{};
	XMFLOAT3 normal{};
	size_t mapIndex = 0;
};

// レイとYaw回転OBBの当たり判定
static bool RaycastOBB_FullRotation(
    const XMFLOAT3& rayO, const XMFLOAT3& rayDir,
    const XMFLOAT3& boxC, const XMFLOAT3& boxH, const XMFLOAT3& boxRotDeg,
    float maxDist, float* outT, XMFLOAT3* outNormal)
{
    XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(
        XMConvertToRadians(boxRotDeg.x),
        XMConvertToRadians(boxRotDeg.y),
        XMConvertToRadians(boxRotDeg.z)
    );
    XMMATRIX invRotMat = XMMatrixTranspose(rotMat);

    XMFLOAT3 relO = { rayO.x - boxC.x, rayO.y - boxC.y, rayO.z - boxC.z };
    XMVECTOR vRelO = XMLoadFloat3(&relO);
    XMVECTOR vDir = XMLoadFloat3(&rayDir);

    XMVECTOR vLocalO = XMVector3TransformNormal(vRelO, invRotMat);
    XMVECTOR vLocalD = XMVector3TransformNormal(vDir, invRotMat);

    XMFLOAT3 oL, dL;
    XMStoreFloat3(&oL, vLocalO);
    XMStoreFloat3(&dL, vLocalD);

    float tmin = 0.0f, tmax = maxDist;
    int hitAxis = -1;
    float hitSign = 1.0f;

    auto slab = [&](float o, float d, float h, int axis) -> bool
        {
            if (std::fabs(d) < 1e-6f)
                return (o >= -h && o <= h);

            float t1 = (-h - o) / d;
            float t2 = (+h - o) / d;
            float sign = (d > 0.0f) ? -1.0f : 1.0f;

            if (t1 > t2) { std::swap(t1, t2); sign = -sign; }

            if (t1 > tmin) { tmin = t1; hitAxis = axis; hitSign = sign; }
            tmax = (std::min)(tmax, t2);

            return tmin <= tmax;
        };

    if (!slab(oL.x, dL.x, boxH.x, 0)) return false;
    if (!slab(oL.y, dL.y, boxH.y, 1)) return false;
    if (!slab(oL.z, dL.z, boxH.z, 2)) return false;

    if (tmin < 0.0f || tmin > maxDist) return false;

    if (outT) *outT = tmin;
    if (outNormal)
    {
        XMFLOAT3 nL = { 0, 0, 0 };
        if (hitAxis == 0) nL.x = hitSign;
        else if (hitAxis == 1) nL.y = hitSign;
        else nL.z = hitSign;

        XMVECTOR vLocalN = XMLoadFloat3(&nL);
        XMVECTOR vWorldN = XMVector3TransformNormal(vLocalN, rotMat);
        vWorldN = XMVector3Normalize(vWorldN);
        XMStoreFloat3(outNormal, vWorldN);
    }
    return true;
}


// レイとフィールド受け手群の当たり判定
static bool RaycastToReceivers(
    const XMFLOAT3& rayO, const XMFLOAT3& rayDir, float maxDist,
    const std::vector<MAPDATA>& receivers, const MAPDATA* skip,
    RayHit* outHit)
{
    bool found = false;
    float bestT = maxDist;
    RayHit best{};

    for (size_t i = 0; i < receivers.size(); ++i)
    {
        const auto& m = receivers[i];
        if (skip == &m) continue;

        if (!Field_IsReceiver(m.no)) continue;

        float t;
        XMFLOAT3 n;
        if (RaycastOBB_FullRotation(rayO, rayDir, m.pos, Field_GetHalfSize(m),
            m.rotate, maxDist, &t, &n))
        {
            if (!found || t < bestT)
            {
                found = true;
                bestT = t;
                best.hit = true;
                best.t = t;
                best.point = rayO + rayDir * t;
                best.normal = n;
                best.mapIndex = i;
            }
        }
    }

    if (outHit) *outHit = best;
    return found;
}

static bool RaycastToReceiversDirectional(
    const XMFLOAT3& lightPos,
    const XMFLOAT3& samplePoint,
    float maxDist,
    const std::vector<MAPDATA>& receivers,
    const MAPDATA* skipCaster,
    RayHit* outHit)
{

    XMFLOAT3 dir = Normalize(samplePoint - lightPos);
    float distToSample = Length(samplePoint - lightPos);

    bool found = false;
    float bestT = maxDist;
    RayHit best{};

    for (size_t i = 0; i < receivers.size(); ++i)
    {
        const auto& m = receivers[i];
        if (skipCaster == &m) continue;

        if (!Field_IsReceiver(m.no)) continue;

        float t;
        XMFLOAT3 n;
        if (RaycastOBB_FullRotation(lightPos, dir, m.pos, Field_GetHalfSize(m),
            m.rotate, maxDist, &t, &n))
        {
            if (t < distToSample) continue;

            if (!found || t < bestT)
            {
                found = true;
                bestT = t;
                best.hit = true;
                best.t = t;
                best.point = lightPos + dir * t;
                best.normal = n;
                best.mapIndex = i;
            }
        }
    }

    if (outHit) *outHit = best;
    return found;
}


static bool IsCasterBetweenLightAndWall(
    const XMFLOAT3& lightPos,
    const MAPDATA& caster,
    const std::vector<MAPDATA>& map)
{
    XMFLOAT3 casterCenter = Field_GetColliderCenter(caster);
    XMFLOAT3 toCenter = casterCenter - lightPos;
    float distToCaster = Length(toCenter);

    if (distToCaster < 1e-4f) return false;

    XMFLOAT3 dir = Normalize(toCenter);

    bool foundWall = false;
    float nearestWallT = FLT_MAX;

    for (size_t i = 0; i < map.size(); ++i)
    {
        const auto& m = map[i];
        if (&m == &caster) continue;

        if (!Field_IsReceiver(m.no)) continue;

        float t;
        XMFLOAT3 n;
        if (RaycastOBB_FullRotation(lightPos, dir, m.pos, Field_GetHalfSize(m),
            m.rotate, 200.0f, &t, &n))
        {
            if (t < nearestWallT)
            {
                nearestWallT = t;
                foundWall = true;
            }
        }
    }

    if (!foundWall) return false;

    return (distToCaster < nearestWallT);
}


// OBBのサンプルポイントを取得
static void GetOBBSamplePoints_FullRotation(
    const XMFLOAT3& c, const XMFLOAT3& h, const XMFLOAT3& rotDeg,
    int edgeSamples, std::vector<XMFLOAT3>& out)
{
    XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(
        XMConvertToRadians(rotDeg.x),
        XMConvertToRadians(rotDeg.y),
        XMConvertToRadians(rotDeg.z)
    );

    const XMFLOAT3 local[8] = {
        {-h.x, -h.y, -h.z}, {+h.x, -h.y, -h.z},
        {+h.x, +h.y, -h.z}, {-h.x, +h.y, -h.z},
        {-h.x, -h.y, +h.z}, {+h.x, -h.y, +h.z},
        {+h.x, +h.y, +h.z}, {-h.x, +h.y, +h.z},
    };

    XMFLOAT3 world[8];
    for (int i = 0; i < 8; i++)
    {
        XMVECTOR vLocal = XMLoadFloat3(&local[i]);
        XMVECTOR vWorld = XMVector3TransformNormal(vLocal, rotMat);
        XMFLOAT3 rotated;
        XMStoreFloat3(&rotated, vWorld);
        world[i] = XMFLOAT3{ c.x + rotated.x, c.y + rotated.y, c.z + rotated.z };
    }

    out.clear();

    for (int i = 0; i < 8; i++)
        out.push_back(world[i]);

    if (edgeSamples > 0)
    {
        const int edges[12][2] = {
            {0,1}, {1,2}, {2,3}, {3,0},
            {4,5}, {5,6}, {6,7}, {7,4},
            {0,4}, {1,5}, {2,6}, {3,7}
        };

        for (int e = 0; e < 12; e++)
        {
            XMFLOAT3 a = world[edges[e][0]];
            XMFLOAT3 b = world[edges[e][1]];

            for (int i = 1; i < edgeSamples; i++)
            {
                float t = (float)i / (float)edgeSamples;
                out.push_back(XMFLOAT3{
                    a.x + (b.x - a.x) * t,
                    a.y + (b.y - a.y) * t,
                    a.z + (b.z - a.z) * t
                    });
            }
        }
    }
}
// 法線から直交基底を構築
static void BuildOrthoBasis(const XMFLOAT3& n, XMFLOAT3& u, XMFLOAT3& v)
{
    XMFLOAT3 t = (std::fabs(n.y) < 0.99f) ? XMFLOAT3{ 0,1,0 } : XMFLOAT3{ 1,0,0 };
    u = Normalize(Cross(t, n));
    v = Cross(n, u);
}

// 近接点の重複を除去
static void RemoveDuplicates(std::vector<XMFLOAT3>& pts, float eps)
{
    const float e2 = eps * eps;
    std::vector<XMFLOAT3> result;
    result.reserve(pts.size());

    for (const auto& p : pts)
    {
        bool dup = false;
        for (const auto& q : result)
        {
            if (LengthSq(p - q) < e2) { dup = true; break; }
        }
        if (!dup) result.push_back(p);
    }
    pts.swap(result);
}

// 点群の重心を計算
static XMFLOAT3 ComputeCentroid(const std::vector<XMFLOAT3>& pts)
{
    XMFLOAT3 sum = { 0, 0, 0 };
    for (const auto& p : pts) sum = sum + p;
    return pts.empty() ? sum : sum * (1.0f / (float)pts.size());
}

// 2D凸包計算（Andrew's monotone chainアルゴリズム）
struct P2 { float x, y; int idx; };
static float Cross2(const P2& a, const P2& b, const P2& c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

static std::vector<int> ConvexHull2D(std::vector<P2>& pts)
{
    if (pts.size() < 3) return {};

    std::sort(pts.begin(), pts.end(), [](const P2& a, const P2& b) {
        return (a.x != b.x) ? (a.x < b.x) : (a.y < b.y);
        });

    std::vector<P2> hull;

    for (auto& p : pts)
    {
        while (hull.size() >= 2 && Cross2(hull[hull.size() - 2], hull.back(), p) <= 0)
            hull.pop_back();
        hull.push_back(p);
    }

    size_t lower = hull.size();
    for (int i = (int)pts.size() - 2; i >= 0; i--)
    {
        while (hull.size() > lower && Cross2(hull[hull.size() - 2], hull.back(), pts[i]) <= 0)
            hull.pop_back();
        hull.push_back(pts[i]);
    }

    hull.pop_back();

    std::vector<int> result;
    for (auto& p : hull) result.push_back(p.idx);
    return result;
}

// 点群のAABBを計算
static void ComputeAABB(const std::vector<XMFLOAT3>& pts, XMFLOAT3& mn, XMFLOAT3& mx)
{
    if (pts.empty()) { mn = mx = { 0,0,0 }; return; }

    mn = mx = pts[0];
    for (const auto& p : pts)
    {
        mn.x = (std::min)(mn.x, p.x); mn.y = (std::min)(mn.y, p.y); mn.z = (std::min)(mn.z, p.z);
        mx.x = (std::max)(mx.x, p.x); mx.y = (std::max)(mx.y, p.y); mx.z = (std::max)(mx.z, p.z);
    }
}



void Shadow_Clear(ShadowPrism& prism)
{
    prism.poly.clear();
    prism.baseWorld.clear();
    prism.topWorld.clear();
    prism.isValid = false;
    prism.lightDir = { 0, 0, 0 };
}

bool Shadow_NeedsRebuild(
    const ShadowPrism& prism,
    const XMFLOAT3& lightPos,
    const MAPDATA& caster,
    float threshold)
{
    if (!prism.isValid) return true;

    float t2 = threshold * threshold;
    if (LengthSq(lightPos - prism.cachedLightPos) > t2) return true;
    if (LengthSq(caster.pos - prism.cachedCasterPos) > t2) return true;

    if (std::fabs(caster.rotate.x - prism.cachedCasterRotate.x) > 0.5f) return true;
    if (std::fabs(caster.rotate.y - prism.cachedCasterRotate.y) > 0.5f) return true;
    if (std::fabs(caster.rotate.z - prism.cachedCasterRotate.z) > 0.5f) return true;

    return false;
}


bool Shadow_Build(
    ShadowPrism& out,
    const MAPDATA& caster,
    const XMFLOAT3& lightPos,
    const std::vector<MAPDATA>& receivers,
    const ShadowBuildConfig& config)
{
    Shadow_Clear(out);

    if (!IsCasterBetweenLightAndWall(lightPos, caster, receivers))
    {
        return false;
    }

    XMFLOAT3 casterCenter = Field_GetColliderCenter(caster);

    XMFLOAT3 lightDir = Normalize(casterCenter - lightPos);

    std::vector<XMFLOAT3> samples;
    GetOBBSamplePoints_FullRotation(casterCenter, Field_GetHalfSize(caster),
        caster.rotate, config.edgeSamples, samples);

    struct HitData { XMFLOAT3 point; XMFLOAT3 normal; };
    std::vector<HitData> hits;
    hits.reserve(samples.size());

    for (const auto& sample : samples)
    {
        RayHit hit;
        if (RaycastToReceiversDirectional(lightPos, sample, config.maxCastDist,
            receivers, &caster, &hit))
        {
            hits.push_back({ hit.point, hit.normal });
        }
    }

    if (hits.size() < 3) return false;

    XMFLOAT3 planeN = Normalize(hits[0].normal);
    std::vector<XMFLOAT3> basePts;

    for (const auto& h : hits)
    {
        if (Dot(Normalize(h.normal), planeN) > config.samePlaneDot)
            basePts.push_back(h.point);
    }

    RemoveDuplicates(basePts, config.mergeEpsilon);
    if (basePts.size() < 3) return false;

    XMFLOAT3 u, v;
    BuildOrthoBasis(planeN, u, v);
    XMFLOAT3 centroid = ComputeCentroid(basePts);

    std::vector<P2> pts2D;
    for (int i = 0; i < (int)basePts.size(); i++)
    {
        XMFLOAT3 d = basePts[i] - centroid;
        pts2D.push_back({ Dot(d, u), Dot(d, v), i });
    }

    std::vector<int> hullIdx = ConvexHull2D(pts2D);
    if (hullIdx.size() < 3) return false;

    out.origin = centroid;
    out.n = planeN;
    out.u = u;
    out.v = v;
    out.thickness = config.thickness;

    out.lightDir = lightDir;
    XMFLOAT3 extrude = lightDir * (-config.thickness);

    for (int idx : hullIdx)
    {
        const XMFLOAT3& pt = basePts[idx];
        out.baseWorld.push_back(pt);
        out.topWorld.push_back(pt + extrude);

        XMFLOAT3 d = pt - centroid;
        out.poly.push_back({ Dot(d, u), Dot(d, v) });
    }

    std::vector<XMFLOAT3> allPts = out.baseWorld;
    allPts.insert(allPts.end(), out.topWorld.begin(), out.topWorld.end());
    ComputeAABB(allPts, out.aabbMin, out.aabbMax);

    out.isValid = true;
    out.cachedLightPos = lightPos;
    out.cachedCasterPos = caster.pos;
    out.cachedCasterRotate = caster.rotate;

    return true;
}

void ShadowManager::Initialize()
{
    m_Shadows.clear();
    m_CasterIndices.clear();
}

void ShadowManager::Finalize()
{
    ClearAll();
}

void ShadowManager::ClearAll()
{
    m_Shadows.clear();
    m_CasterIndices.clear();
}

bool ShadowManager::HasValidShadows() const
{
    for (const auto& shadow : m_Shadows)
    {
        if (shadow.isValid) return true;
    }
    return false;
}


void ShadowManager::UpdateAllShadows(
    const XMFLOAT3& lightPos,
    const std::vector<MAPDATA>& map,
    const ShadowBuildConfig& config)
{

    std::vector<int> newCasterIndices;
    for (int i = 0; i < (int)map.size(); ++i)
    {
        if (Field_IsShadows(map[i].no))
        {
            newCasterIndices.push_back(i);
        }
    }

    bool castersChanged = (newCasterIndices != m_CasterIndices);

    if (castersChanged)
    {
        m_CasterIndices = newCasterIndices;
        m_Shadows.resize(m_CasterIndices.size());

        for (size_t i = 0; i < m_Shadows.size(); ++i)
        {
            Shadow_Clear(m_Shadows[i]);
            m_Shadows[i].casterIndex = m_CasterIndices[i];
        }
    }

    for (size_t i = 0; i < m_CasterIndices.size(); ++i)
    {
        int casterIdx = m_CasterIndices[i];
        const MAPDATA& caster = map[casterIdx];
        ShadowPrism& shadow = m_Shadows[i];

        if (Shadow_NeedsRebuild(shadow, lightPos, caster, config.rebuildThreshold))
        {
            Shadow_Build(shadow, caster, lightPos, map, config);
            shadow.casterIndex = casterIdx;
        }
    }
}

