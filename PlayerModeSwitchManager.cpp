//PlayerModeSwitchManager.cpp
#include "PlayerModeSwitchManager.h"

#include "field.h"
#include "collision.h"
#include "player3D.h"
#include "Player2D.h"
#include "keyboard.h"

#include <cfloat>
#include <algorithm>

#include "debug.h"
#include "MathUtil.h"
using namespace mu;

static bool debugMode = TRUE;

static const float kSwitchMaxDist = 2.0f; // 3Dプレイヤーから2Dプレイヤーへの変身可能距離
static const float kPlayer2DThickness = 0.05f; // 2Dプレイヤーの厚み（当たり判定用）
static const float kWallOffset = 0.02f; // 壁からの微小オフセット
static const float kTo3DFrontOffset = -1.0f; // 2D→3D変身時の前方オフセット
static const float kGroundSearchUp = 3.0f; // 2Dプレイヤーの地面探索上方向範囲
static const float kGroundSearchDown = 10.0f; // 2Dプレイヤーの地面探索下方向範囲

static PLAYER_MODE g_Mode = MODE_3D; 

static const auto TABKey = KK_TAB;

// フィールドの半分のサイズを取得
static XMFLOAT3 Field_GetHalfSize(const MAPDATA& m)
{
    return XMFLOAT3{
    BOX_RADIUS * m.scale.x,
    BOX_RADIUS * m.scale.y,
    BOX_RADIUS * m.scale.z
    };
}
// フィールドが壁かどうかを取得
static bool Field_IsWall(FIELD t)
{
    switch (t)
    {
    case FIELD_OBJ_1:
        return true;
    default:
        return false;
    }
}

// レイとAABBの当たり判定（面情報付き）
static bool RaycastAABB_Face(
    const XMFLOAT3& rayO,
    const XMFLOAT3& rayD_in,
    const XMFLOAT3& boxC,
    const XMFLOAT3& boxHalf,
    float maxDist,
    BOX_FACE* outFace,
    XMFLOAT3* outNormal,
    float* outT)
{
    XMFLOAT3 rayD = Normalize(rayD_in);

    float tmin = 0.0f;
    float tmax = maxDist;

    BOX_FACE hitFace = FACE_NONE;
    XMFLOAT3 hitN = { 0,0,0 };

    auto slab = [&](float ro, float rd, float c, float h, BOX_FACE negFace, BOX_FACE posFace, XMFLOAT3 nNeg, XMFLOAT3 nPos)->bool
        {
            if (fabsf(rd) < 1e-6f)
            {
                return (ro >= c - h && ro <= c + h);
            }

            float inv = 1.0f / rd;
            float t1 = (c - h - ro) * inv;
            float t2 = (c + h - ro) * inv;

            BOX_FACE f1 = negFace;
            BOX_FACE f2 = posFace;
            XMFLOAT3 n1 = nNeg;
            XMFLOAT3 n2 = nPos;

            if (t1 > t2) { std::swap(t1, t2); std::swap(f1, f2); std::swap(n1, n2); }

            if (t1 > tmin) { tmin = t1; hitFace = f1; hitN = n1; }
            if (t2 < tmax) { tmax = t2; }

            return tmin <= tmax;
        };

    if (!slab(rayO.x, rayD.x, boxC.x, boxHalf.x, FACE_NEG_X, FACE_POS_X, { -1,0,0 }, { 1,0,0 })) return false;
    if (!slab(rayO.y, rayD.y, boxC.y, boxHalf.y, FACE_NEG_Y, FACE_POS_Y, { 0,-1,0 }, { 0,1,0 })) return false;
    if (!slab(rayO.z, rayD.z, boxC.z, boxHalf.z, FACE_NEG_Z, FACE_POS_Z, { 0,0,-1 }, { 0,0,1 })) return false;

    if (tmin < 0.0f || tmin > maxDist) return false;

    if (outFace)   *outFace = hitFace;
    if (outNormal) *outNormal = hitN;
    if (outT)      *outT = tmin;
    return true;
}

