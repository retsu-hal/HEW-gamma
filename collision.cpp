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
// 儃乕儖偲僼傿乕儖僪偺摉偨傝敾掕
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

// 儃僢僋僗偺敿暘偺僒僀僘傪庢摼
static XMFLOAT3 Field_GetHalfSize(const MAPDATA& m)
{
	return XMFLOAT3{
	BOX_RADIUS * m.scale.x,
	BOX_RADIUS * m.scale.y,
	BOX_RADIUS * m.scale.z
	};
}

// 儚乕儖僪嵗昗傪僗僋儕乕儞嵗昗偵曄姺
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

// 3D儔僀儞傪昤夋
static void DrawLine3D(const XMFLOAT3& a, const XMFLOAT3& b, ImU32 col, float thick = 1.0f)
{
	ScreenPt sa = WorldToScreen(a);
	ScreenPt sb = WorldToScreen(b);
	if (sa.valid && sb.valid)
		ImGui::GetBackgroundDrawList()->AddLine(sa.pos, sb.pos, col, thick);
}
// 3D億僀儞僩傪昤夋
static void DrawPoint3D(const XMFLOAT3& p, ImU32 col, float size = 4.0f)
{
	ScreenPt sp = WorldToScreen(p);
	if (sp.valid)
		ImGui::GetBackgroundDrawList()->AddCircleFilled(sp.pos, size, col);
}


// AABB傪昤夋
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

// OBB傪Yaw夞揮偱昤夋
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

// 懭墌懱傪昤夋
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


// 僾儗僀儎乕偺屌懱僐儔僀僟乕拞怱傪庢摼
static XMFLOAT3 GetPlayerSolidCollider()
{
	PLAYER* p = GetPlayer3D();
	XMFLOAT3 c = p->Position;
	c.y += Player3D_GetSolidHalfSize().y;
	return c;
}
// 僾儗僀儎乕2D偺屌懱僐儔僀僟乕拞怱傪庢摼
static XMFLOAT3 GetPlayer2DSolidCollider()
{
	PLAYER* p = GetPlayer2D();
	XMFLOAT3 c = p->Position;
	c.y += Player2D_GetSolidHalfSize().y + 0.1f;
	return c;
}
// 僾儗僀儎乕偺僩儕僈乕僐儔僀僟乕拞怱傪庢摼
static XMFLOAT3 GetPlayerTriggerCollider()
{
	PLAYER* p = GetPlayer3D();
	XMFLOAT3 c = p->Position;
	c.y += Player3D_GetTriggerHalfSize().y;
	return c;
}

// 僼傿乕儖僪偑屌懱偐偳偆偐傪庢摼
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
// 僼傿乕儖僪偑僩儕僈乕偐偳偆偐傪庢摼
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

// OBB vs OBB 夝嶼乮Yaw夞揮偺傒懳墳乯
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

// 懭墌懱 vs OBB 夝嶼乮Yaw夞揮偺傒懳墳乯
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

