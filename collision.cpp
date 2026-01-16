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