// レイとOBBの当たり判定（面情報付き）
static bool RaycastOBB_Face(
    const XMFLOAT3& rayO,
    const XMFLOAT3& rayD_in,
    const MAPDATA& box,
    float maxDist,
    BOX_FACE* outFace,
    XMFLOAT3* outNormalW,
    float* outT)
{
    const XMFLOAT3 half = Field_GetHalfSize(box);

    const XMMATRIX R = XMMatrixRotationRollPitchYaw(
        XMConvertToRadians(box.rotate.x),
        XMConvertToRadians(box.rotate.y),
        XMConvertToRadians(box.rotate.z));

    const XMMATRIX invR = XMMatrixTranspose(R);

    XMVECTOR O = XMLoadFloat3(&rayO);
    XMVECTOR D = XMLoadFloat3(&rayD_in);
    D = XMVector3Normalize(D);

    XMVECTOR C = XMLoadFloat3(&box.pos);

    XMVECTOR Orel = O - C;
    XMVECTOR OlocV = XMVector3TransformNormal(Orel, invR);
    XMVECTOR DlocV = XMVector3TransformNormal(D, invR);

    XMFLOAT3 Oloc, Dloc;
    XMStoreFloat3(&Oloc, OlocV);
    XMStoreFloat3(&Dloc, DlocV);

    BOX_FACE faceL = FACE_NONE;
    XMFLOAT3 nL = { 0,0,0 };
    float t = 0.0f;

    if (!RaycastAABB_Face(Oloc, Dloc, XMFLOAT3(0, 0, 0), half, maxDist, &faceL, &nL, &t))
        return false;

    XMVECTOR nW = XMVector3TransformNormal(XMLoadFloat3(&nL), R);
    nW = XMVector3Normalize(nW);

    if (outFace) *outFace = faceL;
    if (outNormalW) XMStoreFloat3(outNormalW, nW);
    if (outT) *outT = t;
    return true;
}


// 指定面に2Dプレイヤーが収まるかどうか
static bool CanFitOnFace(BOX_FACE face, const XMFLOAT3& fieldHalf, const XMFLOAT3& p2Half)
{
    switch (face)
    {
    case FACE_POS_X: case FACE_NEG_X:
        return (p2Half.y <= fieldHalf.y) && (p2Half.z <= fieldHalf.z);
    case FACE_POS_Z: case FACE_NEG_Z:
        return (p2Half.y <= fieldHalf.y) && (p2Half.x <= fieldHalf.x);
    case FACE_POS_Y: case FACE_NEG_Y:
        return (p2Half.x <= fieldHalf.x) && (p2Half.z <= fieldHalf.z);
    default:
        return false;
    }
}

// 面に対応するYaw角度を取得
static float FaceYawDeg(BOX_FACE face)
{
    switch (face)
    {
    case FACE_POS_Z: return 0.0f;
    case FACE_NEG_Z: return 180.0f;
    case FACE_POS_X: return 90.0f;
    case FACE_NEG_X: return -90.0f;
    default:         return 0.0f;
    }
}

// 指定面に対する2D回転を計算
static XMFLOAT3 Calc2DRotationFromFace(const MAPDATA& box, BOX_FACE face)
{
    float yawDeg = box.rotate.y + FaceYawDeg(face) + 180.0f;
    return { 0.0f, yawDeg, 0.0f };
}

// 指定面上の2D位置を計算
static XMFLOAT3 Calc2DPositionOnFace_OBB(
    const MAPDATA& box,
    BOX_FACE face,
    const XMFLOAT3& p3Pos,
    const XMFLOAT3& p2Half)
{
    const XMFLOAT3 half = Field_GetHalfSize(box);

    const XMMATRIX R = XMMatrixRotationRollPitchYaw(
        XMConvertToRadians(box.rotate.x),
        XMConvertToRadians(box.rotate.y),
        XMConvertToRadians(box.rotate.z));
    const XMMATRIX invR = XMMatrixTranspose(R);

    XMVECTOR C = XMLoadFloat3(&box.pos);
    XMVECTOR P = XMLoadFloat3(&p3Pos);

    XMVECTOR PlocV = XMVector3TransformNormal(P - C, invR);
    XMFLOAT3 local;
    XMStoreFloat3(&local, PlocV);

	// 面に収まるようにクランプ
    float minX = -half.x + p2Half.x;
    float maxX = half.x - p2Half.x;
    float minY = -half.y + p2Half.y;
    float maxY = half.y - p2Half.y;
    float minZ = -half.z + p2Half.z;
    float maxZ = half.z - p2Half.z;

	// クランプ
    local.x = Clamp(local.x, minX, maxX);
    local.y = Clamp(local.y, minY, maxY);
    local.z = Clamp(local.z, minZ, maxZ);

	// 面に合わせて位置調整
    if (face == FACE_POS_X) local.x = half.x + (kPlayer2DThickness + kWallOffset);
    if (face == FACE_NEG_X) local.x = -half.x - (kPlayer2DThickness + kWallOffset);
    if (face == FACE_POS_Z) local.z = half.z + (kPlayer2DThickness + kWallOffset);
    if (face == FACE_NEG_Z) local.z = -half.z - (kPlayer2DThickness + kWallOffset);
    if (face == FACE_POS_Y) local.y = half.y + (kPlayer2DThickness + kWallOffset);
    if (face == FACE_NEG_Y) local.y = -half.y - (kPlayer2DThickness + kWallOffset);

    XMVECTOR L = XMLoadFloat3(&local);
    XMVECTOR Pw = XMVector3TransformNormal(L, R) + C;

    XMFLOAT3 out;
    XMStoreFloat3(&out, Pw);
    return out;
}
// 指定XZ位置の地面のトップY座標を取得
static bool FindGroundTopY_OnXZ(float x, float z, float startY, float maxDown, float* outTopY)
{
    std::vector<MAPDATA>& map = GetFieldMap();
    if (map.size() == 0) return false;

    bool found = false;
    float bestTop = -FLT_MAX;

    for (size_t i = 0; i < map.size(); ++i)
    {
        const MAPDATA& m = map[i];

        XMFLOAT3 half = Field_GetHalfSize(m);
        if (x < m.pos.x - half.x || x > m.pos.x + half.x) continue;
        if (z < m.pos.z - half.z || z > m.pos.z + half.z) continue;

        float top = m.pos.y + half.y;

        if (top <= startY && top >= startY - maxDown)
        {
            if (top > bestTop) { bestTop = top; found = true; }
        }
    }

    if (found && outTopY) *outTopY = bestTop;
    return found;
}

