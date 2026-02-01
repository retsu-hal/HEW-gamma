//Pushing_Obj_Manager.cpp
#include "Pushing_Obj_Manager.h"

#include "field.h"
#include "collision.h"
#include "player3D.h"
#include "keyboard.h"
#include "KeyBind.h"
#include "Input.h"

#include <cfloat>
#include <algorithm>

#include "MathUtil.h"
using namespace mu;

// Constants
static const float kPushMaxDist = 5.0f;     // Maximum distance to detect pushable object
static const float kPushSpeed = 0.02f;      // Speed at which objects are pushed

// State
static PUSH_STATE g_PushState = PUSH_STATE_NONE;
static PUSH_TARGET g_CurrentTarget;

// Get field half size
static XMFLOAT3 Field_GetHalfSize(const MAPDATA& m)
{
    return XMFLOAT3{
        BOX_RADIUS * m.scale.x,
        BOX_RADIUS * m.scale.y,
        BOX_RADIUS * m.scale.z
    };
}

// Check if field type is pushable
static bool Field_IsPushable(FIELD t)
{
    switch (t)
    {
    case FIELD_OBJ_2:
        return true;
    default:
        return false;
    }
}

// Ray vs AABB intersection
static bool RaycastAABB(
    const XMFLOAT3& rayO,
    const XMFLOAT3& rayD_in,
    const XMFLOAT3& boxC,
    const XMFLOAT3& boxHalf,
    float maxDist,
    XMFLOAT3* outNormal,
    float* outT)
{
    XMFLOAT3 rayD = Normalize(rayD_in);

    float tmin = 0.0f;
    float tmax = maxDist;
    XMFLOAT3 hitN = { 0, 0, 0 };

    auto slab = [&](float ro, float rd, float c, float h, XMFLOAT3 nNeg, XMFLOAT3 nPos) -> bool
        {
            if (fabsf(rd) < 1e-6f)
            {
                return (ro >= c - h && ro <= c + h);
            }

            float inv = 1.0f / rd;
            float t1 = (c - h - ro) * inv;
            float t2 = (c + h - ro) * inv;

            XMFLOAT3 n1 = nNeg;
            XMFLOAT3 n2 = nPos;

            if (t1 > t2) { std::swap(t1, t2); std::swap(n1, n2); }

            if (t1 > tmin) { tmin = t1; hitN = n1; }
            if (t2 < tmax) { tmax = t2; }

            return tmin <= tmax;
        };

    if (!slab(rayO.x, rayD.x, boxC.x, boxHalf.x, { -1, 0, 0 }, { 1, 0, 0 })) return false;
    if (!slab(rayO.y, rayD.y, boxC.y, boxHalf.y, { 0, -1, 0 }, { 0, 1, 0 })) return false;
    if (!slab(rayO.z, rayD.z, boxC.z, boxHalf.z, { 0, 0, -1 }, { 0, 0, 1 })) return false;

    if (tmin < 0.0f || tmin > maxDist) return false;

    if (outNormal) *outNormal = hitN;
    if (outT) *outT = tmin;
    return true;
}

// Ray vs OBB intersection
static bool RaycastOBB(
    const XMFLOAT3& rayO,
    const XMFLOAT3& rayD_in,
    const MAPDATA& box,
    float maxDist,
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

    XMFLOAT3 nL = { 0, 0, 0 };
    float t = 0.0f;

    if (!RaycastAABB(Oloc, Dloc, XMFLOAT3(0, 0, 0), half, maxDist, &nL, &t))
        return false;

    XMVECTOR nW = XMVector3TransformNormal(XMLoadFloat3(&nL), R);
    nW = XMVector3Normalize(nW);

    if (outNormalW) XMStoreFloat3(outNormalW, nW);
    if (outT) *outT = t;
    return true;
}

// Find pushable target from player's forward direction
static bool FindPushableTarget(PUSH_TARGET* outT)
{
    PLAYER* p3 = GetPlayer3D();
    if (!p3) return false;

    // Ray origin at player center
    XMFLOAT3 rayO = p3->Position;
    rayO.y += Player3D_GetSolidHalfSize().y; // Center height

    // Ray direction is player's forward
    XMFLOAT3 rayD = Player3D_GetForward();

    std::vector<MAPDATA>& map = GetFieldMap();
    if (map.size() == 0) return false;

    float bestT = FLT_MAX;
    PUSH_TARGET best{};
    best.fieldIndex = -1;

    for (size_t i = 0; i < map.size(); ++i)
    {
        const MAPDATA& m = map[i];
        if (!Field_IsPushable(m.no)) continue;

        XMFLOAT3 normal;
        float t;

        if (RaycastOBB(rayO, rayD, m, kPushMaxDist, &normal, &t))
        {
            if (t < bestT)
            {
                bestT = t;
                best.fieldIndex = (int)i;
                // Push direction is opposite of hit normal (push into the object)
                best.pushDirection = { -normal.x, 0.0f, -normal.z };
            }
        }
    }

    if (best.fieldIndex < 0) return false;
    if (outT) *outT = best;
    return true;
}

