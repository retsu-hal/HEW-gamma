#include "Collision.h"
#include "player3D.h"


#include "camera.h"
#include "direct3d.h"
#include "debug.h"
#include "Player2D.h"
using namespace DirectX;

static bool debugMode = TRUE;


//=========================================================================================================
// ボールとフィールドの当たり判定
//=========================================================================================================

// ボックスの半分のサイズを取得
static XMFLOAT3 Field_GetHalfSize(const MAPDATA& m)
{
	return XMFLOAT3{
	BOX_RADIUS * m.scale.x,
	BOX_RADIUS * m.scale.y,
	BOX_RADIUS * m.scale.z
	};
}
// プレイヤーの当たり判定中心座標を取得
static XMFLOAT3 GetPlayerSolidCollider()
{
	PLAYER3D* p = GetPlayer3D();
	XMFLOAT3 c = p->Position;

	XMFLOAT3 half = Player3D_GetSolidHalfSize();
	c.y += half.y;
	return c;
}
// 2Dプレイヤーの当たり判定中心座標を取得
static XMFLOAT3 GetPlayer2DSolidCollider()
{
	PLAYER2D* p = GetPlayer2D();
	XMFLOAT3 c = p->Position;

	XMFLOAT3 half = Player3D_GetSolidHalfSize();
	c.y += half.y + 0.1f;
	return c;
}

// プレイヤーのトリガー当たり判定中心座標を取得
static XMFLOAT3 GetPlayerTriggerCollider()
{
	PLAYER3D* p = GetPlayer3D();
	XMFLOAT3 c = p->Position;

	XMFLOAT3 half = Player3D_GetTriggerHalfSize();
	c.y += half.y;
	return c;
}

// 2Dベクトルの内積・長さ・正規化
static float Dot2D(const XMFLOAT3& a, const XMFLOAT3& b) { return a.x * b.x + a.z * b.z; }
static float Len2D(const XMFLOAT3& v) { return sqrtf(v.x * v.x + v.z * v.z); }
static XMFLOAT3 Normalize2D(XMFLOAT3 v)
{
	float l = Len2D(v);
	if (l < 1e-6f) return XMFLOAT3(0, 0, 1);
	v.x /= l; v.z /= l; v.y = 0.0f;
	return v;
}
// 値を範囲内に収める
static float Clampf(float v, float a, float b) { return (v < a) ? a : (v > b) ? b : v; }
// 3Dベクトルの加算・減算・乗算
static XMFLOAT3 Add3(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
static XMFLOAT3 Sub3(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
static XMFLOAT3 Mul3(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x * b.x, a.y * b.y, a.z * b.z }; }
static XMFLOAT3 Mul3f(const XMFLOAT3& a, float s) { return { a.x * s, a.y * s, a.z * s }; }
// 3Dベクトルの内積・長さ・正規化
static float Len3(const XMFLOAT3& v) { return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z); }
static XMFLOAT3 Normalize3(XMFLOAT3 v)
{
	float l = Len3(v);
	if (l < 1e-6f) return { 0,1,0 };
	v.x /= l; v.y /= l; v.z /= l;
	return v;
}
// Y軸回転
static XMFLOAT3 RotateY(const XMFLOAT3& v, float yawRad)
{
	const float c = cosf(yawRad);
	const float s = sinf(yawRad);
	return { v.x * c + v.z * s, v.y, -v.x * s + v.z * c };
}


