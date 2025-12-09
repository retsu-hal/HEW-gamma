#include "Collision.h"
#include "player3D.h"

int Player3DField_Collision()
{
    int hit = HIT_NONE;
    PLAYER3D* player3D = GetPlayer3D();
    MAPDATA* Map = GetFieldMap();
    size_t fieldSize = GetFieldMapSize();
    if (!Map || fieldSize == 0) return hit;

    for (size_t i = 0; i < fieldSize; ++i)
    {
        // CSV Type値 (0:箱, 1:OBJ_1...)
        if (Map[i].no != FIELD_BOX) continue; // 箱以外はスキップ（画像のTypeカラム利用）

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
}