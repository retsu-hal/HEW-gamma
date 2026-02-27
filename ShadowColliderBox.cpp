#include "ShadowColliderBox.h"
#include <algorithm>
#include <cmath>
#include <cfloat>
#include "UtilMath.h"

using namespace mu;
using namespace DirectX;

//=========================================================================================================
// グローバル（この翻訳単位のみ）
//=========================================================================================================

// シャドー管理（フィールド全体で1つ）
static ShadowManager g_ShadowManager;

//=========================================================================================================
// 内部ユーティリティ：Field分類
//=========================================================================================================

// 影を受ける側（Receiver）判定
// ※現状は FIELD_OBJ_1 のみを壁扱いとしている
static bool Field_IsReceiver(FIELD t)
{
	switch (t)
	{
	case FIELD_OBJ_1:
	case FIELD_WALL:
		return true;
	default:
		return false;
	}
}

// 影を落とす側（Caster）判定
// ※プレイヤー2Dの「影（壁面）」を構築する対象
static bool Field_IsShadows(FIELD t)
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

//=========================================================================================================
// 外部公開：ShadowManager取得
//=========================================================================================================

ShadowManager* GetShadowManager()
{
	return &g_ShadowManager;
}

//=========================================================================================================
// 内部ユーティリティ：Fieldコライダー情報
//=========================================================================================================

// フィールドOBBの半サイズを取得
// - useCustomCollider が有効なら colliderHalf を使用
// - そうでなければ BOX_RADIUS * scale
static XMFLOAT3 Field_GetHalfSize(const MAPDATA& m)
{
	if (m.useCustomCollider)
		return m.colliderHalf;

	return { BOX_RADIUS * m.scale.x, BOX_RADIUS * m.scale.y, BOX_RADIUS * m.scale.z };
}

// フィールドコライダー中心を取得
// ※一部のオブジェクト（例：シーソー板）だけ中心がモデル原点とずれるため補正
static XMFLOAT3 Field_GetColliderCenter(const MAPDATA& m)
{
	XMFLOAT3 offset = { 0, 0, 0 };

	// シーソー板（FIELD_SEESAW_2）はコライダー中心が少し上にある
	if (m.no == FIELD_SEESAW_2)
		offset.y = 0.5f;

	if (offset.x == 0 && offset.y == 0 && offset.z == 0)
		return m.pos;

	const XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(m.rotate.x),
		XMConvertToRadians(m.rotate.y),
		XMConvertToRadians(m.rotate.z));

	XMVECTOR vOffset = XMLoadFloat3(&offset);
	XMVECTOR vRotatedOffset = XMVector3TransformNormal(vOffset, rotMat);

	XMFLOAT3 rotatedOffset{};
	XMStoreFloat3(&rotatedOffset, vRotatedOffset);

	return XMFLOAT3{
		m.pos.x + rotatedOffset.x,
		m.pos.y + rotatedOffset.y,
		m.pos.z + rotatedOffset.z
	};
}

//=========================================================================================================
// 内部ユーティリティ：Raycast（OBB）
//=========================================================================================================

struct RayHit
{
	bool hit = false;
	float t = 0.0f;
	XMFLOAT3 point{};
	XMFLOAT3 normal{};
	size_t mapIndex = 0;
};