// トリガーが当たった面を計算（Player3d Yaw基準）
static TRIGGER_SIDE CalcSide_ByCamera(const XMFLOAT3& playerC, const XMFLOAT3& targetC)
{
	PLAYER3D* p = GetPlayer3D();
	if (p)
	{
		const float yawRad = XMConvertToRadians(p->Rotation.y);

		XMFLOAT3 forward = Normalize2D({ sinf(yawRad), 0.0f, cosf(yawRad) });
		XMFLOAT3 right = { forward.z, 0.0f, -forward.x };

		XMFLOAT3 to = { targetC.x - playerC.x, 0.0f, targetC.z - playerC.z };
		float f = Dot2D(to, forward);
		float r = Dot2D(to, right);

		if (fabsf(f) < 1e-5f && fabsf(r) < 1e-5f) return TRIGGER_SIDE_NONE;

		if (fabsf(f) >= fabsf(r)) return (f >= 0.0f) ? TRIGGER_SIDE_FRONT : TRIGGER_SIDE_BACK;
		return (r >= 0.0f) ? TRIGGER_SIDE_RIGHT : TRIGGER_SIDE_LEFT;
	}

	XMFLOAT3 camPos = GetCameraPosition();
	XMFLOAT3 camAt = GetCameraAtPosition();

	XMFLOAT3 forward = Normalize2D({ camAt.x - camPos.x, 0, camAt.z - camPos.z });
	XMFLOAT3 right = { forward.z, 0, -forward.x };

	XMFLOAT3 to = { targetC.x - playerC.x, 0, targetC.z - playerC.z };
	float f = Dot2D(to, forward);
	float r = Dot2D(to, right);

	if (fabsf(f) < 1e-5f && fabsf(r) < 1e-5f) return TRIGGER_SIDE_NONE;

	if (fabsf(f) >= fabsf(r)) return (f >= 0) ? TRIGGER_SIDE_FRONT : TRIGGER_SIDE_BACK;
	return (r >= 0) ? TRIGGER_SIDE_RIGHT : TRIGGER_SIDE_LEFT;
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


// ワールド座標をスクリーン座標に変換（安全版）
struct ScreenPoint// スクリーン座標
{
	ImVec2 pos;
	bool   valid;
};
static ScreenPoint WorldToScreenSafe(const XMFLOAT3& pWS)
{
	using namespace DirectX;

	ScreenPoint out{};
	out.valid = false;

	XMMATRIX view = GetViewMatrix();
	XMMATRIX proj = GetProjectionMatrix();
	XMMATRIX vp = XMMatrixMultiply(view, proj);

	XMVECTOR vWS = XMVectorSet(pWS.x, pWS.y, pWS.z, 1.0f);
	XMVECTOR vView = XMVector3TransformCoord(vWS, view);
	float zView = XMVectorGetZ(vView);
	if (zView <= 0.01f) {
		return out;
	}

	XMVECTOR vClip = XMVector3TransformCoord(vWS, vp);
	XMFLOAT3 ndc;
	XMStoreFloat3(&ndc, vClip);

	if (ndc.x < -1.5f || ndc.x > 1.5f || ndc.y < -1.5f || ndc.y > 1.5f) {
		return out;
	}

	ImGuiViewport* vpIm = ImGui::GetMainViewport();
	const ImVec2   pos = vpIm->Pos;
	const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth();
	const float SCREEN_HEIGHT = (float)Direct3D_GetBackBufferHeight();

	float x = pos.x + (ndc.x * 0.5f + 0.5f) * SCREEN_WIDTH;
	float y = pos.y + (-ndc.y * 0.5f + 0.5f) * SCREEN_HEIGHT;

	out.pos = ImVec2(x, y);
	out.valid = true;
	return out;
}

// AABB同士の当たり判定
static bool AABB_Intersect(const XMFLOAT3& c0, const XMFLOAT3& h0,
	const XMFLOAT3& c1, const XMFLOAT3& h1) {

	if (fabsf(c0.x - c1.x) > (h0.x + h1.x)) return false;
	if (fabsf(c0.y - c1.y) > (h0.y + h1.y)) return false;
	if (fabsf(c0.z - c1.z) > (h0.z + h1.z)) return false;
	return true;
}

// OBB（Y軸回転のみ）に対する最近接点を計算
static XMFLOAT3 ClosestPointOBB_Yaw(const XMFLOAT3& p, const XMFLOAT3& c, const XMFLOAT3& h, float yawRad)
{
	XMFLOAT3 d = Sub3(p, c);
	XMFLOAT3 dl = RotateY(d, -yawRad);

	dl.x = Clampf(dl.x, -h.x, +h.x);
	dl.y = Clampf(dl.y, -h.y, +h.y);
	dl.z = Clampf(dl.z, -h.z, +h.z);

	XMFLOAT3 dw = RotateY(dl, yawRad);
	return Add3(c, dw);
}
// OBBのローカルX/Z軸を取得（Y軸回転のみ）
static void OBB_GetAxes_Yaw(float yawDeg, XMFLOAT3* outAxisX, XMFLOAT3* outAxisZ)
{
	const float yaw = XMConvertToRadians(yawDeg);

	// forward: yaw=0 -> +Z
	XMFLOAT3 forward = Normalize2D({ sinf(yaw), 0.0f, cosf(yaw) });
	// right
	XMFLOAT3 right = { forward.z, 0.0f, -forward.x };

	*outAxisX = right;   // local X axis in world (XZ)
	*outAxisZ = forward; // local Z axis in world (XZ)
}
// OBB同士の当たり判定（Y軸回転のみ）
static bool OBB_Intersect_Yaw(const XMFLOAT3& cA, const XMFLOAT3& hA, float yawA,
	const XMFLOAT3& cB, const XMFLOAT3& hB, float yawB)
{
	if (fabsf(cA.y - cB.y) > (hA.y + hB.y)) return false;

	XMFLOAT3 Ax, Az, Bx, Bz;
	OBB_GetAxes_Yaw(yawA, &Ax, &Az);
	OBB_GetAxes_Yaw(yawB, &Bx, &Bz);

	XMFLOAT3 T = { cB.x - cA.x, 0.0f, cB.z - cA.z };

	float t0 = Dot2D(T, Ax);
	float t1 = Dot2D(T, Az);

	float R00 = Dot2D(Ax, Bx);
	float R01 = Dot2D(Ax, Bz);
	float R10 = Dot2D(Az, Bx);
	float R11 = Dot2D(Az, Bz);

	const float eps = 1e-6f;
	float AbsR00 = fabsf(R00) + eps;
	float AbsR01 = fabsf(R01) + eps;
	float AbsR10 = fabsf(R10) + eps;
	float AbsR11 = fabsf(R11) + eps;

	float a0 = hA.x;
	float a1 = hA.z;

	float b0 = hB.x;
	float b1 = hB.z;

	if (fabsf(t0) > (a0 + b0 * AbsR00 + b1 * AbsR01)) return false;
	if (fabsf(t1) > (a1 + b0 * AbsR10 + b1 * AbsR11)) return false;

	float tB0 = fabsf(t0 * R00 + t1 * R10);
	if (tB0 > (b0 + a0 * AbsR00 + a1 * AbsR10)) return false;

	float tB1 = fabsf(t0 * R01 + t1 * R11);
	if (tB1 > (b1 + a0 * AbsR01 + a1 * AbsR11)) return false;

	return true;
}
// 楕円体 vs OBB（Y軸回転のみ） の当たり判定と解決
static bool Resolve_Ellipsoid_OBB_Yaw(const XMFLOAT3& ellCenterW, const XMFLOAT3& ellRadiiW,
	const XMFLOAT3& boxCenterW,const XMFLOAT3& boxHalfW, float boxYawDeg, XMFLOAT3* outPushW,
	XMFLOAT3* outNormalW)
{
	if (outPushW)  *outPushW = { 0,0,0 };
	if (outNormalW) *outNormalW = { 0,1,0 };

	XMFLOAT3 invR = { 1.0f / ellRadiiW.x, 1.0f / ellRadiiW.y, 1.0f / ellRadiiW.z };

	XMFLOAT3 cS = Mul3(ellCenterW, invR);
	XMFLOAT3 bS = Mul3(boxCenterW, invR);
	XMFLOAT3 hS = Mul3(boxHalfW, invR);

	const float yawRad = XMConvertToRadians(boxYawDeg);

	XMFLOAT3 qS = ClosestPointOBB_Yaw(cS, bS, hS, yawRad);
	XMFLOAT3 dS = Sub3(cS, qS);
	float dist = Len3(dS);

	XMFLOAT3 nS = { 0,1,0 };
	float pen = 0.0f;

	if (dist >= 1.0f) return false;

	if (dist > 1e-6f)
	{
		nS = Mul3f(dS, 1.0f / dist);
		pen = 1.0f - dist;
	}
	else
	{
		XMFLOAT3 lp = RotateY(Sub3(cS, bS), -yawRad);

		float dxFace = hS.x - fabsf(lp.x);
		float dyFace = hS.y - fabsf(lp.y);
		float dzFace = hS.z - fabsf(lp.z);

		XMFLOAT3 localN = { 0,1,0 };

		if (dxFace <= dyFace && dxFace <= dzFace)
		{
			localN = { (lp.x >= 0.0f) ? 1.0f : -1.0f, 0, 0 };
			pen = 1.0f + dxFace;
		}
		else if (dyFace <= dzFace)
		{
			localN = { 0, (lp.y >= 0.0f) ? 1.0f : -1.0f, 0 };
			pen = 1.0f + dyFace;
		}
		else
		{
			localN = { 0, 0, (lp.z >= 0.0f) ? 1.0f : -1.0f };
			pen = 1.0f + dzFace;
		}

		nS = RotateY(localN, yawRad);
	}

	XMFLOAT3 pushS = Mul3f(nS, pen);

	XMFLOAT3 pushW = { pushS.x * ellRadiiW.x, pushS.y * ellRadiiW.y, pushS.z * ellRadiiW.z };

	XMFLOAT3 nW = Normalize3({ nS.x * ellRadiiW.x, nS.y * ellRadiiW.y, nS.z * ellRadiiW.z });

	if (outPushW) *outPushW = pushW;
	if (outNormalW) *outNormalW = nW;
	return true;
}


// プレイヤーとフィールドの当たり判定
int Player3DField_Collision()
{
	int hit = HIT_NONE;

	PLAYER3D* player3D = GetPlayer3D();
	std::vector<MAPDATA>& Map = GetFieldMap();
	if (!player3D || Map.empty()) return hit;

	player3D->isGround = false;

	const XMFLOAT3 ellR = Player3D_GetSolidHalfSize();

	XMFLOAT3 ellC = GetPlayerSolidCollider();

	for (size_t i = 0; i < Map.size(); ++i)
	{
		if (!Field_IsSolid(Map[i].no)) continue;

		const XMFLOAT3 boxC = Map[i].pos;
		const XMFLOAT3 boxH = Field_GetHalfSize(Map[i]);

		float boxYawDeg = 0.0f;
		if (Map[i].no == FIELD_OBJ_1)
		{
			boxYawDeg = Map[i].rotate.y;
		}

		XMFLOAT3 pushW{}, nW{};
		if (!Resolve_Ellipsoid_OBB_Yaw(ellC, ellR, boxC, boxH, boxYawDeg, &pushW, &nW))
			continue;

		ellC = Add3(ellC, pushW);

		float ax = fabsf(nW.x), ay = fabsf(nW.y), az = fabsf(nW.z);

		if (ay >= ax && ay >= az)
		{
			if (nW.y > 0.0f)
			{
				player3D->isGround = true;
				player3D->Velocity.y = 0.0f;
				hit = HIT_GROUND;
			}
			else
			{
				if (player3D->Velocity.y > 0.0f) player3D->Velocity.y = 0.0f;
			}
		}
		else if (ax >= az)
		{
			player3D->Velocity.x = 0.0f;
			hit = (nW.x > 0.0f) ? HIT_WALL_PlusX : HIT_WALL_NegX;
		}
		else
		{
			player3D->Velocity.z = 0.0f;
			hit = (nW.z > 0.0f) ? HIT_WALL_PlusZ : HIT_WALL_NegZ;
		}
	}

	player3D->Position.x = ellC.x;
	player3D->Position.y = ellC.y - ellR.y;
	player3D->Position.z = ellC.z;

	return hit;
}

// プレイヤーとトリガーの当たり判定
bool Collision_PlayerTrigger(TRIGGER_HIT* outHit, float extraRange)
{
	if (outHit) *outHit = TRIGGER_HIT{};

	PLAYER3D* p = GetPlayer3D();
	if (!p) return false;

	auto& map = GetFieldMap();
	if (map.empty()) return false;

	// プレイヤーのトリガーOBB
	const XMFLOAT3 pHalf = Player3D_GetTriggerHalfSize();
	const XMFLOAT3 pC = GetPlayerTriggerCollider();
	const float pYaw = p->Rotation.y;

	bool found = false;
	float bestD2 = 1e30f;
	TRIGGER_HIT best;

	for (size_t i = 0; i < map.size(); ++i)
	{
		if (!Field_IsTrigger(map[i].no)) continue;

		// フィールドのOBB
		const XMFLOAT3 tC = map[i].pos;

		XMFLOAT3 tHalf = Field_GetHalfSize(map[i]);
		tHalf.x += extraRange;
		tHalf.y += extraRange;
		tHalf.z += extraRange;

		const float tYaw = map[i].rotate.y;

		if (!OBB_Intersect_Yaw(pC, pHalf, pYaw, tC, tHalf, tYaw)) continue;

		float dx = tC.x - pC.x;
		float dz = tC.z - pC.z;
		float d2 = dx * dx + dz * dz;

		if (!found || d2 < bestD2)
		{
			found = true;
			bestD2 = d2;
			best.hit = true;
			best.mapIndex = i;
			best.type = map[i].no;
			best.side = CalcSide_ByCamera(pC, tC);
		}
	}

	if (!found) return false;
	if (outHit) *outHit = best;
	return true;
}

// AABBをデバッグ描画
static void DebugDrawAABB(const XMFLOAT3& center,
	const XMFLOAT3& half, ImU32 color) {

	ImDrawList* draw = ImGui::GetBackgroundDrawList();

	XMFLOAT3 c = center;
	XMFLOAT3 h = half;

	XMFLOAT3 corners[8] =
	{
		{c.x - h.x, c.y - h.y, c.z - h.z},
		{c.x + h.x, c.y - h.y, c.z - h.z},
		{c.x + h.x, c.y + h.y, c.z - h.z},
		{c.x - h.x, c.y + h.y, c.z - h.z},
		{c.x - h.x, c.y - h.y, c.z + h.z},
		{c.x + h.x, c.y - h.y, c.z + h.z},
		{c.x + h.x, c.y + h.y, c.z + h.z},
		{c.x - h.x, c.y + h.y, c.z + h.z},
	};

	ScreenPoint pts[8];
	for (int i = 0; i < 8; ++i)
		pts[i] = WorldToScreenSafe(corners[i]);

	auto Line = [&](int a, int b)
		{
			if (pts[a].valid && pts[b].valid)
			{
				draw->AddLine(pts[a].pos, pts[b].pos, color, 1.0f);
			}
		};


	Line(0, 1); Line(1, 2); Line(2, 3); Line(3, 0);

	Line(4, 5); Line(5, 6); Line(6, 7); Line(7, 4);

	Line(0, 4); Line(1, 5); Line(2, 6); Line(3, 7);
}
// OBBをYaw回転付きでデバッグ描画
static void DebugDrawOBB_Yaw(const XMFLOAT3& center,
	const XMFLOAT3& half, float yawDeg, ImU32 color)
{
	ImDrawList* draw = ImGui::GetBackgroundDrawList();

	const float yaw = XMConvertToRadians(yawDeg);
	const float c = cosf(yaw);
	const float s = sinf(yaw);

	XMFLOAT3 local[8] =
	{
		{-half.x, -half.y, -half.z},
		{+half.x, -half.y, -half.z},
		{+half.x, +half.y, -half.z},
		{-half.x, +half.y, -half.z},
		{-half.x, -half.y, +half.z},
		{+half.x, -half.y, +half.z},
		{+half.x, +half.y, +half.z},
		{-half.x, +half.y, +half.z},
	};

	XMFLOAT3 corners[8];
	for (int i = 0; i < 8; ++i)
	{
		const float x = local[i].x;
		const float z = local[i].z;

		const float rx = x * c + z * s;
		const float rz = -x * s + z * c;

		corners[i] = { center.x + rx, center.y + local[i].y, center.z + rz };
	}

	ScreenPoint pts[8];
	for (int i = 0; i < 8; ++i)
		pts[i] = WorldToScreenSafe(corners[i]);

	auto Line = [&](int a, int b)
		{
			if (pts[a].valid && pts[b].valid)
				draw->AddLine(pts[a].pos, pts[b].pos, color, 1.0f);
		};

	Line(0, 1); Line(1, 2); Line(2, 3); Line(3, 0);
	Line(4, 5); Line(5, 6); Line(6, 7); Line(7, 4);
	Line(0, 4); Line(1, 5); Line(2, 6); Line(3, 7);
}
// 楕円体をデバッグ描画
static void DebugDrawEllipsoid(const XMFLOAT3& center, const XMFLOAT3& radii, ImU32 color, int segments = 32)
{
	ImDrawList* draw = ImGui::GetBackgroundDrawList();
	const float twoPi = 6.28318530718f;

	auto DrawRing = [&](int plane)
		{
			XMFLOAT3 prev{};
			bool hasPrev = false;

			for (int i = 0; i <= segments; ++i)
			{
				float t = twoPi * (float)i / (float)segments;
				float ct = cosf(t), st = sinf(t);

				XMFLOAT3 p = center;
				if (plane == 0) { p.x += ct * radii.x; p.z += st * radii.z; }
				if (plane == 1) { p.x += ct * radii.x; p.y += st * radii.y; }
				if (plane == 2) { p.y += ct * radii.y; p.z += st * radii.z; }

				ScreenPoint sp = WorldToScreenSafe(p);
				if (hasPrev)
				{
					ScreenPoint spPrev = WorldToScreenSafe(prev);
					if (spPrev.valid && sp.valid)
						draw->AddLine(spPrev.pos, sp.pos, color, 1.0f);
				}
				prev = p;
				hasPrev = true;
			}
		};

	DrawRing(0);
	DrawRing(1);
	DrawRing(2);
}

// 当たり判定のデバッグ描画
void Collision_DebugDraw() {

	// プレイヤーのAABB描画
	PLAYER3D* player = GetPlayer3D();// プレイヤー取得
	XMFLOAT3 playerHalf = Player3D_GetSolidHalfSize();
	XMFLOAT3 playerC = GetPlayerSolidCollider();

	XMFLOAT3 playerHalf_t = Player3D_GetTriggerHalfSize();
	XMFLOAT3 playerC_t = GetPlayerTriggerCollider();
	if (player)
	{
		DebugDrawEllipsoid(playerC, playerHalf, IM_COL32(0, 255, 0, 255));// プレイヤーのAABB描画
		DebugDrawOBB_Yaw(playerC_t, playerHalf_t, player->Rotation.y, IM_COL32(255, 255, 255, 255));// プレイヤーのAABB描画
	}

	PLAYER2D* player2D = GetPlayer2D();
	XMFLOAT3 player2DHalf = Player2D_GetSolidHalfSize();
	XMFLOAT3 player2DC = GetPlayer2DSolidCollider();

	if (player2D)
	{
		DebugDrawOBB_Yaw(player2DC, player2DHalf, player2D->Rotation.y, IM_COL32(0, 255, 0, 255));// プレイヤーのAABB描画
	}
	

	// フィールドのAABB描画
	std::vector<MAPDATA>& map = GetFieldMap();
	if (map.size() == 0) return;
	for (size_t i = 0; i < map.size(); ++i)
	{
		if (!Field_IsTrigger(map[i].no)) continue;
		const XMFLOAT3 boxHalf = Field_GetHalfSize(map[i]);
		const XMFLOAT3& boxC = map[i].pos;
		const float boxYaw = map[i].rotate.y;
		bool triggered_t = OBB_Intersect_Yaw(playerC_t, playerHalf_t, player->Rotation.y, boxC, boxHalf, boxYaw);
		ImU32 col = triggered_t ? IM_COL32(255, 0, 0, 255) : IM_COL32(0, 255, 255, 255);
		DebugDrawOBB_Yaw(boxC, boxHalf,map[i].rotate.y, col);
	}


	if (debugMode)
	{
		/*ImGui::Begin("Debug - han");
		if (ImGui::TreeNode("Collision.cpp"))
		{
			

			ImGui::TreePop();
		}
		ImGui::End();*/
	}


	/*for (size_t i = 0; i < map.size(); i++)
	{
		if (!Field_IsSolid(map[i].no)) continue;
		const XMFLOAT3& boxHalf = Field_GetHalfSize(map[i]);
		const XMFLOAT3& boxC = map[i].pos;
		ImU32 col = IM_COL32(0, 255, 255, 255);
		DebugDrawAABB(boxC, boxHalf, col);
	}*/
}
