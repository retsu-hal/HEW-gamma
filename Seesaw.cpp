//Seesaw.cpp
#include "Seesaw.h"
#include "player3D.h"
#include "Collision.h"
#include <cmath>

#include "debug.h"
#include "camera.h"
#include "direct3d.h"
static bool g_SeesawDebugDraw = true;

void Seesaw_SetDebugDraw(bool enable)
{
	g_SeesawDebugDraw = enable;
}

struct SeesawPartConfig
{
	FIELD partType;
	XMFLOAT3 offsetPos;
	XMFLOAT3 scale;
	XMFLOAT3 rotate;
	XMFLOAT3 colliderHalf;
	float colliderOffsetY;
	bool useCustomCollider;
};

static const SeesawPartConfig SEESAW_PARTS[] =
{
	{
		FIELD_SEESAW_1,
		XMFLOAT3(0.0f, -0.2f, 0.0f),
		XMFLOAT3(0.45f, 0.6f, 0.3f),
		XMFLOAT3(0.0f, 0.0f, 0.0f),
		XMFLOAT3(0.0f, 0.0f, 0.0f),
		0.0f,
		false
	},
	{
		FIELD_SEESAW_2,
		XMFLOAT3(0.0f, -0.22f, 0.0f),
		XMFLOAT3(1.2f, 1.0f, 1.3f),
		XMFLOAT3(0.0f, 0.0f, 0.0f),
		XMFLOAT3(0.37f, 0.2f, 2.7f),
		0.5f,
		true
	},
};

static const int SEESAW_PART_COUNT = sizeof(SEESAW_PARTS) / sizeof(SEESAW_PARTS[0]);

static std::vector<SeesawData> g_Seesaws;


XMFLOAT3 Field_GetColliderHalfSize(const MAPDATA& m)
{
	if (m.useCustomCollider)
	{
		return m.colliderHalf;
	}
	return XMFLOAT3{
		BOX_RADIUS * m.scale.x,
		BOX_RADIUS * m.scale.y,
		BOX_RADIUS * m.scale.z
	};
}

XMFLOAT3 Field_GetColliderHalfSize(int mapIndex)
{
	return Field_GetColliderHalfSize(GetFieldMap()[mapIndex]);
}

XMFLOAT3 Field_GetHalfSize(int mapIndex)
{
	const MAPDATA& m = GetFieldMap()[mapIndex];
	return XMFLOAT3{
		BOX_RADIUS * m.scale.x,
		BOX_RADIUS * m.scale.y,
		BOX_RADIUS * m.scale.z
	};
}

void Seesaw_Initialize()
{
	g_Seesaws.clear();
}

void Seesaw_Finalize()
{
	g_Seesaws.clear();
}

void Seesaw_ClearAll()
{
	g_Seesaws.clear();
}

int Seesaw_Create(float x, float y, float z, std::vector<MAPDATA>& mapData)
{
	SeesawData seesaw;
	seesaw.pos = XMFLOAT3(x, y, z);
	seesaw.rotate = XMFLOAT3(0.0f, 0.0f, 0.0f);

	seesaw.params.boardLength = SEESAW_PARTS[1].colliderHalf.z;
	seesaw.params.tiltAxis = SEESAW_TILT_X;

	for (int i = 0; i < SEESAW_PART_COUNT; ++i)
	{
		const auto& part = SEESAW_PARTS[i];

		MAPDATA data;
		data.pos = XMFLOAT3(
			x + part.offsetPos.x,
			y + part.offsetPos.y,
			z + part.offsetPos.z
		);
		data.no = part.partType;
		data.scale = part.scale;
		data.rotate = part.rotate;
		data.colliderHalf = part.colliderHalf;
		data.useCustomCollider = part.useCustomCollider;

		seesaw.partIndices.push_back((int)mapData.size());
		mapData.push_back(data);
	}

	int index = (int)g_Seesaws.size();
	g_Seesaws.push_back(seesaw);
	return index;
}

std::vector<SeesawData>& Seesaw_GetAll()
{
	return g_Seesaws;
}

SeesawData* Seesaw_Get(int index)
{
	if (index < 0 || index >= (int)g_Seesaws.size())
		return nullptr;
	return &g_Seesaws[index];
}

int Seesaw_GetCount()
{
	return (int)g_Seesaws.size();
}

