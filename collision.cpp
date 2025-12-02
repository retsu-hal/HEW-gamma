#include "Collision.h"
#include "player3D.h"

//=========================================================================================================
// ボールとフィールドの当たり判定
//=========================================================================================================
float Player3DField_Collision()
{
	float hit = false;
	PLAYER3D* player3D = GetPlayer3D();
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
		if (Map[i].pos.y - BOX_RADIUS < player3D->Position.y && player3D->Position.y < BoxTop - 0.1f)
		{
			if (Map[i].pos.z - BOX_RADIUS < player3D->Position.z && player3D->Position.z < Map[i].pos.z + BOX_RADIUS)
			{
				if (Map[i].pos.x - BOX_RADIUS < player3D->Position.x + PLAYER3D_RADIUS && player3D->Position.x < Map[i].pos.x + BOX_RADIUS)
				{//BOXの-X面にぶつかった
					player3D->Position.x += (Map[i].pos.x - BOX_RADIUS) - (player3D->Position.x + PLAYER3D_RADIUS);
					player3D->Velocity.x *= -COE;//反発させる
					hit = COLLISION_HIT::HIT_WALL_NegX;
				}
				else if (Map[i].pos.x + BOX_RADIUS > player3D->Position.x - PLAYER3D_RADIUS && player3D->Position.x > Map[i].pos.x + BOX_RADIUS)
				{//BOXの+X面にぶつかった
					player3D->Position.x += (Map[i].pos.x + BOX_RADIUS) - (player3D->Position.x - PLAYER3D_RADIUS);
					player3D->Velocity.x *= -COE;//反発させる
					hit = COLLISION_HIT::HIT_WALL_PlusX;

				}
			}
			else if (Map[i].pos.x - BOX_RADIUS < player3D->Position.x && player3D->Position.x < Map[i].pos.x + BOX_RADIUS)
			{
				if (Map[i].pos.z - BOX_RADIUS < player3D->Position.z + PLAYER3D_RADIUS && player3D->Position.z < Map[i].pos.z + BOX_RADIUS)
				{//BOXの-Z面にぶつかった
					player3D->Position.z += (Map[i].pos.z - BOX_RADIUS) - (player3D->Position.z + PLAYER3D_RADIUS);
					player3D->Velocity.z *= -COE;//反発させる
					hit = COLLISION_HIT::HIT_WALL_NegZ;
				}
				else if (Map[i].pos.z + BOX_RADIUS > player3D->Position.z - PLAYER3D_RADIUS && player3D->Position.z > Map[i].pos.z + BOX_RADIUS)
				{//BOXの+Z面にぶつかった
					player3D->Position.z += (Map[i].pos.z + BOX_RADIUS) - (player3D->Position.z - PLAYER3D_RADIUS);
					player3D->Velocity.z *= -COE;//反発させる
					hit = COLLISION_HIT::HIT_WALL_PlusZ;

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