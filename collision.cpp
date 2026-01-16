#include "Collision.h"
#include "player3D.h"
#include "camera.h"
#include "direct3d.h"
#include "Player2D.h"
#include "debug.h"
#include "MathUtil.h"

#include <vector>


using namespace DirectX;
using namespace mu;

static bool debugMode = TRUE;

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

//=========================================================================================================
// ボールとフィールドの当たり判定
//=========================================================================================================

void Collision_SetShadowPrisms(const std::vector<const ShadowPrism*>& prisms)
{
	g_ShadowPrisms = prisms;
}

const std::vector<const ShadowPrism*>& Collision_GetShadowPrisms()
{
	return g_ShadowPrisms;
}

void Collision_SetShadowPrism(const ShadowPrism* prism)
{
	g_ShadowPrisms.clear();
	if (prism && prism->isValid)
	{
		g_ShadowPrisms.push_back(prism);
	}
}

const ShadowPrism* Collision_GetShadowPrism()
{
	return g_ShadowPrisms.empty() ? nullptr : g_ShadowPrisms[0];
}

static ImU32 MakeColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	return ((ImU32)a << 24) | ((ImU32)b << 16) | ((ImU32)g << 8) | (ImU32)r;
}

// ボックスの半分のサイズを取得
static XMFLOAT3 Field_GetHalfSize(const MAPDATA& m)
{
	return XMFLOAT3{
	BOX_RADIUS * m.scale.x,
	BOX_RADIUS * m.scale.y,
	BOX_RADIUS * m.scale.z
	};
}

// ワールド座標をスクリーン座標に変換
struct ScreenPt { ImVec2 pos; bool valid; };
static ScreenPt WorldToScreen(const XMFLOAT3& p)
{
	ScreenPt out{};
	out.valid = false;

	XMMATRIX vp = GetViewMatrix() * GetProjectionMatrix();

	XMVECTOR vW = XMVectorSet(p.x, p.y, p.z, 1.0f);
	XMVECTOR vV = XMVector3TransformCoord(vW, GetViewMatrix());

	if (XMVectorGetZ(vV) <= 0.01f) return out;

	XMVECTOR vC = XMVector3TransformCoord(vW, vp);
	XMFLOAT3 ndc;
	XMStoreFloat3(&ndc, vC);

	if (ndc.x < -1.5f || ndc.x > 1.5f || ndc.y < -1.5f || ndc.y > 1.5f)
		return out;

	ImGuiViewport* vp_ = ImGui::GetMainViewport();
	float w = (float)Direct3D_GetBackBufferWidth();
	float h = (float)Direct3D_GetBackBufferHeight();

	out.pos.x = vp_->Pos.x + (ndc.x * 0.5f + 0.5f) * w;
	out.pos.y = vp_->Pos.y + (-ndc.y * 0.5f + 0.5f) * h;
	out.valid = true;
	return out;
}

// 3Dラインを描画
static void DrawLine3D(const XMFLOAT3& a, const XMFLOAT3& b, ImU32 col, float thick = 1.0f)
{
	ScreenPt sa = WorldToScreen(a);
	ScreenPt sb = WorldToScreen(b);
	if (sa.valid && sb.valid)
		ImGui::GetBackgroundDrawList()->AddLine(sa.pos, sb.pos, col, thick);
}
// 3Dポイントを描画
static void DrawPoint3D(const XMFLOAT3& p, ImU32 col, float size = 4.0f)
{
	ScreenPt sp = WorldToScreen(p);
	if (sp.valid)
		ImGui::GetBackgroundDrawList()->AddCircleFilled(sp.pos, size, col);
}