//---------------------------------------------------------------------------------------------------------
// プレイヤーモード切り替え管理
static bool GetSwitchTargetFromPlayer3D(SWITCH_TARGET* outT)
{
    PLAYER* p3 = GetPlayer3D();
    if (!p3) return false;

    XMFLOAT3 rayO = p3->Position;
    XMFLOAT3 rayD = Player3D_GetForward();

    std::vector<MAPDATA>& map = GetFieldMap();
    if (map.size() == 0) return false;

    float bestT = FLT_MAX;
    SWITCH_TARGET best{};

    for (size_t i = 0; i < map.size(); ++i)
    {
        const MAPDATA& m = map[i];
        if (!Field_IsWall(map[i].no)) continue;

        XMFLOAT3 half = Field_GetHalfSize(m);

        BOX_FACE face;
        XMFLOAT3 normal;
        float t;

        if (RaycastOBB_Face(rayO, rayD, m, kSwitchMaxDist, &face, &normal, &t))
        {
            if (t < bestT)
            {
                bestT = t;
                best.fieldIndex = (int)i;
                best.face = face;
                best.normal = normal;
            }
        }
    }

    if (best.fieldIndex < 0) return false;
    if (outT) *outT = best;
    return true;
}

// 3D→2D変身試行
static bool TrySwitch3DTo2D()
{
    SWITCH_TARGET tgt;
    if (!GetSwitchTargetFromPlayer3D(&tgt)) return false;

    std::vector<MAPDATA>& map = GetFieldMap();
    const MAPDATA& box = map[tgt.fieldIndex];
    XMFLOAT3 boxHalf = Field_GetHalfSize(box);

    XMFLOAT3 p2Half = Player2D_GetSolidHalfSize();
    if (!CanFitOnFace(tgt.face, boxHalf, p2Half))
        return false;

    PLAYER* p3 = GetPlayer3D();
    XMFLOAT3 p2Pos = Calc2DPositionOnFace_OBB(box, tgt.face, p3->Position, p2Half);
    XMFLOAT3 p2Rot = Calc2DRotationFromFace(box, tgt.face);

    Player2D_InitAt(p2Pos, p2Rot);
    Player2D_SetActive(true);
    Player3D_SetActive(false);

    g_Mode = MODE_2D;
    return true;
}

// 2D→3D変身試行
static bool TrySwitch2DTo3D()
{
    PLAYER* p2 = GetPlayer2D();
    if (!p2) return false;

    float yawRad = XMConvertToRadians(p2->Rotation.y);
    XMFLOAT3 fwd = { sinf(yawRad), 0.0f, cosf(yawRad) };

    XMFLOAT3 front = fwd;

    XMFLOAT3 landing = p2->Position + front * kTo3DFrontOffset;

    float startY = p2->Position.y + kGroundSearchUp;
    float topY;
    if (!FindGroundTopY_OnXZ(landing.x, landing.z, startY, kGroundSearchDown, &topY))
        return false;

    XMFLOAT3 p3Half = Player3D_GetSolidHalfSize();
    XMFLOAT3 p3Pos = { landing.x, topY + p3Half.y, landing.z };

    XMFLOAT3 p3Rot = { 0.0f, p2->Rotation.y, 0.0f };

    Player3D_InitAt(p3Pos, p3Rot);
    Player3D_SetActive(true);

    Player2D_SetActive(false);
    Player2D_Uninit();

    g_Mode = MODE_3D;
    return true;
}



void PlayerModeSwitchManager_Init()
{
    g_Mode = MODE_3D;
}

PLAYER_MODE PlayerModeSwitchManager_GetMode()
{
    return g_Mode;
}

void PlayerModeSwitchManager_Update()
{
    if (debugMode)
    {
        ImGui::Begin("Debug - han");
        if (ImGui::TreeNode("PlSwitch.cpp"))
        {
            ImGui::Text("PlayerMode : %s", g_Mode == MODE_3D ? "3D" : "2D");
            ImGui::TreePop();
        }
        ImGui::End();
    }


    if (!Keyboard_IsKeyDownTrigger(TABKey)) return;

    if (g_Mode == MODE_3D)
    {
        TrySwitch3DTo2D();
    }
    else
    {
        TrySwitch2DTo3D();
    }
}