XMFLOAT3 Seesaw_GetBoardColliderHalf(int seesawIndex)
{
	if (seesawIndex < 0 || seesawIndex >= (int)g_Seesaws.size())
		return XMFLOAT3(0.5f, 0.5f, 0.5f);

	return SEESAW_PARTS[1].colliderHalf;
}

float Seesaw_GetPlayerPosition(int seesawIndex, const XMFLOAT3& playerPos)
{
	if (seesawIndex < 0 || seesawIndex >= (int)g_Seesaws.size())
		return 0.0f;

	const SeesawData& seesaw = g_Seesaws[seesawIndex];
	if (seesaw.partIndices.size() < 2)
		return 0.0f;

	int boardIdx = seesaw.partIndices[1];
	std::vector<MAPDATA>& mapData = GetFieldMap();
	if (boardIdx < 0 || boardIdx >= (int)mapData.size())
		return 0.0f;

	const MAPDATA& board = mapData[boardIdx];

	float dx = playerPos.x - board.pos.x;
	float dz = playerPos.z - board.pos.z;

	float yawRad = XMConvertToRadians(board.rotate.y);
	float localX = dx * cosf(-yawRad) - dz * sinf(-yawRad);
	float localZ = dx * sinf(-yawRad) + dz * cosf(-yawRad);

	float projection = 0.0f;
	float boardLength = seesaw.params.boardLength;

	if (seesaw.params.tiltAxis == SEESAW_TILT_X)
	{
		projection = localZ;
	}
	else
	{
		projection = localX;
	}

	if (boardLength > 0.01f)
		return projection / boardLength;
	return 0.0f;
}

bool Seesaw_IsPlayerOnBoard(int seesawIndex, const XMFLOAT3& playerPos)
{
	if (seesawIndex < 0 || seesawIndex >= (int)g_Seesaws.size())
		return false;

	const SeesawData& seesaw = g_Seesaws[seesawIndex];
	if (seesaw.partIndices.size() < 2)
		return false;

	int boardIdx = seesaw.partIndices[1];
	std::vector<MAPDATA>& mapData = GetFieldMap();

	if (boardIdx < 0 || boardIdx >= (int)mapData.size())
		return false;

	const MAPDATA& board = mapData[boardIdx];
	XMFLOAT3 boardHalf = Field_GetColliderHalfSize(board);

	float colliderCenterY = board.pos.y + SEESAW_PARTS[1].colliderOffsetY;


	float dx = playerPos.x - board.pos.x;
	float dz = playerPos.z - board.pos.z;

	float yawRad = XMConvertToRadians(board.rotate.y);
	float localX = dx * cosf(-yawRad) - dz * sinf(-yawRad);
	float localZ = dx * sinf(-yawRad) + dz * cosf(-yawRad);

	const float playerRadius = 0.45f;
	if (fabsf(localX) > boardHalf.x + playerRadius) return false;
	if (fabsf(localZ) > boardHalf.z + playerRadius) return false;

	float boardLength = seesaw.params.boardLength;
	float normalizedPos = 0.0f;

	if (seesaw.params.tiltAxis == SEESAW_TILT_X)
	{
		float clampedZ = fmaxf(-boardLength, fminf(boardLength, localZ));
		normalizedPos = (boardLength > 0.01f) ? (clampedZ / boardLength) : 0.0f;
	}
	else
	{
		float clampedX = fmaxf(-boardLength, fminf(boardLength, localX));
		normalizedPos = (boardLength > 0.01f) ? (clampedX / boardLength) : 0.0f;
	}

	normalizedPos = fmaxf(-1.0f, fminf(1.0f, normalizedPos));

	float tiltRad = XMConvertToRadians(seesaw.params.tiltAngle);

	float heightOffset = 0.0f;
	if (seesaw.params.tiltAxis == SEESAW_TILT_X)
	{
		heightOffset = sinf(tiltRad) * localZ;
	}
	else
	{
		heightOffset = sinf(tiltRad) * localX;
	}

	float boardHeightAtPlayer = colliderCenterY + heightOffset;
	float boardTopY = boardHeightAtPlayer + boardHalf.y;
	float playerFootY = playerPos.y;
	float heightDiff = playerFootY - boardTopY;

	return (heightDiff >= -0.25f && heightDiff <= 0.5f);
}

