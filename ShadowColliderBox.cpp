#include "ShadowColliderBox.h"
#include <algorithm>
#include <cmath>
#include "MathUtil.h"
using namespace mu;
using namespace DirectX;

static ShadowManager g_ShadowManager;

ShadowManager* GetShadowManager()
{
    return &g_ShadowManager;
}


// フィールドの半分のサイズを取得
static XMFLOAT3 Field_GetHalfSize(const MAPDATA& m)
{
	return { BOX_RADIUS * m.scale.x, BOX_RADIUS * m.scale.y, BOX_RADIUS * m.scale.z };
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
static bool RaycastOBB_Yaw(
	const XMFLOAT3& rayO, const XMFLOAT3& rayDir,
	const XMFLOAT3& boxC, const XMFLOAT3& boxH, float boxYawDeg,
	float maxDist, float* outT, XMFLOAT3* outNormal)
{
	const float yaw = XMConvertToRadians(boxYawDeg);

	XMFLOAT3 oL = RotateY(rayO - boxC, -yaw);
	XMFLOAT3 dL = RotateY(rayDir, -yaw);

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
		*outNormal = Normalize(RotateY(nL, yaw));
	}
	return true;
}

// レイとフィールド受け手群の当たり判定
static bool RaycastReceivers(
	const XMFLOAT3& rayO, const XMFLOAT3& rayDirN,
	float maxDist,
	const std::vector<MAPDATA>& receivers,
	RayHit* outHit,
	const MAPDATA* skipOne = nullptr)
{
	bool found = false;
	float bestT = maxDist;
	RayHit best{};

	for (size_t i = 0; i < receivers.size(); ++i)
	{
		const auto& m = receivers[i];
		if (skipOne == &m) continue;


		if (!(m.no == FIELD_WALL || m.no == FIELD_GROUND || m.no == FIELD_OBJ_BOX || m.no == FIELD_OBJ_1))
			continue;

		const XMFLOAT3 c = m.pos;
		const XMFLOAT3 h = Field_GetHalfSize(m);
		const float yaw = m.rotate.y;

		float t = 0.0f;
		XMFLOAT3 nW{};
		if (!RaycastOBB_Yaw(rayO, rayDirN, c, h, yaw, maxDist, &t, &nW))
			continue;

		if (!found || t < bestT)
		{
			found = true;
			bestT = t;
			best.hit = true;
			best.t = t;
			best.normal = nW;
			best.point = rayO + rayDirN * t;
			best.mapIndex = i;
		}
	}

	if (!found) return false;
	if (outHit) *outHit = best;
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

        if (m.no != FIELD_OBJ_1) continue;

        float t;
        XMFLOAT3 n;
        if (RaycastOBB_Yaw(rayO, rayDir, m.pos, Field_GetHalfSize(m),
            m.rotate.y, maxDist, &t, &n))
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

// OBBのサンプルポイントを取得
static void GetOBBSamplePoints(
    const XMFLOAT3& c, const XMFLOAT3& h, float yawDeg,
    int edgeSamples, std::vector<XMFLOAT3>& out)
{
    const float yaw = XMConvertToRadians(yawDeg);

    const XMFLOAT3 local[8] = {
        {-h.x, -h.y, -h.z}, {+h.x, -h.y, -h.z},
        {+h.x, +h.y, -h.z}, {-h.x, +h.y, -h.z},
        {-h.x, -h.y, +h.z}, {+h.x, -h.y, +h.z},
        {+h.x, +h.y, +h.z}, {-h.x, +h.y, +h.z},
    };

    out.clear();

    for (int i = 0; i < 8; i++)
        out.push_back(c + RotateY(local[i], yaw));

    if (edgeSamples > 0)
    {
        const int edges[12][2] = {
            {0,1}, {1,2}, {2,3}, {3,0},
            {4,5}, {5,6}, {6,7}, {7,4},
            {0,4}, {1,5}, {2,6}, {3,7}
        };

        for (int e = 0; e < 12; e++)
        {
            XMFLOAT3 a = c + RotateY(local[edges[e][0]], yaw);
            XMFLOAT3 b = c + RotateY(local[edges[e][1]], yaw);

            for (int i = 1; i < edgeSamples; i++)
            {
                float t = (float)i / (float)edgeSamples;
                out.push_back(a + (b - a) * t);
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
    if (std::fabs(caster.rotate.y - prism.cachedCasterYaw) > 0.5f) return true;

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

    std::vector<XMFLOAT3> samples;
    GetOBBSamplePoints(caster.pos, Field_GetHalfSize(caster),
        caster.rotate.y, config.edgeSamples, samples);

    struct HitData { XMFLOAT3 point; XMFLOAT3 normal; };
    std::vector<HitData> hits;
    hits.reserve(samples.size());

    for (const auto& sample : samples)
    {
        XMFLOAT3 dir = Normalize(sample - lightPos);
        RayHit hit;
        if (RaycastToReceivers(lightPos, dir, config.maxCastDist, receivers, &caster, &hit))
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

    XMFLOAT3 extrude = planeN * config.thickness;

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
    out.cachedCasterYaw = caster.rotate.y;

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
        if (map[i].no == FIELD_OBJ_2)
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

