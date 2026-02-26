
#include <cfloat>
#include <cmath>
#include "Collision.h"
#include "player3D.h"
#include "Player2D.h"


using namespace DirectX;
using namespace mu;

static std::vector<const ShadowPrism*> g_ShadowPrisms;
static ShadowDebugOptions g_ShadowDebugOpts;

struct ExtraDebugBox
{
	bool isOBB = false;
	XMFLOAT3 center{};
	XMFLOAT3 half{};
	XMFLOAT3 rotDeg{};
	ImU32 color = 0;
};

static std::vector<ExtraDebugBox> g_ExtraBoxes;

static int    s_LastStandingPrismIndex = -1;
static XMFLOAT3 s_LastShadowTopPos = { 0, 0, 0 };
static int    s_GraceFrames = 0;

static XMFLOAT3 s_LastStandDirXZ = { 1, 0, 0 };
static bool     s_HasStandDir = false;

static bool     s_StandSegValid = false;
static bool     s_StandSegWalkable = false;
static XMFLOAT3 s_StandSegA = { 0,0,0 };
static XMFLOAT3 s_StandSegB = { 0,0,0 };
static XMFLOAT3 s_StandSegAB = { 1,0,0 };
static XMFLOAT3 s_StandDirXZ = { 1,0,0 };   // normalized XZ direction along the stand segment
static float    s_StandLenXZ = 1.0f;        // |AB| in XZ (for slope delta)
static float    s_FrameSlopeDeltaY = 0.0f;  // added to Position.y in MoveAndCollision()
static XMFLOAT3 s_StandClosestQ = { 0,0,0 };

// Debug vectors
static XMFLOAT3 s_DbgMoveInXZ = { 0,0,0 };
static XMFLOAT3 s_DbgMoveOutXZ = { 0,0,0 };
static XMFLOAT3 s_DbgMoveSlope = { 0,0,0 };



// シャドウの当たり判定の状態をリセット（プレイヤーが地面から離れたときなどに呼び出す）
void Collision_ResetShadowContactState()
{
	s_LastStandingPrismIndex = -1;
	s_LastShadowTopPos = { 0, 0, 0 };
	s_GraceFrames = 0;
}

// プリズムリストを設定
void Collision_SetShadowPrisms(const std::vector<const ShadowPrism*>& prisms)
{
	g_ShadowPrisms = prisms;
}

// 現在のプリズムリストを取得
const std::vector<const ShadowPrism*>& Collision_GetShadowPrisms()
{
	return g_ShadowPrisms;
}

// 単一のプリズムを設定（接地判定などで使用）
void Collision_SetShadowPrism(const ShadowPrism* prism)
{
	g_ShadowPrisms.clear();
	if (prism && prism->isValid)
	{
		g_ShadowPrisms.push_back(prism);
	}
}

// 最初のプリズムを取得（接地判定などで使用）
const ShadowPrism* Collision_GetShadowPrism()
{
	return g_ShadowPrisms.empty() ? nullptr : g_ShadowPrisms[0];
}

// デバッグオプションの設定
static ImU32 MakeColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	return ((ImU32)a << 24) | ((ImU32)b << 16) | ((ImU32)g << 8) | (ImU32)r;
}


// プレイヤーの当たり判定用コライダーの中心座標を取得
static XMFLOAT3 GetPlayerSolidCollider()
{
	PLAYER* p = GetPlayer3D();
	XMFLOAT3 c = p->Position;
	c.y += Player3D_GetSolidHalfSize().y;
	return c;
}
// プレイヤー2Dの当たり判定用コライダーの中心座標を取得
static XMFLOAT3 GetPlayer2DSolidCollider()
{
	PLAYER* p = GetPlayer2D();
	XMFLOAT3 c = p->Position;
	c.y += Player2D_GetSolidHalfSize().y + 0.1f;
	return c;
}
// プレイヤーのトリガー用コライダーの中心座標を取得
static XMFLOAT3 GetPlayerTriggerCollider()
{
	PLAYER* p = GetPlayer3D();
	XMFLOAT3 c = p->Position;
	c.y += Player3D_GetTriggerHalfSize().y;
	return c;
}

// フィールドが当たり判定用かどうかを取得
static bool Field_IsSolid(FIELD t)
{
	switch (t)
	{
	case FIELD_GROUND:
	case FIELD_WALL:
	case FIELD_OBJ_BOX:
	case FIELD_EMPTY_BOX:
	case FIELD_OBJ_1:
	case FIELD_SEESAW_1:
	case FIELD_SEESAW_2:
	case FIELD_MANHOLE:
	case FIELD_OBJ_2:
	case FIELD_OBJ_3:

		return true;
	default:
		return false;
	}
}

// フィールドがトリガーかどうかを取得
static bool Field_IsTrigger(FIELD t)
{
	switch (t)
	{
	case FIELD_GOAL:
	case FIELD_OBJ_1:
	case FIELD_STAGE_1:
	case FIELD_STAGE_2:
	case FIELD_STAGE_3:
		return true;
	default:
		return false;
	}
}

// OBB同士の当たり判定（Y軸回転のみ対応）
static bool OBB_Intersect_Yaw(
	const XMFLOAT3& cA, const XMFLOAT3& hA, float yawA,
	const XMFLOAT3& cB, const XMFLOAT3& hB, float yawB)
{
	if (fabsf(cA.y - cB.y) > (hA.y + hB.y)) return false;

	auto getAxes = [](float yaw, XMFLOAT3& ax, XMFLOAT3& az) {
		float y = XMConvertToRadians(yaw);
		XMFLOAT3 fwd = { sinf(y), 0, cosf(y) };
		ax = { fwd.z, 0, -fwd.x };
		az = fwd;
		};

	XMFLOAT3 Ax, Az, Bx, Bz;
	getAxes(yawA, Ax, Az);
	getAxes(yawB, Bx, Bz);

	XMFLOAT3 T = { cB.x - cA.x, 0, cB.z - cA.z };

	float t0 = Dot2D(T, Ax), t1 = Dot2D(T, Az);
	float R00 = Dot2D(Ax, Bx), R01 = Dot2D(Ax, Bz);
	float R10 = Dot2D(Az, Bx), R11 = Dot2D(Az, Bz);

	const float eps = 1e-6f;
	float aR00 = fabsf(R00) + eps, aR01 = fabsf(R01) + eps;
	float aR10 = fabsf(R10) + eps, aR11 = fabsf(R11) + eps;

	if (fabsf(t0) > hA.x + hB.x * aR00 + hB.z * aR01) return false;
	if (fabsf(t1) > hA.z + hB.x * aR10 + hB.z * aR11) return false;
	if (fabsf(t0 * R00 + t1 * R10) > hB.x + hA.x * aR00 + hA.z * aR10) return false;
	if (fabsf(t0 * R01 + t1 * R11) > hB.z + hA.x * aR01 + hA.z * aR11) return false;

	return true;
}