// 任意回転OBBへのレイ判定（ローカルへ変換してAABBとして判定）
// outNormal はワールド法線
static bool RaycastOBB_FullRotation(
	const XMFLOAT3& rayO, const XMFLOAT3& rayDir,
	const XMFLOAT3& boxC, const XMFLOAT3& boxH, const XMFLOAT3& boxRotDeg,
	float maxDist, float* outT, XMFLOAT3* outNormal)
{
	const XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(boxRotDeg.x),
		XMConvertToRadians(boxRotDeg.y),
		XMConvertToRadians(boxRotDeg.z));
	const XMMATRIX invRotMat = XMMatrixTranspose(rotMat);

	XMFLOAT3 relO = { rayO.x - boxC.x, rayO.y - boxC.y, rayO.z - boxC.z };
	XMVECTOR vRelO = XMLoadFloat3(&relO);
	XMVECTOR vDir = XMLoadFloat3(&rayDir);

	XMVECTOR vLocalO = XMVector3TransformNormal(vRelO, invRotMat);
	XMVECTOR vLocalD = XMVector3TransformNormal(vDir, invRotMat);

	XMFLOAT3 oL{}, dL{};
	XMStoreFloat3(&oL, vLocalO);
	XMStoreFloat3(&dL, vLocalD);

	float tmin = 0.0f;
	float tmax = maxDist;

	int hitAxis = -1;     // 0=x,1=y,2=z
	float hitSign = 1.0f; // 面の向き（±）

	auto slab = [&](float o, float d, float h, int axis) -> bool
		{
			if (std::fabs(d) < 1e-6f)
				return (o >= -h && o <= h);

			float t1 = (-h - o) / d;
			float t2 = (+h - o) / d;

			// d>0 のとき -面に当たる（法線は -）
			float sign = (d > 0.0f) ? -1.0f : 1.0f;

			if (t1 > t2)
			{
				std::swap(t1, t2);
				sign = -sign;
			}

			if (t1 > tmin)
			{
				tmin = t1;
				hitAxis = axis;
				hitSign = sign;
			}

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
		else                   nL.z = hitSign;

		XMVECTOR vLocalN = XMLoadFloat3(&nL);
		XMVECTOR vWorldN = XMVector3TransformNormal(vLocalN, rotMat);
		vWorldN = XMVector3Normalize(vWorldN);
		XMStoreFloat3(outNormal, vWorldN);
	}
	return true;
}

// Receiver集合へのレイ判定（最短ヒット）
// skip は caster 自身など「無視したいMAPDATA」のポインタ（同一参照を比較）
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
		const MAPDATA& m = receivers[i];
		if (skip == &m) continue;
		if (!Field_IsReceiver(m.no)) continue;

		float t = 0.0f;
		XMFLOAT3 n{};
		if (RaycastOBB_FullRotation(rayO, rayDir, m.pos, Field_GetHalfSize(m), m.rotate, maxDist, &t, &n))
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