// Check if object can move in the given direction (collision check)
static bool CanObjectMove(int fieldIndex, const XMFLOAT3& moveDir, float moveAmount)
{
    std::vector<MAPDATA>& map = GetFieldMap();
    if (fieldIndex < 0 || fieldIndex >= (int)map.size()) return false;

    const MAPDATA& obj = map[fieldIndex];
    XMFLOAT3 objHalf = Field_GetHalfSize(obj);

    // Calculate new position
    XMFLOAT3 newPos = obj.pos;
    newPos.x += moveDir.x * moveAmount;
    newPos.z += moveDir.z * moveAmount;

    // Check collision with other solid objects
    for (size_t i = 0; i < map.size(); ++i)
    {
        if ((int)i == fieldIndex) continue;

        const MAPDATA& other = map[i];

        // Skip non-solid objects
        if (other.no == FIELD_GOAL) continue;

        XMFLOAT3 otherHalf = Field_GetHalfSize(other);

        // Simple AABB overlap check
        float dx = fabsf(newPos.x - other.pos.x);
        float dy = fabsf(newPos.y - other.pos.y);
        float dz = fabsf(newPos.z - other.pos.z);

        float overlapX = (objHalf.x + otherHalf.x) - dx;
        float overlapY = (objHalf.y + otherHalf.y) - dy;
        float overlapZ = (objHalf.z + otherHalf.z) - dz;

        if (overlapX > 0.0f && overlapY > 0.0f && overlapZ > 0.0f)
        {
            return false; // Collision detected
        }
    }

    return true;
}

// Apply push movement to the object
static void ApplyPushMovement(int fieldIndex, const XMFLOAT3& pushDir)
{
    std::vector<MAPDATA>& map = GetFieldMap();
    if (fieldIndex < 0 || fieldIndex >= (int)map.size()) return;

    // Check if object can move
    if (!CanObjectMove(fieldIndex, pushDir, kPushSpeed))
    {
        return; // Can't move, blocked by something
    }

    // Move the object
    map[fieldIndex].pos.x += pushDir.x * kPushSpeed;
    map[fieldIndex].pos.z += pushDir.z * kPushSpeed;
}

void PlayerPushManager_Init()
{
    g_PushState = PUSH_STATE_NONE;
    g_CurrentTarget.fieldIndex = -1;
    g_CurrentTarget.pushDirection = { 0, 0, 0 };
}

void PlayerPushManager_Finalize()
{
    g_PushState = PUSH_STATE_NONE;
    g_CurrentTarget.fieldIndex = -1;
}

void PlayerPushManager_Update()
{
    PLAYER* p3 = GetPlayer3D();
    if (!p3)
    {
        g_PushState = PUSH_STATE_NONE;
        g_CurrentTarget.fieldIndex = -1;
        return;
    }

    // Check if action key is held
    bool actionHeld = IsInputPress(ActionKey, gPad);

    // Check if player is moving forward (using velocity)
    XMFLOAT3 playerFwd = Player3D_GetForward();
    float velDot = Dot2D(p3->Velocity, playerFwd);
    bool movingForward = velDot > 0.001f;

    switch (g_PushState)
    {
    case PUSH_STATE_NONE:
    {
        // Check if we should start pushing
        if (actionHeld && movingForward)
        {
            PUSH_TARGET target;
            if (FindPushableTarget(&target))
            {
                g_CurrentTarget = target;
                g_PushState = PUSH_STATE_PUSHING;

                // Set player animation to push
                p3->CurrentAnimIndex = PLAYER_ANIM_PUSH;
            }
        }
    }
    break;

    case PUSH_STATE_PUSHING:
    {
        // Check if we should stop pushing
        if (!actionHeld || !movingForward)
        {
            g_PushState = PUSH_STATE_NONE;
            g_CurrentTarget.fieldIndex = -1;

            // Reset player animation
            p3->CurrentAnimIndex = PLAYER_ANIM_IDLE;
            break;
        }

        // Verify target is still valid and in range
        PUSH_TARGET newTarget;
        if (!FindPushableTarget(&newTarget) || newTarget.fieldIndex != g_CurrentTarget.fieldIndex)
        {
            // Lost the target
            g_PushState = PUSH_STATE_NONE;
            g_CurrentTarget.fieldIndex = -1;
            p3->CurrentAnimIndex = PLAYER_ANIM_IDLE;
            break;
        }

        // Update push direction based on player's current forward
        g_CurrentTarget.pushDirection = Player3D_GetForward();

        // Apply push movement to the object
        ApplyPushMovement(g_CurrentTarget.fieldIndex, g_CurrentTarget.pushDirection);

        // Keep push animation
        p3->CurrentAnimIndex = PLAYER_ANIM_PUSH;
    }
    break;
    }
}

PUSH_STATE PlayerPushManager_GetState()
{
    return g_PushState;
}

bool PlayerPushManager_IsPushing()
{
    return g_PushState == PUSH_STATE_PUSHING;
}

const PUSH_TARGET* PlayerPushManager_GetCurrentTarget()
{
    if (g_CurrentTarget.fieldIndex < 0) return nullptr;
    return &g_CurrentTarget;
}