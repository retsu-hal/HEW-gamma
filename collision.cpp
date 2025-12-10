#include "Collision.h"
#include "player3D.h"

<<<<<<< HEAD
#include "camera.h"
#include "direct3d.h"
#include "debug.h"
using namespace DirectX;

//=========================================================================================================
// ボールとフィールドの当たり判定
//=========================================================================================================

float Player3DField_Collision()
=======
int Player3DField_Collision()
>>>>>>> 73db079adbbd2c5bc2d828804b3c84a05ea683c0
{
    int hit = HIT_NONE;
    PLAYER3D* player3D = GetPlayer3D();
    MAPDATA* Map = GetFieldMap();
    size_t fieldSize = GetFieldMapSize();
    if (!Map || fieldSize == 0) return hit;

    for (size_t i = 0; i < fieldSize; ++i)
    {
        // CSV Type値 (0:箱, 1:OBJ_1...)
        if (Map[i].no != FIELD_GROUND) continue; // 箱以外はスキップ（画像のTypeカラム利用）

        float BoxTop = Map[i].pos.y + BOX_RADIUS;

<<<<<<< HEAD
				}
			}
		}
		//床との当たり判定
		else
		{
			if ((Map[i].pos.z - BOX_RADIUS) < player3D->Position.z && player3D->Position.z < (Map[i].pos.z + BOX_RADIUS))
			{
				if ((Map[i].pos.x - BOX_RADIUS) < player3D->Position.x && player3D->Position.x < (Map[i].pos.x + BOX_RADIUS))
				{
					if ((Map[i].pos.y - BOX_RADIUS) < (player3D->Position.y + PLAYER3D_RADIUS) && player3D->Position.y < (Map[i].pos.y - BOX_RADIUS))
					{
						player3D->Position.y += (Map[i].pos.y - BOX_RADIUS) - (player3D->Position.y + PLAYER3D_RADIUS);
						player3D->Velocity.y *= -COE;
					}
					else if (BoxTop > (player3D->Position.y - PLAYER3D_RADIUS) && player3D->Position.y > BoxTop)
					{
						player3D->Position.y += (BoxTop)-(player3D->Position.y - PLAYER3D_RADIUS);
						player3D->Velocity.y = player3D->Velocity.y * (-COE * 1.0f);
						hit = COLLISION_HIT::HIT_WALL_NegX;
					}
				}
			}
		}
		i++;
		
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
	MAPDATA* map = GetFieldMap();


	XMFLOAT3 playerHalf = Player3D_GetDetectHalfSize();
	XMFLOAT3 playerC = player->Position;


	DebugDrawAABB(playerC, playerHalf, IM_COL32(0, 255, 0, 255));


	XMFLOAT3 boxHalf(BOX_RADIUS, BOX_RADIUS, BOX_RADIUS);

	for (int i = 0; map[i].no != FIELD_MAX; ++i)
	{
		const XMFLOAT3& boxC = map[i].pos;

		bool triggered = AABB_Intersect(playerC, playerHalf,
			boxC, boxHalf);

		ImU32 col = triggered ? IM_COL32(255, 0, 0, 255): IM_COL32(0, 255, 255, 255);

		DebugDrawAABB(boxC, boxHalf, col);
	}
=======
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
            if ((Map[i].pos.z - BOX_RADIUS) < player3D->Position.z && player3D->Position.z < (Map[i].pos.z + BOX_RADIUS))
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
            }
        }
    }
    return hit;
>>>>>>> 73db079adbbd2c5bc2d828804b3c84a05ea683c0
}