// 楕円とOBBの当たり判定（Y軸回転のみ対応）
static bool Resolve_Ellipsoid_OBB_Yaw(
	const XMFLOAT3& ellC, const XMFLOAT3& ellR,
	const XMFLOAT3& boxC, const XMFLOAT3& boxH, float boxYaw,
	XMFLOAT3* outPush, XMFLOAT3* outNormal)
{
	if (outPush) *outPush = { 0,0,0 };
	if (outNormal) *outNormal = { 0,1,0 };

	XMFLOAT3 invR = { 1.0f / ellR.x, 1.0f / ellR.y, 1.0f / ellR.z };
	XMFLOAT3 cS = Mul(ellC, invR);
	XMFLOAT3 bS = Mul(boxC, invR);
	XMFLOAT3 hS = Mul(boxH, invR);

	float yawRad = XMConvertToRadians(boxYaw);

	XMFLOAT3 d = cS - bS;
	XMFLOAT3 dl = RotateY(d, -yawRad);
	dl.x = Clamp(dl.x, -hS.x, hS.x);
	dl.y = Clamp(dl.y, -hS.y, hS.y);
	dl.z = Clamp(dl.z, -hS.z, hS.z);
	XMFLOAT3 qS = bS + RotateY(dl, yawRad);

	XMFLOAT3 dS = cS - qS;
	float dist = Length(dS);

	if (dist >= 1.0f) return false;

	XMFLOAT3 nS;
	float pen;

	if (dist > 1e-6f)
	{
		nS = dS * (1.0f / dist);
		pen = 1.0f - dist;
	}
	else
	{
		XMFLOAT3 lp = RotateY(cS - bS, -yawRad);
		float dx = hS.x - fabsf(lp.x);
		float dy = hS.y - fabsf(lp.y);
		float dz = hS.z - fabsf(lp.z);

		XMFLOAT3 ln = { 0,1,0 };
		if (dx <= dy && dx <= dz) { ln = { (lp.x >= 0) ? 1.0f : -1.0f, 0, 0 }; pen = 1.0f + dx; }
		else if (dy <= dz) { ln = { 0, (lp.y >= 0) ? 1.0f : -1.0f, 0 }; pen = 1.0f + dy; }
		else { ln = { 0, 0, (lp.z >= 0) ? 1.0f : -1.0f }; pen = 1.0f + dz; }

		nS = RotateY(ln, yawRad);
	}

	XMFLOAT3 pushS = nS * pen;
	XMFLOAT3 pushW = { pushS.x * ellR.x, pushS.y * ellR.y, pushS.z * ellR.z };
	XMFLOAT3 nW = Normalize(XMFLOAT3{ nS.x * ellR.x, nS.y * ellR.y, nS.z * ellR.z });

	if (outPush) *outPush = pushW;
	if (outNormal) *outNormal = nW;
	return true;
}

// プレイヤーのトリガーが当たった面を計算
static TRIGGER_SIDE CalcTriggerSide(const XMFLOAT3& playerC, const XMFLOAT3& targetC)
{
	PLAYER* p = GetPlayer3D();
	if (!p) return TRIGGER_SIDE_NONE;

	float yaw = XMConvertToRadians(p->Rotation.y);
	XMFLOAT3 fwd = Normalize2D({ sinf(yaw), 0, cosf(yaw) });
	XMFLOAT3 right = { fwd.z, 0, -fwd.x };
	XMFLOAT3 to = { targetC.x - playerC.x, 0, targetC.z - playerC.z };

	float f = Dot2D(to, fwd);
	float r = Dot2D(to, right);

	if (fabsf(f) < 1e-5f && fabsf(r) < 1e-5f) return TRIGGER_SIDE_NONE;
	if (fabsf(f) >= fabsf(r)) return (f >= 0) ? TRIGGER_SIDE_FRONT : TRIGGER_SIDE_BACK;
	return (r >= 0) ? TRIGGER_SIDE_RIGHT : TRIGGER_SIDE_LEFT;
}

// シャドウの当たり判定のデバッグ描画オプションを設定
void Collision_SetShadowDebugOptions(const ShadowDebugOptions& options)
{
	g_ShadowDebugOpts = options;
}
// シャドウの当たり判定のデバッグ描画オプションを取得
void Collision_DebugClearExtraBoxes()
{
	g_ExtraBoxes.clear();
}


void Collision_DebugAddExtraAABB(const XMFLOAT3& c, const XMFLOAT3& h,
	unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	ExtraDebugBox box;
	box.isOBB = false;
	box.center = c;
	box.half = h;
	box.color = MakeColor(r, g, b, a);
	g_ExtraBoxes.push_back(box);
}

void Collision_DebugAddExtraOBB(const XMFLOAT3& c, const XMFLOAT3& h, const XMFLOAT3& rot,
	unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	ExtraDebugBox box;
	box.isOBB = true;
	box.center = c;
	box.half = h;
	box.rotDeg = rot;
	box.color = MakeColor(r, g, b, a);
	g_ExtraBoxes.push_back(box);
}

bool Resolve_OBB_OBB_ZY(
	const XMFLOAT3& posA, const XMFLOAT3& halfA, float rotZRadA,
	const XMFLOAT3& posB, const XMFLOAT3& halfB, float rotYDegB,
	XMFLOAT3* outPush, XMFLOAT3* outNorm)
{
	XMMATRIX rotMatA = XMMatrixRotationZ(rotZRadA);

	float rotYRadB = XMConvertToRadians(rotYDegB);
	XMMATRIX rotMatB = XMMatrixRotationY(rotYRadB);

	XMFLOAT3 axesA[3], axesB[3];

	XMVECTOR axA0 = XMVector3TransformNormal(XMVectorSet(1, 0, 0, 0), rotMatA);
	XMVECTOR axA1 = XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), rotMatA);
	XMVECTOR axA2 = XMVector3TransformNormal(XMVectorSet(0, 0, 1, 0), rotMatA);
	XMStoreFloat3(&axesA[0], axA0);
	XMStoreFloat3(&axesA[1], axA1);
	XMStoreFloat3(&axesA[2], axA2);

	XMVECTOR axB0 = XMVector3TransformNormal(XMVectorSet(1, 0, 0, 0), rotMatB);
	XMVECTOR axB1 = XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), rotMatB);
	XMVECTOR axB2 = XMVector3TransformNormal(XMVectorSet(0, 0, 1, 0), rotMatB);
	XMStoreFloat3(&axesB[0], axB0);
	XMStoreFloat3(&axesB[1], axB1);
	XMStoreFloat3(&axesB[2], axB2);

	XMVECTOR vD = XMVectorSet(posA.x - posB.x, posA.y - posB.y, posA.z - posB.z, 0);

	float halfExtA[3] = { halfA.x, halfA.y, halfA.z };
	float halfExtB[3] = { halfB.x, halfB.y, halfB.z };

	float minPen = FLT_MAX;
	XMFLOAT3 minAxis = { 0, 1, 0 };

	auto TestAxis = [&](XMVECTOR axis) -> bool
		{
			float len = XMVectorGetX(XMVector3Length(axis));
			if (len < 1e-6f) return true;

			axis = XMVector3Normalize(axis);

			float rA = 0, rB = 0;
			for (int i = 0; i < 3; i++)
			{
				XMVECTOR axi = XMLoadFloat3(&axesA[i]);
				rA += halfExtA[i] * fabsf(XMVectorGetX(XMVector3Dot(axis, axi)));
			}
			for (int i = 0; i < 3; i++)
			{
				XMVECTOR axi = XMLoadFloat3(&axesB[i]);
				rB += halfExtB[i] * fabsf(XMVectorGetX(XMVector3Dot(axis, axi)));
			}

			float dist = fabsf(XMVectorGetX(XMVector3Dot(vD, axis)));
			float pen = (rA + rB) - dist;

			if (pen < 0) return false;

			if (pen < minPen)
			{
				minPen = pen;
				XMStoreFloat3(&minAxis, axis);
			}
			return true;
		};

	if (!TestAxis(axA0)) return false;
	if (!TestAxis(axA1)) return false;
	if (!TestAxis(axA2)) return false;

	if (!TestAxis(axB0)) return false;
	if (!TestAxis(axB1)) return false;
	if (!TestAxis(axB2)) return false;

	if (!TestAxis(XMVector3Cross(axA0, axB0))) return false;
	if (!TestAxis(XMVector3Cross(axA0, axB1))) return false;
	if (!TestAxis(XMVector3Cross(axA0, axB2))) return false;
	if (!TestAxis(XMVector3Cross(axA1, axB0))) return false;
	if (!TestAxis(XMVector3Cross(axA1, axB1))) return false;
	if (!TestAxis(XMVector3Cross(axA1, axB2))) return false;
	if (!TestAxis(XMVector3Cross(axA2, axB0))) return false;
	if (!TestAxis(XMVector3Cross(axA2, axB1))) return false;
	if (!TestAxis(XMVector3Cross(axA2, axB2))) return false;

	XMVECTOR normAxis = XMLoadFloat3(&minAxis);
	if (XMVectorGetX(XMVector3Dot(vD, normAxis)) < 0)
	{
		normAxis = XMVectorNegate(normAxis);
	}

	XMVECTOR pushVec = normAxis * minPen;
	XMStoreFloat3(outPush, pushVec);
	XMStoreFloat3(outNorm, XMVector3Normalize(normAxis));

	return true;
}