static void UpdateSingleSeesaw(int index, float deltaTime)
{
	if (index < 0 || index >= (int)g_Seesaws.size())
		return;

	SeesawData& seesaw = g_Seesaws[index];
	if (seesaw.partIndices.size() < 2)
		return;

	SeesawParams& params = seesaw.params;
	PLAYER* player = GetPlayer3D();

	float targetAngle = 0.0f;
	bool playerOnBoard = player && Seesaw_IsPlayerOnBoard(index, player->Position);

	if (playerOnBoard)
	{
		float posOnBoard = Seesaw_GetPlayerPosition(index, player->Position);
		posOnBoard = fmaxf(-1.0f, fminf(1.0f, posOnBoard));
		targetAngle = posOnBoard * params.maxTiltAngle;
	}
	else if (params.autoReturn)
	{
		targetAngle = 0.0f;
	}
	else
	{
		targetAngle = params.tiltAngle;
	}

	float angleDiff = targetAngle - params.tiltAngle;
	float speed = playerOnBoard ? params.tiltSpeed : params.returnSpeed;

	if (fabsf(angleDiff) > 0.05f && speed > 0.01f)
	{
		float maxChange = speed * deltaTime;
		if (fabsf(angleDiff) <= maxChange)
			params.tiltAngle = targetAngle;
		else
			params.tiltAngle += (angleDiff > 0 ? 1.0f : -1.0f) * maxChange;
	}
	else if (fabsf(angleDiff) <= 0.05f)
	{
		params.tiltAngle = targetAngle;
	}

	params.tiltAngle = fmaxf(-params.maxTiltAngle, fminf(params.maxTiltAngle, params.tiltAngle));

	int boardIdx = seesaw.partIndices[1];
	std::vector<MAPDATA>& mapData = GetFieldMap();
	if (boardIdx >= 0 && boardIdx < (int)mapData.size())
	{
		if (params.tiltAxis == SEESAW_TILT_X)
		{
			mapData[boardIdx].rotate.x = params.tiltAngle;
			mapData[boardIdx].rotate.z = 0.0f;
		}
		else // SEESAW_TILT_Z
		{
			mapData[boardIdx].rotate.x = 0.0f;
			mapData[boardIdx].rotate.z = params.tiltAngle;
		}
	}
}

void Seesaw_UpdateAll(float deltaTime)
{
	for (int i = 0; i < (int)g_Seesaws.size(); ++i)
	{
		UpdateSingleSeesaw(i, deltaTime);
	}
}