// 僩儕僈乕偑摉偨偭偨柺傪寁嶼乮Player3d Yaw婎弨乯
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
// AABB傪僨僶僢僌昤夋儕僗僩偵捛壛
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
// OBB傪僨僶僢僌昤夋儕僗僩偵捛壛
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
	const XMFLOAT3& posA, const XMFLOAT3& halfA, float rotZRadA,  // 僾儗僀儎乕乮Z夞揮乯
	const XMFLOAT3& posB, const XMFLOAT3& halfB, float rotYDegB,  // 僼傿乕儖僪乮Y夞揮乯
	XMFLOAT3* outPush, XMFLOAT3* outNorm)
{
	// 僾儗僀儎乕偺Z夞揮峴楍乮2D暯柺偱偺夞揮乯
	XMMATRIX rotMatA = XMMatrixRotationZ(rotZRadA);

	// 僼傿乕儖僪偺Y夞揮峴楍
	float rotYRadB = XMConvertToRadians(rotYDegB);
	XMMATRIX rotMatB = XMMatrixRotationY(rotYRadB);

	// 奺OBB偺儘乕僇儖幉傪庢摼
	XMFLOAT3 axesA[3], axesB[3];

	// 僾儗僀儎乕偺幉乮Z夞揮乯
	XMVECTOR axA0 = XMVector3TransformNormal(XMVectorSet(1, 0, 0, 0), rotMatA);
	XMVECTOR axA1 = XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), rotMatA);
	XMVECTOR axA2 = XMVector3TransformNormal(XMVectorSet(0, 0, 1, 0), rotMatA);
	XMStoreFloat3(&axesA[0], axA0);
	XMStoreFloat3(&axesA[1], axA1);
	XMStoreFloat3(&axesA[2], axA2);

	// 僼傿乕儖僪偺幉乮Y夞揮乯
	XMVECTOR axB0 = XMVector3TransformNormal(XMVectorSet(1, 0, 0, 0), rotMatB);
	XMVECTOR axB1 = XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), rotMatB);
	XMVECTOR axB2 = XMVector3TransformNormal(XMVectorSet(0, 0, 1, 0), rotMatB);
	XMStoreFloat3(&axesB[0], axB0);
	XMStoreFloat3(&axesB[1], axB1);
	XMStoreFloat3(&axesB[2], axB2);

	// 拞怱娫儀僋僩儖
	XMVECTOR vD = XMVectorSet(posA.x - posB.x, posA.y - posB.y, posA.z - posB.z, 0);

	float halfExtA[3] = { halfA.x, halfA.y, halfA.z };
	float halfExtB[3] = { halfB.x, halfB.y, halfB.z };

	float minPen = FLT_MAX;
	XMFLOAT3 minAxis = { 0, 1, 0 };

	// 15幉偺SAT敾掕
	auto TestAxis = [&](XMVECTOR axis) -> bool
		{
			float len = XMVectorGetX(XMVector3Length(axis));
			if (len < 1e-6f) return true;  // 暯峴側幉偼僗僉僢僾

			axis = XMVector3Normalize(axis);

			// 奺OBB偺搳塭敿宎
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

			if (pen < 0) return false;  // 暘棧幉敪尒 仺 徴撍側偟

			if (pen < minPen)
			{
				minPen = pen;
				XMStoreFloat3(&minAxis, axis);
			}
			return true;
		};

	// A 偺3幉
	if (!TestAxis(axA0)) return false;
	if (!TestAxis(axA1)) return false;
	if (!TestAxis(axA2)) return false;

	// B 偺3幉
	if (!TestAxis(axB0)) return false;
	if (!TestAxis(axB1)) return false;
	if (!TestAxis(axB2)) return false;

	// 奜愊幉乮9幉乯
	if (!TestAxis(XMVector3Cross(axA0, axB0))) return false;
	if (!TestAxis(XMVector3Cross(axA0, axB1))) return false;
	if (!TestAxis(XMVector3Cross(axA0, axB2))) return false;
	if (!TestAxis(XMVector3Cross(axA1, axB0))) return false;
	if (!TestAxis(XMVector3Cross(axA1, axB1))) return false;
	if (!TestAxis(XMVector3Cross(axA1, axB2))) return false;
	if (!TestAxis(XMVector3Cross(axA2, axB0))) return false;
	if (!TestAxis(XMVector3Cross(axA2, axB1))) return false;
	if (!TestAxis(XMVector3Cross(axA2, axB2))) return false;

	// 墴偟弌偟曽岦傪寛掕乮A傪B偐傜棧偡曽岦乯
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
// 2D梡僩儕僈乕曽岦敾掕乮僾儗僀儎乕偺夞揮傪峫椂乯
//=========================================================================================================
static TRIGGER_SIDE CalcTriggerSide2D(const XMFLOAT3& playerPos, float playerRotZ, const XMFLOAT3& triggerPos)
{
	// 僾儗僀儎乕偐傜僩儕僈乕傊偺儀僋僩儖
	float dx = triggerPos.x - playerPos.x;
	float dy = triggerPos.y - playerPos.y;

	// 僾儗僀儎乕偺夞揮傪峫椂偟偰儘乕僇儖嵗昗偵曄姺
	float radZ = XMConvertToRadians(playerRotZ);
	float cosZ = cosf(-radZ);  // 媡夞揮偱儘乕僇儖嵗昗傊
	float sinZ = sinf(-radZ);

	float localX = dx * cosZ - dy * sinZ;
	float localY = dx * sinZ + dy * cosZ;

	// 儘乕僇儖嵗昗偱偺曽岦敾掕
	if (fabsf(localX) > fabsf(localY))
	{
		return (localX > 0) ? TRIGGER_SIDE_RIGHT : TRIGGER_SIDE_LEFT;
	}
	else
	{
		return (localY > 0) ? TRIGGER_SIDE_TOP : TRIGGER_SIDE_BOTTOM;
	}
}