// AABBを描画
static void DebugDrawAABB(const XMFLOAT3& c, const XMFLOAT3& h, ImU32 col)
{
	XMFLOAT3 corners[8] = {
		{c.x - h.x, c.y - h.y, c.z - h.z}, {c.x + h.x, c.y - h.y, c.z - h.z},
		{c.x + h.x, c.y + h.y, c.z - h.z}, {c.x - h.x, c.y + h.y, c.z - h.z},
		{c.x - h.x, c.y - h.y, c.z + h.z}, {c.x + h.x, c.y - h.y, c.z + h.z},
		{c.x + h.x, c.y + h.y, c.z + h.z}, {c.x - h.x, c.y + h.y, c.z + h.z},
	};

	const int edges[12][2] = {
		{0,1},{1,2},{2,3},{3,0}, {4,5},{5,6},{6,7},{7,4}, {0,4},{1,5},{2,6},{3,7}
	};

	for (int i = 0; i < 12; i++)
		DrawLine3D(corners[edges[i][0]], corners[edges[i][1]], col);
}

// OBBをYaw回転で描画
static void DebugDrawOBB_Yaw(const XMFLOAT3& c, const XMFLOAT3& h, float yawDeg, ImU32 col)
{
	float yaw = XMConvertToRadians(yawDeg);

	XMFLOAT3 local[8] = {
		{-h.x,-h.y,-h.z}, {+h.x,-h.y,-h.z}, {+h.x,+h.y,-h.z}, {-h.x,+h.y,-h.z},
		{-h.x,-h.y,+h.z}, {+h.x,-h.y,+h.z}, {+h.x,+h.y,+h.z}, {-h.x,+h.y,+h.z},
	};

	XMFLOAT3 corners[8];
	for (int i = 0; i < 8; i++)
		corners[i] = c + RotateY(local[i], yaw);

	const int edges[12][2] = {
		{0,1},{1,2},{2,3},{3,0}, {4,5},{5,6},{6,7},{7,4}, {0,4},{1,5},{2,6},{3,7}
	};

	for (int i = 0; i < 12; i++)
		DrawLine3D(corners[edges[i][0]], corners[edges[i][1]], col);
}

// 楕円体を描画
static void DebugDrawEllipsoid(const XMFLOAT3& c, const XMFLOAT3& r, ImU32 col, int seg = 24)
{
	const float PI2 = 6.28318530718f;

	auto drawRing = [&](int plane) {
		XMFLOAT3 prev;
		for (int i = 0; i <= seg; i++)
		{
			float t = PI2 * i / seg;
			XMFLOAT3 p = c;
			if (plane == 0) { p.x += cosf(t) * r.x; p.z += sinf(t) * r.z; }
			if (plane == 1) { p.x += cosf(t) * r.x; p.y += sinf(t) * r.y; }
			if (plane == 2) { p.y += cosf(t) * r.y; p.z += sinf(t) * r.z; }
			if (i > 0) DrawLine3D(prev, p, col);
			prev = p;
		}
		};

	drawRing(0); drawRing(1); drawRing(2);
}


static void DebugDrawShadowPrism(const ShadowPrism& prism, const ShadowDebugOptions& opts)
{
	if (!prism.isValid || prism.poly.size() < 3) return;

	int n = (int)prism.baseWorld.size();
	ImU32 colPrism = opts.prismColor;
	ImU32 colFaded = (colPrism & 0x00FFFFFF) | 0x60000000;

	if (opts.drawPrism)
	{
		for (int i = 0; i < n; i++)
			DrawLine3D(prism.baseWorld[i], prism.baseWorld[(i + 1) % n], colPrism, 2.0f);

		if (!prism.topWorld.empty())
		{
			for (int i = 0; i < n; i++)
				DrawLine3D(prism.topWorld[i], prism.topWorld[(i + 1) % n], colPrism, 2.0f);

			for (int i = 0; i < n; i++)
				DrawLine3D(prism.baseWorld[i], prism.topWorld[i], colFaded, 1.0f);
		}
	}

	if (opts.drawNormal)
	{
		XMFLOAT3 nEnd = prism.origin + prism.n * 0.5f;
		DrawLine3D(prism.origin, nEnd, opts.normalColor, 2.0f);
		DrawPoint3D(prism.origin, opts.normalColor, 5.0f);
	}

	if (opts.drawVertices)
	{
		for (const auto& p : prism.baseWorld)
			DrawPoint3D(p, opts.vertexColor, 4.0f);
	}

	if (opts.drawAABB)
	{
		XMFLOAT3 c = (prism.aabbMin + prism.aabbMax) * 0.5f;
		XMFLOAT3 h = (prism.aabbMax - prism.aabbMin) * 0.5f;
		DebugDrawAABB(c, h, opts.aabbColor);
	}
}


