// FieldSeesaw.cpp
#include "FieldSeesaw.h"

#include "player3D.h"
#include "Collision.h"
#include "debug.h"
#include "camera.h"
#include "direct3d.h"
#include "UtilDebug.h"

#include <cmath>

using namespace DirectX;

//=========================================================================================================
// 内部設定（部品ごとの配置/当たり）
//=========================================================================================================

struct SeesawPartConfig
{
	FIELD   partType;
	XMFLOAT3 offsetPos;         // シーソー原点からの配置オフセット
	XMFLOAT3 scale;             // 見た目スケール（BOX_RADIUS基準）
	XMFLOAT3 rotate;            // 初期回転
	XMFLOAT3 colliderHalf;      // カスタム当たり半径（useCustomCollider時）
	float   colliderOffsetY;    // 当たり中心のYオフセット（板のみ）
	bool    useCustomCollider;
};

static const SeesawPartConfig SEESAW_PARTS[] =
{
	// ベース
	{
		FIELD_SEESAW_1,
		XMFLOAT3(0.0f, -0.2f, 0.0f),
		XMFLOAT3(0.45f, 0.6f, 0.3f),
		XMFLOAT3(0.0f, 0.0f, 0.0f),
		XMFLOAT3(0.0f, 0.0f, 0.0f),
		0.0f,
		false
	},
	// 板（傾く）
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

//=========================================================================================================
// 内部状態
//=========================================================================================================
static std::vector<SeesawData> g_Seesaws;

//=========================================================================================================
// 初期化 / 破棄
//=========================================================================================================

void Seesaw_Initialize() { g_Seesaws.clear(); }
void Seesaw_Finalize() { g_Seesaws.clear(); }
void Seesaw_ClearAll() { g_Seesaws.clear(); }

//=========================================================================================================
// 生成（ベース/板をMapDataに追加）
//=========================================================================================================

int Seesaw_Create(float x, float y, float z, std::vector<MAPDATA>& mapData)
{
	SeesawData seesaw;
	seesaw.pos = XMFLOAT3(x, y, z);
	seesaw.rotate = XMFLOAT3(0.0f, 0.0f, 0.0f);

	// 板の長さ（正規化用）
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

//=========================================================================================================
// 参照
//=========================================================================================================

std::vector<SeesawData>& Seesaw_GetAll() { return g_Seesaws; }

SeesawData* Seesaw_Get(int index)
{
	if (index < 0 || index >= (int)g_Seesaws.size())
		return nullptr;
	return &g_Seesaws[index];
}

int Seesaw_GetCount() { return (int)g_Seesaws.size(); }

XMFLOAT3 Seesaw_GetBoardColliderHalf(int seesawIndex)
{
	if (seesawIndex < 0 || seesawIndex >= (int)g_Seesaws.size())
		return XMFLOAT3(0.5f, 0.5f, 0.5f);
	return SEESAW_PARTS[1].colliderHalf;
}

//=========================================================================================================
// プレイヤーが板のどの位置にいるか（-1..+1）
//=========================================================================================================

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

	// board中心からの相対（Yawのみローカル化）
	float dx = playerPos.x - board.pos.x;
	float dz = playerPos.z - board.pos.z;

	float yawRad = XMConvertToRadians(board.rotate.y);
	float localX = dx * cosf(-yawRad) - dz * sinf(-yawRad);
	float localZ = dx * sinf(-yawRad) + dz * cosf(-yawRad);

	float boardLength = seesaw.params.boardLength;
	if (boardLength < 0.01f) return 0.0f;

	float position = 0.0f;
	if (seesaw.params.tiltAxis == SEESAW_TILT_X) position = localZ / boardLength;
	else                                        position = localX / boardLength;

	return fmaxf(-1.0f, fminf(1.0f, position));
}

// 板表面のY（プレイヤー接地判定用）
static float GetBoardSurfaceHeightAt(
	const SeesawData& seesaw,
	const MAPDATA& board,
	float localX,
	float localZ,
	const XMFLOAT3& boardHalf)
{
	// 当たり中心Y（板は colliderOffsetY を持つ）
	float baseCenterY = board.pos.y + SEESAW_PARTS[1].colliderOffsetY;

	float tiltRad = XMConvertToRadians(seesaw.params.tiltAngle);
	float heightOffset = 0.0f;

	// 傾き角により片側が上下する（小角近似でなくsinを使用）
	if (seesaw.params.tiltAxis == SEESAW_TILT_X) heightOffset = sinf(tiltRad) * localZ;
	else                                        heightOffset = sinf(tiltRad) * localX;

	// boardHalf.y を足して上面のYへ
	return baseCenterY + heightOffset + boardHalf.y;
}

//=========================================================================================================
// 板の上に乗っている判定（XZ範囲 + 高さ差）
//=========================================================================================================

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
	XMFLOAT3 boardHalf = Field_GetCollisionHalfSize(board); // field.cpp側（整理対象外）

	float dx = playerPos.x - board.pos.x;
	float dz = playerPos.z - board.pos.z;

	float yawRad = XMConvertToRadians(board.rotate.y);
	float cosYaw = cosf(-yawRad);
	float sinYaw = sinf(-yawRad);

	float localX = dx * cosYaw - dz * sinYaw;
	float localZ = dx * sinYaw + dz * cosYaw;

	// 多少の余裕を持たせて搭乗判定
	const float playerRadius = 0.45f;
	const float xMargin = boardHalf.x + playerRadius;
	const float zMargin = boardHalf.z + playerRadius;

	if (fabsf(localX) > xMargin) return false;
	if (fabsf(localZ) > zMargin) return false;

	float boardSurfaceY = GetBoardSurfaceHeightAt(seesaw, board, localX, localZ, boardHalf);
	float playerFootY = playerPos.y;

	float heightDiff = playerFootY - boardSurfaceY;

	const float toleranceBelow = -1.2f; // 下方向許容（落下中の貫通を多少許す）
	const float toleranceAbove = 0.8f;  // 上方向許容

	return (heightDiff >= toleranceBelow && heightDiff <= toleranceAbove);
}