static bool Resolve_Ellipsoid_OBB_FullRotation(
	const XMFLOAT3& ellC, const XMFLOAT3& ellR,
	const XMFLOAT3& boxC, const XMFLOAT3& boxH, const XMFLOAT3& boxRotDeg,
	XMFLOAT3* outPush, XMFLOAT3* outNormal)
{
	if (outPush) *outPush = { 0, 0, 0 };
	if (outNormal) *outNormal = { 0, 1, 0 };

	XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(boxRotDeg.x),
		XMConvertToRadians(boxRotDeg.y),
		XMConvertToRadians(boxRotDeg.z)
	);
	XMMATRIX invRotMat = XMMatrixTranspose(rotMat);

	XMFLOAT3 d = {
		ellC.x - boxC.x,
		ellC.y - boxC.y,
		ellC.z - boxC.z
	};

	XMVECTOR vD = XMLoadFloat3(&d);
	XMVECTOR vLocal = XMVector3TransformNormal(vD, invRotMat);
	XMFLOAT3 localD;
	XMStoreFloat3(&localD, vLocal);

	XMFLOAT3 closest = {
		fmaxf(-boxH.x, fminf(boxH.x, localD.x)),
		fmaxf(-boxH.y, fminf(boxH.y, localD.y)),
		fmaxf(-boxH.z, fminf(boxH.z, localD.z))
	};

	XMVECTOR vClosest = XMLoadFloat3(&closest);
	XMVECTOR vClosestWorld = XMVector3TransformNormal(vClosest, rotMat);
	XMFLOAT3 closestWorld;
	XMStoreFloat3(&closestWorld, vClosestWorld);
	closestWorld.x += boxC.x;
	closestWorld.y += boxC.y;
	closestWorld.z += boxC.z;

	XMFLOAT3 invR = { 1.0f / ellR.x, 1.0f / ellR.y, 1.0f / ellR.z };
	XMFLOAT3 toEll = {
		(ellC.x - closestWorld.x) * invR.x,
		(ellC.y - closestWorld.y) * invR.y,
		(ellC.z - closestWorld.z) * invR.z
	};

	float dist = sqrtf(toEll.x * toEll.x + toEll.y * toEll.y + toEll.z * toEll.z);

	if (dist >= 1.0f) return false;

	XMFLOAT3 nS;
	float pen;

	if (dist > 1e-6f)
	{
		nS = { toEll.x / dist, toEll.y / dist, toEll.z / dist };
		pen = 1.0f - dist;
	}
	else
	{
		XMFLOAT3 absLocal = { fabsf(localD.x), fabsf(localD.y), fabsf(localD.z) };
		float dx = boxH.x - absLocal.x;
		float dy = boxH.y - absLocal.y;
		float dz = boxH.z - absLocal.z;

		XMFLOAT3 localN = { 0, 1, 0 };
		if (dx <= dy && dx <= dz)
		{
			localN = { (localD.x >= 0) ? 1.0f : -1.0f, 0, 0 };
			pen = 1.0f + dx;
		}
		else if (dy <= dz)
		{
			localN = { 0, (localD.y >= 0) ? 1.0f : -1.0f, 0 };
			pen = 1.0f + dy;
		}
		else
		{
			localN = { 0, 0, (localD.z >= 0) ? 1.0f : -1.0f };
			pen = 1.0f + dz;
		}

		XMVECTOR vLocalN = XMLoadFloat3(&localN);
		XMVECTOR vWorldN = XMVector3TransformNormal(vLocalN, rotMat);
		XMFLOAT3 worldN;
		XMStoreFloat3(&worldN, vWorldN);

		nS = { worldN.x * invR.x, worldN.y * invR.y, worldN.z * invR.z };
		float nLen = sqrtf(nS.x * nS.x + nS.y * nS.y + nS.z * nS.z);
		if (nLen > 1e-6f)
		{
			nS.x /= nLen; nS.y /= nLen; nS.z /= nLen;
		}
	}

	XMFLOAT3 pushW = { nS.x * pen * ellR.x, nS.y * pen * ellR.y, nS.z * pen * ellR.z };

	XMFLOAT3 nW = { nS.x * ellR.x, nS.y * ellR.y, nS.z * ellR.z };
	float nLen = sqrtf(nW.x * nW.x + nW.y * nW.y + nW.z * nW.z);
	if (nLen > 1e-6f)
	{
		nW.x /= nLen; nW.y /= nLen; nW.z /= nLen;
	}

	if (outPush) *outPush = pushW;
	if (outNormal) *outNormal = nW;

	return true;
}

int Seesaw_PlayerCollision()
{
	int hit = HIT_NONE;

	PLAYER* player = GetPlayer3D();
	if (!player) return hit;

	if (g_Seesaws.empty()) return hit;

	std::vector<MAPDATA>& mapData = GetFieldMap();

	XMFLOAT3 ellR = Player3D_GetSolidHalfSize();
	XMFLOAT3 ellC = player->Position;
	ellC.y += ellR.y;

	bool positionUpdated = false;

	for (int i = 0; i < (int)g_Seesaws.size(); ++i)
	{
		const SeesawData& seesaw = g_Seesaws[i];
		if (seesaw.partIndices.size() < 2) continue;

		int boardIdx = seesaw.partIndices[1];
		if (boardIdx < 0 || boardIdx >= (int)mapData.size()) continue;

		MAPDATA& board = mapData[boardIdx];

		XMFLOAT3 boardHalf = Field_GetColliderHalfSize(board);

		XMFLOAT3 boardCenter = {
			board.pos.x,
			board.pos.y + SEESAW_PARTS[1].colliderOffsetY,
			board.pos.z
		};

		XMFLOAT3 push, norm;
		if (!Resolve_Ellipsoid_OBB_FullRotation(
			ellC, ellR,
			boardCenter, boardHalf, board.rotate,
			&push, &norm))
		{
			continue;
		}

		ellC.x += push.x;
		ellC.y += push.y;
		ellC.z += push.z;
		positionUpdated = true;

		float ax = fabsf(norm.x), ay = fabsf(norm.y), az = fabsf(norm.z);

		if (ay >= ax && ay >= az)
		{
			if (norm.y > 0)
			{
				player->isGround = true;
				if (player->Velocity.y < 0)
					player->Velocity.y = 0;
				hit = HIT_GROUND;
			}
			else if (player->Velocity.y > 0)
			{
				player->Velocity.y = 0;
			}
		}
		else if (ax >= az)
		{
			player->Velocity.x = 0;
			hit = (norm.x > 0) ? HIT_WALL_PlusX : HIT_WALL_NegX;
		}
		else
		{
			player->Velocity.z = 0;
			hit = (norm.z > 0) ? HIT_WALL_PlusZ : HIT_WALL_NegZ;
		}
	}

	if (positionUpdated)
	{
		player->Position.x = ellC.x;
		player->Position.y = ellC.y - ellR.y;
		player->Position.z = ellC.z;
	}

	return hit;
}