// 光源→サンプル点の方向でReceiver集合に当てる（サンプル点より奥のヒットは無視）
// ※Directionalというより「光源基準の射線」
static bool RaycastToReceiversDirectional(
	const XMFLOAT3& lightPos,
	const XMFLOAT3& samplePoint,
	float maxDist,
	const std::vector<MAPDATA>& receivers,
	const MAPDATA* skipCaster,
	RayHit* outHit)
{
	const XMFLOAT3 dir = Normalize(samplePoint - lightPos);
	const float distToSample = Length(samplePoint - lightPos);

	bool found = false;
	float bestT = maxDist;
	RayHit best{};

	for (size_t i = 0; i < receivers.size(); ++i)
	{
		const MAPDATA& m = receivers[i];
		if (skipCaster == &m) continue;
		if (!Field_IsReceiver(m.no)) continue;

		float t = 0.0f;
		XMFLOAT3 n{};
		if (RaycastOBB_FullRotation(lightPos, dir, m.pos, Field_GetHalfSize(m), m.rotate, maxDist, &t, &n))
		{
			// サンプル点より手前のヒットは「遮蔽」として扱わない
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

//=========================================================================================================
// 内部ユーティリティ：Caster前提判定（光源と壁の間にCasterが存在するか）
//=========================================================================================================

// 光源→壁の射線上にCasterが存在する場合のみ「影生成候補」とする
// 目的：壁から見て「Casterが手前にないのに影を作る」ケースを抑制
static bool IsCasterBetweenLightAndWall(
	const XMFLOAT3& lightPos,
	const MAPDATA& caster,
	const std::vector<MAPDATA>& map)
{
	const XMFLOAT3 casterCenter = Field_GetColliderCenter(caster);
	const XMFLOAT3 toCenter = casterCenter - lightPos;
	const float distToCaster = Length(toCenter);
	if (distToCaster < 1e-4f) return false;

	const XMFLOAT3 dir = Normalize(toCenter);

	bool foundWall = false;
	float nearestWallT = FLT_MAX;

	for (size_t i = 0; i < map.size(); ++i)
	{
		const MAPDATA& m = map[i];
		if (&m == &caster) continue;
		if (!Field_IsReceiver(m.no)) continue;

		float t = 0.0f;
		XMFLOAT3 n{};
		if (RaycastOBB_FullRotation(lightPos, dir, m.pos, Field_GetHalfSize(m), m.rotate, 200.0f, &t, &n))
		{
			if (t < nearestWallT)
			{
				nearestWallT = t;
				foundWall = true;
			}
		}
	}

	if (!foundWall) return false;

	// 壁より手前にCasterがあるか
	return (distToCaster < nearestWallT);
}

//=========================================================================================================
// 内部ユーティリティ：サンプル点生成 / 幾何処理
//=========================================================================================================

// OBBの頂点＋辺上サンプル点をワールドで生成（任意回転対応）
static void GetOBBSamplePoints_FullRotation(
	const XMFLOAT3& c, const XMFLOAT3& h, const XMFLOAT3& rotDeg,
	int edgeSamples, std::vector<XMFLOAT3>& out)
{
	const XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(rotDeg.x),
		XMConvertToRadians(rotDeg.y),
		XMConvertToRadians(rotDeg.z));

	const XMFLOAT3 local[8] = {
		{-h.x, -h.y, -h.z}, {+h.x, -h.y, -h.z},
		{+h.x, +h.y, -h.z}, {-h.x, +h.y, -h.z},
		{-h.x, -h.y, +h.z}, {+h.x, -h.y, +h.z},
		{+h.x, +h.y, +h.z}, {-h.x, +h.y, +h.z},
	};

	XMFLOAT3 world[8];
	for (int i = 0; i < 8; ++i)
	{
		XMVECTOR vLocal = XMLoadFloat3(&local[i]);
		XMVECTOR vWorld = XMVector3TransformNormal(vLocal, rotMat);

		XMFLOAT3 rotated{};
		XMStoreFloat3(&rotated, vWorld);
		world[i] = XMFLOAT3{ c.x + rotated.x, c.y + rotated.y, c.z + rotated.z };
	}

	out.clear();
	out.reserve(8 + 12 * (std::max)(0, edgeSamples - 1));

	for (int i = 0; i < 8; ++i)
		out.push_back(world[i]);

	if (edgeSamples <= 0) return;

	const int edges[12][2] = {
		{0,1}, {1,2}, {2,3}, {3,0},
		{4,5}, {5,6}, {6,7}, {7,4},
		{0,4}, {1,5}, {2,6}, {3,7}
	};

	for (int e = 0; e < 12; ++e)
	{
		const XMFLOAT3 a = world[edges[e][0]];
		const XMFLOAT3 b = world[edges[e][1]];

		for (int i = 1; i < edgeSamples; ++i)
		{
			const float t = (float)i / (float)edgeSamples;
			out.push_back(XMFLOAT3{
				a.x + (b.x - a.x) * t,
				a.y + (b.y - a.y) * t,
				a.z + (b.z - a.z) * t
				});
		}
	}
}

// 法線nから直交基底(u,v)を構築
static void BuildOrthoBasis(const XMFLOAT3& n, XMFLOAT3& u, XMFLOAT3& v)
{
	const XMFLOAT3 t = (std::fabs(n.y) < 0.99f) ? XMFLOAT3{ 0,1,0 } : XMFLOAT3{ 1,0,0 };
	u = Normalize(Cross(t, n));
	v = Cross(n, u);
}

// 近い点の重複を除去
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

// 重心（平均）を計算
static XMFLOAT3 ComputeCentroid(const std::vector<XMFLOAT3>& pts)
{
	XMFLOAT3 sum = { 0, 0, 0 };
	for (const auto& p : pts) sum = sum + p;
	return pts.empty() ? sum : sum * (1.0f / (float)pts.size());
}

// 2D凸包（Andrewのmonotone chain）
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

	const size_t lower = hull.size();
	for (int i = (int)pts.size() - 2; i >= 0; --i)
	{
		while (hull.size() > lower && Cross2(hull[hull.size() - 2], hull.back(), pts[i]) <= 0)
			hull.pop_back();
		hull.push_back(pts[i]);
	}

	hull.pop_back();

	std::vector<int> result;
	result.reserve(hull.size());
	for (auto& p : hull) result.push_back(p.idx);
	return result;
}

// 点群からAABBを計算
static void ComputeAABB(const std::vector<XMFLOAT3>& pts, XMFLOAT3& mn, XMFLOAT3& mx)
{
	if (pts.empty())
	{
		mn = mx = { 0,0,0 };
		return;
	}

	mn = mx = pts[0];
	for (const auto& p : pts)
	{
		mn.x = (std::min)(mn.x, p.x); mn.y = (std::min)(mn.y, p.y); mn.z = (std::min)(mn.z, p.z);
		mx.x = (std::max)(mx.x, p.x); mx.y = (std::max)(mx.y, p.y); mx.z = (std::max)(mx.z, p.z);
	}
}

//=========================================================================================================
// 外部公開：ShadowPrismユーティリティ
//=========================================================================================================

void Shadow_Clear(ShadowPrism& prism)
{
	prism.poly.clear();
	prism.baseWorld.clear();
	prism.topWorld.clear();
	prism.groundPoly.clear();
	prism.standSegments.clear();

	prism.groundMaxY = 0.0f;
	prism.groundBandY = 0.25f;

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

	const float t2 = threshold * threshold;

	if (LengthSq(lightPos - prism.cachedLightPos) > t2) return true;
	if (LengthSq(caster.pos - prism.cachedCasterPos) > t2) return true;

	// 回転の変化は「度」で比較（ざっくり）
	if (std::fabs(caster.rotate.x - prism.cachedCasterRotate.x) > 0.5f) return true;
	if (std::fabs(caster.rotate.y - prism.cachedCasterRotate.y) > 0.5f) return true;
	if (std::fabs(caster.rotate.z - prism.cachedCasterRotate.z) > 0.5f) return true;

	return false;
}

//=========================================================================================================
// 外部公開：Shadow_Build（影プリズム構築）
//=========================================================================================================

bool Shadow_Build(
	ShadowPrism& out,
	const MAPDATA& caster,
	const XMFLOAT3& lightPos,
	const std::vector<MAPDATA>& receivers,
	const ShadowBuildConfig& config)
{
	Shadow_Clear(out);

	// 前提：光源と壁(Receiver)の間にCasterがあるときのみ影を作る
	if (!IsCasterBetweenLightAndWall(lightPos, caster, receivers))
		return false;

	const XMFLOAT3 casterCenter = Field_GetColliderCenter(caster);
	const XMFLOAT3 lightDir = Normalize(casterCenter - lightPos);

	// 1) CasterのOBBからサンプル点を生成
	std::vector<XMFLOAT3> samples;
	GetOBBSamplePoints_FullRotation(
		casterCenter,
		Field_GetHalfSize(caster),
		caster.rotate,
		config.edgeSamples,
		samples);

	// 2) 各サンプル点に対して光源からレイを飛ばし、Receiver面への当たりを収集
	struct HitData { XMFLOAT3 point; XMFLOAT3 normal; };
	std::vector<HitData> hits;
	hits.reserve(samples.size());

	for (const auto& sample : samples)
	{
		RayHit hit;
		if (RaycastToReceiversDirectional(lightPos, sample, config.maxCastDist, receivers, &caster, &hit))
			hits.push_back({ hit.point, hit.normal });
	}

	if (hits.size() < 3) return false;

	// 3) 最初のヒット面と同一平面（近い法線）だけを採用して影のベース点群を作る
	const XMFLOAT3 basePlaneN = Normalize(hits[0].normal);
	std::vector<XMFLOAT3> basePts;
	basePts.reserve(hits.size());

	for (const auto& h : hits)
	{
		if (Dot(Normalize(h.normal), basePlaneN) > config.samePlaneDot)
			basePts.push_back(h.point);
	}

	RemoveDuplicates(basePts, config.mergeEpsilon);
	if (basePts.size() < 3) return false;

	// 4) ベース点群から凸包を取り、プリズム断面ポリゴンを作る
	const XMFLOAT3 centroid = ComputeCentroid(basePts);

	// 平面法線の向きを「光源側」に揃える
	XMFLOAT3 planeN = basePlaneN;
	const XMFLOAT3 toLight = lightPos - centroid;
	if (Dot(planeN, toLight) < 0.0f)
		planeN = planeN * -1.0f;

	XMFLOAT3 u{}, v{};
	BuildOrthoBasis(planeN, u, v);

	std::vector<P2> pts2D;
	pts2D.reserve(basePts.size());

	for (int i = 0; i < (int)basePts.size(); ++i)
	{
		const XMFLOAT3 d = basePts[i] - centroid;
		pts2D.push_back({ Dot(d, u), Dot(d, v), i });
	}

	std::vector<int> hullIdx = ConvexHull2D(pts2D);
	if (hullIdx.size() < 3) return false;

	// 5) 出力に格納（baseWorld/topWorld/poly）
	out.origin = centroid;
	out.n = planeN;
	out.u = u;
	out.v = v;
	out.thickness = config.thickness;
	out.lightDir = Normalize(toLight);

	const XMFLOAT3 extrude = planeN * config.thickness;

	for (int idx : hullIdx)
	{
		const XMFLOAT3& pt = basePts[idx];
		out.baseWorld.push_back(pt);
		out.topWorld.push_back(pt + extrude);

		const XMFLOAT3 d = pt - centroid;
		out.poly.push_back({ Dot(d, u), Dot(d, v) });
	}

	// 6) 立ちエッジ（TopContactで着地判定に使う）を生成
	out.groundBandY = config.groundBandY;
	out.groundMaxY = -FLT_MAX;
	for (const auto& p : out.baseWorld)
		out.groundMaxY = (std::max)(out.groundMaxY, p.y);

	// 「平面内の上方向」を求め、凸包の上側チェーンを抽出して standSegments にする
	const XMFLOAT3 worldUp = { 0.0f, 1.0f, 0.0f };
	const float upDotN = Dot(worldUp, out.n);

	XMFLOAT3 upInPlane = worldUp - out.n * upDotN;
	float upLen = Length(upInPlane);
	const bool hasUpInPlane = (upLen > 1e-5f);
	if (hasUpInPlane) upInPlane = upInPlane * (1.0f / upLen);

	XMFLOAT2 d2 = { 0.0f, 1.0f };
	if (hasUpInPlane)
		d2 = { Dot(upInPlane, out.u), Dot(upInPlane, out.v) };

	float d2Len = sqrtf(d2.x * d2.x + d2.y * d2.y);
	if (d2Len < 1e-6f) { d2 = { 0.0f, 1.0f }; d2Len = 1.0f; }
	d2.x /= d2Len; d2.y /= d2Len;

	// 平面内の左右方向
	const XMFLOAT2 s2 = { -d2.y, d2.x };

	const int bn = (int)out.baseWorld.size();
	int idxMinS = 0, idxMaxS = 0;
	float minS = FLT_MAX, maxS = -FLT_MAX;

	for (int i = 0; i < bn; ++i)
	{
		const XMFLOAT2& p = out.poly[i];
		const float s = p.x * s2.x + p.y * s2.y;
		if (s < minS) { minS = s; idxMinS = i; }
		if (s > maxS) { maxS = s; idxMaxS = i; }
	}

	auto BuildChain = [&](int startIdx, int endIdx, int step) -> std::vector<int>
		{
			std::vector<int> chain;
			chain.reserve(bn + 1);

			int i = startIdx;
			chain.push_back(i);

			// 入力が壊れていても無限ループしないためのガード
			for (int guard = 0; guard < bn + 2 && i != endIdx; ++guard)
			{
				i = (i + step + bn) % bn;
				chain.push_back(i);
			}
			return chain;
		};

	auto ScoreChain = [&](const std::vector<int>& chain) -> float
		{
			float score = 0.0f;
			for (int idx : chain)
			{
				const XMFLOAT2& p = out.poly[idx];
				score += p.x * d2.x + p.y * d2.y;
			}
			return score;
		};

	const std::vector<int> chainA = BuildChain(idxMinS, idxMaxS, +1);
	const std::vector<int> chainB = BuildChain(idxMinS, idxMaxS, -1);

	const float scoreA = ScoreChain(chainA);
	const float scoreB = ScoreChain(chainB);
	const std::vector<int>& upperChain = (scoreA >= scoreB) ? chainA : chainB;

	out.standSegments.clear();

	// 傾きが急すぎるエッジを除外（ほぼ垂直の縁は「立てない」）
	const float kMaxSlopeDeg = 80.0f;
	const float kTanMax = tanf(XMConvertToRadians(kMaxSlopeDeg));

	auto AddStandSeg = [&](const XMFLOAT3& A, const XMFLOAT3& B)
		{
			const XMFLOAT3 d = B - A;
			if (LengthSq(d) < 1e-6f) return;

			const float lenXZ = sqrtf(d.x * d.x + d.z * d.z);
			if (lenXZ < 1e-4f) return;

			const float dy = fabsf(d.y);
			if (dy > kTanMax * lenXZ + 1e-5f) return;

			ShadowEdgeSegment s;
			s.a = A;
			s.b = B;
			out.standSegments.push_back(s);
		};

	if (upperChain.size() >= 2)
	{
		for (int k = 0; k + 1 < (int)upperChain.size(); ++k)
		{
			const int i0 = upperChain[k];
			const int i1 = upperChain[k + 1];
			AddStandSeg(out.baseWorld[i0], out.baseWorld[i1]);
		}
	}

	// 7) AABB作成（広域判定用）
	std::vector<XMFLOAT3> allPts = out.baseWorld;
	allPts.insert(allPts.end(), out.topWorld.begin(), out.topWorld.end());
	ComputeAABB(allPts, out.aabbMin, out.aabbMax);

	// 8) キャッシュ更新
	out.isValid = true;
	out.cachedLightPos = lightPos;
	out.cachedCasterPos = caster.pos;
	out.cachedCasterRotate = caster.rotate;

	return true;
}

//=========================================================================================================
// ShadowManager 実装
//=========================================================================================================

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

// 光源位置とマップから全Casterの影プリズムを更新する
void ShadowManager::UpdateAllShadows(
	const XMFLOAT3& lightPos,
	const std::vector<MAPDATA>& map,
	const ShadowBuildConfig& config)
{
	// 1) caster候補のインデックスを収集
	std::vector<int> newCasterIndices;
	for (int i = 0; i < (int)map.size(); ++i)
	{
		if (Field_IsShadows(map[i].no))
			newCasterIndices.push_back(i);
	}

	// 2) caster集合が変わったら配列を作り直す
	const bool castersChanged = (newCasterIndices != m_CasterIndices);
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

	// 3) 個別更新（必要なら再構築）
	for (size_t i = 0; i < m_CasterIndices.size(); ++i)
	{
		const int casterIdx = m_CasterIndices[i];
		const MAPDATA& caster = map[casterIdx];
		ShadowPrism& shadow = m_Shadows[i];

		if (Shadow_NeedsRebuild(shadow, lightPos, caster, config.rebuildThreshold))
		{
			Shadow_Build(shadow, caster, lightPos, map, config);
			shadow.casterIndex = casterIdx;
		}
	}
}