// プレイヤーの固体コライダー中心を取得
static XMFLOAT3 GetPlayerSolidCollider()
{
	PLAYER* p = GetPlayer3D();
	XMFLOAT3 c = p->Position;
	c.y += Player3D_GetSolidHalfSize().y;
	return c;
}
// プレイヤー2Dの固体コライダー中心を取得
static XMFLOAT3 GetPlayer2DSolidCollider()
{
	PLAYER* p = GetPlayer2D();
	XMFLOAT3 c = p->Position;
	c.y += Player2D_GetSolidHalfSize().y + 0.1f;
	return c;
}
// プレイヤーのトリガーコライダー中心を取得
static XMFLOAT3 GetPlayerTriggerCollider()
{
	PLAYER* p = GetPlayer3D();
	XMFLOAT3 c = p->Position;
	c.y += Player3D_GetTriggerHalfSize().y;
	return c;
}

// フィールドが固体かどうかを取得
static bool Field_IsSolid(FIELD t)
{
	switch (t)
	{
	case FIELD_GROUND:
	case FIELD_WALL:
	case FIELD_OBJ_BOX:
	case FIELD_OBJ_1:
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
	case FIELD_OBJ_2:
		return true;
	default:
		return false;
	}
}

// OBB vs OBB 解算（Yaw回転のみ対応）
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

// 楕円体 vs OBB 解算（Yaw回転のみ対応）
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

// トリガーが当たった面を計算（Player3d Yaw基準）
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


void Collision_SetShadowDebugOptions(const ShadowDebugOptions& options)
{
	g_ShadowDebugOpts = options;
}

