#include "Collision.h"

//=========================================================================================================
// ボールとフィールドの当たり判定
//=========================================================================================================
//float BallField_Collision()
//{
//	float hit = false;
//	BALL* Ball = GetBall();
//	MAPDATA* Map = GetFieldMap();
//	int i = 0;
//
//	while (Map[i].no != FIELD_MAX)
//	{
//		float BoxTop;
//		switch (Map[i].no)
//		{
//		default:
//			BoxTop = Map[i].pos.y + BOX_RADIUS; // 普通のBOX
//			break;
//		}
//		//壁との当たり判定
//		if (Map[i].pos.y - BOX_RADIUS < Ball->Position.y && Ball->Position.y < BoxTop - 0.1f)
//		{
//			if (Map[i].pos.z - BOX_RADIUS < Ball->Position.z && Ball->Position.z < Map[i].pos.z + BOX_RADIUS)
//			{
//				if (Map[i].pos.x - BOX_RADIUS < Ball->Position.x + BALL_RADIUS && Ball->Position.x < Map[i].pos.x + BOX_RADIUS)
//				{//BOXの-X面にぶつかった
//					Ball->Position.x += (Map[i].pos.x - BOX_RADIUS) - (Ball->Position.x + BALL_RADIUS);
//					Ball->Velocity.x *= -COE;//反発させる
//					hit = COLLISION_HIT::HIT_WALL_NegX;
//				}
//				else if (Map[i].pos.x + BOX_RADIUS > Ball->Position.x - BALL_RADIUS && Ball->Position.x > Map[i].pos.x + BOX_RADIUS)
//				{//BOXの+X面にぶつかった
//					Ball->Position.x += (Map[i].pos.x + BOX_RADIUS) - (Ball->Position.x - BALL_RADIUS);
//					Ball->Velocity.x *= -COE;//反発させる
//					hit = COLLISION_HIT::HIT_WALL_PlusX;
//
//				}
//			}
//			else if (Map[i].pos.x - BOX_RADIUS < Ball->Position.x && Ball->Position.x < Map[i].pos.x + BOX_RADIUS)
//			{
//				if (Map[i].pos.z - BOX_RADIUS < Ball->Position.z + BALL_RADIUS && Ball->Position.z < Map[i].pos.z + BOX_RADIUS)
//				{//BOXの-Z面にぶつかった
//					Ball->Position.z += (Map[i].pos.z - BOX_RADIUS) - (Ball->Position.z + BALL_RADIUS);
//					Ball->Velocity.z *= -COE;//反発させる
//					hit = COLLISION_HIT::HIT_WALL_NegZ;
//				}
//				else if (Map[i].pos.z + BOX_RADIUS > Ball->Position.z - BALL_RADIUS && Ball->Position.z > Map[i].pos.z + BOX_RADIUS)
//				{//BOXの+Z面にぶつかった
//					Ball->Position.z += (Map[i].pos.z + BOX_RADIUS) - (Ball->Position.z - BALL_RADIUS);
//					Ball->Velocity.z *= -COE;//反発させる
//					hit = COLLISION_HIT::HIT_WALL_PlusZ;
//
//				}
//			}
//		}
//		//床との当たり判定
//		else
//		{
//			if ((Map[i].pos.z - BOX_RADIUS) < Ball->Position.z && Ball->Position.z < (Map[i].pos.z + BOX_RADIUS))
//			{
//				if ((Map[i].pos.x - BOX_RADIUS) < Ball->Position.x && Ball->Position.x < (Map[i].pos.x + BOX_RADIUS))
//				{
//					if ((Map[i].pos.y - BOX_RADIUS) < (Ball->Position.y + BALL_RADIUS) && Ball->Position.y < (Map[i].pos.y - BOX_RADIUS))
//					{
//						Ball->Position.y += (Map[i].pos.y - BOX_RADIUS) - (Ball->Position.y + BALL_RADIUS);
//						Ball->Velocity.y *= -COE;
//					}
//					else if (BoxTop > (Ball->Position.y - BALL_RADIUS) && Ball->Position.y > BoxTop)
//					{
//						Ball->Position.y += (BoxTop)-(Ball->Position.y - BALL_RADIUS);
//						Ball->Velocity.y = Ball->Velocity.y * (-COE * 1.0f);
//						hit = COLLISION_HIT::HIT_WALL_NegX;
//					}
//				}
//			}
//		}
//		i++;
//		
//	}
//	return hit;
//}