bool OBB_Intersect_ZY(
	const XMFLOAT3& posA, const XMFLOAT3& halfA, float rotZRadA,
	const XMFLOAT3& posB, const XMFLOAT3& halfB, float rotYDegB)
{
	XMFLOAT3 push, norm;
	return Resolve_OBB_OBB_ZY(posA, halfA, rotZRadA, posB, halfB, rotYDegB, &push, &norm);
}


static XMFLOAT3 ClosestPointOnSegment(const XMFLOAT3& p, const XMFLOAT3& a, const XMFLOAT3& b)
{
	XMFLOAT3 ab = { b.x - a.x, b.y - a.y, b.z - a.z };
	XMFLOAT3 ap = { p.x - a.x, p.y - a.y, p.z - a.z };

	float abLenSq = ab.x * ab.x + ab.y * ab.y + ab.z * ab.z;
	if (abLenSq < 1e-8f)
		return a;

	float t = (ap.x * ab.x + ap.y * ab.y + ap.z * ab.z) / abLenSq;
	t = Clamp(t, 0.0f, 1.0f);

	return XMFLOAT3(
		a.x + ab.x * t,
		a.y + ab.y * t,
		a.z + ab.z * t
	);
}

static float ClosestParamOnSegment(const XMFLOAT3& p, const XMFLOAT3& a, const XMFLOAT3& b)
{
	XMFLOAT3 ab = b - a;
	XMFLOAT3 ap = p - a;
	float abLenSq = Dot(ab, ab);
	if (abLenSq < 1e-8f) return 0.0f;
	return Clamp(Dot(ap, ab) / abLenSq, 0.0f, 1.0f);
}

static XMFLOAT3 ClosestPointOnOBB_Full(
	const XMFLOAT3& point,
	const XMFLOAT3& boxC, const XMFLOAT3& boxH, float boxYawDeg)
{
	float yawRad = XMConvertToRadians(boxYawDeg);
	float cy = cosf(yawRad), sy = sinf(yawRad);

	XMFLOAT3 axisX = { cy, 0, sy };
	XMFLOAT3 axisY = { 0, 1, 0 };
	XMFLOAT3 axisZ = { -sy, 0, cy };

	XMFLOAT3 d = point - boxC;
	float projX = Clamp(Dot(d, axisX), -boxH.x, boxH.x);
	float projY = Clamp(Dot(d, axisY), -boxH.y, boxH.y);
	float projZ = Clamp(Dot(d, axisZ), -boxH.z, boxH.z);

	return boxC + axisX * projX + axisY * projY + axisZ * projZ;
}

static float SegmentToOBB_Closest(
	const XMFLOAT3& segA, const XMFLOAT3& segB,
	const XMFLOAT3& boxC, const XMFLOAT3& boxH, float boxYawDeg,
	XMFLOAT3* outSegPoint, XMFLOAT3* outBoxPoint)
{
	XMFLOAT3 segDir = segB - segA;

	float t = 0.5f;
	XMFLOAT3 bestSeg, bestBox;
	float bestDistSq = FLT_MAX;

	for (int iter = 0; iter < 8; ++iter)
	{
		XMFLOAT3 segPt = segA + segDir * t;
		XMFLOAT3 boxPt = ClosestPointOnOBB_Full(segPt, boxC, boxH, boxYawDeg);
		float tNew = ClosestParamOnSegment(boxPt, segA, segB);
		XMFLOAT3 segPtNew = segA + segDir * tNew;

		XMFLOAT3 diff = segPtNew - boxPt;
		float distSq = Dot(diff, diff);

		if (distSq < bestDistSq)
		{
			bestDistSq = distSq;
			bestSeg = segPtNew;
			bestBox = boxPt;
		}

		if (fabsf(tNew - t) < 1e-5f) break;
		t = tNew;
	}

	for (int e = 0; e < 2; ++e)
	{
		XMFLOAT3 segPt = (e == 0) ? segA : segB;
		XMFLOAT3 boxPt = ClosestPointOnOBB_Full(segPt, boxC, boxH, boxYawDeg);
		XMFLOAT3 diff = segPt - boxPt;
		float distSq = Dot(diff, diff);
		if (distSq < bestDistSq)
		{
			bestDistSq = distSq;
			bestSeg = segPt;
			bestBox = boxPt;
		}
	}

	if (outSegPoint) *outSegPoint = bestSeg;
	if (outBoxPoint) *outBoxPoint = bestBox;
	return sqrtf(bestDistSq);
}

static XMFLOAT3 GetOBBEscapeNormal(
	const XMFLOAT3& point,
	const XMFLOAT3& boxC, const XMFLOAT3& boxH, float boxYawDeg,
	float* outPenetration)
{
	float yawRad = XMConvertToRadians(boxYawDeg);
	float cy = cosf(yawRad), sy = sinf(yawRad);

	XMFLOAT3 axes[3] = {
		{ cy, 0, sy },
		{ 0, 1, 0 },
		{ -sy, 0, cy }
	};
	float halfs[3] = { boxH.x, boxH.y, boxH.z };

	XMFLOAT3 d = point - boxC;
	float minPen = FLT_MAX;
	XMFLOAT3 bestNormal = { 0, 1, 0 };

	for (int i = 0; i < 3; ++i)
	{
		float proj = Dot(d, axes[i]);
		float pen = halfs[i] - fabsf(proj);
		if (pen < minPen)
		{
			minPen = pen;
			bestNormal = axes[i] * ((proj >= 0) ? 1.0f : -1.0f);
		}
	}

	if (outPenetration) *outPenetration = minPen;
	return bestNormal;
}

static void ClosestPointsOnSegments(
	const XMFLOAT3& a0, const XMFLOAT3& a1,
	const XMFLOAT3& b0, const XMFLOAT3& b1,
	XMFLOAT3& closestA, XMFLOAT3& closestB)
{
	XMFLOAT3 d1 = a1 - a0;
	XMFLOAT3 d2 = b1 - b0;
	XMFLOAT3 r = a0 - b0;

	float a = Dot(d1, d1);
	float e = Dot(d2, d2);
	float f = Dot(d2, r);

	float s, t;

	if (a < 1e-8f && e < 1e-8f)
	{
		s = t = 0.0f;
	}
	else if (a < 1e-8f)
	{
		s = 0.0f;
		t = Clamp(f / e, 0.0f, 1.0f);
	}
	else
	{
		float c = Dot(d1, r);
		if (e < 1e-8f)
		{
			t = 0.0f;
			s = Clamp(-c / a, 0.0f, 1.0f);
		}
		else
		{
			float b = Dot(d1, d2);
			float denom = a * e - b * b;

			if (denom != 0.0f)
				s = Clamp((b * f - c * e) / denom, 0.0f, 1.0f);
			else
				s = 0.0f;

			t = (b * s + f) / e;

			if (t < 0.0f)
			{
				t = 0.0f;
				s = Clamp(-c / a, 0.0f, 1.0f);
			}
			else if (t > 1.0f)
			{
				t = 1.0f;
				s = Clamp((b - c) / a, 0.0f, 1.0f);
			}
		}
	}

	closestA = a0 + d1 * s;
	closestB = b0 + d2 * t;
}

static XMFLOAT3 ClosestPointOnOBB(
	const XMFLOAT3& point,
	const XMFLOAT3& boxC, const XMFLOAT3& boxH, float boxYawDeg)
{
	float yawRad = XMConvertToRadians(boxYawDeg);
	float cosY = cosf(yawRad);
	float sinY = sinf(yawRad);

	XMFLOAT3 d = point - boxC;
	XMFLOAT3 localP = {
		d.x * cosY + d.z * sinY,
		d.y,
		-d.x * sinY + d.z * cosY
	};

	XMFLOAT3 clamped = {
		Clamp(localP.x, -boxH.x, boxH.x),
		Clamp(localP.y, -boxH.y, boxH.y),
		Clamp(localP.z, -boxH.z, boxH.z)
	};

	return XMFLOAT3{
		boxC.x + clamped.x * cosY - clamped.z * sinY,
		boxC.y + clamped.y,
		boxC.z + clamped.x * sinY + clamped.z * cosY
	};
}