void Collision_DebugClearExtraBoxes()
{
	g_ExtraBoxes.clear();
}
// AABBをデバッグ描画リストに追加
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
// OBBをデバッグ描画リストに追加
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
	const XMFLOAT3& posA, const XMFLOAT3& halfA, float rotZRadA,  // プレイヤー（Z回転）
	const XMFLOAT3& posB, const XMFLOAT3& halfB, float rotYDegB,  // フィールド（Y回転）
	XMFLOAT3* outPush, XMFLOAT3* outNorm)
{
	// プレイヤーのZ回転行列（2D平面での回転）
	XMMATRIX rotMatA = XMMatrixRotationZ(rotZRadA);

	// フィールドのY回転行列
	float rotYRadB = XMConvertToRadians(rotYDegB);
	XMMATRIX rotMatB = XMMatrixRotationY(rotYRadB);

	// 各OBBのローカル軸を取得
	XMFLOAT3 axesA[3], axesB[3];

	// プレイヤーの軸（Z回転）
	XMVECTOR axA0 = XMVector3TransformNormal(XMVectorSet(1, 0, 0, 0), rotMatA);
	XMVECTOR axA1 = XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), rotMatA);
	XMVECTOR axA2 = XMVector3TransformNormal(XMVectorSet(0, 0, 1, 0), rotMatA);
	XMStoreFloat3(&axesA[0], axA0);
	XMStoreFloat3(&axesA[1], axA1);
	XMStoreFloat3(&axesA[2], axA2);

	// フィールドの軸（Y回転）
	XMVECTOR axB0 = XMVector3TransformNormal(XMVectorSet(1, 0, 0, 0), rotMatB);
	XMVECTOR axB1 = XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), rotMatB);
	XMVECTOR axB2 = XMVector3TransformNormal(XMVectorSet(0, 0, 1, 0), rotMatB);
	XMStoreFloat3(&axesB[0], axB0);
	XMStoreFloat3(&axesB[1], axB1);
	XMStoreFloat3(&axesB[2], axB2);

	// 中心間ベクトル
	XMVECTOR vD = XMVectorSet(posA.x - posB.x, posA.y - posB.y, posA.z - posB.z, 0);

	float halfExtA[3] = { halfA.x, halfA.y, halfA.z };
	float halfExtB[3] = { halfB.x, halfB.y, halfB.z };

	float minPen = FLT_MAX;
	XMFLOAT3 minAxis = { 0, 1, 0 };

	// 15軸のSAT判定
	auto TestAxis = [&](XMVECTOR axis) -> bool
		{
			float len = XMVectorGetX(XMVector3Length(axis));
			if (len < 1e-6f) return true;  // 平行な軸はスキップ

			axis = XMVector3Normalize(axis);

			// 各OBBの投影半径
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

			if (pen < 0) return false;  // 分離軸発見 → 衝突なし

			if (pen < minPen)
			{
				minPen = pen;
				XMStoreFloat3(&minAxis, axis);
			}
			return true;
		};

	// A の3軸
	if (!TestAxis(axA0)) return false;
	if (!TestAxis(axA1)) return false;
	if (!TestAxis(axA2)) return false;

	// B の3軸
	if (!TestAxis(axB0)) return false;
	if (!TestAxis(axB1)) return false;
	if (!TestAxis(axB2)) return false;

	// 外積軸（9軸）
	if (!TestAxis(XMVector3Cross(axA0, axB0))) return false;
	if (!TestAxis(XMVector3Cross(axA0, axB1))) return false;
	if (!TestAxis(XMVector3Cross(axA0, axB2))) return false;
	if (!TestAxis(XMVector3Cross(axA1, axB0))) return false;
	if (!TestAxis(XMVector3Cross(axA1, axB1))) return false;
	if (!TestAxis(XMVector3Cross(axA1, axB2))) return false;
	if (!TestAxis(XMVector3Cross(axA2, axB0))) return false;
	if (!TestAxis(XMVector3Cross(axA2, axB1))) return false;
	if (!TestAxis(XMVector3Cross(axA2, axB2))) return false;

	// 押し出し方向を決定（AをBから離す方向）
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

//=========================================================================================================
// 2D用トリガー方向判定（プレイヤーの回転を考慮）
//=========================================================================================================
static TRIGGER_SIDE CalcTriggerSide2D(const XMFLOAT3& playerPos, float playerRotZ, const XMFLOAT3& triggerPos)
{
	// プレイヤーからトリガーへのベクトル
	float dx = triggerPos.x - playerPos.x;
	float dy = triggerPos.y - playerPos.y;

	// プレイヤーの回転を考慮してローカル座標に変換
	float radZ = XMConvertToRadians(playerRotZ);
	float cosZ = cosf(-radZ);  // 逆回転でローカル座標へ
	float sinZ = sinf(-radZ);

	float localX = dx * cosZ - dy * sinZ;
	float localY = dx * sinZ + dy * cosZ;

	// ローカル座標での方向判定
	if (fabsf(localX) > fabsf(localY))
	{
		return (localX > 0) ? TRIGGER_SIDE_RIGHT : TRIGGER_SIDE_LEFT;
	}
	else
	{
		return (localY > 0) ? TRIGGER_SIDE_TOP : TRIGGER_SIDE_BOTTOM;
	}
}


// プレイヤー3Dとフィールドの当たり判定
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

		float boxYaw = (map[i].no == FIELD_OBJ_1) ? map[i].rotate.y : 0.0f;

		XMFLOAT3 push, norm;
		if (!Resolve_Ellipsoid_OBB_Yaw(ellC, ellR, map[i].pos,
			Field_GetHalfSize(map[i]), boxYaw, &push, &norm))
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

