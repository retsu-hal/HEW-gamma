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

static const float PLAYER_COLLISION_OFFSET_Y = 0.9f;

int Player3DField_Collision()
{
    int hit = HIT_NONE;

    PLAYER3D* player3D = GetPlayer3D();
	std::vector<MAPDATA>& Map = GetFieldMap();
    if (!player3D || Map.size() == 0) return hit;

	XMFLOAT3 playerHalf = Player3D_GetDetectHalfSize();
	XMFLOAT3 playerPos = player3D->Position;
	playerPos.y += PLAYER_COLLISION_OFFSET_Y;

    for (size_t i = 0; i < Map.size(); ++i)
    {
        // CSV Type値 (0:箱, 1:OBJ_1...)
        if (Map[i].no != FIELD_GROUND) continue; // 箱以外はスキップ（画像のTypeカラム利用）

        XMFLOAT3 boxPos = Map[i].pos;
		XMFLOAT3 boxHalf(BOX_RADIUS, BOX_RADIUS, BOX_RADIUS);

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
		
		else if(overlapX <= overlapY && overlapX <= overlapZ)
		{
			debugHit = 2;
			if(dx > 0.0f)
			{
				playerPos.x = boxPos.x + boxHalf.x + playerHalf.x;
				player3D->Velocity.x = 0.0f;
				hit = HIT_WALL_PlusX;
			}
			else
			{
				playerPos.x = boxPos.x - boxHalf.x - playerHalf.x;
				player3D->Velocity.x = 0.0f;
				hit = HIT_WALL_NegX;
			}
		}
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
	player3D->Position.y = playerPos.y - PLAYER_COLLISION_OFFSET_Y;
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
	const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth();
	const float SCREEN_HEIGHT = (float)Direct3D_GetBackBufferHeight();

	float x = pos.x + (ndc.x * 0.5f + 0.5f) * SCREEN_WIDTH;
	float y = pos.y + (-ndc.y * 0.5f + 0.5f) * SCREEN_HEIGHT;

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

	// プレイヤーのAABB描画
	PLAYER3D* player = GetPlayer3D();// プレイヤー取得
	if (!player) return;
	XMFLOAT3 playerHalf = Player3D_GetDetectHalfSize();
	XMFLOAT3 playerC = player->Position;
	playerC.y += PLAYER_COLLISION_OFFSET_Y;
	DebugDrawAABB(playerC, playerHalf, IM_COL32(0, 255, 0, 255));// プレイヤーのAABB描画


	// フィールドのAABB描画
	//MAPDATA* map = GetFieldMap();
	//size_t fieldSize = GetFieldMapSize();
	//if(!map || fieldSize == 0) return;
	//XMFLOAT3 boxHalf(BOX_RADIUS, BOX_RADIUS, BOX_RADIUS);
	//for (size_t i = 0; i < fieldSize; ++i)
	//{
	//	const XMFLOAT3& boxC = map[i].pos;
	//	bool triggered = AABB_Intersect(playerC, playerHalf, boxC, boxHalf);// 当たっているかどうか
	//	ImU32 col = triggered ? IM_COL32(255, 0, 0, 255) : IM_COL32(0, 255, 255, 255);// 当たっているなら赤、そうでなければシアン
	//	DebugDrawAABB(boxC, boxHalf, col);
	//}

}