bool Resolve_Capsule2D_OBB(
	const Capsule2D& capsule,
	const XMFLOAT3& boxCenter, const XMFLOAT3& boxHalf, float boxYawDeg,
	XMFLOAT3* outPush, XMFLOAT3* outNormal)
{
	if (outPush) *outPush = { 0, 0, 0 };
	if (outNormal) *outNormal = { 0, 1, 0 };

	XMFLOAT3 capTop = capsule.GetTop();
	XMFLOAT3 capBottom = capsule.GetBottom();

	XMFLOAT3 boundHalf = capsule.GetBoundingHalfSize();
	float dx = fabsf(capsule.center.x - boxCenter.x);
	float dy = fabsf(capsule.center.y - boxCenter.y);
	float dz = fabsf(capsule.center.z - boxCenter.z);
	if (dx > boundHalf.x + boxHalf.x + 0.1f ||
		dy > boundHalf.y + boxHalf.y + 0.1f ||
		dz > boundHalf.z + boxHalf.z + 0.1f)
		return false;

	XMFLOAT3 closestSeg, closestBox;
	float dist = SegmentToOBB_Closest(capTop, capBottom, boxCenter, boxHalf, boxYawDeg,
		&closestSeg, &closestBox);

	const float kContactEps = 0.01f;
	if (dist > capsule.radius + kContactEps)
		return false;

	bool isPenetrating = (dist < capsule.radius);

	float penetration;
	XMFLOAT3 normal;

	if (dist > 1e-5f)
	{
		normal = Normalize(closestSeg - closestBox);
		penetration = isPenetrating ? (capsule.radius - dist) : 0.0f;
	}
	else
	{

		normal = GetOBBEscapeNormal(closestSeg, boxCenter, boxHalf, boxYawDeg, &penetration);
		penetration += capsule.radius;
	}

	if (outPush) *outPush = normal * penetration;
	if (outNormal) *outNormal = normal;
	return true;
}

bool Capsule2D_Intersect_OBB(
	const Capsule2D& capsule,
	const XMFLOAT3& boxCenter, const XMFLOAT3& boxHalf, float boxYawDeg)
{
	XMFLOAT3 push, normal;
	return Resolve_Capsule2D_OBB(capsule, boxCenter, boxHalf, boxYawDeg, &push, &normal);
}

//=========================================================================================================
// 3Dプレイヤーの当たり判定
//=========================================================================================================

// 3Dプレイヤーとフィールドの当たり判定を行い、当たった面を返す
int Player3DField_Collision()
{
	int hit = HIT_NONE;

	PLAYER* player = GetPlayer3D();
	std::vector<MAPDATA>& map = GetFieldMap();
	if (!player || map.empty()) return hit;

	player->isGround = false;
	XMFLOAT3 ellR = Player3D_GetSolidHalfSize();
	XMFLOAT3 ellC = GetPlayerSolidCollider();

	for (size_t i = 0; i < map.size(); ++i)
	{
		if (!Field_IsSolid(map[i].no)) continue;

		// シーソーは片方だけ当たり判定を行う
		if (map[i].no == FIELD_SEESAW_2) continue;

		float boxYaw = (map[i].no == FIELD_OBJ_1) ? map[i].rotate.y : 0.0f;

		XMFLOAT3 push, norm;
		// 楕円とOBBの当たり判定を行い、衝突している場合は押し出しベクトルと法線を取得
		if (!Resolve_Ellipsoid_OBB_Yaw(ellC, ellR, map[i].pos,
			Field_GetCollisionHalfSize(map[i]), boxYaw, &push, &norm))
			continue;

		ellC = ellC + push;

		float ax = fabsf(norm.x), ay = fabsf(norm.y), az = fabsf(norm.z);

		if (ay >= ax && ay >= az)
		{
			if (norm.y > 0) { player->isGround = true; player->Velocity.y = 0; hit = HIT_GROUND; }
			else if (player->Velocity.y > 0) player->Velocity.y = 0;
		}
		else if (ax >= az)
		{
			player->Velocity.x = 0;
			hit = (norm.x > 0) ? HIT_WALL_PlusX : HIT_WALL_NegX;
		}
		else
		{
			player->Velocity.z = 0;
			hit = (norm.z > 0) ? HIT_WALL_PlusZ : HIT_WALL_NegZ;
		}
	}

	player->Position.x = ellC.x;
	player->Position.y = ellC.y - ellR.y;
	player->Position.z = ellC.z;

	return hit;
}


//=========================================================================================================
// 2Dプレイヤーの当たり判定
//=========================================================================================================

// 2Dプレイヤーのトリガーが当たった面を計算
static TRIGGER_SIDE CalcTriggerSide2D(const XMFLOAT3& playerPos, float playerRotZ, const XMFLOAT3& triggerPos)
{

	float dx = triggerPos.x - playerPos.x;
	float dy = triggerPos.y - playerPos.y;

	float radZ = XMConvertToRadians(playerRotZ);
	float cosZ = cosf(-radZ);
	float sinZ = sinf(-radZ);

	float localX = dx * cosZ - dy * sinZ;
	float localY = dx * sinZ + dy * cosZ;

	if (fabsf(localX) > fabsf(localY))
	{
		return (localX > 0) ? TRIGGER_SIDE_RIGHT : TRIGGER_SIDE_LEFT;
	}
	else
	{
		return (localY > 0) ? TRIGGER_SIDE_TOP : TRIGGER_SIDE_BOTTOM;
	}
}

// 2Dプレイヤーとフィールドの当たり判定を行い、当たった面を返す
int Player2DField_Collision()
{
	int hit = HIT_NONE;

	PLAYER* player = GetPlayer2D();
	std::vector<MAPDATA>& map = GetFieldMap();
	if (!player || map.empty()) return hit;

	player->isGround = false;
	Capsule2D capsule = Player2D_GetCapsule();
	const int MAX_ITERATIONS = 4;

	for (int iter = 0; iter < MAX_ITERATIONS; ++iter)
	{
		bool hitThisIter = false;

		for (size_t i = 0; i < map.size(); ++i)
		{
			if (!Field_IsSolid(map[i].no)) continue;

			XMFLOAT3 push, norm;

			if (!Resolve_Capsule2D_OBB(
				capsule,
				map[i].pos, Field_GetCollisionHalfSize(map[i]), map[i].rotate.y,
				&push, &norm))
				continue;

			if (fabsf(push.x) > 1e-8f || fabsf(push.y) > 1e-8f || fabsf(push.z) > 1e-8f)
			{
				capsule.center = capsule.center + push;
				hitThisIter = true;
			}

			float ax = fabsf(norm.x), ay = fabsf(norm.y), az = fabsf(norm.z);

			if (ay >= ax && ay >= az)
			{
				if (norm.y > 0)
				{
					player->isGround = true;
					player->Velocity.y = 0;
					hit = HIT_GROUND;
				}
				else if (player->Velocity.y > 0)
				{
					player->Velocity.y = 0;
				}
			}
			else if (ax >= az)
			{
				player->Velocity.x = 0;
				hit = (norm.x > 0) ? HIT_WALL_PlusX : HIT_WALL_NegX;
			}
			else
			{
				player->Velocity.z = 0;
				hit = (norm.z > 0) ? HIT_WALL_PlusZ : HIT_WALL_NegZ;
			}
		}

		if (!hitThisIter) break;
	}

	float totalHeight = PLAYER2D_CAPSULE_HEIGHT + PLAYER2D_CAPSULE_RADIUS * 2.0f;
	player->Position.x = capsule.center.x;
	player->Position.y = capsule.center.y - totalHeight * 0.5f;
	player->Position.z = capsule.center.z;

	return hit;
}

//=========================================================================================================
// プレイヤーのトリガー判定
//=========================================================================================================