// 僾儗僀儎乕3D偲僼傿乕儖僪偺摉偨傝敾掕
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

	// 2D僾儗僀儎乕偺摉偨傝敾掕拞怱乮懌尦婎弨偐傜拞怱婎弨傊曄姺乯
	XMFLOAT3 colliderCenter = XMFLOAT3(
		player->Position.x,
		player->Position.y + halfSize.y,
		player->Position.z
	);

	// 僾儗僀儎乕偺Z幉夞揮妏搙乮搙仺儔僕傾儞乯
	float playerZRot = XMConvertToRadians(player->Rotation.z);

	for (size_t i = 0; i < map.size(); ++i)
	{
		if (!Field_IsSolid(map[i].no)) continue;

		XMFLOAT3 push, norm;

		// OBB vs OBB乮僾儗僀儎乕偼Z夞揮丄僼傿乕儖僪偼Y夞揮乯
		if (!Resolve_OBB_OBB_ZY(
			colliderCenter, halfSize, playerZRot,      // 僾儗僀儎乕乮Z夞揮乯
			map[i].pos, Field_GetHalfSize(map[i]), map[i].rotate.y,  // 僼傿乕儖僪乮Y夞揮乯
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
			// 忋壓偺徴撍
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
			// 嵍塃偺暻徴撍
			player->Velocity.x = 0;
			hit = (norm.x > 0) ? HIT_WALL_PlusX : HIT_WALL_NegX;
		}
		else
		{
			// 慜屻偺暻徴撍
			player->Velocity.z = 0;
			hit = (norm.z > 0) ? HIT_WALL_PlusZ : HIT_WALL_NegZ;
		}
	}

	// 拞怱嵗昗偐傜懌尦嵗昗傊栠偡
	player->Position.x = colliderCenter.x;
	player->Position.y = colliderCenter.y - halfSize.y;
	player->Position.z = colliderCenter.z;

	return hit;
}



// 僾儗僀儎乕偺僩儕僈乕摉偨傝敾掕
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

		// OBB vs OBB 岎嵎敾掕
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
// 揰偑僾儕僘儉撪偵偁傞偐僠僃僢僋乮2D懡妏宍敾掕乯
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
// 僾儗僀儎乕2D偲塭僾儕僘儉偺摉偨傝敾掕
//=========================================================================================================
static float DotXZ(const XMFLOAT3& a, const XMFLOAT3& b)
{
	return a.x * b.x + a.z * b.z;
}

static float ProjectRadiusOnAxis_Yaw(const XMFLOAT3& axis, const XMFLOAT3& half, float yawDeg)
{
	const float yaw = XMConvertToRadians(yawDeg);
	const XMFLOAT3 right = { cosf(yaw), 0.0f, -sinf(yaw) };
	const XMFLOAT3 fwd = { sinf(yaw), 0.0f,  cosf(yaw) };
	const XMFLOAT3 up = { 0.0f, 1.0f, 0.0f };

	// axis is expected to be unit length (ShadowPrism builds an orthonormal basis)
	return fabsf(Dot(axis, right)) * half.x
		+ fabsf(Dot(axis, up)) * half.y
		+ fabsf(Dot(axis, fwd)) * half.z;
}

