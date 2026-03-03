#include "Switch_Light.h"
#include "field.h"
#include "Camera.h"
#include "keyboard.h"
#include "Collision.h"

#include "debug.h"

using namespace DirectX;

static bool g_LightMode = false;
static bool g_TabWasDown = false;

// Index into g_MapData for the OBJ_3 we're controlling (-1 = not found)
static int g_LightObjIndex = -1;

// OBJ_3 collision half-size (ball radius as ellipsoid)
static const XMFLOAT3 kOBJ3_CollisionHalf = { 0.25f, 0.25f, 0.25f };

//=========================================================================================================
// Which field types should OBJ_3 collide with?
//=========================================================================================================
static bool OBJ3_IsSolid(FIELD type)
{
	switch (type)
	{
	case FIELD_GROUND:
	case FIELD_WALL:
	case FIELD_OBJ_BOX:
	case FIELD_OBJ_1:
	case FIELD_OBJ_2:
	case FIELD_BENCH:
	case FIELD_DUSTBOX:
	case FIELD_FOUNTAIN:
	case FIELD_POLE:
	case FIELD_FENCE:
	case FIELD_SEESAW_1:
	case FIELD_EMPTY_BOX:
		return true;
	default:
		return false;
	}
}

//=========================================================================================================
// extern from CollisionGeometry.cpp (same ones used by Player3D)
//=========================================================================================================
extern bool Resolve_Ellipsoid_OBB_Yaw(
	const XMFLOAT3& ellC, const XMFLOAT3& ellR,
	const XMFLOAT3& boxC, const XMFLOAT3& boxH, float boxYaw,
	XMFLOAT3* outPush, XMFLOAT3* outNormal);

//=========================================================================================================
// OBJ_3 vs all field objects collision
//=========================================================================================================
static void SwitchLight_CheckCollision(XMFLOAT3& objPos)
{
	std::vector<MAPDATA>& map = GetFieldMap();

	XMFLOAT3 ellC = objPos;
	XMFLOAT3 ellR = kOBJ3_CollisionHalf;

	// Multiple passes for corner resolution
	for (int pass = 0; pass < 3; pass++)
	{
		for (size_t i = 0; i < map.size(); i++)
		{
			// Don't collide with itself
			if ((int)i == g_LightObjIndex) continue;

			if (!OBJ3_IsSolid(map[i].no)) continue;

			// Get box yaw (same logic as Player3D collision)
			float boxYaw = (map[i].no == FIELD_OBJ_1) ? map[i].rotate.y : 0.0f;

			XMFLOAT3 boxHalf = Field_GetCollisionHalfSize(map[i]);

			XMFLOAT3 push, norm;
			if (!Resolve_Ellipsoid_OBB_Yaw(ellC, ellR, map[i].pos, boxHalf, boxYaw, &push, &norm))
				continue;

			// Push OBJ_3 out of the collision
			ellC.x += push.x;
			ellC.y += push.y;
			ellC.z += push.z;
		}
	}

	objPos = ellC;
}

void SwitchLight_Initialize(void)
{
	g_LightMode = false;
	g_TabWasDown = false;
	g_LightObjIndex = -1;
}

void SwitchLight_Finalize(void)
{
	// Nothing to release
}

// Find the first FIELD_OBJ_3 in the map data
static int FindLightObjIndex()
{
	std::vector<MAPDATA>& map = GetFieldMap();
	for (size_t i = 0; i < map.size(); i++)
	{
		if (map[i].no == FIELD_OBJ_3)
		{
			return (int)i;
		}
	}
	return -1;
}

void SwitchLight_Update(void)
{
	// Detect Tab key press (rising edge only)
	bool tabIsDown = Keyboard_IsKeyDown(KK_TAB);
	if (tabIsDown && !g_TabWasDown)
	{
		g_LightMode = !g_LightMode;

		// Find the OBJ_3 when entering light mode
		if (g_LightMode)
		{
			g_LightObjIndex = FindLightObjIndex();
		}
	}
	g_TabWasDown = tabIsDown;

	if (!g_LightMode) return;
	if (g_LightObjIndex < 0) return;

	std::vector<MAPDATA>& map = GetFieldMap();
	if (g_LightObjIndex >= (int)map.size()) return;

	MAPDATA& obj = map[g_LightObjIndex];

	// --- Move the OBJ_3 with arrow / numpad keys ---
	float speed = 5.0f / 60.0f;

	if (Keyboard_IsKeyDown(KK_NUMPAD8) || Keyboard_IsKeyDown(KK_UP)) {
		obj.pos.z += speed;
	}
	if (Keyboard_IsKeyDown(KK_NUMPAD2) || Keyboard_IsKeyDown(KK_DOWN)) {
		obj.pos.z -= speed;
	}
	if (Keyboard_IsKeyDown(KK_NUMPAD7) || Keyboard_IsKeyDown(KK_LEFT)) {
		obj.pos.x -= speed;
	}
	if (Keyboard_IsKeyDown(KK_NUMPAD9) || Keyboard_IsKeyDown(KK_RIGHT)) {
		obj.pos.x += speed;
	}
	if (Keyboard_IsKeyDown(KK_NUMPAD4)) {
		obj.pos.y -= speed;
	}
	if (Keyboard_IsKeyDown(KK_NUMPAD6)) {
		obj.pos.y += speed;
	}
	// --- Collision check and resolution (same system as Player3D) ---
	SwitchLight_CheckCollision(obj.pos);

	// Camera follows the OBJ_3
	LightCamera_Update();
}

bool SwitchLight_IsLightMode(void)
{
	return g_LightMode;
}
// Getter so camera.cpp can read the OBJ_3 position
XMFLOAT3 SwitchLight_GetLightObjPosition(void)
{
	if (g_LightObjIndex >= 0)
	{
		std::vector<MAPDATA>& map = GetFieldMap();
		if (g_LightObjIndex < (int)map.size())
		{
			return map[g_LightObjIndex].pos;
		}
	}
	return XMFLOAT3(0, 0, 0);
}