// 3Dプレイヤーとフィールドのトリガーの当たり判定
bool Collision_PlayerTrigger(TRIGGER_HIT* outHit, float extraRange)
{
	if (outHit) *outHit = TRIGGER_HIT{};

	PLAYER* p = GetPlayer3D();
	if (!p) return false;

	auto& map = GetFieldMap();
	if (map.empty()) return false;

	XMFLOAT3 pHalf = Player3D_GetTriggerHalfSize();
	XMFLOAT3 pC = GetPlayerTriggerCollider();
	float pYaw = p->Rotation.y;

	bool found = false;
	float bestD2 = 1e30f;
	TRIGGER_HIT best;

	for (size_t i = 0; i < map.size(); ++i)
	{
		if (!Field_IsTrigger(map[i].no)) continue;

		XMFLOAT3 tHalf = Field_GetCollisionHalfSize(map[i]);
		tHalf.x += extraRange; tHalf.y += extraRange; tHalf.z += extraRange;

		if (!OBB_Intersect_Yaw(pC, pHalf, pYaw, map[i].pos, tHalf, map[i].rotate.y))
			continue;

		float dx = map[i].pos.x - pC.x;
		float dz = map[i].pos.z - pC.z;
		float d2 = dx * dx + dz * dz;

		if (!found || d2 < bestD2)
		{
			found = true;
			bestD2 = d2;
			best.hit = true;
			best.mapIndex = i;
			best.type = map[i].no;
			best.side = CalcTriggerSide(pC, map[i].pos);
		}
	}

	if (!found) return false;
	if (outHit) *outHit = best;
	return true;
}

// 2Dプレイヤーとフィールドのトリガーの当たり判定
bool Collision_Player2DTrigger(TRIGGER_HIT* outHit, float extraRange)
{
	if (outHit) *outHit = TRIGGER_HIT{};

	PLAYER* p = GetPlayer2D();
	if (!p) return false;

	auto& map = GetFieldMap();
	if (map.empty()) return false;

	Capsule2D capsule = Player2D_GetCapsule();

	Capsule2D expandedCapsule = capsule;
	expandedCapsule.radius += extraRange;

	bool found = false;
	float bestD2 = 1e30f;
	TRIGGER_HIT best;

	for (size_t i = 0; i < map.size(); ++i)
	{
		if (!Field_IsTrigger(map[i].no)) continue;

		XMFLOAT3 tHalf = Field_GetCollisionHalfSize(map[i]);

		if (!Capsule2D_Intersect_OBB(expandedCapsule, map[i].pos, tHalf, map[i].rotate.y))
			continue;

		float dx = map[i].pos.x - capsule.center.x;
		float dy = map[i].pos.y - capsule.center.y;
		float d2 = dx * dx + dy * dy;

		if (!found || d2 < bestD2)
		{
			found = true;
			bestD2 = d2;
			best.hit = true;
			best.mapIndex = (int)i;
			best.type = map[i].no;
			best.side = CalcTriggerSide2D(capsule.center, p->Rotation.z, map[i].pos);
		}
	}

	if (!found) return false;
	if (outHit) *outHit = best;
	return true;
}


//=========================================================================================================
// 2Dプレイヤーのシャドウの当たり判定
//=========================================================================================================

// 2DプレイヤーのXZ平面での点と多角形の内外判定
static bool PointInPolygon2D(float px, float py, const std::vector<XMFLOAT2>& poly)
{
	if (poly.size() < 3) return false;

	bool inside = false;
	int n = (int)poly.size();

	for (int i = 0, j = n - 1; i < n; j = i++)
	{
		float xi = poly[i].x, yi = poly[i].y;
		float xj = poly[j].x, yj = poly[j].y;

		if (((yi > py) != (yj > py)) &&
			(px < (xj - xi) * (py - yi) / (yj - yi) + xi))
		{
			inside = !inside;
		}
	}
	return inside;
}
// 2DプレイヤーのXZ平面でのドット積
static float DotXZ(const XMFLOAT3& a, const XMFLOAT3& b)
{
	return a.x * b.x + a.z * b.z;
}
// シャドウの辺に対するプレイヤーの半径の投影を計算（Y軸回転のみ対応）
static float ProjectRadiusOnAxis_Yaw(const XMFLOAT3& axis, const XMFLOAT3& half, float yawDeg)
{
	const float yaw = XMConvertToRadians(yawDeg);
	const XMFLOAT3 right = { cosf(yaw), 0.0f, -sinf(yaw) };
	const XMFLOAT3 fwd = { sinf(yaw), 0.0f,  cosf(yaw) };
	const XMFLOAT3 up = { 0.0f, 1.0f, 0.0f };

	return fabsf(Dot(axis, right)) * half.x
		+ fabsf(Dot(axis, up)) * half.y
		+ fabsf(Dot(axis, fwd)) * half.z;
}

static float PointToPolygonEdgeDist2D(
	const XMFLOAT2& p, const std::vector<XMFLOAT2>& poly,
	XMFLOAT2* outClosest, XMFLOAT2* outEdgeNormal)
{
	float bestDistSq = FLT_MAX;
	XMFLOAT2 bestClosest = p;
	XMFLOAT2 bestNormal = { 0, 1 };
	int n = (int)poly.size();

	for (int i = 0; i < n; ++i)
	{
		XMFLOAT2 a = poly[i];
		XMFLOAT2 b = poly[(i + 1) % n];

		XMFLOAT2 ab = { b.x - a.x, b.y - a.y };
		XMFLOAT2 ap = { p.x - a.x, p.y - a.y };

		float abLenSq = ab.x * ab.x + ab.y * ab.y;
		float t = 0.0f;
		if (abLenSq > 1e-8f)
			t = Clamp((ap.x * ab.x + ap.y * ab.y) / abLenSq, 0.0f, 1.0f);

		XMFLOAT2 closest = { a.x + ab.x * t, a.y + ab.y * t };
		float dx = p.x - closest.x;
		float dy = p.y - closest.y;
		float distSq = dx * dx + dy * dy;

		if (distSq < bestDistSq)
		{
			bestDistSq = distSq;
			bestClosest = closest;

			float eLen = sqrtf(abLenSq);
			if (eLen > 1e-6f)
			{
				bestNormal = { -ab.y / eLen, ab.x / eLen };
			}
		}
	}

	if (outClosest) *outClosest = bestClosest;
	if (outEdgeNormal) *outEdgeNormal = bestNormal;
	return sqrtf(bestDistSq);
}

static float SignedDistToPolygon2D(
	const XMFLOAT2& p, const std::vector<XMFLOAT2>& poly,
	XMFLOAT2* outNormal)
{
	int n = (int)poly.size();
	if (n < 3) return FLT_MAX;

	float maxSignedDist = -FLT_MAX;
	int closestEdge = 0;

	for (int i = 0; i < n; ++i)
	{
		XMFLOAT2 a = poly[i];
		XMFLOAT2 b = poly[(i + 1) % n];
		XMFLOAT2 e = { b.x - a.x, b.y - a.y };
		float eLen = sqrtf(e.x * e.x + e.y * e.y);
		if (eLen < 1e-6f) continue;

		XMFLOAT2 nIn = { -e.y / eLen, e.x / eLen };
		XMFLOAT2 ap = { p.x - a.x, p.y - a.y };
		float dist = ap.x * nIn.x + ap.y * nIn.y;

		if (-dist > maxSignedDist)
		{
			maxSignedDist = -dist;
			closestEdge = i;
		}
	}

	XMFLOAT2 a = poly[closestEdge];
	XMFLOAT2 b = poly[(closestEdge + 1) % n];
	XMFLOAT2 e = { b.x - a.x, b.y - a.y };
	float eLen = sqrtf(e.x * e.x + e.y * e.y);
	if (eLen > 1e-6f && outNormal)
	{
		*outNormal = { -e.y / eLen, e.x / eLen };
	}

	if (maxSignedDist <= 0.0f)
	{
		XMFLOAT2 dummy;
		float dist = PointToPolygonEdgeDist2D(p, poly, &dummy, outNormal);
		return -dist;
	}
	else
	{
		XMFLOAT2 dummy;
		return PointToPolygonEdgeDist2D(p, poly, &dummy, outNormal);
	}
}