static ImVec2 SeesawWorldToScreen(const XMFLOAT3& p, bool* valid)
{
	*valid = false;

	XMMATRIX vp = GetViewMatrix() * GetProjectionMatrix();
	XMVECTOR vW = XMVectorSet(p.x, p.y, p.z, 1.0f);
	XMVECTOR vV = XMVector3TransformCoord(vW, GetViewMatrix());

	if (XMVectorGetZ(vV) <= 0.01f) return ImVec2(0, 0);

	XMVECTOR vC = XMVector3TransformCoord(vW, vp);
	XMFLOAT3 ndc;
	XMStoreFloat3(&ndc, vC);

	if (ndc.x < -1.5f || ndc.x > 1.5f || ndc.y < -1.5f || ndc.y > 1.5f)
		return ImVec2(0, 0);

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	float w = (float)Direct3D_GetBackBufferWidth();
	float h = (float)Direct3D_GetBackBufferHeight();

	*valid = true;
	return ImVec2(
		viewport->Pos.x + (ndc.x * 0.5f + 0.5f) * w,
		viewport->Pos.y + (-ndc.y * 0.5f + 0.5f) * h
	);
}

static void SeesawDrawLine3D(const XMFLOAT3& a, const XMFLOAT3& b, ImU32 col, float thick = 1.0f)
{
	bool va, vb;
	ImVec2 sa = SeesawWorldToScreen(a, &va);
	ImVec2 sb = SeesawWorldToScreen(b, &vb);
	if (va && vb)
		ImGui::GetBackgroundDrawList()->AddLine(sa, sb, col, thick);
}

static void SeesawDrawOBB_FullRotation(const XMFLOAT3& center, const XMFLOAT3& half,
	const XMFLOAT3& rotDeg, ImU32 col)
{
	XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(rotDeg.x),
		XMConvertToRadians(rotDeg.y),
		XMConvertToRadians(rotDeg.z)
	);

	XMFLOAT3 localCorners[8] = {
		{-half.x, -half.y, -half.z}, {+half.x, -half.y, -half.z},
		{+half.x, +half.y, -half.z}, {-half.x, +half.y, -half.z},
		{-half.x, -half.y, +half.z}, {+half.x, -half.y, +half.z},
		{+half.x, +half.y, +half.z}, {-half.x, +half.y, +half.z},
	};

	XMFLOAT3 worldCorners[8];
	for (int i = 0; i < 8; i++)
	{
		XMVECTOR vLocal = XMLoadFloat3(&localCorners[i]);
		XMVECTOR vWorld = XMVector3TransformNormal(vLocal, rotMat);
		XMFLOAT3 rotated;
		XMStoreFloat3(&rotated, vWorld);
		worldCorners[i] = XMFLOAT3(
			center.x + rotated.x,
			center.y + rotated.y,
			center.z + rotated.z
		);
	}

	const int edges[12][2] = {
		{0,1},{1,2},{2,3},{3,0},
		{4,5},{5,6},{6,7},{7,4},
		{0,4},{1,5},{2,6},{3,7}
	};

	for (int i = 0; i < 12; i++)
		SeesawDrawLine3D(worldCorners[edges[i][0]], worldCorners[edges[i][1]], col, 2.0f);
}

static void SeesawDrawPoint3D(const XMFLOAT3& p, ImU32 col, float size = 4.0f)
{
	bool valid;
	ImVec2 sp = SeesawWorldToScreen(p, &valid);
	if (valid)
		ImGui::GetBackgroundDrawList()->AddCircleFilled(sp, size, col);
}

