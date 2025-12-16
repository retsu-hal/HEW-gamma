#include "Collision.h"
#include "player3D.h"


#include "camera.h"
#include "direct3d.h"
#include "debug.h"
using namespace DirectX;

//=========================================================================================================
// ボールとフィールドの当たり判定
//=========================================================================================================


int Player3DField_Collision()
{
    int hit = HIT_NONE;
    PLAYER3D* player3D = GetPlayer3D();
	std::vector<MAPDATA>& Map = GetFieldMap();
    if (Map.size() == 0) return hit;

    for (size_t i = 0; i < Map.size(); ++i)
    {

        float BoxTop = Map[i].pos.y + BOX_RADIUS;


		// 壁との当たり判定
		if (Map[i].pos.y - BOX_RADIUS < player3D->Position.y && player3D->Position.y < BoxTop - 0.1f)
		{
			if (Map[i].pos.z - BOX_RADIUS < player3D->Position.z && player3D->Position.z < Map[i].pos.z + BOX_RADIUS)
			{
				if (Map[i].pos.x - BOX_RADIUS < player3D->Position.x + PLAYER3D_RADIUS && player3D->Position.x < Map[i].pos.x + BOX_RADIUS)
				{//-X面判定
					player3D->Position.x += (Map[i].pos.x - BOX_RADIUS) - (player3D->Position.x + PLAYER3D_RADIUS);
					player3D->Velocity.x *= -COE;
					hit = HIT_WALL_NegX;
				}
				else if (Map[i].pos.x + BOX_RADIUS > player3D->Position.x - PLAYER3D_RADIUS && player3D->Position.x > Map[i].pos.x + BOX_RADIUS)
				{//+X面判定
					player3D->Position.x += (Map[i].pos.x + BOX_RADIUS) - (player3D->Position.x - PLAYER3D_RADIUS);
					player3D->Velocity.x *= -COE;
					hit = HIT_WALL_PlusX;
				}
			}
			else if (Map[i].pos.x - BOX_RADIUS < player3D->Position.x && player3D->Position.x < Map[i].pos.x + BOX_RADIUS)
			{
				if (Map[i].pos.z - BOX_RADIUS < player3D->Position.z + PLAYER3D_RADIUS && player3D->Position.z < Map[i].pos.z + BOX_RADIUS)
				{//-Z面判定
					player3D->Position.z += (Map[i].pos.z - BOX_RADIUS) - (player3D->Position.z + PLAYER3D_RADIUS);
					player3D->Velocity.z *= -COE;
					hit = HIT_WALL_NegZ;
				}
				else if (Map[i].pos.z + BOX_RADIUS > player3D->Position.z - PLAYER3D_RADIUS && player3D->Position.z > Map[i].pos.z + BOX_RADIUS)
				{//+Z面判定
					player3D->Position.z += (Map[i].pos.z + BOX_RADIUS) - (player3D->Position.z - PLAYER3D_RADIUS);
					player3D->Velocity.z *= -COE;
					hit = HIT_WALL_PlusZ;
				}
			}
		}
		// 床との当たり判定
		else
		{
			//レイキャスト方式に変更
			/*if ((Map[i].pos.z - BOX_RADIUS) < player3D->Position.z && player3D->Position.z < (Map[i].pos.z + BOX_RADIUS))
			{
				if ((Map[i].pos.x - BOX_RADIUS) < player3D->Position.x && player3D->Position.x < (Map[i].pos.x + BOX_RADIUS))
				{
					if ((Map[i].pos.y - BOX_RADIUS) < (player3D->Position.y + PLAYER3D_RADIUS) && player3D->Position.y < (Map[i].pos.y - BOX_RADIUS))
					{
						player3D->Position.y += (Map[i].pos.y - BOX_RADIUS) - (player3D->Position.y + PLAYER3D_RADIUS);
						player3D->Velocity.y *= -COE;
						hit = HIT_GROUND;
					}
					else if (BoxTop > (player3D->Position.y - PLAYER3D_RADIUS) && player3D->Position.y > BoxTop)
					{
						player3D->Position.y += (BoxTop)-(player3D->Position.y - PLAYER3D_RADIUS);
						player3D->Velocity.y = player3D->Velocity.y * (-COE * 1.0f);
						hit = HIT_WALL_NegX;
					}
				}
			}*/
		}
	}
	return hit;
}


struct ScreenPoint// スクリーン座標
{
	ImVec2 pos;
	bool   valid;
};

static ScreenPoint WorldToScreenSafe(const XMFLOAT3& pWS)// World Space -> Screen Space
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
	const ImVec2   size = vpIm->Size;

	float x = pos.x + (ndc.x * 0.5f + 0.5f) * size.x;
	float y = pos.y + (-ndc.y * 0.5f + 0.5f) * size.y;

	out.pos = ImVec2(x, y);
	out.valid = true;
	return out;
}

static void DebugDrawAABB(const XMFLOAT3& center,
	const XMFLOAT3& half, ImU32 color) {// AABBをデバッグ描画

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


static bool AABB_Intersect(const XMFLOAT3& c0, const XMFLOAT3& h0,
	const XMFLOAT3& c1, const XMFLOAT3& h1) {// AABB同士当たり判定

	if (fabsf(c0.x - c1.x) > (h0.x + h1.x)) return false;
	if (fabsf(c0.y - c1.y) > (h0.y + h1.y)) return false;
	if (fabsf(c0.z - c1.z) > (h0.z + h1.z)) return false;
	return true;
}

void Collision_DebugDraw() {// 当たり判定のデバッグ描画

	PLAYER3D* player = GetPlayer3D();
	std::vector<MAPDATA>& Map = GetFieldMap();

	XMFLOAT3 playerHalf = Player3D_GetDetectHalfSize();
	XMFLOAT3 playerC = player->Position;


	DebugDrawAABB(playerC, playerHalf, IM_COL32(0, 255, 0, 255));


	XMFLOAT3 boxHalf(BOX_RADIUS, BOX_RADIUS, BOX_RADIUS);

}

bool Collision_RayToField(
	const XMFLOAT3& start,
	const XMFLOAT3& dir,
	float maxDist,
	float* hitY
)
{
	MAPDATA* Map = GetFieldMap();
	size_t fieldSize = GetFieldMapSize();
	if (!Map || fieldSize == 0) return false;

	bool hit = false;
	float nearestY = -FLT_MAX;

	for (size_t i = 0; i < fieldSize; ++i)
	{
		if (Map[i].no != FIELD_GROUND) continue;

		// AABB（箱）
		float minX = Map[i].pos.x - BOX_RADIUS;
		float maxX = Map[i].pos.x + BOX_RADIUS;
		float minZ = Map[i].pos.z - BOX_RADIUS;
		float maxZ = Map[i].pos.z + BOX_RADIUS;
		float topY = Map[i].pos.y + BOX_RADIUS;

		// 下向きレイ専用（dir = 0,-1,0 前提）
		if (start.x < minX || start.x > maxX) continue;
		if (start.z < minZ || start.z > maxZ) continue;

		float dy = start.y - topY;
		if (dy < 0.0f || dy > maxDist) continue;

		// 一番近い床を採用
		if (!hit || topY > nearestY)
		{
			nearestY = topY;
			hit = true;
		}
	}

	if (hit && hitY)
	{
		*hitY = nearestY;
	}

	return hit;
}
