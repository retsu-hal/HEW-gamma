#include "Collision.h"

#include "Player3D.h"
#include "Player2D.h"

//=========================================================================================================
// ボールとフィールドの当たり判定
//=========================================================================================================
float Player3DField_Collision()
{
    float hit = false;
    PLAYER3D* Player3D = GetPlayer3D();
    MAPDATA* Map = GetFieldMap();
    int i = 0;

    while (Map[i].no != FIELD_MAX)
    {
		float BoxTop;
		switch (Map[i].no)
		{
		default:
			BoxTop = Map[i].pos.y + BOX_RADIUS; // 普通のBOX
			break;
		}
		//壁との当たり判定
		if (Map[i].pos.y - BOX_RADIUS < Player3D->Position.y && Player3D->Position.y < BoxTop - 0.1f)
		{
			if (Map[i].pos.z - BOX_RADIUS < Player3D->Position.z && Player3D->Position.z < Map[i].pos.z + BOX_RADIUS)
			{
				if (Map[i].pos.x - BOX_RADIUS < Player3D->Position.x + PLAYER3D_RADIUS && Player3D->Position.x < Map[i].pos.x + BOX_RADIUS)
				{//BOXの-X面にぶつかった
					Player3D->Position.x += (Map[i].pos.x - BOX_RADIUS) - (Player3D->Position.x + PLAYER3D_RADIUS);
					hit = COLLISION_HIT::HIT_WALL_NegX;
				}
				else if (Map[i].pos.x + BOX_RADIUS > Player3D->Position.x - PLAYER3D_RADIUS && Player3D->Position.x > Map[i].pos.x + BOX_RADIUS)
				{//BOXの+X面にぶつかった
					Player3D->Position.x += (Map[i].pos.x + BOX_RADIUS) - (Player3D->Position.x - PLAYER3D_RADIUS);
					hit = COLLISION_HIT::HIT_WALL_PlusX;

				}
			}
			else if (Map[i].pos.x - BOX_RADIUS < Player3D->Position.x && Player3D->Position.x < Map[i].pos.x + BOX_RADIUS)
			{
				if (Map[i].pos.z - BOX_RADIUS < Player3D->Position.z + PLAYER3D_RADIUS && Player3D->Position.z < Map[i].pos.z + BOX_RADIUS)
				{//BOXの-Z面にぶつかった
					Player3D->Position.z += (Map[i].pos.z - BOX_RADIUS) - (Player3D->Position.z + PLAYER3D_RADIUS);
					hit = COLLISION_HIT::HIT_WALL_NegZ;
				}
				else if (Map[i].pos.z + BOX_RADIUS > Player3D->Position.z - PLAYER3D_RADIUS && Player3D->Position.z > Map[i].pos.z + BOX_RADIUS)
				{//BOXの+Z面にぶつかった
					Player3D->Position.z += (Map[i].pos.z + BOX_RADIUS) - (Player3D->Position.z - PLAYER3D_RADIUS);
					hit = COLLISION_HIT::HIT_WALL_PlusZ;

				}
			}
		}
		//床との当たり判定
		else
		{
			if ((Map[i].pos.z - BOX_RADIUS) < Player3D->Position.z && Player3D->Position.z < (Map[i].pos.z + BOX_RADIUS))
			{
				if ((Map[i].pos.x - BOX_RADIUS) < Player3D->Position.x && Player3D->Position.x < (Map[i].pos.x + BOX_RADIUS))
				{
					if ((Map[i].pos.y - BOX_RADIUS) < (Player3D->Position.y + PLAYER3D_RADIUS) && Player3D->Position.y < (Map[i].pos.y - BOX_RADIUS))
					{
						Player3D->Position.y += (Map[i].pos.y - BOX_RADIUS) - (Player3D->Position.y + PLAYER3D_RADIUS);
					}
					else if (BoxTop > (Player3D->Position.y - PLAYER3D_RADIUS) && Player3D->Position.y > BoxTop)
					{
						Player3D->Position.y += (BoxTop)-(Player3D->Position.y - PLAYER3D_RADIUS);
						hit = COLLISION_HIT::HIT_WALL_NegX;
					}
				}
			}
		}
		i++;

	}
	return hit;
}