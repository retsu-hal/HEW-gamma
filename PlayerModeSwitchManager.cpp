//PlayerModeSwitchManager.cpp
#include "PlayerModeSwitchManager.h"
#include <cfloat>
#include "field.h"
#include "player3D.h"
#include "Player2D.h"
#include "keyboard.h"
#include "MathUtil.h"
using namespace mu;


#include "debug.h"
#include "camera.h"
static bool debugMode;

static const float kSwitchMaxDist = 2.0f;
static const float kPlayer2DThickness = 0.05f;
static const float kWallOffset = 0.02f;
static const float kTo3DFrontOffset = -1.0f;
static const float kGroundSearchUp = 3.0f;
static const float kGroundSearchDown = 10.0f;

static PLAYER_MODE g_Mode = MODE_3D;

// 3Dと2Dの切り替えに使用するキー
static const auto TABKey = KK_TAB;

// フィールドがトリガー用かどうかを取得
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

// 3D空間の線を描画する関数
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

// 3D空間の線とOBBの当たり判定（Y軸回転のみ対応）
static bool RaycastOBB_Face(
    const XMFLOAT3& rayO,
    const XMFLOAT3& rayD_in,
    const MAPDATA& box,
    float maxDist,
    BOX_FACE* outFace,
    XMFLOAT3* outNormalW,
    float* outT)
{
    const XMFLOAT3 half = Field_GetCollisionHalfSize(box);

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


// 3Dボックスの面に応じた2D回転を取得
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

// 3Dボックスの面に応じた2D回転を取得
static XMFLOAT3 Calc2DRotationFromFace(const MAPDATA& box, BOX_FACE face)
{
    float yawDeg = box.rotate.y + FaceYawDeg(face) + 180.0f;
    return { 0.0f, yawDeg, 0.0f };
}


// 3Dボックスの面に応じた2D位置を取得
static XMFLOAT3 Calc2DPositionOnWallSurface(
    const MAPDATA& box,
    BOX_FACE face,
    const XMFLOAT3& p3Pos,
    const XMFLOAT3& faceNormalW)
{
    const XMFLOAT3 half = Field_GetCollisionHalfSize(box);

    const XMMATRIX R = XMMatrixRotationRollPitchYaw(
        XMConvertToRadians(box.rotate.x),
        XMConvertToRadians(box.rotate.y),
        XMConvertToRadians(box.rotate.z));
    const XMMATRIX invR = XMMatrixTranspose(R);

    XMVECTOR C = XMLoadFloat3(&box.pos);
    XMVECTOR P = XMLoadFloat3(&p3Pos);

    // 3D空間のプレイヤー位置を、ボックスのローカル空間に変換
    XMVECTOR PlocV = XMVector3TransformNormal(P - C, invR);
    XMFLOAT3 local;
    XMStoreFloat3(&local, PlocV);

    // 面から少し離すためのオフセット
    // 2Dプレイヤーの厚み + 壁からの微小な距離
    float offset = kPlayer2DThickness + kWallOffset;

    switch (face)
    {
    case FACE_POS_X: local.x = half.x + offset; break;
    case FACE_NEG_X: local.x = -half.x - offset; break;
    case FACE_POS_Z: local.z = half.z + offset; break;
    case FACE_NEG_Z: local.z = -half.z - offset; break;
    case FACE_POS_Y: local.y = half.y + offset; break;
    case FACE_NEG_Y: local.y = -half.y - offset; break;
    default: break;
    }

    // ローカル空間の位置を、再びワールド空間に変換
    XMVECTOR L = XMLoadFloat3(&local);
    XMVECTOR Pw = XMVector3TransformNormal(L, R) + C;

    XMFLOAT3 out;
    XMStoreFloat3(&out, Pw);

    // 2DのY座標は、3DのY座標をそのまま使用
    const float kKeepYWhenWallNY = 0.20f;
    if (fabsf(faceNormalW.y) < kKeepYWhenWallNY)
    {
        out.y = p3Pos.y;
    }

    return out;
}


// 指定したXZ座標の地面の高さを取得する関数
static bool FindGroundTopY_OnXZ(float x, float z, float startY, float maxDown, float* outTopY)
{
    std::vector<MAPDATA>& map = GetFieldMap();
    if (map.size() == 0) return false;

    bool found = false;
    float bestTop = -FLT_MAX;

    for (size_t i = 0; i < map.size(); ++i)
    {
        const MAPDATA& m = map[i];

        XMFLOAT3 half = Field_GetCollisionHalfSize(m);
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

// 3Dプレイヤーの前方に、スイッチ可能な壁があるかを判定し、あればその情報を取得する関数
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


// 3Dから2Dへの切り替えを試みる関数
static bool TrySwitch3DTo2D()
{
    SWITCH_TARGET tgt;
    if (!GetSwitchTargetFromPlayer3D(&tgt))
    {
        if (debugMode)
        {
            ImGui::Begin("Debug - han");
            if (ImGui::TreeNode("SwitchFail"))
            {
                ImGui::Text("No wall found in front");
                ImGui::TreePop();
            }
            ImGui::End();
        }
        return false;
    }

    std::vector<MAPDATA>& map = GetFieldMap();
    const MAPDATA& box = map[tgt.fieldIndex];

    PLAYER* p3 = GetPlayer3D();

    // 2Dプレイヤーの位置を、3Dプレイヤーの位置をもとに、当たった壁の面に応じて計算
    XMFLOAT3 p2Pos = Calc2DPositionOnWallSurface(box, tgt.face, p3->Position, tgt.normal);
    XMFLOAT3 p2Rot = Calc2DRotationFromFace(box, tgt.face);

    if (debugMode)
    {
        ImGui::Begin("Debug - han");
        if (ImGui::TreeNode("SwitchOK"))
        {
            ImGui::Text("Hit fieldIndex: %d", tgt.fieldIndex);
            ImGui::Text("Hit face: %d", tgt.face);
            ImGui::Text("3D pos: (%.2f, %.2f, %.2f)", p3->Position.x, p3->Position.y, p3->Position.z);
            ImGui::Text("2D pos: (%.2f, %.2f, %.2f)", p2Pos.x, p2Pos.y, p2Pos.z);
            ImGui::Text("2D rot: (%.2f, %.2f, %.2f)", p2Rot.x, p2Rot.y, p2Rot.z);
            ImGui::TreePop();
        }
        ImGui::End();
    }

    Player2D_InitAt(p2Pos, p2Rot);
    Player2D_SetActive(true);
    Player3D_SetActive(false);

    g_Mode = MODE_2D;
    return true;
}


// 2Dから3Dへの切り替えを試みる関数
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

    Camera_Reset2DState();

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

// プレイヤーのモード切替処理
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