void Seesaw_DebugDraw()
{
	if (!g_SeesawDebugDraw) return;
	if (g_Seesaws.empty()) return;

	std::vector<MAPDATA>& mapData = GetFieldMap();
	PLAYER* player = GetPlayer3D();

	for (int i = 0; i < (int)g_Seesaws.size(); ++i)
	{
		const SeesawData& seesaw = g_Seesaws[i];
		if (seesaw.partIndices.size() < 2) continue;

		int boardIdx = seesaw.partIndices[1];
		if (boardIdx < 0 || boardIdx >= (int)mapData.size()) continue;

		const MAPDATA& board = mapData[boardIdx];
		XMFLOAT3 boardHalf = Field_GetColliderHalfSize(board);

		XMFLOAT3 boardColliderCenter = {
		   board.pos.x,
		   board.pos.y + SEESAW_PARTS[1].colliderOffsetY,
		   board.pos.z
		};

		bool playerOnBoard = player && Seesaw_IsPlayerOnBoard(i, player->Position);

		ImU32 boardColor = playerOnBoard ?
			IM_COL32(255, 100, 100, 255) :
			IM_COL32(100, 255, 100, 255);

		SeesawDrawOBB_FullRotation(boardColliderCenter, boardHalf, board.rotate, boardColor);

		SeesawDrawPoint3D(boardColliderCenter, IM_COL32(255, 255, 0, 255), 5.0f);

		if (seesaw.partIndices.size() >= 1)
		{
			int baseIdx = seesaw.partIndices[0];
			if (baseIdx >= 0 && baseIdx < (int)mapData.size())
			{
				const MAPDATA& base = mapData[baseIdx];
				XMFLOAT3 baseHalf = Field_GetColliderHalfSize(base);
				SeesawDrawOBB_FullRotation(base.pos, baseHalf, base.rotate,
					IM_COL32(100, 100, 255, 255));
			}
		}

		SeesawDrawPoint3D(seesaw.pos, IM_COL32(255, 0, 255, 255), 6.0f);

		XMFLOAT3 axisEnd = board.pos;
		if (seesaw.params.tiltAxis == SEESAW_TILT_X)
		{
			axisEnd.x += 0.5f;
			SeesawDrawLine3D(board.pos, axisEnd, IM_COL32(255, 0, 0, 255), 3.0f);
		}
		else
		{
			axisEnd.z += 0.5f;
			SeesawDrawLine3D(board.pos, axisEnd, IM_COL32(0, 0, 255, 255), 3.0f);
		}

		if (seesaw.partIndices.size() >= 1)
		{
			int baseIdx = seesaw.partIndices[0];
			if (baseIdx >= 0 && baseIdx < (int)mapData.size())
			{
				const MAPDATA& base = mapData[baseIdx];
				XMFLOAT3 baseHalf = Field_GetColliderHalfSize(base);
				SeesawDrawOBB_FullRotation(base.pos, baseHalf, base.rotate, IM_COL32(128, 128, 128, 255));
			}
		}

	}

	ImGui::Begin("Seesaw Debug");
	ImGui::Text("Seesaw Count: %d", (int)g_Seesaws.size());

	for (int i = 0; i < (int)g_Seesaws.size(); ++i)
	{
		const SeesawData& seesaw = g_Seesaws[i];
		if (ImGui::TreeNode((void*)(intptr_t)i, "Seesaw %d", i))
		{
			ImGui::Text("Position: (%.2f, %.2f, %.2f)",
				seesaw.pos.x, seesaw.pos.y, seesaw.pos.z);
			ImGui::Text("Tilt Angle: %.2f deg", seesaw.params.tiltAngle);
			ImGui::Text("Max Tilt: %.2f deg", seesaw.params.maxTiltAngle);
			ImGui::Text("Board Length: %.2f", seesaw.params.boardLength);

			if (player)
			{
				bool onBoard = Seesaw_IsPlayerOnBoard(i, player->Position);
				ImGui::Text("Player On Board: %s", onBoard ? "YES" : "NO");
				if (onBoard)
				{
					float posOnBoard = Seesaw_GetPlayerPosition(i, player->Position);
					ImGui::Text("Player Position: %.2f", posOnBoard);
				}
			}

			if (seesaw.partIndices.size() >= 2)
			{
				int boardIdx = seesaw.partIndices[1];
				if (boardIdx >= 0 && boardIdx < (int)mapData.size())
				{
					const MAPDATA& board = mapData[boardIdx];
					XMFLOAT3 half = Field_GetColliderHalfSize(board);
					ImGui::Text("Board Collider Half: (%.2f, %.2f, %.2f)",
						half.x, half.y, half.z);
					ImGui::Text("Board Rotate: (%.2f, %.2f, %.2f)",
						board.rotate.x, board.rotate.y, board.rotate.z);
				}
			}

			ImGui::TreePop();
		}
	}

	ImGui::Checkbox("Enable Debug Draw", &g_SeesawDebugDraw);
	ImGui::End();
}