int Player2DField_Collision()
{
	int hit = HIT_NONE;

	PLAYER* player = GetPlayer2D();
	std::vector<MAPDATA>& map = GetFieldMap();
	if (!player || map.empty()) return hit;

	player->isGround = false;
	XMFLOAT3 halfSize = Player2D_GetSolidHalfSize();

	// 2Dプレイヤーの当たり判定中心（足元基準から中心基準へ変換）
	XMFLOAT3 colliderCenter = XMFLOAT3(
		player->Position.x,
		player->Position.y + halfSize.y,
		player->Position.z
	);

	// プレイヤーのZ軸回転角度（度→ラジアン）
	float playerZRot = XMConvertToRadians(player->Rotation.z);

	for (size_t i = 0; i < map.size(); ++i)
	{
		if (!Field_IsSolid(map[i].no)) continue;

		XMFLOAT3 push, norm;

		// OBB vs OBB（プレイヤーはZ回転、フィールドはY回転）
		if (!Resolve_OBB_OBB_ZY(
			colliderCenter, halfSize, playerZRot,      // プレイヤー（Z回転）
			map[i].pos, Field_GetHalfSize(map[i]), map[i].rotate.y,  // フィールド（Y回転）
			&push, &norm))
			continue;

		colliderCenter = XMFLOAT3(
			colliderCenter.x + push.x,
			colliderCenter.y + push.y,
			colliderCenter.z + push.z
		);

		float ax = fabsf(norm.x), ay = fabsf(norm.y), az = fabsf(norm.z);

		if (ay >= ax && ay >= az)
		{
			// 上下の衝突
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
			// 左右の壁衝突
			player->Velocity.x = 0;
			hit = (norm.x > 0) ? HIT_WALL_PlusX : HIT_WALL_NegX;
		}
		else
		{
			// 前後の壁衝突
			player->Velocity.z = 0;
			hit = (norm.z > 0) ? HIT_WALL_PlusZ : HIT_WALL_NegZ;
		}
	}

	// 中心座標から足元座標へ戻す
	player->Position.x = colliderCenter.x;
	player->Position.y = colliderCenter.y - halfSize.y;
	player->Position.z = colliderCenter.z;

	return hit;
}



// プレイヤーのトリガー当たり判定
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

		XMFLOAT3 tHalf = Field_GetHalfSize(map[i]);
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

bool Collision_Player2DTrigger(TRIGGER_HIT* outHit, float extraRange)
{
	if (outHit) *outHit = TRIGGER_HIT{};

	PLAYER* p = GetPlayer2D();
	if (!p) return false;

	auto& map = GetFieldMap();
	if (map.empty()) return false;

	XMFLOAT3 pHalf = Player2D_GetSolidHalfSize();
	pHalf.x += extraRange;
	pHalf.y += extraRange;
	pHalf.z += extraRange;

	XMFLOAT3 pC = XMFLOAT3(
		p->Position.x,
		p->Position.y + Player2D_GetSolidHalfSize().y,
		p->Position.z
	);

	float pZRot = XMConvertToRadians(p->Rotation.z);

	bool found = false;
	float bestD2 = 1e30f;
	TRIGGER_HIT best;

	for (size_t i = 0; i < map.size(); ++i)
	{
		if (!Field_IsTrigger(map[i].no)) continue;

		XMFLOAT3 tHalf = Field_GetHalfSize(map[i]);
		tHalf.x += extraRange;
		tHalf.y += extraRange;
		tHalf.z += extraRange;

		// OBB vs OBB 交差判定
		if (!OBB_Intersect_ZY(pC, pHalf, pZRot,
			map[i].pos, tHalf, map[i].rotate.y))
			continue;

		float dx = map[i].pos.x - pC.x;
		float dy = map[i].pos.y - pC.y;
		float d2 = dx * dx + dy * dy;

		if (!found || d2 < bestD2)
		{
			found = true;
			bestD2 = d2;
			best.hit = true;
			best.mapIndex = (int)i;
			best.type = map[i].no;
			best.side = CalcTriggerSide2D(pC, p->Rotation.z, map[i].pos);
		}
	}

	if (!found) return false;
	if (outHit) *outHit = best;
	return true;
}