bool Player2DShadow_Collision()
{
	PLAYER* player = GetPlayer2D();
	if (!player) return false;

	const std::vector<const ShadowPrism*>& prisms = Collision_GetShadowPrisms();
	if (prisms.empty()) return false;

	Capsule2D capsule = Player2D_GetCapsule();

	const float skin = 0.01f;
	const float eps = 1e-6f;

	XMFLOAT3 dXZ = { player->Velocity.x, 0.0f, player->Velocity.z };
	bool hitAny = false;

	s_FrameSlopeDeltaY = 0.0f;
	s_DbgMoveInXZ = dXZ;
	s_DbgMoveOutXZ = dXZ;
	s_DbgMoveSlope = { 0,0,0 };

	if (s_HasStandDir && s_LastStandingPrismIndex >= 0 && player->Velocity.y <= 0.02f)
	{
		float dl = sqrtf(dXZ.x * dXZ.x + dXZ.z * dXZ.z);
		if (dl > 1e-6f)
		{
			float d = dXZ.x * s_StandDirXZ.x + dXZ.z * s_StandDirXZ.z;
			dXZ.x = s_StandDirXZ.x * d;
			dXZ.z = s_StandDirXZ.z * d;
		}
	}

	auto Area2 = [](const std::vector<XMFLOAT2>& p) -> float {
		float a = 0.0f;
		int n = (int)p.size();
		for (int i = 0, j = n - 1; i < n; j = i++)
			a += p[j].x * p[i].y - p[i].x * p[j].y;
		return a;
		};
	auto Dot2 = [](const XMFLOAT2& a, const XMFLOAT2& b) { return a.x * b.x + a.y * b.y; };
	auto Sub2 = [](const XMFLOAT2& a, const XMFLOAT2& b) -> XMFLOAT2 { return { a.x - b.x, a.y - b.y }; };
	auto Len2 = [](const XMFLOAT2& v) { return sqrtf(v.x * v.x + v.y * v.y); };

	auto CapsuleProjection = [&](const XMFLOAT3& axis) -> float {
		XMFLOAT3 capAxis = {
			-sinf(capsule.rotationZ),
			cosf(capsule.rotationZ),
			0.0f
		};
		return capsule.radius + fabsf(Dot(axis, capAxis)) * capsule.halfHeight;
		};


	for (int prismIdx = 0; prismIdx < (int)prisms.size(); ++prismIdx)
	{
		const ShadowPrism* prism = prisms[prismIdx];
		if (!prism || !prism->isValid) continue;
		if (prism->poly.size() < 3) continue;

		if (s_StandSegValid && s_StandSegWalkable && prismIdx == s_LastStandingPrismIndex && player->Velocity.y <= 0.02f)
			continue;

		if (prism->n.y > 0.70f) continue;

		const float moveLen = sqrtf(dXZ.x * dXZ.x + dXZ.z * dXZ.z);
		const float gate = capsule.radius + capsule.halfHeight + moveLen + 0.3f;

		if (capsule.center.x < prism->aabbMin.x - gate || capsule.center.x > prism->aabbMax.x + gate ||
			capsule.center.z < prism->aabbMin.z - gate || capsule.center.z > prism->aabbMax.z + gate)
			continue;

		std::vector<XMFLOAT2> poly = prism->poly;
		if (Area2(poly) < 0.0f)
			std::reverse(poly.begin(), poly.end());

		const XMFLOAT3 rel = capsule.center - prism->origin;
		const float u0 = Dot(rel, prism->u);
		const float v0 = Dot(rel, prism->v);
		const float n0 = Dot(rel, prism->n);
		const XMFLOAT2 p0 = { u0, v0 };

		const float rN = CapsuleProjection(prism->n);
		const bool overlapN = (n0 >= -rN) && (n0 <= prism->thickness + rN);

		if (!overlapN) continue;

		{
			const float bottomN = n0 - rN;
			if (prism->n.y > 0.7f && bottomN >= prism->thickness - 0.02f)
				continue;
		}

		if (moveLen < eps) continue;

		XMFLOAT2 dp = { DotXZ(dXZ, prism->u), DotXZ(dXZ, prism->v) };
		if (fabsf(dp.x) + fabsf(dp.y) < 1e-7f) continue;

		bool insideExp0 = true;
		for (int i = 0; i < (int)poly.size(); ++i)
		{
			const XMFLOAT2 a = poly[i];
			const XMFLOAT2 b = poly[(i + 1) % (int)poly.size()];
			const XMFLOAT2 e = { b.x - a.x, b.y - a.y };

			XMFLOAT2 nIn = { -e.y, e.x };
			float nl = Len2(nIn);
			if (nl < 1e-6f) continue;
			nIn.x /= nl; nIn.y /= nl;

			const XMFLOAT3 axisW = prism->u * nIn.x + prism->v * nIn.y;
			const float rEdge = CapsuleProjection(axisW) + skin;

			const float s0 = Dot2(nIn, Sub2(p0, a));
			if (s0 < -rEdge) { insideExp0 = false; break; }
		}

		if (insideExp0)
		{
			for (int pass = 0; pass < 3; ++pass)
			{
				XMFLOAT2 dpNow = { DotXZ(dXZ, prism->u), DotXZ(dXZ, prism->v) };
				if (fabsf(dpNow.x) + fabsf(dpNow.y) < 1e-7f) break;

				bool changed = false;
				for (int i = 0; i < (int)poly.size(); ++i)
				{
					const XMFLOAT2 a = poly[i];
					const XMFLOAT2 b = poly[(i + 1) % (int)poly.size()];
					const XMFLOAT2 e = { b.x - a.x, b.y - a.y };

					XMFLOAT2 nIn = { -e.y, e.x };
					float nl = Len2(nIn);
					if (nl < 1e-6f) continue;
					nIn.x /= nl; nIn.y /= nl;

					const XMFLOAT3 axisW = prism->u * nIn.x + prism->v * nIn.y;
					const float rEdge = CapsuleProjection(axisW) + skin;

					const float s0 = Dot2(nIn, Sub2(p0, a));
					const float slack = s0 + rEdge;

					if (slack > 0.05f) continue;

					const float vIn = Dot2(nIn, dpNow);
					if (vIn <= 0.0f) continue;

					XMFLOAT3 axisXZ = { axisW.x, 0.0f, axisW.z };
					const float len2xz = axisXZ.x * axisXZ.x + axisXZ.z * axisXZ.z;
					if (len2xz < 1e-6f) continue;

					const float proj = (dXZ.x * axisXZ.x + dXZ.z * axisXZ.z) / len2xz;
					if (proj > 0.0f)
					{
						float clipAmount = proj;
						if (slack > 0.0f)
						{
							float blend = 1.0f - (slack / 0.05f);
							clipAmount *= Clamp(blend, 0.0f, 1.0f);
						}

						dXZ.x -= axisXZ.x * clipAmount;
						dXZ.z -= axisXZ.z * clipAmount;
						changed = true;
						hitAny = true;
					}
				}
				if (!changed) break;
			}
		}
		else
		{
			float tEnter = 0.0f;
			float tExit = 1.0f;
			bool canEnter = true;

			for (int i = 0; i < (int)poly.size(); ++i)
			{
				const XMFLOAT2 a = poly[i];
				const XMFLOAT2 b = poly[(i + 1) % (int)poly.size()];
				const XMFLOAT2 e = { b.x - a.x, b.y - a.y };

				XMFLOAT2 nIn = { -e.y, e.x };
				float nl = Len2(nIn);
				if (nl < 1e-6f) continue;
				nIn.x /= nl; nIn.y /= nl;

				const XMFLOAT3 axisW = prism->u * nIn.x + prism->v * nIn.y;
				const float rEdge = CapsuleProjection(axisW) + skin;

				const float s0 = Dot2(nIn, Sub2(p0, a));
				const float sv = Dot2(nIn, dp);

				if (fabsf(sv) < 1e-6f)
				{
					if (s0 < -rEdge) { canEnter = false; break; }
					continue;
				}

				float t = (-rEdge - s0) / sv;

				if (sv > 0.0f)
				{
					if (t > tEnter) tEnter = t;
				}
				else
				{
					if (t < tExit) tExit = t;
				}

				if (tEnter > tExit) { canEnter = false; break; }
			}

			if (canEnter && tEnter >= 0.0f && tEnter <= 1.0f)
			{
				const float tStop = (std::max)(0.0f, tEnter - 1e-4f);
				dXZ.x *= tStop;
				dXZ.z *= tStop;
				hitAny = true;
			}
		}
	}

	s_DbgMoveOutXZ = dXZ;
	s_FrameSlopeDeltaY = 0.0f;
	if (s_StandSegValid && s_StandSegWalkable && s_HasStandDir && s_LastStandingPrismIndex >= 0 && player->Velocity.y <= 0.02f && s_StandLenXZ > 1e-6f)
	{
		float amount = dXZ.x * s_StandDirXZ.x + dXZ.z * s_StandDirXZ.z;
		s_FrameSlopeDeltaY = s_StandSegAB.y * (amount / s_StandLenXZ);
	}
	s_DbgMoveSlope = { dXZ.x, s_FrameSlopeDeltaY, dXZ.z };

	player->Velocity.x = dXZ.x;
	player->Velocity.z = dXZ.z;

	return hitAny;
}