// Stop horizontal movement BEFORE we enter the prism (prevents "walk in then snap back").
// Call this BEFORE Position += Velocity in Player2D_Gravity().
bool Player2DShadow_BlockMoveAtContact(float skin)
{
	PLAYER* player = GetPlayer2D();
	if (!player) return false;

	const std::vector<const ShadowPrism*>& prisms = Collision_GetShadowPrisms();
	if (prisms.empty()) return false;

	const XMFLOAT3 half = Player2D_GetSolidHalfSize();

	// current center (before move)
	const XMFLOAT3 center = { player->Position.x, player->Position.y + half.y, player->Position.z };

	// current intended horizontal displacement for this frame (Position += Velocity uses "per-frame" displacement)
	const XMFLOAT3 velXZ = { player->Velocity.x, 0.0f, player->Velocity.z };

	// if we are almost not moving, nothing to do
	if (fabsf(velXZ.x) + fabsf(velXZ.z) < 1e-6f) return false;

	for (const ShadowPrism* prism : prisms)
	{
		if (!prism || !prism->isValid) continue;
		if (prism->poly.size() < 3) continue;

		// Quick AABB gate (expanded by half + skin + move amount)
		const float moveMargin = sqrtf(velXZ.x * velXZ.x + velXZ.z * velXZ.z) + skin + 0.05f;
		if (center.x < prism->aabbMin.x - half.x - moveMargin || center.x > prism->aabbMax.x + half.x + moveMargin ||
			center.z < prism->aabbMin.z - half.z - moveMargin || center.z > prism->aabbMax.z + half.z + moveMargin)
		{
			continue;
		}

		// local coordinates in prism basis
		const XMFLOAT3 rel = center - prism->origin;
		const float localU = Dot(rel, prism->u);
		const float localV = Dot(rel, prism->v);
		const float localN = Dot(rel, prism->n);

		// prism U-V bounds from polygon (local 2D)
		float minU = FLT_MAX, maxU = -FLT_MAX;
		float minV = FLT_MAX, maxV = -FLT_MAX;
		for (const auto& p : prism->poly)
		{
			minU = (std::min)(minU, p.x);
			maxU = (std::max)(maxU, p.x);
			minV = (std::min)(minV, p.y);
			maxV = (std::max)(maxV, p.y);
		}

		// Player OBB projected radius on prism axes (important!)
		const float rU = ProjectRadiusOnAxis_Yaw(prism->u, half, player->Rotation.y);
		const float rV = ProjectRadiusOnAxis_Yaw(prism->v, half, player->Rotation.y);
		const float rN = ProjectRadiusOnAxis_Yaw(prism->n, half, player->Rotation.y);

		// Require overlap in the other two axes to treat as a side contact candidate
		const bool overlapV = (localV <= maxV + rV) && (localV >= minV - rV);
		const bool overlapN = (localN <= prism->thickness + rN) && (localN >= -rN);
		const bool overlapU = (localU <= maxU + rU) && (localU >= minU - rU);

		// Horizontal deltas in prism axes (ignore Y to avoid gravity causing side-blocks)
		const float dU = DotXZ(velXZ, prism->u);
		const float dN = DotXZ(velXZ, prism->n);

		// +U / -U sides:
		// 旧版は「外側にいる(outside>0) かつ 1フレームで跨ぐ」時だけ止めていたため、
		// 一度接触(or 少しめり込み)した状態で押し続けると inside 側へ進めてしまう。
		// ここでは「接触域(skin)に入る/入っている」状態で内向きに動こうとしたら必ず止める。
		if (overlapV && overlapN)
		{
			const float outsideUpos = localU - (maxU + rU);      // >0: +U外側, <=0: +U面より内側(接触含む)
			if (dU < 0.0f && (outsideUpos + dU) <= skin)         // 内向き(-U)に進んで skin 以内に入るなら停止
			{
				player->Velocity.x = 0.0f;
				player->Velocity.z = 0.0f;
				player->blockMovement = true;
				return true;
			}

			const float outsideUneg = (minU - rU) - localU;      // >0: -U外側
			if (dU > 0.0f && (outsideUneg - dU) <= skin)         // 内向き(+U)に進んで skin 以内に入るなら停止
			{
				player->Velocity.x = 0.0f;
				player->Velocity.z = 0.0f;
				player->blockMovement = true;
				return true;
			}
		}

		// +N / -N sides (thickness faces):
		if (overlapU && overlapV)
		{
			const float outsideNpos = localN - (prism->thickness + rN); // >0: +N外側
			if (dN < 0.0f && (outsideNpos + dN) <= skin)                // 内向き(-N)へ進んで skin 以内なら停止
			{
				player->Velocity.x = 0.0f;
				player->Velocity.z = 0.0f;
				player->blockMovement = true;
				return true;
			}

			const float outsideNneg = (-rN) - localN;                   // >0: -N外側
			if (dN > 0.0f && (outsideNneg - dN) <= skin)                // 内向き(+N)へ進んで skin 以内なら停止
			{
				player->Velocity.x = 0.0f;
				player->Velocity.z = 0.0f;
				player->blockMovement = true;
				return true;
			}
		}
	}

	return false;
}