//=========================================================================================================
// 点がプリズム内にあるかチェック（2D多角形判定）
//=========================================================================================================
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

//=========================================================================================================
// プレイヤー2Dと影プリズムの当たり判定
//=========================================================================================================
bool Player2DShadow_Collision()
{
	PLAYER* player = GetPlayer2D();
	if (!player) return false;

	const std::vector<const ShadowPrism*>& prisms = Collision_GetShadowPrisms();
	if (prisms.empty()) return false;

	XMFLOAT3 halfSize = Player2D_GetSolidHalfSize();

	bool hitAny = false;

	// 複数回の反復で安定させる
	const int maxIterations = 4;

	for (int iter = 0; iter < maxIterations; iter++)
	{
		bool hitThisIter = false;

		// プレイヤーの中心位置（毎回更新）
		XMFLOAT3 playerCenter = XMFLOAT3(
			player->Position.x,
			player->Position.y + halfSize.y,
			player->Position.z
		);

		for (const ShadowPrism* prism : prisms)
		{
			if (!prism || !prism->isValid) continue;
			if (prism->poly.size() < 3) continue;

			// AABB早期キャンセル
			float margin = halfSize.x + 0.1f;
			if (playerCenter.x < prism->aabbMin.x - margin ||
				playerCenter.x > prism->aabbMax.x + margin ||
				playerCenter.y < prism->aabbMin.y - halfSize.y - margin ||
				playerCenter.y > prism->aabbMax.y + margin ||
				playerCenter.z < prism->aabbMin.z - margin ||
				playerCenter.z > prism->aabbMax.z + margin)
			{
				continue;
			}

			// プレイヤー位置をプリズムのローカル座標に変換
			XMFLOAT3 rel = playerCenter - prism->origin;
			float localU = Dot(rel, prism->u);
			float localV = Dot(rel, prism->v);
			float localN = Dot(rel, prism->n);

			// プリズムのU-V範囲を計算
			float minU = FLT_MAX, maxU = -FLT_MAX;
			float minV = FLT_MAX, maxV = -FLT_MAX;
			for (const auto& p : prism->poly)
			{
				minU = (std::min)(minU, p.x);
				maxU = (std::max)(maxU, p.x);
				minV = (std::min)(minV, p.y);
				maxV = (std::max)(maxV, p.y);
			}

			// プレイヤーの半径
			float playerRadiusU = halfSize.x;
			float playerRadiusV = halfSize.y;
			float playerRadiusN = halfSize.z;

			// 各方向の侵入量を計算
			float penU_pos = (maxU + playerRadiusU) - localU;
			float penU_neg = localU - (minU - playerRadiusU);
			float penV_pos = (maxV + playerRadiusV) - localV;
			float penV_neg = localV - (minV - playerRadiusV);
			float penN_pos = (prism->thickness + playerRadiusN) - localN;
			float penN_neg = localN + playerRadiusN;

			// 全方向で侵入しているかチェック
			if (penU_pos <= 0 || penU_neg <= 0 ||
				penV_pos <= 0 || penV_neg <= 0 ||
				penN_pos <= 0 || penN_neg <= 0)
			{
				continue;
			}

			// 最小侵入量の方向を見つける
			float minPen = FLT_MAX;
			int pushDir = 0;

			if (penU_pos < minPen) { minPen = penU_pos; pushDir = 1; }
			if (penU_neg < minPen) { minPen = penU_neg; pushDir = 2; }
			if (penV_pos < minPen) { minPen = penV_pos; pushDir = 3; }
			if (penV_neg < minPen) { minPen = penV_neg; pushDir = 4; }
			if (penN_pos < minPen) { minPen = penN_pos; pushDir = 5; }
			if (penN_neg < minPen) { minPen = penN_neg; pushDir = 6; }

			if (pushDir == 0 || minPen <= 0.001f) continue;

			// 押し出しベクトルを計算
			XMFLOAT3 pushDir3 = { 0, 0, 0 };
			bool isGroundHit = false;

			switch (pushDir)
			{
			case 1:  // +U方向に押し出し
				pushDir3 = prism->u;
				break;
			case 2:  // -U方向に押し出し
				pushDir3 = { -prism->u.x, -prism->u.y, -prism->u.z };
				break;
			case 3:  // +V方向に押し出し（上方向＝着地の可能性）
				pushDir3 = prism->v;
				if (prism->v.y > 0.5f) isGroundHit = true;
				break;
			case 4:  // -V方向に押し出し（下から頭をぶつけた）
				pushDir3 = { -prism->v.x, -prism->v.y, -prism->v.z };
				break;
			case 5:  // +N方向に押し出し
				pushDir3 = prism->n;
				if (prism->n.y > 0.5f) isGroundHit = true;
				break;
			case 6:  // -N方向に押し出し
				pushDir3 = { -prism->n.x, -prism->n.y, -prism->n.z };
				break;
			}

			// 押し出しを適用
			player->Position.x += pushDir3.x * minPen;
			player->Position.y += pushDir3.y * minPen;
			player->Position.z += pushDir3.z * minPen;

			// 速度の該当成分を停止
			float velDot = Dot(player->Velocity, pushDir3);
			if (velDot < 0)  // 衝突方向に向かっている場合のみ停止
			{
				player->Velocity.x -= velDot * pushDir3.x;
				player->Velocity.y -= velDot * pushDir3.y;
				player->Velocity.z -= velDot * pushDir3.z;
			}

			// 接地判定
			if (isGroundHit && player->Velocity.y <= 0.01f)
			{
				player->isGround = true;
			}

			hitThisIter = true;
			hitAny = true;
		}

		// この反復で衝突がなければ終了
		if (!hitThisIter) break;
	}

	return hitAny;
}