bool Player2DShadow_TopContact()
{
	PLAYER* player = GetPlayer2D();
	if (!player) return false;

	const std::vector<const ShadowPrism*>& prisms = Collision_GetShadowPrisms();
	if (prisms.empty()) return false;

	Capsule2D capsule = Player2D_GetCapsule();

	const float kSkin = 0.01f;
	const float kStandWidth = capsule.radius + 0.12f; // How far (in XZ) the player can be from the ledge to land
	const float kContactUp = 0.25f;      // Allow slight gap above ledge
	const float kContactDown = -(capsule.radius + 0.45f); // Allow a bit of penetration / high fall
	const float kJumpEscapeVel = 0.02f;
	const bool  isRising = (player->Velocity.y > kJumpEscapeVel);

	const float kMaxWalkSlopeTan = 57.0f;

	auto Dot3 = [](const XMFLOAT3& a, const XMFLOAT3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; };
	auto LenSqXZ = [](const XMFLOAT3& v) { return v.x * v.x + v.z * v.z; };

	// Capsule projection radius onto an arbitrary axis (same as your wall-collision helper).
	auto CapsuleProjection = [&](const XMFLOAT3& axisW) -> float
		{
			XMFLOAT3 capAxis = { -sinf(capsule.rotationZ), cosf(capsule.rotationZ), 0.0f };
			float axisDot = fabsf(Dot(axisW, capAxis));
			return capsule.radius + axisDot * capsule.halfHeight;
		};

	// Broadphase AABB around capsule
	XMFLOAT3 capHalf = capsule.GetBoundingHalfSize();
	const float aabbPad = 0.30f;
	XMFLOAT3 capMin = capsule.center - (capHalf + XMFLOAT3(aabbPad, aabbPad, aabbPad));
	XMFLOAT3 capMax = capsule.center + (capHalf + XMFLOAT3(aabbPad, aabbPad, aabbPad));

	auto AABBOverlap = [](const XMFLOAT3& aMin, const XMFLOAT3& aMax, const XMFLOAT3& bMin, const XMFLOAT3& bMax) -> bool
		{
			return (aMin.x <= bMax.x && aMax.x >= bMin.x) &&
				(aMin.y <= bMax.y && aMax.y >= bMin.y) &&
				(aMin.z <= bMax.z && aMax.z >= bMin.z);
		};

	// Player foot Y (your Player2D.Position.y is the capsule bottom-most point)
	const float footY = player->Position.y;
	const float footYPrev = footY - player->Velocity.y - s_FrameSlopeDeltaY; // dt==1 in this project

	int bestIdx = -1;
	float bestAbs = 1e9f;
	float bestGroundY = 0.0f;

	XMFLOAT3 bestDirXZ = { 1,0,0 };
	bool bestHasDir = false;

	XMFLOAT3 bestSegA = { 0,0,0 };
	XMFLOAT3 bestSegB = { 0,0,0 };
	XMFLOAT3 bestSegQ = { 0,0,0 };
	XMFLOAT3 bestSegAB = { 1,0,0 };
	float    bestLenXZ = 1.0f;
	bool     bestWalkable = false;

	for (int prismIdx = 0; prismIdx < (int)prisms.size(); ++prismIdx)
	{
		const ShadowPrism* prism = prisms[prismIdx];
		if (!prism || !prism->isValid) continue;
		if (prism->baseWorld.size() < 2) continue;

		XMFLOAT3 pMin = prism->aabbMin - XMFLOAT3(aabbPad, aabbPad, aabbPad);
		XMFLOAT3 pMax = prism->aabbMax + XMFLOAT3(aabbPad, aabbPad, aabbPad);
		if (!AABBOverlap(capMin, capMax, pMin, pMax)) continue;

		// Slab check along prism thickness axis (n)
		XMFLOAT3 rel = capsule.center - prism->origin;
		float n0 = Dot3(rel, prism->n);
		float rN = CapsuleProjection(prism->n);
		const float slabTol = 0.05f;
		if (n0 < -rN - slabTol || n0 > prism->thickness + rN + slabTol)
			continue;

		// Use a depth slice close to where the player currently is.
		float sliceN = n0;
		if (sliceN < 0.0f) sliceN = 0.0f;
		if (sliceN > prism->thickness) sliceN = prism->thickness;
		// Bias to the outer face if the player is already on the outer half.
		/*if (sliceN > prism->thickness * 0.5f)
			sliceN = prism->thickness;*/

		auto ProcessSeg = [&](XMFLOAT3 a, XMFLOAT3 b)
			{
				// Move segment onto the current depth slice (toward the protruded side wall)
				a = a + prism->n * sliceN;
				b = b + prism->n * sliceN;

				// If the segment is almost vertical in world (XZ tiny), it can't act as a ledge.
				XMFLOAT3 e = b - a;
				if (LenSqXZ(e) < 1e-5f) return;

				// Closest point from player (in XZ) to this segment (in XZ)
				XMFLOAT3 p = capsule.center;
				XMFLOAT3 ab = b - a;
				float abxz2 = ab.x * ab.x + ab.z * ab.z;
				if (abxz2 < 1e-6f) return;

				float lenXZ = sqrtf(abxz2);
				float slopeTan = fabsf(ab.y) / (lenXZ + 1e-6f);
				bool  walkable = (slopeTan <= kMaxWalkSlopeTan);

				float t = ((p.x - a.x) * ab.x + (p.z - a.z) * ab.z) / abxz2;
				t = Clamp(t, 0.0f, 1.0f);

				XMFLOAT3 q = a + ab * t;
				float dx = p.x - q.x;
				float dz = p.z - q.z;
				float distXZ = sqrtf(dx * dx + dz * dz);
				if (distXZ > kStandWidth) return;

				float groundY = q.y;
				float distToGround = footY - groundY;

				// Rising: don't snap down onto ledge from below.
				if (isRising && distToGround > 0.0f)
					return;

				bool crossed = (footYPrev >= groundY + kSkin) && (footY <= groundY + kSkin);
				if (!crossed)
				{
					if (distToGround < kContactDown || distToGround > kContactUp)
						return;
				}

				float absDist = fabsf(distToGround);
				if (absDist < bestAbs)
				{
					bestAbs = absDist;
					bestIdx = prismIdx;
					bestGroundY = groundY;

					bestSegA = a;
					bestSegB = b;
					bestSegQ = q;
					bestSegAB = ab;
					bestLenXZ = lenXZ;
					bestWalkable = walkable;

					{
						XMFLOAT3 dir = { ab.x, 0.0f, ab.z };
						float dl = sqrtf(dir.x * dir.x + dir.z * dir.z);
						if (dl > 1e-6f) { dir.x /= dl; dir.z /= dl; bestDirXZ = dir; bestHasDir = true; }
						else { bestHasDir = false; }
					}
				}
			};

		// Preferred: use precomputed ledge segments from Shadow_Build().
		if (!prism->standSegments.empty())
		{
			for (const auto& s : prism->standSegments)
				ProcessSeg(s.a, s.b);
		}
		else
		{
			// Fallback: use all polygon edges if segments weren't generated (degenerate case).
			const int nV = (int)prism->baseWorld.size();
			for (int i = 0; i < nV; ++i)
			{
				ProcessSeg(prism->baseWorld[i], prism->baseWorld[(i + 1) % nV]);
			}
		}
	}

	bool hitAny = false;

	if (bestIdx >= 0)
	{
		// Snap player bottom to groundY.
		float targetY = bestGroundY + kSkin;
		player->Position.y = targetY;
		if (player->Velocity.y < 0.0f)
			player->Velocity.y = 0.0f;
		player->isGround = true;
		hitAny = true;

		s_LastStandingPrismIndex = bestIdx;
		s_GraceFrames = 0;
		s_LastShadowTopPos = { player->Position.x, targetY, player->Position.z };

		s_StandSegValid = true;
		s_StandSegWalkable = bestWalkable;
		s_StandSegA = bestSegA;
		s_StandSegB = bestSegB;
		s_StandSegAB = bestSegAB;
		s_StandClosestQ = bestSegQ;
		s_StandLenXZ = (bestLenXZ > 1e-6f) ? bestLenXZ : 1.0f;

		s_HasStandDir = bestHasDir;
		if (bestHasDir)
		{
			s_StandDirXZ = bestDirXZ;
			s_LastStandDirXZ = bestDirXZ;
		}
	}
	else
	{
		// Coyote time (no position adjustment): allows 1-2 frames grace when stepping off the ledge.
		const int GRACE = 6;
		if (!isRising && s_LastStandingPrismIndex >= 0)
		{
			s_GraceFrames++;
			if (s_GraceFrames <= GRACE)
			{
				player->isGround = true;
				hitAny = true;
				if (player->Velocity.y < 0.0f) player->Velocity.y = 0.0f;
			}
			else
			{
				s_LastStandingPrismIndex = -1;
				s_GraceFrames = 0;
				s_StandSegValid = false;
				s_StandSegWalkable = false;
			}
		}
		else
		{
			s_LastStandingPrismIndex = -1;
			s_GraceFrames = 0;
			s_StandSegValid = false;
			s_StandSegWalkable = false;
		}
	}

	return hitAny;
}