bool Player2DShadow_Collision()
{
	PLAYER* player = GetPlayer2D();
	if (!player) return false;

	const std::vector<const ShadowPrism*>& prisms = Collision_GetShadowPrisms();
	if (prisms.empty()) return false;

	const XMFLOAT3 half = Player2D_GetSolidHalfSize();

	// "skin" prevents tiny numerical penetration and avoids jitter.
	const float skin = 0.01f;
	const float eps = 1e-6f;
	const float hitEps = 1e-4f;

	// We only constrain horizontal movement here (like wall collision).
	XMFLOAT3 dXZ = { player->Velocity.x, 0.0f, player->Velocity.z };

	// If almost not moving horizontally, we still want to allow "top as ground" logic (Y).
	bool hitAny = false;

	auto Area2 = [](const std::vector<XMFLOAT2>& p) -> float
		{
			float a = 0.0f;
			const int n = (int)p.size();
			for (int i = 0, j = n - 1; i < n; j = i++)
				a += p[j].x * p[i].y - p[i].x * p[j].y;
			return a;
		};

	auto Dot2 = [](const XMFLOAT2& a, const XMFLOAT2& b) -> float { return a.x * b.x + a.y * b.y; };
	auto Sub2 = [](const XMFLOAT2& a, const XMFLOAT2& b) -> XMFLOAT2 { return { a.x - b.x, a.y - b.y }; };
	auto Len2 = [](const XMFLOAT2& v) -> float { return sqrtf(v.x * v.x + v.y * v.y); };

	// Sweep segment (p0 -> p0 + dp) against a convex polygon expanded by "rEdge" per edge.
	auto SweepEnterExpanded = [&](const ShadowPrism* prism,
		const std::vector<XMFLOAT2>& polyCCW,
		const XMFLOAT2& p0,
		const XMFLOAT2& dp,
		float yawDeg,
		float* outEnterT) -> bool
		{
			const int n = (int)polyCCW.size();
			float tEnter = 0.0f;
			float tExit = 1.0f;

			for (int i = 0; i < n; ++i)
			{
				const XMFLOAT2 a = polyCCW[i];
				const XMFLOAT2 b = polyCCW[(i + 1) % n];
				const XMFLOAT2 e = { b.x - a.x, b.y - a.y };

				// inward normal (poly is CCW)
				XMFLOAT2 nIn = { -e.y, e.x };
				float nl = Len2(nIn);
				if (nl < 1e-6f) continue;
				nIn.x /= nl; nIn.y /= nl;

				// world axis corresponding to this inward normal
				const XMFLOAT3 axisW = prism->u * nIn.x + prism->v * nIn.y; // unit length (u,v are orthonormal)
				const float rEdge = ProjectRadiusOnAxis_Yaw(axisW, half, yawDeg) + skin;

				const float s0 = Dot2(nIn, Sub2(p0, a));
				const float sv = Dot2(nIn, dp);

				// want: s0 + sv*t >= -rEdge
				if (fabsf(sv) < 1e-6f)
				{
					// Parallel: must already satisfy
					if (s0 < -rEdge) return false;
					continue;
				}

				const float t = (-rEdge - s0) / sv;

				if (sv > 0.0f)
				{
					// entering this halfspace
					if (t > tEnter) tEnter = t;
				}
				else
				{
					// leaving this halfspace
					if (t < tExit) tExit = t;
				}

				if (tEnter > tExit) return false;
			}

			if (outEnterT) *outEnterT = tEnter;
			return true;
		};

	// current collider center (for local projection)
	auto GetCenter = [&]() -> XMFLOAT3
		{
			return { player->Position.x, player->Position.y + half.y, player->Position.z };
		};

	for (const ShadowPrism* prism : prisms)
	{
		if (!prism || !prism->isValid) continue;
		if (prism->poly.size() < 3) continue;

		// Quick horizontal AABB gate
		const float moveLen = sqrtf(dXZ.x * dXZ.x + dXZ.z * dXZ.z);
		const float gate = half.x + half.z + moveLen + 0.10f;
		const XMFLOAT3 cW0 = GetCenter();
		if (cW0.x < prism->aabbMin.x - gate || cW0.x > prism->aabbMax.x + gate ||
			cW0.z < prism->aabbMin.z - gate || cW0.z > prism->aabbMax.z + gate)
		{
			continue;
		}

		// Make a CCW local polygon (u-v plane coordinates)
		std::vector<XMFLOAT2> poly = prism->poly;
		if (Area2(poly) < 0.0f)
			std::reverse(poly.begin(), poly.end());

		// Local coordinates (u,v,n) of player center
		const XMFLOAT3 cW = GetCenter();
		const XMFLOAT3 rel = cW - prism->origin;
		const float u0 = Dot(rel, prism->u);
		const float v0 = Dot(rel, prism->v);
		const float n0 = Dot(rel, prism->n);
		const XMFLOAT2 p0 = { u0, v0 };

		const float rN = ProjectRadiusOnAxis_Yaw(prism->n, half, player->Rotation.y);
		const bool overlapN = (n0 >= -rN) && (n0 <= prism->thickness + rN);

		// ------------------------------------------------------------------
		// TOP (ground-like) contact: only when prism normal points mostly upward.
		// Use prism local "n" as the support direction; mark isGround only for up-facing prisms.
		// ------------------------------------------------------------------
		if (prism->n.y > 0.7f)
		{
			// check footprint (use local 2D polygon; ray-cast point-in-polygon)
			const bool insideFoot = PointInPolygon2D(p0.x, p0.y, poly);

			// bottom of player along n-axis
			const float bottomN = n0 - rN;

			// approaching the top face from above (+n side) with downward velocity
			if (insideFoot && player->Velocity.y <= 0.0f)
			{
				// If we are close enough to the top face this frame, snap to it.
				// top face is at localN = thickness
				const float targetBottomN = prism->thickness;
				const float distToTop = bottomN - targetBottomN;

				if (distToTop <= 0.05f && distToTop >= -0.25f)
				{
					// move player so bottom touches top
					player->Position = player->Position - prism->n * distToTop;

					// cancel velocity component along n (for n≈up this cancels falling)
					const float vN = Dot(player->Velocity, prism->n);
					if (vN < 0.0f)
						player->Velocity = player->Velocity - prism->n * vN;

					player->isGround = true;
					hitAny = true;
				}
			}
		}

		// If we don't overlap prism thickness, side walls can't be touched.
		if (!overlapN) continue;

		// If we're on/above the top face, allow moving within footprint (standing on it).
		{
			const float bottomN = n0 - rN;
			if (prism->n.y > 0.7f && bottomN >= prism->thickness - 0.02f)
				continue;
		}

		// If no horizontal movement, nothing else to do.
		if (moveLen < eps) continue;

		// Project horizontal movement into prism (u,v) coordinates.
		XMFLOAT2 dp = { DotXZ(dXZ, prism->u), DotXZ(dXZ, prism->v) };
		if (fabsf(dp.x) + fabsf(dp.y) < 1e-7f) continue;

		// Determine whether we are already inside the expanded polygon at start.
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
			const float rEdge = ProjectRadiusOnAxis_Yaw(axisW, half, player->Rotation.y) + skin;

			const float s0 = Dot2(nIn, Sub2(p0, a));
			if (s0 < -rEdge) { insideExp0 = false; break; }
		}

		if (!insideExp0)
		{
			// Outside => prevent entering by stopping at the first time we would touch the expanded polygon.
			float tEnter = 0.0f;
			if (SweepEnterExpanded(prism, poly, p0, dp, player->Rotation.y, &tEnter))
			{
				if (tEnter <= 1.0f && tEnter >= 0.0f)
				{
					// If we would enter within this frame, clamp movement just before entry.
					if (tEnter < 1.0f)
					{
						const float tStop = (std::max)(0.0f, (std::min)(1.0f, tEnter - hitEps));
						dXZ.x *= tStop;
						dXZ.z *= tStop;
						hitAny = true;
					}
				}
			}
		}
		else
		{
			// Already inside expanded band (touching) => remove the inward component near the boundary.
			// This gives "wall-like" behavior: push into it => no forward speed; move tangentially => slide.
			for (int pass = 0; pass < 2; ++pass)
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
					const float rEdge = ProjectRadiusOnAxis_Yaw(axisW, half, player->Rotation.y) + skin;

					const float s0 = Dot2(nIn, Sub2(p0, a));
					const float slack = s0 + rEdge; // boundary at 0

					// Only block when we're very close to the boundary (prevents "magnetic" stop far away).
					if (slack > 0.03f) continue;

					// moving inward?
					const float vIn = Dot2(nIn, dpNow);
					if (vIn <= 0.0f) continue;

					// Remove the inward component from dXZ (project out along axis in XZ).
					XMFLOAT3 axisXZ = { axisW.x, 0.0f, axisW.z };
					const float len2xz = axisXZ.x * axisXZ.x + axisXZ.z * axisXZ.z;
					if (len2xz < 1e-6f) continue;

					const float proj = (dXZ.x * axisXZ.x + dXZ.z * axisXZ.z) / len2xz;
					if (proj > 0.0f)
					{
						dXZ.x -= axisXZ.x * proj;
						dXZ.z -= axisXZ.z * proj;
						changed = true;
						hitAny = true;
					}
				}
				if (!changed) break;
			}
		}
	}

	// Write back constrained horizontal displacement
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

	XMFLOAT3 halfSize = Player2D_GetSolidHalfSize();

	bool hitAny = false;

	// 暋悢夞偺斀暅偱埨掕偝偣傞
	const int maxIterations = 4;

	for (int iter = 0; iter < maxIterations; iter++)
	{
		bool hitThisIter = false;

		float footY = player->Position.y;

		// 僾儗僀儎乕偺拞怱埵抲乮枅夞峏怴乯
		XMFLOAT3 playerCenter = XMFLOAT3(
			player->Position.x,
			player->Position.y + halfSize.y,
			player->Position.z
		);

		for (const ShadowPrism* prism : prisms)
		{
			if (!prism || !prism->isValid) continue;
			if (prism->poly.size() < 3) continue;

			// AABB憗婜僉儍儞僙儖
			float margin = halfSize.x + 0.1f;
			if (
				footY > prism->aabbMax.y + margin ||           // 懌尦偑塭傛傝忋偡偓傞
				footY + halfSize.y * 2.0f < prism->aabbMin.y - margin  // 摢偑塭傛傝壓偡偓傞
				)
			{
				continue;
			}

			// 塭偺忋柺Y嵗昗
			float shadowTopY = prism->aabbMax.y;

			// 僾儗僀儎乕偑X-Z斖埻撪偵偄傞偐僠僃僢僋
			bool inXZRange = (playerCenter.x >= prism->aabbMin.x - halfSize.x &&
				playerCenter.x <= prism->aabbMax.x + halfSize.x &&
				playerCenter.z >= prism->aabbMin.z - halfSize.z &&
				playerCenter.z <= prism->aabbMax.z + halfSize.z);

			if (!inXZRange) continue;

			float footToShadowTop = footY - shadowTopY;

			// 懌尦偑塭偺忋柺傛傝彮偟壓乣彮偟忋偺斖埻偵偄傞応崌
			if (footToShadowTop >= -halfSize.y && footToShadowTop <= 0.1f)
			{
				// 壓崀拞傑偨偼掆巭拞偺応崌丄拝抧
				if (player->Velocity.y <= 0.01f)
				{
					// 懌尦傪塭偺忋柺偵攝抲
					player->Position.y = shadowTopY;

					// Y曽岦偺懍搙傪掆巭
					player->Velocity.y = 0.0f;

					// 愙抧僼儔僌傪愝掕
					player->isGround = true;

					hitThisIter = true;
					hitAny = true;
					continue;  // 偙偺塭偲偺張棟偼姰椆
				}
			}

			// 懁柺丒掙柺偲偺徴撍敾掕
			// 僾儗僀儎乕埵抲傪僾儕僘儉偺儘乕僇儖嵗昗偵曄姺
			XMFLOAT3 rel = playerCenter - prism->origin;
			float localU = Dot(rel, prism->u);
			float localV = Dot(rel, prism->v);
			float localN = Dot(rel, prism->n);

			// 僾儕僘儉偺U-V斖埻傪寁嶼
			float minU = FLT_MAX, maxU = -FLT_MAX;
			float minV = FLT_MAX, maxV = -FLT_MAX;
			for (const auto& p : prism->poly)
			{
				minU = (std::min)(minU, p.x);
				maxU = (std::max)(maxU, p.x);
				minV = (std::min)(minV, p.y);
				maxV = (std::max)(maxV, p.y);
			}

			// 僾儗僀儎乕偺敿宎
			float playerRadiusU = halfSize.x;
			float playerRadiusV = halfSize.y;
			float playerRadiusN = halfSize.z;

			// 奺曽岦偺怤擖検傪寁嶼
			float penU_pos = (maxU + playerRadiusU) - localU;
			float penU_neg = localU - (minU - playerRadiusU);
			float penV_pos = (maxV + playerRadiusV) - localV;
			float penV_neg = localV - (minV - playerRadiusV);
			float penN_pos = (prism->thickness + playerRadiusN) - localN;
			float penN_neg = localN + playerRadiusN;

			// 慡曽岦偱怤擖偟偰偄傞偐僠僃僢僋
			if (penU_pos <= 0 || penU_neg <= 0 ||
				penV_pos <= 0 || penV_neg <= 0 ||
				penN_pos <= 0 || penN_neg <= 0)
			{
				continue;
			}

			// 嵟彫怤擖検偺曽岦傪尒偮偗傞乮忋柺偼彍奜丄婛偵張棟嵪傒乯
			float minPen = FLT_MAX;
			int pushDir = 0;

			if (penV_neg < minPen) { minPen = penV_neg; pushDir = 4; }

			if (pushDir == 0 || minPen <= 0.001f) continue;

			switch (pushDir)
			{
			case 4:
			{
				float targetLocalV = minV - playerRadiusV;
				float deltaV = targetLocalV - localV;
				player->Position.y += prism->v.y * deltaV;
				if (player->Velocity.y > 0)
				{
					player->Velocity.y = 0.0f;
				}
			}
			break;

			}


			hitThisIter = true;
			hitAny = true;
		}

		// 偙偺斀暅偱徴撍偑側偗傟偽廔椆
		if (!hitThisIter) break;
	}

	return hitAny;
}


// 僨僶僢僌昤夋
void Collision_DebugDraw()
{
	// 3D僾儗僀儎乕僐儔僀僟乕
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
	// 2D僾儗僀儎乕僐儔僀僟乕
	PLAYER* player2D = GetPlayer2D();
	if (player2D)
	{
		XMFLOAT3 pC = GetPlayer2DSolidCollider();
		XMFLOAT3 pH = Player2D_GetSolidHalfSize();
		DebugDrawOBB_Yaw(pC, pH, player2D->Rotation.y, IM_COL32(0, 255, 0, 255));
	}

	// 僼傿乕儖僪僩儕僈乕
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