//=========================================================================================================
// 更新（角度制御 + mapData反映）
//=========================================================================================================

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
		// その場保持
		targetAngle = params.tiltAngle;
	}

	float angleDiff = targetAngle - params.tiltAngle;
	float speed = playerOnBoard ? params.tiltSpeed : params.returnSpeed;

	// 角度追従（最大変化量でクランプ）
	if (fabsf(angleDiff) > 0.05f && speed > 0.01f)
	{
		float maxChange = speed * deltaTime;
		if (fabsf(angleDiff) <= maxChange) params.tiltAngle = targetAngle;
		else params.tiltAngle += (angleDiff > 0 ? 1.0f : -1.0f) * maxChange;
	}
	else if (fabsf(angleDiff) <= 0.05f)
	{
		params.tiltAngle = targetAngle;
	}

	params.tiltAngle = fmaxf(-params.maxTiltAngle, fminf(params.maxTiltAngle, params.tiltAngle));

	// MapDataへ回転反映（板のみ）
	int boardIdx = seesaw.partIndices[1];
	std::vector<MAPDATA>& mapData = GetFieldMap();
	if (boardIdx >= 0 && boardIdx < (int)mapData.size())
	{
		if (params.tiltAxis == SEESAW_TILT_X)
		{
			mapData[boardIdx].rotate.x = params.tiltAngle;
			mapData[boardIdx].rotate.z = 0.0f;
		}
		else
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

//=========================================================================================================
// プレイヤー衝突（3D：楕円体 vs 回転OBB）
//=========================================================================================================

static bool Resolve_Ellipsoid_OBB_FullRotation(
	const XMFLOAT3& ellC, const XMFLOAT3& ellR,
	const XMFLOAT3& boxC, const XMFLOAT3& boxH, const XMFLOAT3& boxRotDeg,
	XMFLOAT3* outPush, XMFLOAT3* outNormal)
{
	// 既存実装を保持（コメントのみ補強）
	// 手順：
	// 1) OBBをローカルへ変換し、最近傍点を求める
	// 2) 楕円体空間（スケール空間）で距離1未満なら接触/貫通
	// 3) 押し出しベクトルと法線を返す
	// ※ dist==0 付近は面法線ベースで脱出方向を決定

	if (outPush) *outPush = { 0, 0, 0 };
	if (outNormal) *outNormal = { 0, 1, 0 };

	XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(boxRotDeg.x),
		XMConvertToRadians(boxRotDeg.y),
		XMConvertToRadians(boxRotDeg.z)
	);
	XMMATRIX invRotMat = XMMatrixTranspose(rotMat);

	XMFLOAT3 d = { ellC.x - boxC.x, ellC.y - boxC.y, ellC.z - boxC.z };

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
		if (dx <= dy && dx <= dz) { localN = { (localD.x >= 0) ? 1.0f : -1.0f, 0, 0 }; pen = 1.0f + dx; }
		else if (dy <= dz) { localN = { 0, (localD.y >= 0) ? 1.0f : -1.0f, 0 }; pen = 1.0f + dy; }
		else { localN = { 0, 0, (localD.z >= 0) ? 1.0f : -1.0f }; pen = 1.0f + dz; }

		XMVECTOR vLocalN = XMLoadFloat3(&localN);
		XMVECTOR vWorldN = XMVector3TransformNormal(vLocalN, rotMat);
		XMFLOAT3 worldN;
		XMStoreFloat3(&worldN, vWorldN);

		nS = { worldN.x * invR.x, worldN.y * invR.y, worldN.z * invR.z };
		float nLen = sqrtf(nS.x * nS.x + nS.y * nS.y + nS.z * nS.z);
		if (nLen > 1e-6f) { nS.x /= nLen; nS.y /= nLen; nS.z /= nLen; }
	}

	XMFLOAT3 pushW = { nS.x * pen * ellR.x, nS.y * pen * ellR.y, nS.z * pen * ellR.z };

	XMFLOAT3 nW = { nS.x * ellR.x, nS.y * ellR.y, nS.z * ellR.z };
	float nLen = sqrtf(nW.x * nW.x + nW.y * nW.y + nW.z * nW.z);
	if (nLen > 1e-6f) { nW.x /= nLen; nW.y /= nLen; nW.z /= nLen; }

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
		XMFLOAT3 boardHalf = Field_GetCollisionHalfSize(board);

		// 板の当たり中心は colliderOffsetY 分だけ上にずれているため、回転込みで中心を求める
		XMFLOAT3 localOffset = { 0.0f, SEESAW_PARTS[1].colliderOffsetY, 0.0f };
		XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(
			XMConvertToRadians(board.rotate.x),
			XMConvertToRadians(board.rotate.y),
			XMConvertToRadians(board.rotate.z)
		);

		XMVECTOR vOffset = XMLoadFloat3(&localOffset);
		XMVECTOR vRotatedOffset = XMVector3TransformNormal(vOffset, rotMat);
		XMFLOAT3 rotatedOffset;
		XMStoreFloat3(&rotatedOffset, vRotatedOffset);

		XMFLOAT3 boardCenter = {
			board.pos.x + rotatedOffset.x,
			board.pos.y + rotatedOffset.y,
			board.pos.z + rotatedOffset.z
		};

		XMFLOAT3 push, norm;
		if (!Resolve_Ellipsoid_OBB_FullRotation(ellC, ellR, boardCenter, boardHalf, board.rotate, &push, &norm))
			continue;

		ellC.x += push.x;
		ellC.y += push.y;
		ellC.z += push.z;
		positionUpdated = true;

		// 法線の最大成分でヒット種別を決める（簡易）
		float ax = fabsf(norm.x), ay = fabsf(norm.y), az = fabsf(norm.z);

		if (ay >= ax && ay >= az)
		{
			if (norm.y > 0)
			{
				player->isGround = true;
				if (player->Velocity.y < 0) player->Velocity.y = 0;
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

//=========================================================================================================
// デバッグ描画
//=========================================================================================================

void Seesaw_DebugDraw()
{
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
		XMFLOAT3 boardHalf = Field_GetCollisionHalfSize(board);

		// collider中心算出（Updateと同様）
		XMFLOAT3 localOffset = { 0.0f, SEESAW_PARTS[1].colliderOffsetY, 0.0f };
		XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(
			XMConvertToRadians(board.rotate.x),
			XMConvertToRadians(board.rotate.y),
			XMConvertToRadians(board.rotate.z)
		);
		XMVECTOR vOffset = XMLoadFloat3(&localOffset);
		XMVECTOR vRotatedOffset = XMVector3TransformNormal(vOffset, rotMat);
		XMFLOAT3 rotatedOffset;
		XMStoreFloat3(&rotatedOffset, vRotatedOffset);

		XMFLOAT3 boardColliderCenter = {
			board.pos.x + rotatedOffset.x,
			board.pos.y + rotatedOffset.y,
			board.pos.z + rotatedOffset.z
		};

		bool playerOnBoard = player && Seesaw_IsPlayerOnBoard(i, player->Position);
		ImU32 boardColor = playerOnBoard ? IM_COL32(255, 100, 100, 255) : IM_COL32(100, 255, 100, 255);

		DebugDrawOBB_FullRotation(boardColliderCenter, boardHalf, board.rotate, boardColor);
		DrawPoint3D(boardColliderCenter, IM_COL32(255, 255, 0, 255), 5.0f);

		// プレイヤーと板表面の距離可視化
		if (player)
		{
			float dx = player->Position.x - board.pos.x;
			float dz = player->Position.z - board.pos.z;
			float yawRad = XMConvertToRadians(board.rotate.y);
			float localX = dx * cosf(-yawRad) - dz * sinf(-yawRad);
			float localZ = dx * sinf(-yawRad) + dz * cosf(-yawRad);

			float tiltRad = XMConvertToRadians(seesaw.params.tiltAngle);
			float heightOffset = (seesaw.params.tiltAxis == SEESAW_TILT_X) ? (sinf(tiltRad) * localZ) : (sinf(tiltRad) * localX);

			float baseCenterY = board.pos.y + SEESAW_PARTS[1].colliderOffsetY;
			float surfaceY = baseCenterY + heightOffset + boardHalf.y;

			XMFLOAT3 surfacePoint = { player->Position.x, surfaceY, player->Position.z };
			DrawPoint3D(surfacePoint, IM_COL32(0, 255, 255, 255), 6.0f);

			XMFLOAT3 footPoint = { player->Position.x, player->Position.y, player->Position.z };
			DrawLine3D(footPoint, surfacePoint, IM_COL32(255, 0, 255, 200), 2.0f);
		}

		// ベース（部品0）
		if (seesaw.partIndices.size() >= 1)
		{
			int baseIdx = seesaw.partIndices[0];
			if (baseIdx >= 0 && baseIdx < (int)mapData.size())
			{
				const MAPDATA& base = mapData[baseIdx];
				XMFLOAT3 baseHalf = Field_GetCollisionHalfSize(base);
				DebugDrawOBB_FullRotation(base.pos, baseHalf, base.rotate, IM_COL32(128, 128, 128, 255));
			}
		}

		DrawPoint3D(seesaw.pos, IM_COL32(255, 0, 255, 255), 6.0f);
	}

	// ImGui表示（既存維持）
	ImGui::Begin("Seesaw Debug");
	ImGui::Text("Seesaw Count: %d", (int)g_Seesaws.size());

	for (int i = 0; i < (int)g_Seesaws.size(); ++i)
	{
		const SeesawData& seesaw = g_Seesaws[i];
		if (ImGui::TreeNode((void*)(intptr_t)i, "Seesaw %d", i))
		{
			ImGui::Text("Position: (%.2f, %.2f, %.2f)", seesaw.pos.x, seesaw.pos.y, seesaw.pos.z);
			ImGui::Text("Tilt Angle: %.2f deg", seesaw.params.tiltAngle);
			ImGui::Text("Max Tilt: %.2f deg", seesaw.params.maxTiltAngle);
			ImGui::Text("Board Length: %.2f", seesaw.params.boardLength);

			ImGui::TreePop();
		}
	}
	ImGui::End();
}