#include "Collision.h"
#include "player3D.h"


#include "camera.h"
#include "direct3d.h"
#include "debug.h"
using namespace DirectX;

static bool debugMode = TRUE;
int debugHit = 0;


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

// プレイヤーのトリガー当たり判定中心座標を取得
static XMFLOAT3 GetPlayerTriggerCollider()
{
	PLAYER3D* p = GetPlayer3D();
	XMFLOAT3 c = p->Position;

	XMFLOAT3 half = Player3D_GetTriggerHalfSize();
	c.y += half.y;
	return c;
}

// 2Dベクトルの内積
static float Dot2D(const XMFLOAT3& a, const XMFLOAT3& b) { return a.x * b.x + a.z * b.z; }
// 2Dベクトルの長さ
static float Len2D(const XMFLOAT3& v) { return sqrtf(v.x * v.x + v.z * v.z); }
// 2Dベクトルの正規化
static XMFLOAT3 Normalize2D(XMFLOAT3 v)
{
	float l = Len2D(v);
	if (l < 1e-6f) return XMFLOAT3(0, 0, 1);
	v.x /= l; v.z /= l; v.y = 0.0f;
	return v;
}
// カメラ基準で見たときのターゲットの位置を計算
static TRIGGER_SIDE CalcSide_ByCamera(const XMFLOAT3& playerC, const XMFLOAT3& targetC)
{
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

// プレイヤーとフィールドの当たり判定
int Player3DField_Collision()
{
	int hit = HIT_NONE;

	PLAYER3D* player3D = GetPlayer3D();
	std::vector<MAPDATA>& Map = GetFieldMap();
	if (!player3D || Map.size() == 0) return hit;

	player3D->isGround = false;

	XMFLOAT3 playerHalf = Player3D_GetSolidHalfSize();
	XMFLOAT3 playerPos = GetPlayerSolidCollider();

	for (size_t i = 0; i < Map.size(); ++i)
	{
		if (!Field_IsSolid(Map[i].no)) continue;

		XMFLOAT3 boxPos = Map[i].pos;
		XMFLOAT3 boxHalf = Field_GetHalfSize(Map[i]);

		float dx = playerPos.x - boxPos.x;
		float dy = playerPos.y - boxPos.y;
		float dz = playerPos.z - boxPos.z;

		// AABB同士の当たり判定
		float overlapX = (playerHalf.x + boxHalf.x) - fabsf(dx);
		if (overlapX <= 0.0f) continue;
		float overlapY = (playerHalf.y + boxHalf.y) - fabsf(dy);
		if (overlapY <= 0.0f) continue;
		float overlapZ = (playerHalf.z + boxHalf.z) - fabsf(dz);
		if (overlapZ <= 0.0f) continue;

		// 壁との当たり判定
		//  Z軸方向の押し出しが一番浅い
		if (overlapZ <= overlapY && overlapZ <= overlapX)
		{
			debugHit = 1;
			if (dz > 0.0f)
			{
				playerPos.z = boxPos.z + boxHalf.z + playerHalf.z;
				player3D->Velocity.z = 0.0f;
				hit = HIT_WALL_PlusZ;
			}
			else
			{
				playerPos.z = boxPos.z - boxHalf.z - playerHalf.z;
				player3D->Velocity.z = 0.0f;
				hit = HIT_WALL_NegZ;
			}
		}
		//  X軸方向の押し出しが一番浅い
		else if (overlapX <= overlapY && overlapX <= overlapZ)
		{
			debugHit = 2;
			if (dx > 0.0f)
			{
				playerPos.x = boxPos.x + boxHalf.x + playerHalf.x;
				player3D->Velocity.x = 0.0f;
				hit = HIT_WALL_PlusX;
			}
			else
			{
				playerPos.x = boxPos.x - boxHalf.x - playerHalf.x;
			}
		}
		//  Y軸方向の押し出しが一番浅い
		else
		{
			debugHit = 3;
			if (dy > 0.0f)
			{
				playerPos.y = boxPos.y + boxHalf.y + playerHalf.y;
				player3D->Velocity.y = 0.0f;
				player3D->isGround = true;
				hit = HIT_GROUND;
			}
			else
			{
				playerPos.y = boxPos.y - boxHalf.y - playerHalf.y;
				if (player3D->Velocity.y > 0.0f)
				{
					player3D->Velocity.y = 0.0f;
				}
			}
		}
	}

	player3D->Position.x = playerPos.x;
	player3D->Position.y = playerPos.y - playerHalf.y;
	player3D->Position.z = playerPos.z;

	if (debugMode)
	{
		ImGui::Begin("Debug - han");
		if (ImGui::TreeNode("Collision.cpp"))
		{
			ImGui::Text("HIT: %d", hit);
			ImGui::Text("HITMod: %d", debugHit);

			ImGui::TreePop();
		}
		ImGui::End();
	}

	return hit;
}

// AABB同士の当たり判定
static bool AABB_Intersect(const XMFLOAT3& c0, const XMFLOAT3& h0,
	const XMFLOAT3& c1, const XMFLOAT3& h1) {

	if (fabsf(c0.x - c1.x) > (h0.x + h1.x)) return false;
	if (fabsf(c0.y - c1.y) > (h0.y + h1.y)) return false;
	if (fabsf(c0.z - c1.z) > (h0.z + h1.z)) return false;
	return true;
}

// プレイヤーとトリガーの当たり判定
bool Collision_PlayerTrigger(TRIGGER_HIT* outHit, float extraRange)
{
	if (outHit) *outHit = TRIGGER_HIT{};

	PLAYER3D* p = GetPlayer3D();
	if (!p) return false;

	auto& map = GetFieldMap();
	if (map.empty()) return false;

	const XMFLOAT3 pHalf = Player3D_GetTriggerHalfSize();
	const XMFLOAT3 pC = GetPlayerTriggerCollider();
	bool found = false;
	float bestD2 = 1e30f;
	TRIGGER_HIT best;

	for (size_t i = 0; i < map.size(); ++i)
	{
		if (!Field_IsTrigger(map[i].no)) continue;

		const XMFLOAT3 tC = map[i].pos;

		XMFLOAT3 tHalf = Field_GetHalfSize(map[i]);
		tHalf.x += extraRange;
		tHalf.y += extraRange;
		tHalf.z += extraRange;

		if (!AABB_Intersect(pC, pHalf, tC, tHalf)) continue;

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
// 当たり判定のデバッグ描画
void Collision_DebugDraw() {

	// プレイヤーのAABB描画
	PLAYER3D* player = GetPlayer3D();// プレイヤー取得
	if (!player) return;
	XMFLOAT3 playerHalf = Player3D_GetSolidHalfSize();
	XMFLOAT3 playerC = GetPlayerSolidCollider();
	DebugDrawAABB(playerC, playerHalf, IM_COL32(0, 255, 0, 255));// プレイヤーのAABB描画

	XMFLOAT3 playerHalf_t = Player3D_GetTriggerHalfSize();
	XMFLOAT3 playerC_t = GetPlayerTriggerCollider();
	DebugDrawAABB(playerC_t, playerHalf_t, IM_COL32(255, 255, 255, 255));// プレイヤーのAABB描画

	// フィールドのAABB描画
	std::vector<MAPDATA>& map = GetFieldMap();
	if (map.size() == 0) return;
	for (size_t i = 0; i < map.size(); ++i)
	{
		if (!Field_IsTrigger(map[i].no)) continue;
		const XMFLOAT3& boxHalf = Field_GetHalfSize(map[i]);
		const XMFLOAT3& boxC = map[i].pos;
		bool triggered_t = AABB_Intersect(playerC_t, playerHalf_t, boxC, boxHalf);// 当たっているかどうか
		ImU32 col = triggered_t ? IM_COL32(255, 0, 0, 255) : IM_COL32(0, 255, 255, 255);
		DebugDrawAABB(boxC, boxHalf, col);
	}
}