int Collision_Player2D_MoveAndCollision()
{
	PLAYER* player = GetPlayer2D();
	if(!player) return HIT_NONE;

	Player2DShadow_Collision();

	player->Position.x += player->Velocity.x;
	player->Position.y += player->Velocity.y;
	player->Position.z += player->Velocity.z;

	player->Position.y += s_FrameSlopeDeltaY;

	int hit = Player2DField_Collision();

	Player2DShadow_TopContact();

	return hit;
}


// デバッグ描画関数
void Collision_DebugDraw()
{

	// 3Dプレイヤーの当たり判定ボックスのデバッグ描画
	PLAYER* player3D = GetPlayer3D();
	if (player3D)
	{
		XMFLOAT3 pC = GetPlayerSolidCollider();
		XMFLOAT3 pH = Player3D_GetSolidHalfSize();
		DebugDrawEllipsoid(pC, pH, IM_COL32(0, 255, 0, 255));// 緑の楕円でプレイヤーの当たり判定を描画

		XMFLOAT3 tC = GetPlayerTriggerCollider();
		XMFLOAT3 tH = Player3D_GetTriggerHalfSize();
		DebugDrawOBB_Yaw(tC, tH, player3D->Rotation.y, IM_COL32(255, 255, 255, 255));// 白のOBBでプレイヤーのトリガー判定を描画
	}
	
	// 2Dプレイヤーの当たり判定ボックスのデバッグ描画
	PLAYER* player2D = GetPlayer2D();
	if (player2D && player2D->Active)
	{
		Capsule2D capsule = Player2D_GetCapsule();
		DebugDrawCapsule2D(capsule, IM_COL32(0, 255, 0, 255));

		// Shadow slope-walk debug (current stand segment + move projection)
		if (g_ShadowDebugOpts.drawStandSegments && s_StandSegValid)
		{
			ImU32 colSeg = s_StandSegWalkable ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 80, 80, 255);
			DrawLine3D(s_StandSegA, s_StandSegB, colSeg, 6.0f);
			DrawPoint3D(s_StandClosestQ, IM_COL32(255, 255, 255, 255), 5.0f);

			XMFLOAT3 p = capsule.center;
			// input(move) before any clipping
			DrawLine3D(p, p + s_DbgMoveInXZ, IM_COL32(80, 80, 255, 255), 2.0f);
			// after wall clipping (XZ only)
			DrawLine3D(p, p + s_DbgMoveOutXZ, IM_COL32(80, 255, 255, 255), 2.0f);
			// final slope displacement applied this frame (XZ + dY)
			DrawLine3D(p, p + s_DbgMoveSlope, IM_COL32(255, 255, 80, 255), 3.0f);

			// Text overlay
			float slopeTan = fabsf(s_StandSegAB.y) / (s_StandLenXZ + 1e-6f);
			char buf[160];
			std::snprintf(buf, sizeof(buf), "StandPrism=%d  walkable=%d  slopeTan=%.3f  dY=%.3f",
				s_LastStandingPrismIndex, (int)s_StandSegWalkable, slopeTan, s_FrameSlopeDeltaY);

			ScreenPt sp = WorldToScreen(s_StandClosestQ);
			if (sp.valid)
				ImGui::GetBackgroundDrawList()->AddText(sp.pos, IM_COL32(255, 255, 255, 255), buf);
		}

	}

	// フィールドのトリガーの当たり判定ボックスのデバッグ描画
	std::vector<MAPDATA>& map = GetFieldMap();
	if (player3D && !map.empty())
	{
		XMFLOAT3 tC = GetPlayerTriggerCollider();
		XMFLOAT3 tH = Player3D_GetTriggerHalfSize();

		for (size_t i = 0; i < map.size(); ++i)
		{
			if (!Field_IsTrigger(map[i].no)) continue;
			{
				XMFLOAT3 boxH = Field_GetCollisionHalfSize(map[i]);
				bool hit = OBB_Intersect_Yaw(tC, tH, player3D->Rotation.y,
					map[i].pos, boxH, map[i].rotate.y);
				ImU32 col = hit ? IM_COL32(255, 0, 0, 255) : IM_COL32(0, 255, 255, 255);
				DebugDrawOBB_Yaw(map[i].pos, boxH, map[i].rotate.y, col);
			}
		}
	}

	// シャドウプリズムのデバッグ描画
	if (!g_ShadowPrisms.empty())
	{
		static const ImU32 prismColors[] = {
			IM_COL32(255, 50, 50, 220), 
			IM_COL32(50, 255, 50, 220), 
			IM_COL32(50, 50, 255, 220), 
			IM_COL32(255, 255, 50, 220),
			IM_COL32(255, 50, 255, 220),
			IM_COL32(50, 255, 255, 220),
		};
		const int colorCount = sizeof(prismColors) / sizeof(prismColors[0]);

		for (size_t i = 0; i < g_ShadowPrisms.size(); ++i)
		{
			if (g_ShadowPrisms[i] && g_ShadowPrisms[i]->isValid)
			{
				ShadowDebugOptions opts = g_ShadowDebugOpts;
				opts.prismColor = prismColors[i % colorCount];
				DebugDrawShadowPrism(*g_ShadowPrisms[i], opts);
			}
		}
	}

	// その他のデバッグ用の当たり判定ボックス
	for (const auto& box : g_ExtraBoxes)
	{
		if (box.isOBB)
			DebugDrawOBB_Yaw(box.center, box.half, box.rotDeg.y, box.color);
		else
			DebugDrawAABB(box.center, box.half, box.color);
	}

}