// デバッグ描画
void Collision_DebugDraw()
{
	// 3Dプレイヤーコライダー
	PLAYER* player3D = GetPlayer3D();
	if (player3D)
	{
		XMFLOAT3 pC = GetPlayerSolidCollider();
		XMFLOAT3 pH = Player3D_GetSolidHalfSize();
		DebugDrawEllipsoid(pC, pH, IM_COL32(0, 255, 0, 255));

		XMFLOAT3 tC = GetPlayerTriggerCollider();
		XMFLOAT3 tH = Player3D_GetTriggerHalfSize();
		DebugDrawOBB_Yaw(tC, tH, player3D->Rotation.y, IM_COL32(255, 255, 255, 255));
	}
	// 2Dプレイヤーコライダー
	PLAYER* player2D = GetPlayer2D();
	if (player2D)
	{
		XMFLOAT3 pC = GetPlayer2DSolidCollider();
		XMFLOAT3 pH = Player2D_GetSolidHalfSize();
		DebugDrawOBB_Yaw(pC, pH, player2D->Rotation.y, IM_COL32(0, 255, 0, 255));
	}

	// フィールドトリガー
	std::vector<MAPDATA>& map = GetFieldMap();
	if (player3D && !map.empty())
	{
		XMFLOAT3 tC = GetPlayerTriggerCollider();
		XMFLOAT3 tH = Player3D_GetTriggerHalfSize();

		for (size_t i = 0; i < map.size(); ++i)
		{
			if (!Field_IsTrigger(map[i].no)) continue;

			XMFLOAT3 boxH = Field_GetHalfSize(map[i]);
			bool hit = OBB_Intersect_Yaw(tC, tH, player3D->Rotation.y,
				map[i].pos, boxH, map[i].rotate.y);
			ImU32 col = hit ? IM_COL32(255, 0, 0, 255) : IM_COL32(0, 255, 255, 255);
			DebugDrawOBB_Yaw(map[i].pos, boxH, map[i].rotate.y, col);
		}
	}

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

	for (const auto& box : g_ExtraBoxes)
	{
		if (box.isOBB)
			DebugDrawOBB_Yaw(box.center, box.half, box.rotDeg.y, box.color);
		else
			DebugDrawAABB(box.center, box.half, box.color);
	}
}
